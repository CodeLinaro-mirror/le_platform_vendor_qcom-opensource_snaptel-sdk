/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IDataSettingsListener;

/**
 * Specifies backhaul types
 */
enum class BackhaulType {
    ETH           = 0  ,    /** Ethernet Backhaul        */
    USB           = 1  ,    /** USB Backhaul             */
    WLAN          = 2  ,    /** WLAN Backhaul            */
    WWAN          = 3  ,    /** WWAN Backhaul with default profile ID set by */
                            /** @ref telux::data::DataConnectionManager::setDefaultProfile  */
    BLE           = 4  ,    /** Bluetooth Backhaul       */
    MAX_SUPPORTED = 5  ,    /** Max Supported Backhauls  */
};

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
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Set backhaul preference for bridge0 (default bridge) traffic. Bridge0 Traffic routing to
     * backhaul will be attempted on first to least preferred.
     * For instance if backhaul vector contains ETH, USB, and WWAN, bridge0 traffic routing will be
     * attempted on ETH first, then USB and finally WWAN backhaul.
     * Configuration changes will be persistent across reboots.
     *
     * @param [in] backhaulPref     vector of @ref telux::data::BackhaulPref which contains the
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
     * Destructor for IDataSettingsListener
     */
    virtual ~IDataSettingsListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif
