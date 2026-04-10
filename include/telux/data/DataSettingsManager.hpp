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

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IDataSettingsListener;

using urlId = uint32_t;    /** URL identifier.
                               Unique identifier for a group of URLs defined in the system
                               configuration files. This ID is used to reference a specific list
                               of URLs and is immutable from the API. */

/**
 * Set priority between N79 5G and Wlan 5GHz Band
 */
enum class BandPriority {
    N79  = 0 ,              /** N79 has higher priority  */
    WLAN = 1 ,              /** Wlan has higher priority */
};

/**
 * Possible DDS switch types.
 */
enum class DdsType
{
    PERMANENT = 0, /** Permanently switch the DDS SIM Slot. Intended to be used when the client
                       wants to stop data activities on the current DDS SIM slot and start
                       doing data activities on the other SIM slot, on a Dual SIM Dual Standby
                       (DSDS) device. Permanent switch is persistent across reboots. */
    TEMPORARY = 1, /** Temporarily switch the DDS SIM Slot. This is only to be used when there
                       is a voice call on the non-DDS SIM slot and the client wants to temporarily
                       perform data activity on that non-DDS SIM slot, for the duration of the
                       call. After the call ends, clients should do a permanent switch back to the
                       original DDS SIM. Temporary switch is not persistent across reboots. */
};

/**
 * Specifies the DDS switch information.
 */
struct DdsInfo
{
    DdsType type;   /** Specifies DDS switch type */
    SlotId slotId;  /** Specifies which slot is the DDS */
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

/**
 * Represents a mapping between a vector of URL IDs and its associated backhaul.
 *
 * Each URL ID corresponds to a distinct group of URLs defined in the system configuration files.
 * This structure specifies the destination backhaul to which traffic for the
 * associated URL IDs should be routed.
 */
struct UrlIdToBackhaulMapping {
    std::vector<urlId> urlIds;             /** List of URL IDs representing distinct URL groups. */

    BackhaulInfo     backhaul;             /** Destination backhaul to which the traffic for the URL
                                            ID should be routed. */
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
 * @param [in] enabled          True: MacSec is enabled, False: MacSec is disabled.
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

/**
 * This function is called in response to requestCurrentDds API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] currentState  Provides the current DDS status @ref telux::data::DdsInfo.
 * @param [in] error         Return code for whether the operation succeeded or failed.
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to change
 *          and could break backwards compatibility.
 */
using RequestCurrentDdsResponseCb = std::function<void(DdsInfo currentState,
    telux::common::ErrorCode error)>;

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
     * For the factory settings to take effect a reboot is required. Clients can choose if this API
     * invocation should reboot the system or the client would take responsibility of rebooting it.
     *
     * @param [in] operationType    @ref telux::data::OperationType
     * @param [in] callback         callback to get the response to restoreFactorySettings
     * @param [in] isRebootNeeded   true: System is automatically rebooted after reverting
     *                                    to factory settings
     *                              false: System is not rebooted after successful reset
     *
     * @returns Immediate status of restoreFactorySettings i.e. success or suitable status.
     *
     */
    virtual telux::common::Status restoreFactorySettings(OperationType operationType,
        telux::common::ResponseCallback callback = nullptr, bool isRebootNeeded = true) = 0;

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
     */
    virtual telux::common::Status requestBandInterferenceConfig(
        RequestBandInterferenceConfigResponseCb callback) = 0;

    /**
     * Allows the client to perform the DDS switch. Client has the option
     * to either select permanent or temporary switch.
     *
     * @param [in] request          Client has to provide the request
     *                              @ref telux::data::DdsInfo.
     *
     * @param [in] callback         Callback to get response for requestDdsSwitch.
     *                              Possible ErrorCode in @ref telux::common::ResponseCallback:
     *                              - If the DDS switch is performed succesfully
     *                                @ref telux::common::ErrorCode::SUCCESS
     *                              - If the DDS switch request is rejected
     *                                @ref telux::common::ErrorCode::OPERATION_NOT_ALLOWED
     *                                The following scenarios are example of when a switch
     *                                request will be rejected:
     *                                    1. Slot1 is permanent DDS and the client attempts to
     *                                       trigger a permanent DDS switch on slot 1.
     *                                    2. During an MT/MO voice call and the client attempts
     *                                       to trigger a permanent DDS switch.
     *                              - If the DDS switch is allowed but due to some reason DDS
     *                                switch failed @ref telux::common::ErrorCode::GENERIC_FAILURE
     *
     * @returns Status of requestDdsSwitch, i.e., success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestDdsSwitch(DdsInfo request,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request the current DDS slot information
     *
     * @param [in] callback      Callback to get response for requestCurrentDds.
     *
     * @returns Status of requestCurrentDds, i.e., success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestCurrentDds(RequestCurrentDdsResponseCb callback) = 0;

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
     *
     */
    virtual telux::common::Status requestWwanConnectivityConfig(SlotId slotId,
        requestWwanConnectivityConfigResponseCb callback) = 0;


