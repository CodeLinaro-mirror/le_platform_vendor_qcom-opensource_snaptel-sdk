/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file  WiFiSecurityManager.hpp
 * @brief WiFiSecurityManager provides support for detecting, monitoring and
 *        generating security analysis reports for WiFi connections.
 */

#ifndef TELUX_SEC_WIFISECURITYMANAGER_HPP
#define TELUX_SEC_WIFISECURITYMANAGER_HPP

#include <cstdint>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 *  Result of the security analysis for a given AP.
 */
enum class AnalysisResult {

    /**
     *  There was no result for this AP because either the device is moving
     *  or the AP is on the fringes of signal strength.
     */
    NO_RESULT,

    /**
     *  This is the first time this AP is used for connection. No previous
     *  references exist.
     */
    NEW_ASSOCIATION,

    /**
     *  The AP appears safe.
     */
    NO_THREAT_DETECTED,

    /**
     *  The AP is not safe.
     */
    MALICIOUS
};

/**
 *  Result of the threat analysis done by the machine learning algorithm
 *  per access point.
 */
struct MLAlgorithmAnalysis {

    /**
     *  The higher the score higher the possibility this is a malicious AP.
     *  Range is 0 to 100.
     */
    uint32_t threatScore;

    /**
     *  Result of the security analysis for a given AP.
     */
    AnalysisResult result;
};

/**
 *  Threat analysis result from summoning attack's perspective.
 */
struct SummoningAnalysis {

    /**
     *  Result of the security analysis for a given AP.
     */
    AnalysisResult result;
};

/**
 *  Represents security report for a WiFi access point.
 */
struct WiFiSecurityReport {

    /**
     *  Network interface name of the AP.
     */
    std::string ssid;

    /**
     *  MAC address of the AP.
     */
    std::string bssid;

    /**
     *  True if the device is connected to this AP.
     */
    bool isConnectedToAP;

    /**
     *  True if any device can connect to this AP without authentication.
     */
    bool isOpenAP;

    /**
     *  Result of the threat analysis done by the machine learning algorithm.
     */
    MLAlgorithmAnalysis mlAlgorithmAnalysis;

    /**
     *  Threat analysis result from summoning attack's perspective.
     */
    SummoningAnalysis summoningAnalysis;
};

/**
 *  Represents information about a deauthentication attack.
 */
struct DeauthenticationInfo {

    /**
     *  Reason code why disassociation or deauthentication occurred as specified
     *  by the IEEE 802.11 standard.
     */
    int deauthenticationReason;

    /**
     *  True if the AP initiated the disconnection.
     */
    bool didAPInitiateDisconnect;

    /**
     *  The higher the score higher the possibility this is a deauthentication
     *  attack. Range is 0 to 100.
     */
    uint32_t threatScore;
};

/**
 *  Receives security analysis reports for the WiFi access points detected during
 *  scanning for APs in the vicinity. It also provides listener for deauthentication
 *  attack.
 */
class IWiFiReportListener {

 public:
    /**
     * This is invoked when the implementation completes a threat analysis. This analysis
     * is performed at various triggers, for e.g. when a scan for APs is triggered the
     * implementation will perform an analysis and provide a report for every AP it sees
     * in the vicinity.
     *
     * @param[in] report @Ref WiFiSecurityReport result of the WiFi security analysis
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onReportAvailable(WiFiSecurityReport report) { }

    /**
     * Invoked to inform a deauthentication attack is mounted.
     *
     * @param[in] deauthenticationInfo @Ref DeauthenticationInfo security analysis information
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onDeauthenticationAttack(DeauthenticationInfo deauthenticationInfo) { }

    /**
     * Invoked to confirm from user that the given AP is trusted. This is called only once
     * when the device connects to this AP for the very first time. If the application
     * trusts given access point, it should set 'isTrusted' to true otherwise false.
     *
     * Once the users confirms that an AP is trusted, this information will be used by
     * the implementation on future connections and scans to detect threats like evil
     * twin attack.
     *
     * On platforms with access control enabled, caller needs to have TELUX_SEC_WCS_CONFIG
     * permission to invoke this API successfully.
     *
     * @param[in] isTrusted true if trusted false otherwise
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void isTrustedAP(std::string ssid, bool& isTrusted) { }

    /**
     * Destructor for IWiFiReportListener.
     */
    virtual ~IWiFiReportListener() { }
};

/**
 * Provides support for detecting, monitoring and generating security report for
 * WiFi access points.
 */
class IWiFiSecurityManager {

 public:

   /**
    * Registers given listener to receive WiFi connection security report.
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_WCS_REPORT
    * permission to invoke this API successfully.
    *
    * @ref IWiFiReportListener::onWiFiReportAvailable()
    *
    * @param [in] reportListener Receives security reports
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the listener is registered,
    *          otherwise, an appropriate error code
    */
   virtual telux::common::ErrorCode registerListener(
        std::weak_ptr<IWiFiReportListener> reportListener) = 0;

   /**
    * Unregisters the given listener registered previously with @ref registerListener().
    *
    * On platforms with access control enabled, caller needs to have TELUX_SEC_WCS_REPORT
    * permission to invoke this API successfully.
    *
    * @param [in] reportListener Listener to unregister
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the listener is deregistered,
    *          otherwise, an appropriate error code
    */
   virtual telux::common::ErrorCode deRegisterListener(
        std::weak_ptr<IWiFiReportListener> reportListener) = 0;

   /**
    * Destructor of IWiFiSecurityManager. Cleans up as applicable.
    */
   virtual ~IWiFiSecurityManager() {};
};

/** @} */  // end_addtogroup telematics_sec_mgmt

}  // End of namespace sec
}  // End of namespace telux

#endif  // TELUX_SEC_WIFISECURITYMANAGER_HPP
