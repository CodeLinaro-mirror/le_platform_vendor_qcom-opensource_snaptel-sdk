/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file WakeupManager.hpp
 * @brief Provides ability to monitor wakeup due to QMI communication or Wake-on-WLAN (WoW) events.
 */

#ifndef TELUX_POWER_WAKEUPMANAGER_HPP
#define TELUX_POWER_WAKEUPMANAGER_HPP

#include <unistd.h>

#include <bitset>
#include <cstdint>
#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace power {

/** @addtogroup telematics_power_wakeup_manager
 * @{ */

/**
 * Wakeup initiator.
 */
enum class WakeupType {
    /** Source of wakeup could not be determined. */
    UNKNOWN,

    /** A QMI transaction caused the wakeup */
    QMI,

    /** A Wake on Wireless LAN (WoW) packet caused the wakeup. */
    WOW
};

/**
 * Details of the QMI message.
 */
struct QmiWakeupInfo {
    /**
     * QMI service identifier.
     */
    uint32_t serviceId;

    /**
     * QMI source node identifier. Represents the node from which the message originated.
     */
    uint32_t sourceNodeId;

    /**
     * QMI destination node identifier. Represents the node to which message will
     * be delivered.
     */
    uint32_t destinationNodeId;

    /**
     * True, if the msgId field is set and has a valid value for this
     * transaction otherwise false.
     */
    bool isMsgIdValid;

    /**
     * QMI message identifier.
     */
    uint32_t msgId;

    /**
     * True, if the pid field is set and has a valid value for this
     * transaction otherwise false.
     */
    bool isPIDValid;

    /**
     * Linux process ID (PID) of the process which received this message.
     */
    pid_t pid;

    /**
     * True, if the processName field is set and has a valid value for
     * this transaction otherwise false.
     */
    bool isProcessNameValid;

    /**
     * Name of the Linux process (upto 16 characters) who received this message.
     */
    std::string processName;
};

/**
 * Classifies the Wake on Wireless LAN (WoW) event that triggered the device wakeup.
 */
enum class WowWakeupCategory {
    /** The reason the device woke up could not be identified. */
    UNSPECIFIED = -1,

    /**
     * The device woke up due to a Wi-Fi protocol event, such as a change in connection state.
     */
    WLAN_PROTOCOL = 0,

    /**
     * The device woke up because a background networking task could not complete on its own
     * and needed the host to step in, such as a keepalive failure or a DHCP renewal.
     */
    OFFLOAD = 1,

    /**
     * The device woke up because an incoming packet matched a configured filter rule.
     */
    PATTERN_FILTER = 2,

    /**
     * The device woke up upon receiving a Wake-on-LAN magic packet from another device
     * on the network.
     */
    MAGIC_PACKET = 3,

    /**
     * The device woke up due to an internal condition unrelated to network traffic, such as
     * a thermal change, power failure, or a fatal error condition.
     */
    SYSTEM = 4,
};

/**
 * Details of the Wake on Wireless LAN (WoW) packet that caused the wakeup.
 *
 * When the subsystem running the Power Manager service is woken by a WoW packet,
 * this structure carries the relevant information about the triggering packet.
 */
struct WowWakeupInfo {
    /**
     * Timestamp of the WoW wakeup event.
     */
    uint64_t timestamp = 0;

    /**
     * Name of the WLAN interface on which the WoW packet was received.
     */
    std::string interfaceName = "";

    /**
     * MAC address of the WoW packet that triggered the wakeup.
     */
    std::string macAddress = "";

    /**
     * Category of the WoW event that triggered the wakeup.
     *
     * @see WowWakeupCategory for the list of possible values.
     */
    WowWakeupCategory wakeupCategory = WowWakeupCategory::UNSPECIFIED;

    /**
     * Pattern Byte Mask (PBM) data of the WoW packet that triggered the wakeup.
     * Valid only when wakeupCategory is WowWakeupCategory::PATTERN_FILTER.
     */
    std::string pbmBuffer = "";
};

/**
 * Details of the activity that caused system wakeup.
 *
 * @deprecated Use @ref WakeupEventInfo instead.
 */
struct WakeupInfo {
    /**
     * Details of the QMI message.
     */
    QmiWakeupInfo qmiWakeupInfo;

    /**
     * Wakeup initiator.
     */
    WakeupType wakeupType;
};

/**
 * Details of the activity that caused system wakeup.
 *
 * Uses a tagged union to carry only the relevant wakeup-specific data
 * based on the @ref type field:
 * - @ref WakeupType::QMI  → access @ref qmi
 * - @ref WakeupType::WOW  → access @ref wow
 * - @ref WakeupType::UNKNOWN → neither union member is valid
 */
struct WakeupEventInfo {
    /**
     * Wakeup initiator type. Determines which union member is active.
     */
    WakeupType type;

    /**
     * Union of wakeup-specific details. Only the member corresponding
     * to @ref type is valid and properly constructed.
     */
    union {
        /** Valid when type is @ref WakeupType::QMI. */
        QmiWakeupInfo qmi;

        /** Valid when type is @ref WakeupType::WOW. */
        WowWakeupInfo wow;
    };

    /**
     * Constructs a WakeupEventInfo and initialises the union member
     * that corresponds to @p wakeupType.
     *
     * @param[in] wakeupType  The wakeup initiator type.
     */
    explicit WakeupEventInfo(WakeupType wakeupType);

    /**
     * Destroys the active union member.
     */
    ~WakeupEventInfo();

    WakeupEventInfo(const WakeupEventInfo &other);
    WakeupEventInfo &operator=(const WakeupEventInfo &other);
    WakeupEventInfo(WakeupEventInfo &&other) noexcept;
    WakeupEventInfo &operator=(WakeupEventInfo &&other) noexcept;
};