    /**
     * Request device data usage monitoring status
     *
     * This function can be used to obtain the current status of device data usage monitoring.
     *
     * @returns    Returns true if data usage monitoring is enabled, else false.
     *
     */
    virtual bool isDeviceDataUsageMonitoringEnabled() = 0;

    /**
     * Allows the client to set the MacSec state.
     *
     * - If client enables the MacSec, post that the packets over the ethernet link
     *   will be encrypted.
     * - If client disables the MacSec, post that the packets over the ethernet link
     *   will not be encrypted.
     *
     * @param [in] enable          True: enable the MacSec, False: disable the MacSec.
     * @param [in] callback        Callback to get the setMacSecState response.
     *
     * @returns Status of setMacSecState, i.e., success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status setMacSecState(bool enable,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Requests the current MacSec state.
     *
     * @param [in] callback    callback to get response for requestMacSecState.
     *
     * @returns Status of requestMacSecState, i.e., success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     *
     */
    virtual telux::common::Status requestMacSecState(RequestMacSecSateResponseCb callback) = 0;

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
     * Sets the backhaul preference for a specific URL ID or all URL IDs.
     *
     * Enables dynamic reassignment of a backhaul to a URL ID. The URL ID represents a fixed
     * group of distinct URLs defined in the system configuration files. These mappings are
     * immutable post deployment, and individual URLs cannot be altered via any API.
     *
     * This configuration is persistent across reboots or subsystem restart (SSR).
     * @note The new mapping takes effect after the backhaul is restarted.
     *
     * @details The configuration is stored in the mobileap_urlset XML file, which includes the
     * following elements:
     * - UrlSetConfig : Encapsulates a single configuration set for a URL ID.
     * - UrlID: Unique identifier for a URL group.
     * - BackhaulInfo: Preferred backhaul.
     * - UrlList: URLs associated with the ID.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_SETTING permission
     * to invoke this API successfully.
     *
     * @param [in] urlToBackhaulMapping  @ref telux::data::UrlIdToBackhaulMapping structure.
     * @param [in] callback             Optional callback to get the response for
     *                                  setUrlIdToBackhaulMapping.
     *
     * @returns Status of setUrlIdToBackhaulMapping, i.e., success or applicable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */

    virtual telux::common::Status setUrlIdToBackhaulMapping(
        const UrlIdToBackhaulMapping &urlToBackhaulMapping,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Retrieves the current mapping of URL IDs to their configured backhauls.
     *
     * This API returns a list of mappings, where each entry represents a unique URL ID
     * with its corresponding destination backhaul.
     *
     * This API provides a snapshot of the persisted configuration and does not reflect
     * runtime state unless the backhaul has been restarted after a configuration update.
     *
     * @note The API currently supports filtering URL to backhaul mappings by backhaul type.
     *       Consequently, this API is limited to retrieving mappings for a specific backhaul type.
     *
     * @param [in] backhaulType               Backhaul for which to query URL ID binding.
     * @param [out] urlToBackhaulMappingList  A vector of UrlToBackhaulMapping structures. Each
     *                                        structure contains a URL ID and the associated
     *                                        backhaul information.
     *
     * @returns telux::common::ErrorCode indicating the outcome of the operation:
     *         - @ref telux::common::ErrorCode::SUCCESS if the operation is successful.
     *         - @ref telux::common::ErrorCode::GENERIC_FAILURE if the Operation failed due to
     *           an internal error.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getUrlIdToBackhaulMapping(
        const BackhaulType &backhaulType,
        std::vector<UrlIdToBackhaulMapping>& urlToBackhaulMappingList) = 0;

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
     * Provides the current DDS state and is called whenever a DDS switch occurs.
     *
     * @param [in] currentState      Provides the current DDS status.
     *                               - Slot ID on which the DDS switch occured.
     *                               - DDS switch type @ref telux::data::DdsType.
     */
    virtual void onDdsChange(DdsInfo currentState) {}

    /**
     * Destructor for IDataSettingsListener
     */
    virtual ~IDataSettingsListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif
