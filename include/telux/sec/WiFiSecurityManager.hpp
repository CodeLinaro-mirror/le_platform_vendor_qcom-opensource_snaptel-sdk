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
 *  Security analysis result for a given access point (AP).
 */
enum class AnalysisResult {

    /**
     *  There was no result for this AP because either the device is moving
     *  or the AP is on the fringes of signal strength.
     */
    NO_RESULT,

    /**
     *  This is the first time this AP is used for a connection and no previous
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
 *  Machine learning algorithm threat analysis result per AP.
 */
struct MLAlgorithmAnalysis {

    /**
     *  Higher threat scores indicate a higher possibility that
     *  the AP is malicious; range is 0 to 100.
     */
    uint32_t threatScore;

    /**
     *  Result of the security analysis for a given AP.
     */
    AnalysisResult result;
};

/**
 *  Summoning attack threat analysis result.
 */
struct SummoningAnalysis {

    /**
     *  Result of the security analysis for a given AP.
     */
    AnalysisResult result;
};

/**
 *  Represents the security report for a Wi-Fi AP.
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
     *  True if devices can connect to this AP without authentication.
     */
    bool isOpenAP;

    /**
     *  Machine learning algorithm threat analysis result.
     */
    MLAlgorithmAnalysis mlAlgorithmAnalysis;

    /**
     *  Summoning attack threat analysis result.
     */
    SummoningAnalysis summoningAnalysis;
};

/**
 *  Represents information about a deauthentication attack.
 */
struct DeauthenticationInfo {

    /**
     *  Reason code why disassociation or deauthentication occurred
     *  as specified by the IEEE 802.11 standard.
     */
    int deauthenticationReason;

    /**
     *  True if the AP initiated the disconnection.
     */
    bool didAPInitiateDisconnect;

    /**
     *  Higher threat scores indicate a higher possibility that this
     *  is a deauthentication attack; range is 0 to 100.
     */
    uint32_t threatScore;
};

/**
 *  Represents a WiFi access point.
 */
struct ApInfo {
    /**
     *  Network interface name of the AP.
     */
    std::string ssid;

    /**
     *  MAC address of the AP.
     */
    std::string bssid;
};

/**
 *  Receives security analysis reports for the Wi-Fi APs detected while
 *  scanning for APs in the vicinity and provides a listener for deauthentication
 *  attacks.
 */
class IWiFiReportListener {

 public:
    /**
     * Notifies that the implementation completed a threat analysis and that the report is available
     * This analysis is performed at various triggers, for example, when a scan for APs is triggered
     * the implementation will perform an analysis and provide a report for every AP it sees
     * in the vicinity.
     *
     * @param[in] report @ref WiFiSecurityReport result of the Wi-Fi security analysis.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onReportAvailable(WiFiSecurityReport report) { }

    /**
     * Notifies that a deauthentication attack is identified.
     *
     * @param[in] deauthenticationInfo @ref DeauthenticationInfo security analysis information.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onDeauthenticationAttack(DeauthenticationInfo deauthenticationInfo) { }

    /**
     * Gets user confirmation that the given AP is trusted. This is called only once
     * when the device connects to this AP for the very first time. If the application
     * trusts the given AP, it should set 'isTrusted' to True. Otherwise it should be set to false.
     *
     * Once the users confirms that an AP is trusted, this information is saved internally
     * and used later to detect threats like evil twin attacks.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_CONFIG
     * permission to successfully invoke this API.
     *
     * @param[in] accessPoint @ref ApInfo provides information about an AP.
     *
     * @param[in] isTrusted True if trusted; false otherwise.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void isTrustedAP(ApInfo accessPoint, bool& isTrusted) { }

    /**
     * IWiFiReportListener destructor.
     */
    virtual ~IWiFiReportListener() { }
};

/**
 * Provides support for detecting, monitoring, and generating security reports for
 * Wi-Fi APs.
 */
class IWiFiSecurityManager {

 public:

   /**
    * Registers the given listener to receive Wi-Fi connection security reports. These
    * reports will be received by @ref IWiFiReportListener::onReportAvailable().
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_REPORT
    * permission to successfully invoke this API.
    *
    * @param [in] reportListener Receives security reports.
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the listener is registered,
    *          otherwise, an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode registerListener(
        std::weak_ptr<IWiFiReportListener> reportListener) = 0;

   /**
    * Unregisters the given listener registered previously with @ref registerListener().
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_REPORT
    * permission to successfully invoke this API.
    *
    * @param [in] reportListener Listener to unregister.
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the listener is deregistered,
    *          otherwise, an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode deregisterListener(
        std::weak_ptr<IWiFiReportListener> reportListener) = 0;

   /**
    * Lists all the trusted APs.
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_INFO
    * permission to successfully invoke this API.
    *
    * @param [in] trustedAPList List of trusted APs ( @ref ApInfo ).
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the list is retrived otherwise,
    *          an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode getTrustedApList(std::vector<ApInfo>& trustedAPList) = 0;

   /**
    * Removes the given AP from the saved list of trusted APs. If the device connects to the same
    * AP again, @ref IWiFiReportListener::isTrustedAP() will be invoked again.
    *
    * On platforms with access control enabled, the caller needs to have the TELUX_SEC_WCS_CONFIG
    * permission to successfully invoke this API.
    *
    * @param [in] apInfo AP to distrust ( @ref ApInfo ).
    *
    * @returns @ref telux::common::ErrorCode::SUCCESS, if the AP is distrusted otherwise,
    *          an appropriate error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ErrorCode removeApFromTrustedList(ApInfo apInfo) = 0;

   /**
    * IWiFiSecurityManager destructor; cleans up as applicable.
    */
   virtual ~IWiFiSecurityManager() {};
};

/** @} */  // end_addtogroup telematics_sec_mgmt

}  // End of namespace sec
}  // End of namespace telux

#endif  // TELUX_SEC_WIFISECURITYMANAGER_HPP
