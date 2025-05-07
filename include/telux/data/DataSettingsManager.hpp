/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DataSettingsManager.hpp
 *
 * @brief      Data Settings Manager class provides the interface to data subsystem settings.
 */

#ifndef TELUX_DATA_DATASETTINGSMANAGER_HPP
#define TELUX_DATA_DATASETTINGSMANAGER_HPP

#include <memory>

#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {

// Forward declarations
class IDataSettingsListener;

/**
 * Set priority between N79 5G and Wlan 5GHz Band
 */
enum class BandPriority {
    N79  = 0 ,              /** N79 has higher priority  */
    WLAN = 1 ,              /** Wlan has higher priority */
};

/**
 * N79 5G/Wlan 5GHz interference avoidance configuration
 */
struct BandInterferenceConfig {
    BandPriority priority      ;        /** Priority settings for N79/Wlan 5G                    */
    uint32_t wlanWaitTimeInSec = 30 ;   /** If Wlan 5GHz has higher priority and suffers signal
                                            drop, modem will wait for period of time specified here
                                            for Wlan signal to recover before enabeling N79 5G.  */
    uint32_t n79WaitTimeInSec  = 30 ;   /** If N79 has higher priority and suffers signal drop,
                                            modem will wait for period of time specified here for
                                            N79 5G signal to recover before switching Wlan to
                                            5GHz.                                                */
};

struct AutoConnectProfile {
    int    profileId;
    SlotId slotId;
};

struct AutoConnectConfig {
    bool enable;     /** flag indicate if automatic connect enabled */
    bool persistent; /** flag indicate if new configuration taking effect persistently */
};

struct AutoConnectSettings {
    AutoConnectProfile  profile;
    AutoConnectConfig   config;
};

/**
 * Specifies the type of configurations that need to be cleaned up.
 */

 enum CleanupConfigType {
    CLEANUP_CONFIG_WWAN_PROFILE_CACHE, /**< Cleans cached WWAN data profiles related
                                            information. If WWAN data profiles cache is cleaned up,
                                            the information related to VLAN binding to backhaul or
                                            profiles will also be cleaned up. */
    CLEANUP_CONFIG_VLAN_BINDINGS        /**< Cleans VLAN backhaul bindings alone. */
};

/**
 * Represents a set of configs from CleanupConfigType.
 * For example, a value of cleanupConfigTypes.set(CleanupConfigType::CLEANUP_CONFIG_VLAN_BINDINGS)
 * represents that VLAN binding to backhaul/profile needs to be cleaned up.
 */
using CleanupConfigTypes = std::bitset<64>;

/**
 * Cleanup configuration.
 * The cleanup configuration is subscription-specific, and CleanupConfigTypes can contain one or
 * more bits of CleanupConfigType.
 */
struct CleanupConfig {
    SlotId slotId = DEFAULT_SLOT_ID;
    CleanupConfigTypes mask;
};

/**
 * This function is called with the response to requestBackhaulPreference API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] backhaulPref       vector of @ref telux::data::BackhaulPref which contains the
 *                                current order of backhaul preference
 *                                First element is most preferred and last element is least
 *                                preferred backhaul.
 * @param [in] error              Return code for whether the operation succeeded or failed.
 */
using RequestBackhaulPrefResponseCb = std::function<void(
    const std::vector<BackhaulType> backhaulPref, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestBandInterferenceConfig API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] isEnabled          True: interference management is enabled.
 *                                False: interference management is disabled
 * @param [in] config             Current N79 5G /Wlan 5GHz band interference configuration
 *                                Set to nullptr if interference management is disabled
 *                                @ref telux::data::BandInterferenceConfig
 * @param [in] error              Return code for whether the operation succeeded or failed.
 */
using RequestBandInterferenceConfigResponseCb = std::function<void(bool isEnabled,
    std::shared_ptr<BandInterferenceConfig> config, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestMacSecState API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] enabled          True: MacSec is enabled, False: Macsec is disabled.
 * @param [in] error            Return code for whether the operation succeeded or failed.
 */
using RequestMacSecSateResponseCb = std::function<void(bool enabled,
    telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestWwanConnectivityConfig API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] slotId           Slot id for which wwan connectivity is reported.
 * @param [in] isAllowed        True: connectivity allowed, False: connectivity disallowed.
 * @param [in] error            Return code for whether the operation succeeded or failed.
 */
using requestWwanConnectivityConfigResponseCb = std::function<void(SlotId slotId,
    bool isAllowed, telux::common::ErrorCode error)>;

/** @addtogroup telematics_data
 * @{ */

/**
 * @brief Data Settings Manager class provides APIs related to the data subsystem settings.
 *        For example, ability to reset current network settings to factory settings, setting
 *        backhaul priority, and enabling roaming per PDN.
 */