/** Enum of all the possible indications invoked by a wakeup listener. */
enum class WakeupIndicationsType : uint32_t {
    /** Register to receive service status change notifications -
     * @ref telux::common::IServiceStatusListener::onServiceStatusChange
     */
    DEFAULT = 0,

    /** Register to receive QMI wakeup notifications -
     * @ref telux::power::IWakeupListener::onWakeup with @ref WakeupType::QMI
     */
    QMI_WAKEUP = 1,

    /** Register to receive Wake-on-WLAN wakeup notifications -
     * @ref telux::power::IWakeupListener::onWakeup with @ref WakeupType::WOW
     */
    WOW_WAKEUP = 2,
};

/**
 * Bitset representing the list of wakeup indications selected by the client.
 * Extends std::bitset<32> with overloads that accept @ref WakeupIndicationsType
 * directly so callers do not need explicit casts.
 */
class WakeupIndications : public std::bitset<32> {
 public:
    using std::bitset<32>::bitset;
    using std::bitset<32>::set;
    using std::bitset<32>::test;
    WakeupIndications &set(WakeupIndicationsType t) noexcept {
        std::bitset<32>::set(static_cast<size_t>(t));
        return *this;
    }
    bool test(WakeupIndicationsType t) const {
        return std::bitset<32>::test(static_cast<size_t>(t));
    }
};

/**
 * Convenience constant to register for all wakeup indications.
 * Passing this to @ref IWakeupManager::registerListener or
 * @ref IWakeupManager::deRegisterListener covers all currently defined
 * @ref WakeupIndicationsType values as well as any added in future.
 */
const WakeupIndications ALL_WAKEUP_INDICATIONS{0xFFFFFFFFu};

/**
 * Receives notification whenever the subsystem is woken up due to QMI communication
 * or Wake-on-WLAN (WoW) events.
 */
class IWakeupListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * Invoked whenever the system is woken up and provides the details
     * of the activity that caused wakeup.
     *
     * On platforms with access control enabled, caller needs to have TELUX_POWER_WAKEUP_INFO
     * permission to invoke this API successfully.
     *
     * @param[in] wakeupInfo details of the activity that caused the wake up
     */
    virtual void onWakeup(const WakeupEventInfo &wakeupInfo) {
    }

    /**
     * Invoked whenever the system is woken up and provides the details
     * of the activity that caused wakeup.
     *
     * On platforms with access control enabled, caller needs to have TELUX_POWER_WAKEUP_INFO
     * permission to invoke this API successfully.
     *
     * @param[in] wakeupInfo details of the activity that caused the wake up
     *
     * @deprecated Use IWakeupListener::onWakeup(const WakeupEventInfo &wakeupInfo) instead.
     *             @ref telux::power::IWakeupListener::onWakeup
     */
    virtual void onWakeup(WakeupInfo wakeupInfo) {
    }

    /**
     * Destructor for IWakeUpListener.
     */
    virtual ~IWakeupListener() {
    }
};

/**
 * IWakeupManager is used to monitor application processor wakeup due to QMI communication
 * or Wake on Wireless LAN (WoW) events.
 */
class IWakeupManager {
 public:
    /**
     * Registers the given listener for specific events in the Wakeup Manager like
     * service status change or wakeup.
     *
     * @param[in] listener        Pointer to @ref IWakeupListener object that processes
     *                            the notifications
     * @param[in] indicationList  Optional list of specific indications to register for.
     *                            If not provided, registers for all indications
     *                            (@ref ALL_WAKEUP_INDICATIONS).
     *                            @ref WakeupIndicationsType
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the listener is registered,
     *          otherwise, an appropriate error code
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IWakeupListener> listener,
        WakeupIndications indicationList = ALL_WAKEUP_INDICATIONS)
        = 0;

    /**
     * Deregisters the given listener from the list of indications provided via
     * @ref WakeupIndicationsType. If not provided, deregisters from all indications
     * (@ref ALL_WAKEUP_INDICATIONS).
     *
     * For example - if a client registers for both @ref WakeupIndicationsType::QMI_WAKEUP and
     * @ref WakeupIndicationsType::WOW_WAKEUP indications, and during deregistration only provides
     * @ref WakeupIndicationsType::QMI_WAKEUP in the indication list, the listener will be
     * deregistered from QMI_WAKEUP but will remain registered for WOW_WAKEUP.
     *
     * @param[in] listener        Pointer to @ref IWakeupListener object to deregister
     * @param[in] indicationList  Optional list of specific indications to deregister from.
     *                            If not provided, deregisters from all indications
     *                            (@ref ALL_WAKEUP_INDICATIONS).
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the listener is deregistered,
     *          otherwise, an appropriate error code
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deRegisterListener(std::weak_ptr<IWakeupListener> listener,
        WakeupIndications indicationList = ALL_WAKEUP_INDICATIONS)
        = 0;

    /**
     * Gets the wakeup manager's service status.
     *
     * @returns @ref telux::common::ServiceStatus::SERVICE_AVAILABLE if the service is ready
     *          for use, @ref telux::common::ServiceStatus::SERVICE_UNAVAILABLE if the service
     *          is temporarily unavailable (possibly undergoing initialization),
     *          @ref telux::common::ServiceStatus::SERVICE_FAILED if the service needs
     *          re-initialization
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Performs cleanup and destroys the IWakeUpManager instance.
     */
    virtual ~IWakeupManager(){};
};

/** @} */ /* end_addtogroup telematics_power_wakeup_manager */

}  // End of namespace power
}  // End of namespace telux

#endif  // TELUX_POWER_WAKEUPMANAGER_HPP
