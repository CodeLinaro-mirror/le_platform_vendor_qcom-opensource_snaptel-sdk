/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       WlanDeviceManager.hpp
 *
 * @brief      WlanDeviceManager is a primary interface for configuring WLAN (Wireless Local
 *             Area Network). it provide APIs for configuring WLAN connectivity.
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

/** @addtogroup telematics_wlan
 * @{ */

/**
 *@brief     WlanDeviceManager is a primary interface for configuring Wireless LAN.
 *           it provide APIs to enable, configure, activate, and modify modes
 */
class IWlanDeviceManager {
 public:
    /**
     * Checks the readiness status of wlan manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If wlan manager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If wlan manager is temporarily unavailable.
     *          SERVICE_FAILED       -  If wlan manager encountered an irrecoverable failure.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Enable or Disable Wlan Service.
     * Configurations set by @ref telux::wlan::IWlanDeviceManager::setMode must be completed before
     * enabling Wlan.
     * If any of configurations need to be changed after Wlan is enabled, this API must be called
     * with enable set to false followed by a call with enable set to true for the new
     * configurations to take effect.
     * Calling this API with enable, will start hostapd and wpa_supplicant daemons.
     * Further changes to hostapd and wpa_supplicant will require calling
     * @ref telux::wlan::IApInterfaceManager::manageApService and
     * @ref telux::wlan::IStaInterfaceManager::manageStaService respectively.
     * Client shall wait for @ref IWlanListener::onEnableChanged indication to confirm WLAN was
     * enabled/disabled successfully
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
     * Set Wlan mode - number of supported APs, and stations.
     * This API shall be called when wlan is disabled. On enablement, wlan will enable APs and
     * Stations set in this API.
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
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual  telux::common::ErrorCode setMode(int numOfAp, int numOfSta) = 0;

    /**
     * Request Wlan configuration: Returns the configuration that was set using
     * @ref telus::wlan::IWlanDeviceManager::setMode.
     * This might differ from what configuration is has actually been enabled in the system, for
     * instance, when the hardware cannot fully support the configuration that was set.
     * To get the status of current configuration an Wlan enablement,
     * @ref telux::wlan::IWlanDeviceManager::getStatus should be used.
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