class IDataSettingsManager {
public:
    /**
     * Checks the status of Data Settings manager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If Data Settings manager object is ready for service.
     *          SERVICE_UNAVAILABLE  -  If Data Settings manager object is temporarily unavailable.
     *          SERVICE_FAILED       -  If Data Settings manager object encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Resets current network settings to initial setting configured in factory.
     * Factory settings are the initial network settings generated during manufacturing process.
     * After successful reset, device will reboot with factory network settings.
     *
     * @param [in] operationType    @ref telux::data::OperationType
     * @param [in] callback         callback to get the response to restoreFactorySettings
     *
     * @returns Immediate status of restoreFactorySettings i.e. success or suitable status.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change.
     */
    virtual telux::common::Status restoreFactorySettings(OperationType operationType,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Set backhaul preference for bridge0 (default bridge) traffic. Bridge0 Traffic routing to
     * backhaul will be attempted on first to least preferred.
     * For instance if backhaul vector contains ETH, USB, and WWAN, bridge0 traffic routing will be
     * attempted on ETH first, then USB and finally WWAN backhaul.
     * Configuration changes will be persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] backhaulPref     vector of @ref telux::data::BackhaulType which contains the
     *                              order of backhaul preference to be used when connecting to
     *                              external network.
     *                              First element is most preferred and last element is least
     *                              preferred backhaul.
     * @param [in] callback         callback to get response for setBackhaulPreference.
     *
     * @returns Status of setBackhaulPreference i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status setBackhaulPreference(std::vector<BackhaulType> backhaulPref,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request current backhaul preference for bridge0 (default bridge) traffic.
     *
     * @param [in] callback         callback to get response for requestBackhaulPreference.
     *
     * @returns Status of requestBackhaulPreference i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestBackhaulPreference(
        RequestBackhaulPrefResponseCb callback) = 0;

    /**
     * Configure N79 5G and Wlan 5GHz band priority.
     * Sets priority for modem to use either 5GHz Wlan or N79 5G band when they are both available
     * to avoid interference.
     * In case N79 5G is configured as higher priority:
     *    If N79 5G becomes available while 5G Wlan is enabled, Wlan (AP/Sta) will be moved to
     *    2.4 GHz.
     *    If N79 5G becomes unavailable for
     *    @ref telux::data::BandInterferenceConfig::n79WaitTimeInSec time period, Wlan will be
     *    moved to 5GHz.
     * In case Wlan 5GHz is configured as higher priority:
     *    If Wlan 5GHz (AP/Sta) becomes available while N79 5G is enabled, N79 5G will be disabled.
     *    If Wlan 5GHz becomes unavailable for
     *    @ref telux::data::BandInterferenceConfig::wlanWaitTimeInSec period and N79 5G is
     *    available, N79 will be enabled.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] enable           True: enable interference management.
     *                              False: disable interference management
     * @param [in] config           N79 5G /Wlan 5GHz band interference configuration
     *                              @ref telux::data::BandInterferenceConfig
     * @param [in] callback         callback to get response for setBandInterferenceConfig.
     *
     * @returns Status of setBandInterferenceConfig i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status setBandInterferenceConfig(bool enable,
        std::shared_ptr<BandInterferenceConfig> config = nullptr ,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request N79 5G and Wlan 5GHz band priority settings.
     * Request the configurations set by telux::data::setBandInterferenceConfig
     *
     * @param [in] callback         callback to get response for requestBandInterferenceConfig.
     *
     * @returns Status of requestBandInterferenceConfig i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestBandInterferenceConfig(
        RequestBandInterferenceConfigResponseCb callback) = 0;

    /**
     * Allow/Disallow WWAN connectivity.
     * Controls whether system should allow/disallow WWAN connectivity to cellular network.
     * Default setting is allow WWAN connectivity to cellular network.
     * - If client selects to disallow WWAN connectivity, any further attempts to start data
     *   calls using @ref telux::data::IDataConnectionManager::startDataCall will fail with
     *   @ref telux::common::ErrorCode::NOT_SUPPORTED.
     *   Data calls can be connected again only if client selects to allow WWAN connectivity.
     * - If client selects to disallow WWAN connectivity while data calls are already connected,
     *   all WWAN data calls will also be disconnected.
     *   Client will also receive @ref telux::data::IDataConnectionListener::onDataCallInfoChanged
     *   notification with @ref telux::data::IDataCall object status
     *   @ref telux::data::DataCallStatus::NET_NO_NET for all impacted data calls.
     * Configuration changes will be persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] slotId           Slot id on which WWAN connectivity to be allowed/disallowed
     * @param [in] allow            True: allow connectivity, False: disallow connectivity
     * @param [in] callback         optional callback to get response for setWwanConnectivityConfig.
     *
     * @returns Status of setWwanConnectivityConfig i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status setWwanConnectivityConfig(SlotId slotId, bool allow,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request current WWAN connectivity Configuration.
     *
     * @param [in] slotId           Slot id for which WWAN connectivity to be reported.
     * @param [in] callback         callback to get response for requestWwanConnectivityConfig.
     *
     * @returns Status of requestWwanConnectivityConfig i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     *
     */
    virtual telux::common::Status requestWwanConnectivityConfig(SlotId slotId,
        requestWwanConnectivityConfigResponseCb callback) = 0;

    /**
     * This API allows the client to set the MACsec state.
     *
     * - If client enables the MACsec, post that the packets over the ethernet link
     *   will be encrypted.
     * - If client disables the MACsec, post that the packets over the ethernet link
     *   will not be encrypted.
     *
     * @param [in] enable          True: enable the MACsec, False: disable the MACsec.
     * @param [in] callback        callback to get response for setMacSecState.
     *
     * @returns Status of setMacSecState i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status setMacSecState(bool enable,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request the current MacSec state.
     *
     * @param [in] callback    callback to get response for requestMacSecState.
     *
     * @returns Status of requestMacSecState i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     *
     */
    virtual telux::common::Status requestMacSecState(RequestMacSecSateResponseCb callback) = 0;

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
    virtual telux::common::ErrorCode setLatencyConfig(const LatencyConfig &latencyConfig) = 0;

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
     * Cleans up persisted configurations.
     * This API allows the cleanup of persisted configurations, such as:
     * - Cached WWAN data profile information
     * - Data profile to VLAN bindings, etc.
     * Cleanup will be performed based on the bits set in the @ref CleanupConfigTypes provided in
     * @ref CleanupConfig.
     *
     * SlotId in @ref CleanupConfig:
     * - Is not needed and can be kept as default on platforms with single active SIM slot
     *   configurations like DSSA (Dual SIM Single Active).
     * - Will be utilized on platforms with dual active SIM slots like DSDA (Dual SIM Dual Active)
     *   or DSDS (Dual SIM Dual Standby) to delete configurations specific to a particular slot.
     *
     * Some instances when this API would be invoked:
     * - SIM switch is done between 2 SIMs in DSSS/DSSA configuration
     *   @ref telux::tel::IMultiSimManager::switchActiveSlot.
     * - SIM profiles get updated on an eSIM @ref telux::tel::ISimProfileManager::setProfile.
     *
     * @note This API will delete all configurations except those associated with the default
     *  profile @ref IDataConnectionManager::getDefaultProfile. For example, if the user calls
     *  cleanupSettings for @ref CleanupConfigType::CLEANUP_CONFIG_WWAN_PROFILE_CACHE and
     *  @ref CleanupConfigType::CLEANUP_CONFIG_VLAN_BINDINGS, the default profile and VLAN bindings
     *  associated with the default profile will be maintained.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_SETTING
     * permission to invoke this API successfully.
     *
     * @param [in] config   @ref telux::data::CleanupConfig
     *
     * @returns             @ref telux::common::ErrorCode success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode cleanupSettings(CleanupConfig config) = 0;

    /*
     * This API allows the client to set AutoConnect configuration.
     *
     * - If client enables the AutoConnect configuration, the data call would automatically attempt
     * to connect.
     * - If client disables the AutoConnect configuration, and no one else has requested connect
     * the data call, the data call would be down.
     *
     * - If persistent flag set and enable flag is true, on next powerups the data call would up
     * if condition meet, if persistent flag set and enable flag is false, on next powerups the
     * data call would NOT up.
     * - If persistent flag not set, the behavior would not be persistent, on next powerups
     * autoconnect behavior would be same with previous settings.
     *
     * It can be set for only one profile at a time. If called 2nd time for same profile, it will
     * override previous value for that particular profile.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_SETTING
     * permission to successfully invoke this API.
     *
     * @param [in] settings      specify the new settings for auto connect.
     *
     * @returns Status of setAutoConnect i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status setAutoConnect(const AutoConnectSettings &settings) = 0;

    /**
     * Request the current auto connect configuration for specified profile.
     *
     * @param [in]  profile   parameters to specify which profile it request.
     * @param [out] enable    result indicate if auto connect enabled.
     *
     * @returns Status of requestAutoConnect i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestAutoConnect(
        const AutoConnectProfile &profile, bool &enable) = 0;

    /**
     * Register Data Settings Manager as listener for Data Service heath events like data service
     * available or data service not available.
     *
     * @param [in] listener    pointer of IDataSettingsListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IDataSettingsListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IDataSettingsListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IDataSettingsListener> listener) = 0;
};

/**
 * Interface for Data Settings listener object. Client needs to implement this interface to get
 * access to Data Settings services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
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
     * This function is called when WWAN backhaul connectivity config changes.
     *
     * @param [in] slotId                - Slot Id for which connectivity has changed.
     * @param [in] isConnectivityAllowed - Connectivity status allowed/disallowed.
     *
     */
    virtual void onWwanConnectivityConfigChange(SlotId slotId, bool isConnectivityAllowed) {}

    /**
     * Destructor for IDataSettingsListener
     */
    virtual ~IDataSettingsListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif
