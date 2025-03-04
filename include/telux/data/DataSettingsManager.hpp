/*
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DataSettingsManager.hpp
 *
 * @brief      The Data Settings Manager class provides an interface to the data subsystem settings.
 */

#ifndef TELUX_DATA_DATASETTINGSMANAGER_HPP
#define TELUX_DATA_DATASETTINGSMANAGER_HPP

#include <memory>
#include <future>

#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IDataSettingsListener;

/**
 * @brief The Data Settings Manager class provides an interface to the data subsystem settings.
 */
class IDataSettingsManager {
public:
    /**
     * Checks if the data subsystem is ready.
     *
     * @returns  if Settings Manager is ready, false otherwise.
     *
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Waits for the data subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified when data settings manager is ready.
     *
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Switch backhaul to be used by traffic.
     * Provides the ability to re-route clients traffic from one backhaul to another.
     * Clients must call this API for each backhaul switch. For instance, if the default bridge
     * (bridge0) and the on-demand bridge (bridges created by VLANs) need to be re-routed to WLAN,
     * this API must be called twice for the default profile ID and the on-demand profile ID..
     * If destination backhaul is WLAN (WLAN in Station Mode):
     * - Traffic associated with the default and on-demand bridges will be re-routed to WLAN
     *   backhaul.
     * - Client traffic can only be re-routed to WLAN backhaul if the station is connected to an
     *   external access point.
     * - VLANs mapped to WWAN backhaul will be automatically mapped to WLAN backhaul.
     * - Firewall and DMZ rules configured on WLAN backhaul (if configured before calling this API)
     *   will be automatically activated.
     * If destination backhaul is WWAN:
     *  - Any VLAN profile ID mapping configured in the destination backhaul prior to calling this
     *    API will be applied automatically.
     *  - Any firewall or DMZ rule configured on WWAN backhaul before calling this API will be
     *    activated automatically.
     *
     * @param [in] source         Backhaul @ref telux::data::BackhaulInfo to re-route traffic from
     * @param [in] dest           Backhaul @ref telux::data::BackhaulType to re-route traffic to
     * @param [in] applyToAll     Traffic on all source backhauls will be routed to dest backhauls
     *                            if the source backhaul type is
     *                            @ref telux::data::BackhaulType::WWAN, traffic on all WWAN
     *                            backhauls (default and on-demand) will be routed to dest backhaul.
     *                            if dest backhaul type is @ref telux::data::BackhaulType::WWAN
     *                            traffic on source backhaul will be routed to WWAN backhauls
     *                            (default and on-demand) based on vlan-backhaul binding set by
     *                            telux::data::net::IVlanManager::bindToBackhaul
     * @param [in] callback       Optional callback to get the response for switchBackHaul.
     *
     * @returns Status of switchBackHaul, i.e., success or applicable status code
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status switchBackHaul(BackhaulInfo source, BackhaulInfo dest,
        bool applyToAll = false, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
    * Sets the latency level for data traffic that has been marked as prioritized data. This only
    * affects the latency level in the modem on the UE. This does not impact network level latency
    * or QoS. This API allows clients to set the uplink latency level of data they deem as time
    * critical compared to the rest of the data flowing in the system. If the latency level is set
    * to @ref LatencyLevel::LOW, then the implementation will prioritize this data over other
    * non-prioritized data flows. Configuration can be reset by setting
    * @ref LatencyLevel::NORMAL (no priority).
    *
    * This is a global setting applicable to all WWAN profiles. Configuration set via this API is
    * not persistent over reboot or sub-system restart (updated via
    * @ref IDataSettingsListener::onServiceStatusChange). After reboot or SSR, configuration will
    * reset to default, i.e., LatencyLevel::NORMAL (no priority).
    *
    * @param [in] latencyConfig       Latency/priority configuration
    *
    * @returns  Immediate error code of setLatencyConfig, i.e., success or suitable error code.
    *
    * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
    *           break backwards compatibility.
    */
    virtual telux::common::ErrorCode setLatencyConfig(LatencyConfig latencyConfig) = 0;

    /**
    * Get latency level.
    * This API can be used to get the current latency configuration.
    * See @ref setLatencyConfig for more information.
    *
    * @param [out] latencyConfig       Latency/priority configuration
    *
    * @returns Immediate error code of getLatencyConfig, i.e., success or suitable error code.
    *
    * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
    *           break backwards compatibility.
    */
    virtual telux::common::ErrorCode getLatencyConfig(LatencyConfig& latencyConfig) = 0;

    /**
     * Registers a listener to receive data settings notifications.
     *
     *
     * @param [in] listener    Pointer to the IDataSettingsListener object to process received
     *                         notifications.
     *
     * @returns Status of registerListener success or applicable status code
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IDataSettingsListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Point to the IDataSettingsListener object to remove.
     *
     * @returns Status of deregisterListener success or applicable status code
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IDataSettingsListener> listener) = 0;

    /**
     * Destructor for IDataSettingsManager
     */
    virtual ~IDataSettingsManager(){};
};

/**
 * Interface for the Data...Clients need to implement this interface to access Data Settings
 * service notifications like onServiceStatusChange.
 *
 * The listener methods can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IDataSettingsListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for IDataSettingsListener
     */
    virtual ~IDataSettingsListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif
