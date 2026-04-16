/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       WlanDeviceManager.hpp
 *
 * @brief      WlanDeviceManager is a primary interface for configuring Wlan (Wireless Local
 *             Area Network). it provide APIs for configuring Wlan connectivity.
 *
 */

#ifndef TELUX_WLAN_WLANDEVICEMANAGER_HPP
#define TELUX_WLAN_WLANDEVICEMANAGER_HPP

#include <memory>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDefines.hpp>


namespace telux {
namespace wlan {

/** @addtogroup telematics_wlan
 * @{ */

//Forward declaration
class IWlanListener;

/**
 * Wlan Interface State
 */
enum class InterfaceState {
    INACTIVE  = 0x00,   /**<  Interface is Inactive  */
    ACTIVE    = 0x01,   /**<  Interface is Active    */
};

/**
 * Wlan Interface Device
 */
enum class HwDeviceType {
    UNKNOWN   = 0,                  /**<  Wlan device is Unknown   */
    QCA6574   = 1,                  /**<  Wlan device is QCA6574   */
    QCA6696   = 2,                  /**<  Wlan device is QCA6696   */
    QCA6595   = 3,                  /**<  Wlan device is QCA6595   */
};

/**
 * Wlan Interface status
 */
struct InterfaceStatus {
    HwDeviceType     device;                    /**> WiFi hardware type           */
    std::vector<ApStatus>  apStatus;            /**< Vector of active APs status  */
    std::vector<StaStatus> staStatus;           /**< Vector of active Sta status  */
};

/**
 * Configuration for setting Wlan mode with extended parameters.
 */
struct ModeConfig {
    int numOfAp = 0;                    /**< Number of Access Points to be enabled.
                                            If no Access Point is intended to be enabled, this
                                            argument should be set to 0. Configuration of each
                                            AP is accomplished through
                                            @ref telux::wlan::getApInterfaceManager() instance
                                            requested from factory. */
    int numOfSta = 0;                   /**< Number of Stations to be enabled. If no station is
                                            enabled, this argument should be set to 0.
                                            Configuration of each Station is accomplished
                                            through @ref telux::wlan::getStaInterfaceManager()
                                            instance requested from factory. */
    bool isPersistent = false;          /**< If true, mode configuration persists across
                                             Wlan restarts. */
    bool updateImmediately = false;     /**< If true, changes apply without Wlan restart */
};

/** @addtogroup telematics_wlan
 * @{ */

/**
 *@brief     WlanDeviceManager is a primary interface for configuring Wireless LAN.
 *           it provide APIs to enable, configure, activate, and modify modes
 */
class IWlanDeviceManager {
 public:
    /**
     * Checks the readiness status of Wlan manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If Wlan manager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If Wlan manager is temporarily unavailable.
     *          SERVICE_FAILED       -  If Wlan manager encountered an irrecoverable failure.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Enable or Disable Wlan Service.
     * Persistent mode configurations set by @ref telux::wlan::IWlanDeviceManager::setMode must be
     * completed before enabling Wlan. Dynamic mode configurations can be applied without requiring
     * Wlan restart.
     * If any of the persistent mode configurations need to be changed after Wlan is enabled, this
     * API must be called with enable set to false followed by a call with enable set to true
     * for the new mode configurations to take effect.
     * Calling this API with enable, will start hostapd and wpa_supplicant daemons.
     * Further changes to hostapd and wpa_supplicant will require calling
     * @ref telux::wlan::IApInterfaceManager::manageApService and
     * @ref telux::wlan::IStaInterfaceManager::manageStaService respectively.
     * Client shall wait for @ref IWlanListener::onEnableChanged indication to confirm Wlan was
     * enabled/disabled successfully
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_DEVICE_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] enable            true : Enable Wlan, false: Disable Wlan.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual  telux::common::ErrorCode enable(bool enable) = 0;

    /**
     * Configure Wlan operating mode with dynamic update capability.
     * This API handles two distinct mode configuration types with different behaviors:
     *
     * 1. Persistent mode configuration (@ref telux::wlan::ModeConfig::isPersistent = true):
     *    - Is persistent across Wlan restarts.
     *    - Retrieved by @ref telux::wlan::IWlanDeviceManager::getConfig().
     *    - Applied during Wlan enablement.
     *
     * 2. Dynamic mode configuration (@ref telux::wlan::ModeConfig::updateImmediately = true):
     *    - Active only during current session.
     *    - Retrieved by @ref telux::wlan::IWlanDeviceManager::getCurrentConfig().
     *    - Takes effect immediately without Wlan restart.
     *    - Lost on Wlan restart unless also saved as persistent.
     *
     * Behavior depends on Wlan state:
     *
     * When Wlan is disabled:
     *   - Persistent mode configuration can be set
     *     (@ref telux::wlan::ModeConfig::isPersistent = true).
     *   - Dynamic changes (@ref telux::wlan::ModeConfig::updateImmediately = true)
     *     return INVALID_OPERATION.
     *   - Mode Configuration is applied on next Wlan enablement.
     *
     * When Wlan is enabled:
     *   - If updateImmediately=true:
     *       * Changes take effect immediately without restart.
     *       * If @ref telux::wlan::ModeConfig::isPersistent = true, mode configuration is also
     *         saved so that it takes effect on the next start of Wlan.
     *       * If @ref telux::wlan::ModeConfig::isPersistent = false, changes only
     *         apply while the Wlan is active. When Wlan is disabled and enabled again, it will
     *         revert to persistent config/mode.
     *   - If @ref telux::wlan::ModeConfig::updateImmediately=false:
     *       * Requires Wlan restart for the changes to take effect.
     *       * If @ref telux::wlan::ModeConfig::isPersistent = true, saved mode configuration
     *         will apply on next enable.
     *
     * This API executes synchronously. The update is atomic. Any failure results in a rollback of
     * the WLAN configuration to its original state.
     *
     * @param [in] config     Mode configuration parameters @ref telux::wlan::ModeConfig
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_DEVICE_CONFIG
     * permission to invoke this API successfully.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode setMode(const ModeConfig &config) = 0;

    /**
     * Retrieve the persistent Wlan mode configuration.
     *
     * This API returns the mode configuration that persists across Wlan restarts,
     * as set using @ref telux::wlan::IWlanDeviceManager::setMode with isPersistent=true.
     *
     * @note This may differ from the mode configuration currently active in the system. To check
     * the current Wlan status and enablement, use @ref telux::wlan::IWlanDeviceManager::getStatus.
     *
     * @param [in] numAp                Num of configured APs
     * @param [in] numSta               Num of configured Stations
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode getConfig(int& numAp, int& numSta) = 0;

    /**
     * Retrieve the current Wlan mode configuration.
     *
     * This API returns the mode configuration that is currently active in the system:
     * - If called immediately after Wlan is enabled, it reflects the persistent mode configuration
     *   set via @ref telux::wlan::IWlanDeviceManager::setMode with
     *   @ref telux::wlan::ModeConfig::isPersistent = true.
     * - If a dynamic mode update was applied while Wlan was already operational (i.e.,
     *   @ref telux::wlan::ModeConfig::updateImmediately = true), it reflects the most recently
     *   applied dynamic configuration.
     *
     * @note This reflects the number of APs and stations configured to be operational at runtime.
     * It does not reflect the live service state of individual APs or stations, To check
     * the current Wlan status and enablement, use @ref telux::wlan::IWlanDeviceManager::getStatus.
     *
     * @param [in] numAp                Num of configured APs
     * @param [in] numSta               Num of configured Stations
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode getCurrentConfig(int& numAp, int& numSta) = 0;

    /**
     * Request Wlan status: Return Wlan enablement status and Interface status of APs and Station
     * such as active/inactive,
     * network interface name and hardware device they are mapped to.
     * Results are valid only if Wlan is enabled.
     *
     * @param [in] isEnabled            true: Wlan is enabled. false: Wlan is Disabled.
     * @param [in] status               vector of interface status @ref InterfaceStatus.
      *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode getStatus(
        bool& isEnabled, std::vector<InterfaceStatus>& status) = 0;

    /**
     * Register a listener for specific events in the Wlan Manager
     *
     * @param [in] listener    pointer of IWlanListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IWlanListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IWlanListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::ErrorCode deregisterListener(std::weak_ptr<IWlanListener> listener) = 0;

    /**
     * Destructor for IWlanDeviceManager
     */
    virtual ~IWlanDeviceManager(){};

    /**
     * Set Wlan mode - number of supported APs, and stations.
     * This API shall be called when Wlan is disabled. On enablement, Wlan will enable APs and
     * Stations set in this API. The Wlan mode configured using this API is persistent.
     *
     * @param [in] numOfAp           Num of Access Points to be enabled. If no Access Point is
     *                               enabled, this argument should be set to 0.
     *                               Configuration of each AP is accomplished through
     *                               telux::data::wlan::IApManager instance requested from factory.
     * @param [in] numOfSta          Num of Stations to be enabled. If no station is enabled,
     *                               this argument should be set to 0.
     *                               Configuration of each Station is accomplished through
     *                               telux::data::wlan::IStaManager instance requested from factory.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_WLAN_DEVICE_CONFIG
     * permission to invoke this API successfully.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @deprecated   Use telux::wlan::setMode(const ModeConfig&)
     */
    virtual  telux::common::ErrorCode setMode(int numOfAp, int numOfSta) = 0;
};  // end of IWlanDeviceManager


class IWlanListener {
public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * This function is called when Wlan enablement has changed
     *
     * @param [in] enable     True: Wlan is enabled, False: Wlan is disabled
     */
    virtual void onEnableChanged(bool enable) {}

    virtual ~IWlanListener() {}
};

/** @} */ /* end_addtogroup telematics_wlan */
}
}
#endif
