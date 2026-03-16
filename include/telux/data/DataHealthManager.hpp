/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DataHealthManager.hpp
 *
 * @brief      DataHealthManager provides interface for monitoring data health and detecting data
 *             stalls across various network modules including Ethernet, IPA, WWAN and WLAN.
 *             This module supports single client only.
 */

#ifndef TELUX_DATA_DATAHEALTHMANAGER_HPP
#define TELUX_DATA_DATAHEALTHMANAGER_HPP

#include <functional>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

class IDataHealthManagerListener;

/**
 * Network modules that can be monitored for data stalls
 */
enum class DataStallNetworkModule {
    ETHERNET,    /**< Ethernet module */
    IPA,         /**< IPA (IP Accelerator) module */
    WWAN,        /**< WWAN (Wireless Wide Area Network) module */
    WLAN,        /**< WLAN (Wireless Local Area Network) module */
};

/**
 * Reasons for data stall detection being disabled
 */
enum class DataStallDisablementReason {
    INVALID = 0,          /**< No indication / invalid reason */
    MAX_INSTANCE = 1,     /**< Feature disabled as max instances of data stall occurred */
    DURATION_EXPIRY = 2,  /**< Feature disabled due to duration timer expiry */
    USER_TRIGGERED = 3,   /**< Feature disabled by user */
    FACTORY_RESET = 4,    /**< Feature disabled as factory reset is performed */
    ETH_DISABLED = 5,     /**< Feature disabled as Ethernet interface went down */
    WLAN_DISABLED = 6,    /**< Feature disabled as WLAN is disconnected/disabled */
};

/**
 * Reasons for data stall occurrence
 *
 * These provide detailed information about the root cause of a data stall,
 * helping to identify whether the issue is related to IPA channels, driver packet drops, or other
 * network stack components.
 */
enum class DataStallReason {
    INVALID = 0,                 /**< Invalid or unknown reason */
    Q6_DATA_STALL = 1,           /**< Data stall related to Q6 processor communication */
    ETH_IPA_DATA_STALL = 2,      /**< Data stall in the Ethernet IPA data path */
    ETH_SW_DATA_STALL = 3,       /**< Data stall in the Ethernet software processing path */
    ETH_MAC_HW_DATA_STALL = 4,   /**< Data stall in the Ethernet MAC hardware layer */
    IPA_ETH_DATA_STALL = 5,      /**< Data stall in the IPA Ethernet interface */
    IPA_AP_DATA_STALL = 6,       /**< Data stall in the IPA Access Point interface */
    IPA_STA_DATA_STALL = 7,      /**< Data stall in the IPA Station interface */
    IPA_WLAN_DATA_STALL = 8,     /**< Data stall in the IPA WLAN data path */
    IPA_CONF_ETH_DATA_STALL = 9, /**< Data stall due to Ethernet-related IPA configuration
                                      issues */
    IPA_CONF_DATA_STALL = 10,    /**< Data stall due to general IPA configuration issues */
    WLAN_AP_DATA_STALL = 11,     /**< Data stall in the WLAN Access Point interface */
    WLAN_STA_DATA_STALL = 12,    /**< Data stall in the WLAN Station interface */
};

/**
 * Recovery actions that can be triggered
 */
enum class DataStallRecoveryAction {
    NO_ACTION = 0,                  /**< No recovery action */
    LPM_TOGGLE = 1,                 /**< Modem LPM toggle and Modem online */
    MODEM_SSR = 2,                  /**< Modem SSR */
    ETH_IPA_SUSPEND_RESUME = 3,     /**< Suspend resume eth-ipa pipe */
    ETH_SW_TX_CHNL_REINIT = 4,      /**< ETH S/W Tx channel re-initialization */
    ETH_SW_RX_CHNL_REINIT = 5,      /**< ETH S/W Rx channel re-initialization */
    ETH_REINIT = 6,                 /**< ETH interface toggle */
    WLAN_RECONNECT = 7,             /**< WLAN reconnect from QCMAP */
    WLAN_AP_RECONNECT = 8,          /**< WLAN AP reconnect from QCMAP */
    WLAN_STA_RECONNECT = 9,         /**< WLAN STA reconnect */
    WLAN_DRIVER_RELOAD = 10,        /**< Reload WLAN driver */
    IPACM_RESTART = 11,             /**< Restart IPACM */
};

/**
 * Status of recovery operation
 */
enum class DataStallRecoveryResult {
    INVALID,                 /**< No recovery action */
    RECOVERY_SUCCESS,        /**< Recovery action is success and data stall is recovered */
    RECOVERY_ACTION_FAILURE, /**< Recovery action is failed and data stall is not recovered */
    RECOVERY_ACTION_SUCCESS, /**< Recovery action is success but data stall is not recovered */
};

struct ExtendedDataStallDetectionConfig {
    bool activeConnCheckEnable = false; /**< Enable active connectivity check on detecting
                                             data stall. */
    std::string pingServerAddressV4;    /**< IPv4 Server address used for ping check.
                                             Leave empty to disable IPv4 ping check.
                                             If activeConnCheckEnable is false, this field
                                             is ignored.*/
    std::string pingServerAddressV6;    /**< IPv6 Server address used for ping check.
                                             Leave empty to disable IPv6 ping check.
                                             If activeConnCheckEnable is false, this field
                                             is ignored.  */
    std::uint8_t txThreshold = 2;       /**< Minimum TX traffic threshold required to
                                             classify the connection as active.
                                             Valid range: 1-255. Default: 2 packets/sec */
    std::uint8_t rxThreshold = 2;       /**< Minimum RX traffic threshold required to
                                             classify the connection as active.
                                             Valid range: 1-255. Default: 2 packets/sec */
};

struct DataStallConfig {
    DataStallNetworkModule module;              /**< Network module */
    int slotId = DEFAULT_SLOT_ID;               /**< Slot id (Applicable for WWAN alone,
                                                     ignored otherwise) */
    int profileId = -1;                         /**< Profile id (Applicable for WWAN alone,
                                                     ignored otherwise) */
    bool enableRecovery = false;                /**< Enable auto recovery on detecting data
                                                     stall */
    bool enableRecoveryRestart = false;         /**< Enable recovery restart timer if all the
                                                     auto recovery options fails. */
    uint32_t recoveryRestartTimeDuration = 600; /**< Recovery restart timer duration in
                                                     seconds. Specifies the interval HM waits
                                                     before retrying automatic recovery,
                                                     allowing users to perform manual actions
                                                     once all auto‑recovery attempts fail.
                                                     Applicable only when enableRecoveryRestart
                                                     is set to true. Recommended minimum value
                                                     is 600 seconds. */
    uint32_t packetStatsTimerInterval = 20;     /**< Packet statistics polling interval in
                                                     seconds. This interval determines how
                                                     frequently the health manager checks packet
                                                     counters. Shorter intervals provide faster
                                                     stall detection but increase system
                                                     overhead. Recommended minimum value is
                                                     20 seconds */
    ExtendedDataStallDetectionConfig extendedConfig; /**< Extended configuration setting to
                                                          perform active connectivity check and
                                                          set Tx/Rx threshold. (Applicable for
                                                          WWAN alone, ignored otherwise) */
};

/**
 * Data stall detection state information
 */
struct DataStallDetectionState {
    DataStallNetworkModule module;       /**< Network module */
    bool enabled = false;                /**< True if data stall detection is enabled */
    DataStallDisablementReason reason =
        DataStallDisablementReason::INVALID; /**< Reason for disablement, if enabled is set
                                                  to false */
};

/**
 * Information about detected data stall
 */
struct DataStallInfo {
    DataStallNetworkModule module;   /**< Network module where stall was detected */
    uint32_t numOfInstances;         /**< Number of stall instances detected */
    DataStallRecoveryAction action;  /**< Recovery action that will be performed next by
                                          HM to recover */
    DataStallReason reason;          /**< Specific reason/cause for the data stall */
};

/**
 * Status of recovery operation
 */
struct DataStallRecoveryStatus {
    DataStallNetworkModule module;       /**< Network module being recovered */
    DataStallRecoveryAction recoveryAction; /**< Type of recovery that was performed */
    DataStallRecoveryResult status;      /**< Current status of recovery */
    bool isRecoverable;                  /**< True if the data stall is recoverable by the
                                              health manager. False indicates the stall cannot
                                              be automatically recovered and OEM/application
                                              intervention is required to perform recovery
                                              actions */
    DataStallReason reason;              /**< Specific reason/cause for the data stall */

};

/**
 * Restart timer status
 */
enum class DataStallRestartTimerStatus {
    INVALID,                /**< Invalid status */
    STARTED,                /**< Timer started */
    INTERNAL_ERROR,         /**< Internal error occurred */
    DURATION_NOT_SUPPORTED, /**< Duration not supported */
    STOPPED,                /**< Timer stopped */
    EXPIRED,                /**< Timer expired */
};

/**
 * Recovery restart timer status information
 */
struct DataStallRecoveryRestartStatus {
    DataStallNetworkModule module;       /**< Network module */
    DataStallRestartTimerStatus status;  /**< Restart timer status */
    DataStallReason reason;              /**< Specific reason/cause for the data stall */
};

/** @addtogroup telematics_data
 * @{ */

/**
 * @brief IDataHealthManager provides interface for monitoring data health and detecting data
 *        stalls across various network modules. It enables clients to register for data stall
 *        notifications and recovery events that are automatically triggered by the health manager.
 */
class IDataHealthManager {
public:
    /**
     * Checks the status of Data Health Manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE    - If Data Health Manager is ready for service.
     *          SERVICE_UNAVAILABLE  - If Data Health Manager is temporarily unavailable.
     *          SERVICE_FAILED       - If Data Health Manager encountered an irrecoverable
     *                                 failure.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Configure data stall detection and recovery behavior for a given network module.
     *
     * This API allows the client to configure the parameters for data stall monitoring.
     *
     * When configured, the health manager uses this information to monitor data stalls
     * and perform automatic recovery actions on the specified module. Configuration
     * changes persist across reboots.
     *
     * Data stall detection for the specific network module needs to be disabled before
     * updating the config. Use @ref telux::data::IDataHealthManager::getEnabledDataStallModules
     * to check which modules are currently enabled.
     *
     * On platforms with Access control enabled, caller needs to have
     * TELUX_DATA_HEALTH_MONITOR_OPS permission to invoke this API successfully.
     *
     * @param [in] config   Data stall configuration for the target module
     *                      @ref telux::data::DataStallConfig
     *
     * @returns ErrorCode indicating success or suitable error code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backward compatibility.
     */
    virtual telux::common::ErrorCode setDataStallConfig(
        const DataStallConfig &config) = 0;

    /**
     * Retrieve current data stall detection and recovery configuration for a given network
     * module.
     *
     * This API populates the provided configuration structure with the parameters currently
     * programmed in the health manager for the specified module, including recovery enablement
     * and recovery restart timer settings.
     *
     * @param [in,out] config   On input, identifies the target module to query.
     *                          On successful return, contains the active configuration
     *                          for that module @ref telux::data::DataStallConfig.
     *
     * @returns ErrorCode indicating success or suitable error code
     *                    @ref telux::common::ErrorCode.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backward compatibility.
     */
    virtual telux::common::ErrorCode getDataStallConfig(
        DataStallConfig &config) = 0;

    /**
     * Enable or disable data stall detection for specified network module.
     *
     * When enabled, the health manager will monitor the specified network module for data stalls
     * and automatically trigger recovery mechanisms when stalls are detected.
     *
     * On platforms with Access control enabled, Caller needs to have
     * TELUX_DATA_HEALTH_MONITOR_OPS permission to invoke this API successfully.
     *
     * @param [in] enable       True to enable data stall detection, false to disable.
     * @param [in] module       Network module to enable or disable data stall detection for.
     *
     * @returns ErrorCode indicating success or suitable error code
     *                    @ref telux::common::ErrorCode.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode enableDataStallDetection(bool enable,
        DataStallNetworkModule module) = 0;

    /**
     * Retrieve the list of network modules that are enabled for data stall detection.
     *
     * This API populates the provided vector with the network modules that are currently
     * registered/enabled for data stall detection. These modules will trigger corresponding
     * indications in IDataHealthManagerListener when data stalls are detected.
     *
     * @param [out] modules   On successful return, contains the list of registered/enabled
     *                        network modules @ref telux::data::DataStallNetworkModule.
     *
     * @returns ErrorCode indicating success or suitable error code
     *                    @ref telux::common::ErrorCode.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backward compatibility.
     */
    virtual telux::common::ErrorCode getEnabledDataStallModules(
        std::vector<DataStallNetworkModule> &modules) = 0;

    /**
     * Register a listener for data health manager events including data stall detection,
     * detection state changes, and recovery status updates.
     *
     * @param [in] listener    Pointer to IDataHealthManagerListener object that processes the
     *                         notifications
     *
     * @returns Status of registerListener success or suitable status code.
     *          Returns error if another client is already registered.
     *
     * @note    Only single client registration is supported. Attempting to register a second
     *          listener while one is already registered will fail.
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IDataHealthManagerListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Pointer to IDataHealthManagerListener object that needs to be
     *                         removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IDataHealthManagerListener> listener) = 0;

    /**
     * Destructor for IDataHealthManager
     */
    virtual ~IDataHealthManager() {};
};

/**
 * Interface for Data Health Manager listener. Client needs to implement this interface to
 * get data health monitoring notifications including data stall detection, detection
 * state changes, and recovery status updates.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IDataHealthManagerListener {
public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status   New service status @ref telux::common::ServiceStatus
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * This function is called when data stall detection state changes.
     *
     * This notification is sent when:
     * - Data stall detection is explicitly enabled/disabled by a client
     * - health manager automatically disables detection due to max instances, duration expiry,
     *   etc.
     *
     * @param [in] state    Data stall detection state information
     *                      @ref telux::data::DataStallDetectionState
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual void onDataStallDetectionStateChanged(const DataStallDetectionState &state) {}

    /**
     * This function is called when a data stall is detected on one or more network modules.
     *
     * A data stall indicates that data traffic has stopped flowing through the specified
     * network module despite an active connection. This could be due to various reasons
     * including hardware issues, driver problems, or network congestion.
     *
     * @param [in] info         Information about the detected data stall
     *                          @ref telux::data::DataStallInfo
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual void onDataStallDetected(const DataStallInfo &info) {}

    /**
     * This function is called when a auto recovery action is triggered in response to a data
     * stall.
     *
     * When auto recovery is enabled, the health manager automatically initiates recovery actions
     * upon detecting data stalls. The specific recovery action (e.g., interface toggle, driver
     * reload, reconnect) is determined based on the affected network module, stall severity, and
     * number of previous recovery attempts.
     *
     * @param [in] status   Detailed status of the recovery operation
     *                      @ref telux::data::DataStallRecoveryStatus
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual void onDataStallRecoveryTriggered(const DataStallRecoveryStatus &status) {}

    /**
     * This function is called when the recovery restart timer status changes.
     *
     * The recovery restart timer provides a grace period for manual intervention after all
     * automatic recovery attempts have been exhausted. This timer is started when all health
     * manager auto recovery actions have failed and the data stall persists, indicating that
     * automatic recovery mechanisms were unable to resolve the issue.
     *
     * During the timer period, the health manager pauses automatic recovery attempts and expects
     * OEM/application to perform recovery actions.
     *
     * @param [in] status   Recovery restart timer status information including the affected
     *                      network module and current timer state.
     *                      @ref telux::data::DataStallRecoveryRestartStatus
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change and could
     *          break backward compatibility.
     */
    virtual void onDataStallRecoveryRestartTimerUpdate(
        const DataStallRecoveryRestartStatus &status) {}

    /**
     * Destructor for IDataHealthManagerListener
     */
    virtual ~IDataHealthManagerListener() {};
};

/** @} */ /* end_addtogroup telematics_data */

}  // namespace data
}  // namespace telux

#endif  // TELUX_DATA_DATAHEALTHMANAGER_HPP