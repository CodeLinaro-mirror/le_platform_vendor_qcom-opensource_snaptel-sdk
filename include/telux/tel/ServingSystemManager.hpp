/*
 *  Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ServingSystemManager.hpp
 *
 * @brief      Serving System Manager class provides the interface to request and set
 *             service domain preference, radio access technology mode preference for
 *             searching and registering (CS/PS domain, RAT and operation mode), network
 *             time(including SIB16 and SIB9), RF band info, RF band preferences and
 *             capability, ENDC and DCNR information, network rejection information,
 *             LTE CS capability, SMS capability over network, call barring information,
 *             HPLMN time and radio resource control(RRC) state.
 */

#ifndef TELUX_TEL_SERVINGSYSTEMMANAGER_HPP
#define TELUX_TEL_SERVINGSYSTEMMANAGER_HPP

#include <bitset>
#include <future>
#include <memory>
#include <array>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/ServingSystemDefines.hpp>

namespace telux {
namespace tel {

// Forward declaration
class IServingSystemListener;

/** @addtogroup telematics_serving_system
 * @{ */

/**
 * Defines service domain preference
 */
enum class ServiceDomainPreference {
    UNKNOWN = -1,
    CS_ONLY, /**< Circuit-switched only */
    PS_ONLY, /**< Packet-switched only */
    CS_PS, /**< Circuit-switched and packet-switched */
};

/**
 * Defines service domain
 */
enum class ServiceDomain {
    UNKNOWN = -1, /**< Unknown, when the information is not available */
    NO_SRV, /**< No Service */
    CS_ONLY, /**< Circuit-switched only */
    PS_ONLY, /**< Packet-switched only */
    CS_PS, /**< Circuit-switched and packet-switched */
    CAMPED, /**< Device camped on the network according to its provisioning, but not
                 registered */
};

/**
 * Defines service registration state for serving RAT.
 */
enum class ServiceRegistrationState {
    UNKNOWN = -1, /**< Unknown, when the service registration information is not available */
    NO_SERVICE, /**< No service. */
    LIMITED_SERVICE, /**< Limited service. */
    IN_SERVICE, /**< In service. */
    LIMITED_REGIONAL, /**< Limited regional service. */
    POWER_SAVE, /**< Power save */
};

/**
 * @brief Specifies the domain preference for voice call.
 *
 * Voice calls can be established using either the Circuit-Switched (CS)
 * domain or the Packet-Switched (PS) domain (for example, VoLTE or VoNR).
 * For preferred modes (CS_PREF and PS_PREF), the modem may attempt to use
 * the alternate domain when the preferred domain is unavailable.
 */
enum class VoiceDomainPreference {
    UNKNOWN = -1, /**< Voice domain preference is unavailable or not yet determined. */
    CS_ONLY, /**< Circuit-Switched (CS) voice only; PS voice (e.g. VoLTE) is not used. */
    PS_ONLY, /**< Packet-Switched (PS) voice only; CS fallback (CSFB) is not used. */
    CS_PREF, /**< CS preferred; PS voice used as secondary if CS is unavailable. */
    PS_PREF, /**< PS preferred (e.g. VoLTE/VoNR); CS used as fallback if PS is unavailable. */
};

/**
 * @brief Specifies the DRX (Discontinuous Reception) cycle duration.
 *
 * DRX allows the modem to periodically disable receiver activity
 * between paging or downlink monitoring occasions to reduce power
 * consumption. The configured cycle duration defines the interval
 * between these monitoring periods. Longer cycles provide greater
 * power savings but may increase downlink responsiveness and paging
 * latency.
 */
enum class DrxConfig {
    UNKNOWN       = 0x00, /**< DRX cycle is unspecified or unavailable. */
    CYCLE_320_MS  = 0x06, /**< Long DRX cycle of  320 ms. */
    CYCLE_640_MS  = 0x07, /**< Long DRX cycle of  640 ms. */
    CYCLE_1280_MS = 0x08, /**< Long DRX cycle of 1280 ms. */
    CYCLE_2560_MS = 0x09, /**< Long DRX cycle of 2560 ms (maximum power saving). */
};

/**
 * Defines current serving system information
 */
struct ServingSystemInfo {
    RadioTechnology rat; /**< Current serving RAT */
    ServiceDomain domain; /**< Current service domain registered on system for the serving RAT */
    ServiceRegistrationState state; /**< Current service registration state of the serving RAT */
};

/**
 * Defines information of RF bands.
 */
struct RFBandInfo {
    RFBand band; /**< Currently active band */
    uint32_t channel; /**< Currently active channel */
    RFBandWidth bandWidth; /**< Bandwidth information */
};

/**
 * Defines the radio access technology mode preference.
 */
enum RatPrefType {
    PREF_CDMA_1X, /**< CDMA_1X */
    PREF_CDMA_EVDO, /**< CDMA_EVDO */
    PREF_GSM, /**< GSM */
    PREF_WCDMA, /**< WCDMA */
    PREF_LTE, /**< LTE */
    PREF_TDSCDMA, /**< TDSCDMA */
    PREF_NR5G, /**< NR5G in SA or NSA mode */
    PREF_NR5G_NSA, /**< NSA mode of NR5G only. SA is not allowed */
    PREF_NR5G_SA, /**< SA mode of NR5G only. NSA is not allowed */
    PREF_NB1_NTN /**< NB-IoT(NB1) Non Terrestrial Network(NTN) */
};

/**
 * Defines ENDC(E-UTRAN New Radio-Dual Connectivity) Availability status on 5G NR
 */
enum class EndcAvailability {
    UNKNOWN = -1, /**< Status unknown */
    AVAILABLE, /**< ENDC is Available */
    UNAVAILABLE, /**< ENDC is not Available */
};

/**
 * Defines DCNR(Dual Connectivity with NR) Restriction status on 5G NR
 */
enum class DcnrRestriction {
    UNKNOWN = -1, /**< Status unknown */
    RESTRICTED, /**< DCNR is Rescticted */
    UNRESTRICTED, /**< DCNR is not Restricted */
};

/**
 * Defines Dual Connectivity status
 */
struct DcStatus {
    EndcAvailability endcAvailability; /**< ENDC availability */
    DcnrRestriction dcnrRestriction; /**< DCNR restriction */
};

/**
 * Defines Network time information
 */
struct NetworkTimeInfo {
    uint16_t year; /**< Year. */
    uint8_t month; /**< Month. 1 is January and 12 is December. */
    uint8_t day; /**< Day. Range: 1 to 31.  */
    uint8_t hour; /**< Hour. Range: 0 to 23. */
    uint8_t minute; /**< Minute. Range: 0 to 59. */
    uint8_t second; /**< Second. Range: 0 to 59. */
    uint8_t dayOfWeek; /**< Day of the week. 0 is Monday and 6 is Sunday. */
    int8_t timeZone; /**< Offset between UTC and local time in units of 15 minutes (signed value).
                        Actual value = field value * 15 minutes. */
    uint8_t dstAdj; /**< Daylight saving adjustment in hours to obtain local time. Possible values:
                       0, 1, and 2.*/
    std::string nitzTime; /**< Network Identity and Time Zone(NITZ) information in the form
                             "yyyy/mm/dd,hh:mm:ss(+/-)tzh:tzm,dt */
};

/**
 * Defines network registration reject information
 */
struct NetworkRejectInfo {
    ServingSystemInfo rejectSrvInfo; /**< Serving system information where the registration is
                                        rejected.*/
    uint8_t rejectCause; /**< Reject cause values as specified in 3GPP TS 24.008, 3GPP TS 24.301 and
                            3GPP TS 24.501. */
    std::string mcc; /**< Mobile Country Code for rejection*/
    std::string mnc; /**< Mobile Network Code for rejection*/
};

/**
 * Specifies the LTE call type for which the RSRP offset is applied in connected mode.
 */
enum class LteCallType {
    NONE         = 0, /**< None */
    ALL_LTE_CALL = 1, /**< Applies to LTE data and voice call */
    VOLTE_CALL   = 2, /**< Applies to VoLTE calls only */
};

/**
 * Defines the LTE connected-mode serving-cell RSRP offset configuration for a specified
 * @ref LteCallType.
 */
struct LteConnectedModeConfig {
    LteCallType callType; /**< LTE call type for which the RSRP offset is applied in connected
                               mode */
    int32_t rsrqThreshold; /**< RSRQ threshold in dB. The RSRP offset is applied only when RSRQ
                                falls below this value */
    int32_t rsrpOffset; /**< Offset in dB subtracted from RSRP when both call type and RSRQ
                             threshold conditions are met */
};

/**
 * Number of DRX-cycle-length buckets in @ref telux::tel::LteIdleModeConfig::numSamples:
 * index 0 - 320 ms DRX cycle, 1 - 640 ms, 2 - 1280 ms, 3 - 2560 ms.
 */
static constexpr uint8_t LTE_ML1_IDLE_RSRP_N_DRX_CYCLES = 4;

/**
 * Defines LTE idle-mode RSRP offset optimization configuration
 */
struct LteIdleModeConfig {
    bool enabled; /**< Enables or disables the idle-mode RSRP offset optimization feature,
                       When disabled, all other fields in this structure are ignored */
    int8_t rsrqThreshold; /**< Idle-mode RSRQ threshold in dB. The RSRP offset is applied when UE
                             RSRQ falls below this value */
    uint8_t hysteresisDelta; /**< Hysteresis delta in dB added to rsrqThreshold before the offset
                                takes effect; prevents rapid toggling near the threshold boundary */
    uint8_t rsrpOffset; /**< RSRP offset in dB applied to idle-mode RSRP when enabled and the
                       threshold condition (rsrpThreshold and hysteresisDelta) is met */
    std::array<uint8_t, LTE_ML1_IDLE_RSRP_N_DRX_CYCLES> numSamples; /**< Number of RSRQ filter
                       samples per DRX cycle length */
};

/**
 * 16 bit mask that denotes which of the radio access technology mode preference
 * defined in RatPrefType enum are used to set or get RAT preference.
 */
using RatPreference = std::bitset<16>;

/**
 * Defines some of the notifications supported by @ref IServingSystemListener which can be
 * dynamically disabled/enabled. Each entry represents one or more listener callbacks in
 * @ref IServingSystemListener
 */
enum ServingSystemNotificationType {
    SYSTEM_INFO, /* Represents @ref telux::tel::IServingSystemListener::onSystemInfoChanged,
                    @ref telux::tel::IServingSystemListener::onDcStatusChanged,
                    @ref IServingSystemListener::onCallBarringInfoChanged,
                    @ref IServingSystemListener::onSmsCapabilityChanged,
                    @ref IServingSystemListener::onLteCsCapabilityChanged. */
    RF_BAND_INFO, /* Represents @ref telux::tel::IServingSystemListener::onRFBandInfoChanged */
    NETWORK_REJ_INFO, /* Represents @ref telux::tel::IServingSystemListener::onNetworkRejection */
    LTE_SIB16_NETWORK_TIME, /* Represents @ref
                               telux::tel::IServingSystemListener::onNetworkTimeChanged takes an
                               input parameter of LTE for telux::tel::RadioTechnology */
    NR5G_RRC_UTC_TIME, /* Represents @ref telux::tel::IServingSystemListener::onNetworkTimeChanged
                          takes an input parameter of NR5G for telux::tel::RadioTechnology */
    ARFCN_INFO /* Represents the Absolute Radio Frequency Channel Number (ARFCN)
                  information corresponding to the currently active RAT(s), currently, only
                  RRC state information is utilized @ref
                  telux::tel::IServingSystemListener::onRrcStateChanged */
};

/**
 * Bit mask that denotes a set of notifications defined in @ref ServingSystemNotificationType
 */
using ServingSystemNotificationMask = std::bitset<32>;

/**
 * Defines allowed call types supported by the network cell
 */
enum class CallsAllowedInCell {
    UNKNOWN = -1, /**< Unknown calls allowed */
    NORMAL_ONLY, /**< Only normal calls allowed */
    EMERGENCY_ONLY, /**< Only emergency calls allowed */
    NO_CALLS, /**< No calls allowed */
    ALL_CALLS, /**< All calls allowed */
};

/**
 * Defines call barring information.
 */
struct CallBarringInfo {
    RadioTechnology rat; /**< Current serving RAT */
    ServiceDomain domain; /**< Current service domain registered on the system for the serving RAT;
                             valid values are CS_ONLY and PS_ONLY*/
    CallsAllowedInCell callType; /**< Current allowed call type for the cell*/

    bool operator==(const CallBarringInfo &cb) const {
        return (cb.rat == rat) && (cb.domain == domain) && (cb.callType == callType);
    }
};

/**
 * Define SMS support over network for registered RAT. For NB-IoT(NB1) NTN RAT, use
 * @ref telux::tel::NtnSmsStatus
 */
enum class SmsDomain {
    UNKNOWN = -1, /**< Unknown, when the information is not available */
    NO_SMS, /**< Can't receive SMS */
    SMS_ON_IMS, /**< SMS is supported over IMS network */
    SMS_ON_3GPP, /**< SMS is supported over 3GPP network */
};

/**
 * Define SMS service status for NB-IoT(NB1) NTN RAT.
 */
enum class NtnSmsStatus {
    UNKNOWN = -1, /**< Unknown, when SMS service status for NTN is not available. */
    NOT_AVAILABLE, /**< SMS service status over CP is not available. */
    TEMP_FAILURE, /**< SMS service status over CP is not available temporarily. */
    AVAILABLE, /**< SMS service status over CP is available. */
};

/**
 * Define SMS capability for registered RAT.
 */
struct SmsCapability {
    RadioTechnology rat; /**< Current serving RAT */
    SmsDomain domain; /**< Supported SMS domain for currently registered RAT on the network, not
                         applicable for NB1 NTN RAT. */
    NtnSmsStatus smsStatus; /**< SMS service status for NB1 NTN RAT, not applicable for other RATs.
                             */
};

/**
 * Defines LTE CS service capabilities.
 */
enum class LteCsCapability {
    UNKNOWN = -1, /**< Unknown, when the information is not available */
    FULL_SERVICE, /**< Full service on CS domain is available */
    CSFB_NOT_PREFERRED, /**< CSFB is not preferred */
    SMS_ONLY, /**< CS registation is for SMS only */
    LIMITED, /**< CS registation failed for max attach or tracking area updating(TAU) attempts */
    BARRED, /**< CS domain not available */
};

/**
 * Defines NR types.
 */
enum class NrType {
    NSA = 0, /**<  NSA type of NR5G only. */
    SA, /**<  SA type of NR5G only. */
    COMBINED /**<  NSA and SA type of NR5G */
};

/**
 * Defines the radio resource control (RRC) state for WCDMA, LTE, NR5G SA and NSA RATs.
 * In NSA mode, both LTE and NR5G states are displayed. For GSM RAT, RRC state information
 * is not supported.
 */
struct RrcState {
    NetworkMode mode; /**< Network mode for RRC state. */
    union {
        WcdmaRrcState wcdmaRrcState; /**< WCDMA RRC state, @ref telux::tel::WcdmaRrcState. */
        LteRrcState lteRrcState; /**< LTE RRC state, @ref telux::tel::LteRrcState. */
        Nr5gRrcState nr5gRrcStateSa; /**< NR5G RRC state, @ref telux::tel::Nr5gRrcState. */
        struct {
            LteRrcState lteRrcState; /**< LTE RRC state, @ref telux::tel::LteRrcState. */
            Nr5gRrcState nr5gRrcState; /**< NR5G RRC state, @ref telux::tel::Nr5gRrcState. */
        } rrcStateForNsa;
    };
};

/**
 * @brief IRFBandList is used to retrieve or set the RF band preferences for all
 * RATs, and retrieve the supported RF band capabilities of the device.
 */
class IRFBandList {
 public:
    /**
     * @brief Set GSM RF bands
     *
     * @param [in] bands     List of GSM RF bands @ref telux::tel::GsmRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void setGsmBands(std::vector<GsmRFBand> bands) = 0;
    /**
     * @brief Set WCDMA RF bands
     *
     * @param [in] bands     List of WCDMA RF bands @ref telux::tel::WcdmaRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void setWcdmaBands(std::vector<WcdmaRFBand> bands) = 0;
    /**
     * @brief Set LTE RF bands
     *
     * @param [in] bands     List of LTE RF bands @ref telux::tel::LteRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void setLteBands(std::vector<LteRFBand> bands) = 0;
    /**
     * @brief Set NR5G RF bands
     *
     * @param [in] type     NR5G type @ref telux::tel::NrType
     * @param [in] bands    List of NR5G RF bands @ref telux::tel::NrRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void setNrBands(NrType type, std::vector<NrRFBand> bands) = 0;

    /**
     * @brief Check if specific GSM RF band is present
     *
     * @param [in] band    @ref telux::tel::GsmRFBand
     */
    virtual bool isGSMBandPresent(GsmRFBand band) = 0;
    /**
     * @brief Check if specific WCDMA RF band is present
     *
     * @param [in] band    @ref telux::tel::WcdmaRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual bool isWcdmaBandPresent(WcdmaRFBand band) = 0;
    /**
     * @brief Check if specific LTE RF band is present
     *
     * @param [in] band    @ref telux::tel::LteRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual bool isLteBandPresent(LteRFBand band) = 0;
    /**
     * @brief Check if specific NR5G RF band is present
     *
     * @param [in] type    NR5G type @ref telux::tel::NrType
     * @param [in] band    @ref telux::tel::NrRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual bool isNrBandPresent(NrType type, NrRFBand band) = 0;

    /**
     * @brief Get GSM RF bands
     *
     * @return   List of GSM RF bands @ref telux::tel::GsmRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual std::vector<GsmRFBand> getGsmBands() = 0;
    /**
     * @brief Get WCDMA RF bands
     *
     * @return   List of WCDMA RF bands @ref telux::tel::WcdmaRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual std::vector<WcdmaRFBand> getWcdmaBands() = 0;
    /**
     * @brief Get LTE RF bands
     *
     * @return   List of LTE RF bands @ref telux::tel::LteRFBand
     */
    virtual std::vector<LteRFBand> getLteBands() = 0;
    /**
     * @brief Retrieve NR5G RF bands based on @telux::tel::NrType. For example, use
     * @ref telux::tel::NrType::COMBINED to retrieve the supported NR5G RF band capabilities
     * of the device and @telux::tel::NrType::NSA to retrieve and set NSA RF band preferences.
     *
     * @param [in] type    NR5G type @ref telux::tel::NrType
     * @return             List of NR5G RF bands @ref telux::tel::NrRFBand
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual std::vector<NrRFBand> getNrBands(NrType type) = 0;
    /**
     * Destructor for IRFBandList
     */
    virtual ~IRFBandList() {
    }
};

/**
 * @brief RFBandListBuilder is used to build @ref telux::tel::IRFBandList.
 *
 * Add the desired RF bands for different RATs. After configuring the desired bands,
 * invoke the @ref telux::tel::RFBandListBuilder::build API to build @telux::tel::IRFBandList.
 */
class RFBandListBuilder {
 public:
    /**
     * @brief Constructs a RFBandListBuilder.
     */
    RFBandListBuilder();

    /**
     * @brief Construct desired GSM RF bands.
     *
     * @param [in] bands   List of GSM RF bands @ref telux::tel::GsmRFBand
     * @return             Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    RFBandListBuilder &addGsmRFBands(std::vector<GsmRFBand> bands);
    /**
     * @brief Construct desired WCDMA RF bands.
     *
     * @param [in] bands   List of WCDMA RF bands @ref telux::tel::WcdmaRFBand
     * @return             Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    RFBandListBuilder &addWcdmaRFBands(std::vector<WcdmaRFBand> bands);
    /**
     * @brief Construct desired LTE RF bands.
     *
     * @param [in] bands   List of LTE RF bands @ref telux::tel::LteRFBand
     * @return             Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    RFBandListBuilder &addLteRFBands(std::vector<LteRFBand> bands);
    /**
     * @brief Construct desired bands based on NrType.
     *
     * @param [in] type    NR5G type @ref telux::tel::NrType
     * @param [in] bands   List of NR5G RF bands @ref telux::tel::NrRFBand
     * @return             Reference to this builder for method chaining.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    RFBandListBuilder &addNrRFBands(NrType type, std::vector<NrRFBand> bands);
    /**
     * @brief This API builds the RF band list. If API call is successful, it will return an
     * instance of @ref telux::tel::IRFBandList and there is no error. Otherwise, it
     * will return nullptr and the error code.
     *
     * @param[out] errorCode @ref telux::tel::ErrorCode indicating any possible failure
     *                       reasons during construction.
     *
     * @return Shared pointer to the constructed RF band list.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    std::shared_ptr<IRFBandList> build(telux::common::ErrorCode &errorCode);

 private:
    std::shared_ptr<IRFBandList> rfBandList_;
};

/**
 * This function is called with the response to requestRatPreference API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] preference     @ref RatPreference
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not
 *                            @ref ErrorCode
 */
using RatPreferenceCallback
    = std::function<void(RatPreference preference, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestServiceDomainPreference
 * API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] preference   @ref ServiceDomainPreference
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not
 *                          @ref ErrorCode
 */
using ServiceDomainPreferenceCallback
    = std::function<void(ServiceDomainPreference preference, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestNetworkTime API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] info       @ref NetworkTimeInfo
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not @ref ErrorCode
 *
 */
using NetworkTimeResponseCallback
    = std::function<void(NetworkTimeInfo info, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestRFBandInfo API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] bandInfo     @ref RFBandInfo
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not
 *                          @ref ErrorCode
 *
 */
using RFBandInfoCallback = std::function<void(RFBandInfo bandInfo, telux::common::ErrorCode error)>;

/**
 * This function is called in response to the requestRFBandPreferences API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] prefList     @ref telux::tel::IRFBandList instance
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not @ref ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to
 *         change and could break backwards compatibility.
 */
using RFBandPrefCallback
    = std::function<void(std::shared_ptr<IRFBandList> prefList, telux::common::ErrorCode error)>;

/**
 * This function is called in response to the requestRFBandCapability API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] capabilityList     @ref telux::tel::IRFBandList instance
 * @param [in] error              Return code which indicates whether the operation
 *                                succeeded or not @ref ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to
 *         change and could break backwards compatibility.
 */
using RFBandCapabilityCallback = std::function<void(
    std::shared_ptr<IRFBandList> capabilityList, telux::common::ErrorCode error)>;

/**
 * This function is called in response to the requestRrcState API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] rrcState     @ref telux::tel::RrcState
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to
 *         change and could break backwards compatibility.
 */
using RrcStateCallback
    = std::function<void(telux::tel::RrcState rrcState, telux::common::ErrorCode error)>;

class IServingSystemManager {
 public:
    /**
     * Checks the status of serving subsystem and returns the result.
     *
     * @returns True if serving subsystem is ready for service otherwise false.
     *
     * @deprecated Use IServingSystemManager::getServiceStatus() API.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for serving subsystem to be ready.
     *
     * @returns  A future that caller can wait on to be notified when serving
     *           subsystem is ready.
     *
     * @deprecated Use InitResponseCb in PhoneFactory::getServingSystemManager instead, to
     *             get notified about subsystem readiness.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * This status indicates whether the IServingSystemManager object is in a usable state.
     *
     * @returns SERVICE_AVAILABLE    -  If Serving System manager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If Serving System manager is temporarily unavailable.
     *          SERVICE_FAILED       -  If Serving System manager encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Set the preferred radio access technology mode that the device should use
     * to acquire service.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] ratPref       Radio access technology mode preference.
     * @param [in] callback      Callback function to get the response of set RAT
     *                           mode preference.
     *
     * @returns Status of setRatPreference i.e. success or suitable error code.
     */
    virtual telux::common::Status setRatPreference(
        RatPreference ratPref, common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Request for preferred radio access technology mode.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [in] callback  Callback function to get the response of request
     *                       preferred RAT mode.
     *
     * @returns Status of requestRatPreference i.e. success or suitable error
     *          code.
     */
    virtual telux::common::Status requestRatPreference(RatPreferenceCallback callback) = 0;

    /**
     * Initiate service domain preference like CS, PS or CS_PS and receive the
     * response asynchronously.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] serviceDomain  @ref ServiceDomainPreference.
     *
     * @param [in] callback       Callback function to get the response of
     *                            set service domain preference request.
     *
     * @returns Status of setServiceDomainPreference i.e. success or suitable
     *          error code.
     */
    virtual telux::common::Status setServiceDomainPreference(
        ServiceDomainPreference serviceDomain, common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Request for Service Domain Preference.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [in] callback    Callback function to get the response of request
     *                         service domain preference.
     *
     * @returns Status of requestServiceDomainPreference i.e. success or suitable
     *          error code.
     */
    virtual telux::common::Status requestServiceDomainPreference(
        ServiceDomainPreferenceCallback callback)
        = 0;

    /**
     * @brief Sets the preferred voice domain used for voice calls.
     *
     * This configuration determines whether voice calls prefer the Circuit-Switched (CS) domain,
     * Packet-Switched (PS) domain (for example, VoLTE or VoNR), or a combination of both when
     * multiple voice domain options are available.
     *
     * @ref telux::tel::ServingSystemManager::setServiceDomainPreference defines the service domains
     * that are permitted (CS and/or PS) and takes precedence over Voice Domain Preference. If a
     * voice domain preference is configured for a domain that is not allowed by
     * ServiceDomainPreference, that domain will not be used.
     *
     * On platforms with access control enabled, the caller must have the
     * TELUX_TEL_SRV_SYSTEM_CONFIG permission to invoke this API.
     *
     * The configured preference persists across subsystem restart (SSR) and device reboot.
     *
     * @param[in] preference Desired voice domain preference. See @ref
     * telux::tel::VoiceDomainPreference.
     *
     * @return ErrorCode indicating the result of the operation. Returns an appropriate error code,
     * such as @ref telux::common::ErrorCode::INVALID_ARG, when an invalid voice domain preference
     * is specified.
     *
     * @note Eval: This is a new API under evaluation. It is subject to change and may not be
     * backward compatible.
     */
    virtual telux::common::ErrorCode setVoiceDomainPreference(VoiceDomainPreference preference) = 0;

    /**
     * Gets the currently configured Voice Domain Preference synchronously.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] preference  @ref telux::tel::VoiceDomainPreference, valid only if the return
     * value is @ref telux::common::ErrorCode::SUCCESS.
     *
     * @returns ErrorCode of getVoiceDomainPreference i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getVoiceDomainPreference(VoiceDomainPreference &preference)
        = 0;

    /**
     * Sets the DRX (Discontinuous Reception) cycle duration.
     *
     * On platforms with access control enabled, the caller must have the
     * TELUX_TEL_SRV_SYSTEM_CONFIG permission to invoke this API successfully.
     *
     * The DRX configuration is not persistent across SSR or device reboot.
     * It must be configured again after an SSR or reboot, if required.
     *
     * @param[in] config Requested DRX cycle duration. See @ref telux::tel::DrxConfig.
     *
     * @return ErrorCode indicating the result of the operation. Returns success
     *         if the DRX configuration is set successfully; otherwise, returns
     *         an appropriate error code, such as @ref telux::tel::ErrorCode::INVALID_ARG when an
     * invalid DRX configuration value is provided.
     *
     * @note Eval: This is a new API under evaluation. It is subject to
     *       change and may not be backward compatible.
     */
    virtual telux::common::ErrorCode setDrxConfig(DrxConfig config) = 0;

    /**
     * Gets the currently configured DRX cycle duration.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] config      @ref DrxConfig, valid only if the return value is
     *                          @ref telux::common::ErrorCode::SUCCESS.
     *
     * @return ErrorCode of getDrxConfig i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getDrxConfig(DrxConfig &config) = 0;

    /**
     * Get the Serving system information. Supports only 3GPP RATs.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] sysInfo  Serving system information
     *                       @ref ServingSystemInfo
     *
     * @returns Status of getServingSystemInfo i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status getSystemInfo(ServingSystemInfo &sysInfo) = 0;

    /**
     * Request for Dual Connectivity status on 5G NR.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @returns @ref DcStatus
     */
    virtual telux::tel::DcStatus getDcStatus() = 0;

    /**
     * Get network time information asynchronously.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [in] callback    Callback function to get the response of get
     *                         network time information request.
     *
     * @returns Status of requestNetworkTime i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status requestNetworkTime(NetworkTimeResponseCallback callback) = 0;

    /**
     * Retrieves the LTE(SIB16) network time from the UE.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [in] callback    Callback function to get the response of an LTE(SIB16)
     *                         network time information request.
     *
     * @returns Status of requestLteSib16NetworkTime i.e. success or suitable error code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status requestLteSib16NetworkTime(NetworkTimeResponseCallback callback)
        = 0;

    /**
     * Retrieves the NR5G RRC(SIB9) UTC time from the UE.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [in] callback    Callback function to get the response of NR5G
     *                         RRC(SIB9) UTC time information request.
     *
     * @returns Status of requestNr5gRrcUtcTime i.e. success or suitable error code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status requestNr5gRrcUtcTime(NetworkTimeResponseCallback callback) = 0;

    /**
     * Get the information about the band that the device is currently using.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @note The active bandwidth information @ref telux::tel::RFBandInfo::bandWidth is not
     * supported for NB1 NTN RAT.
     *
     * @param [in] callback    Callback function to get the response of get
     *                         RF band information request.
     *
     * @returns Status of requestRFBandInfo i.e. success or suitable error code.
     *
     */
    virtual telux::common::Status requestRFBandInfo(RFBandInfoCallback callback) = 0;

    /**
     * Get network registration reject information.
     * When a device is detached from the network due to registration rejection, the network
     * will return relevant information such as the reason for the rejection.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] rejectInfo  Network reject information @ref NetworkRejectInfo
     *
     * @returns Status of getNetworkRejectInfo i.e. success or suitable error code.
     *
     * @deprecated This API will not be supported in future releases.
     */
    virtual telux::common::Status getNetworkRejectInfo(NetworkRejectInfo &rejectInfo) = 0;

    /**
     * Gets the call barring information for the currently registered cell of a device.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] barringInfo  List of call barring information @ref CallBarringInfo
     *
     * @returns Status of getCallBarringInfo, i.e., success or a suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status getCallBarringInfo(std::vector<CallBarringInfo> &barringInfo) = 0;

    /**
     * Get the SMS capability over IMS/3GPP network for registered radio access technology (RAT).
     * @note telux::tel::SmsDomain is not applicable for NB-IoT(NB1) NTN, use
     * telux::tel::NtnSmsStatus for SMS capability on NB-IoT(NB1) NTN.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] smsCapability  SMS capability @ref telux::tel::SmsCapability
     *
     * @returns Status of getSmsCapabilityOverNetwork i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status getSmsCapabilityOverNetwork(SmsCapability &smsCapability) = 0;

    /**
     * Get the circuit-switched(CS) service capabilities of the LTE network.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] lteCapability  LTE CS capability @ref LteCsCapability
     *
     * @returns Status of getLteCsCapability i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status getLteCsCapability(LteCsCapability &lteCapability) = 0;

    /**
     * Request RF band preferences for all RATs except NB1 NTN.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [in] callback    Callback function to retrieve the response of get
     *                         RF band preferences request.
     *
     * @returns Status of requestRFBandPreferences i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status requestRFBandPreferences(RFBandPrefCallback callback) = 0;

    /**
     * Set the preferred RF band capabilities for the device to acquire service.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_CONFIG
     * permission to invoke this API successfully.
     *
     * @note This API is not supported for NB1 NTN RAT. To update bands preferences corresponding
     * for the NB1 NTN, use @ref telux::satcom::INtnManager::updateSystemSelectionSpecifiers
     *
     * @param [in] prefList      Use RFBandListBuilder to build @ref telux::tel::IRFBandList
     *                           instance.
     * @param [in] callback      Optional callback function to get the response of
     *                           set RF band preferences request.
     *
     * @returns  Status of setRFBandPreferences i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status setRFBandPreferences(
        std::shared_ptr<IRFBandList> prefList, common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Set the LTE RSRP offset configuration in connected mode.
     *
     * The configuration specifies the call scenario, RSRQ threshold, and RSRP
     * offset to be applied during LTE connected mode. The configured values
     * persist across subsystem restarts (SSR) and device reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] config        LTE connected-mode configuration.
     *                           @ref telux::tel::LteConnectedModeConfig
     *
     * @returns ErrorCode        indicating the status of the operation
     *                           i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode setLteConnectedModeRsrpConfig(
        const LteConnectedModeConfig config)
        = 0;

    /**
     * Get the LTE RSRP offset configuration in connected mode.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] config    Parameter to hold the fetched LTE connected-mode configuration.
     *                        @ref telux::tel::LteConnectedModeConfig
     *
     * @returns ErrorCode     indicating the status of the operation
     *                        i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getLteConnectedModeRsrpConfig(LteConnectedModeConfig &config)
        = 0;

    /**
     * Set the LTE idle-mode RSRP offset optimization configuration.
     * The configuration specifies the criteria and offset values used for LTE
     * idle-mode signal reporting optimization.
     * The configured values persist across subsystem restarts (SSR) and device reboots.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] config        LTE idle-mode optimization configuration.
     *                           @ref telux::tel::LteIdleModeConfig
     *
     * @returns ErrorCode        indicating the status of the operation
     *                           i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode setLteIdleModeRsrpConfig(const LteIdleModeConfig config) = 0;

    /**
     * Get the LTE RSRP offset optimization configuration in idle mode.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [out] config    Parameter to hold the fetched LTE idle-mode configuration.
     *                        @ref telux::tel::LteIdleModeConfig
     *
     * @returns ErrorCode     indicating the status of the operation
     *                        i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getLteIdleModeRsrpConfig(LteIdleModeConfig &config) = 0;

    /**
     * Request supported RF band capabilities for the device.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @note This API is not supported for NB1 NTN RAT.
     *
     * @param [in] callback    Callback function to retrieve the response of get
     *                         RF band capability request.
     *
     * @returns Status of requestRFBandCapability i.e. success or suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status requestRFBandCapability(RFBandCapabilityCallback callback) = 0;

    /**
     * Set the HPLMN search timer value, which determines the background scan rate for home
     * networks. After an SSR or reboot, the user-set value will revert to the default value and the
     * client will need to reset the timer value.
     *
     * On platforms with access control enabled, the caller needs to have
     * TELUX_TEL_SRV_SYSTEM_CONFIG permission to invoke this API successfully.
     *
     * @param [in] time           Indicates HPLMN search timer value in minutes.
     *                            Valid value range [0, 71582] and 0xFFFFFFFF. A timer value of
     *                            0xFFFFFFFF means use the SIM-defined timer. Any timer value less
     *                            than the minimum periodic search timer value configured in
     *                            EFNASCONFIG(Non Access Stratum Configuration) will be set to the
     *                            minimum value. If the timer value is set to 0 and timer value in
     *                            EFNASCONFIG is invalid, the search timer will not start. If HPLMN
     *                            timer is running and the remaining timer duration is greater than
     *                            then duration set then modem will stop and restart the timer with
     *                            the duration received in set request.
     *
     * @returns @ref telux::common::ErrorCode of setHplmnSearchTime i.e. success or suitable error
     *          code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode setHplmnSearchTime(uint32_t time) = 0;

    /**
     * Get the HPLMN search timer value.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to invoke this API successfully.
     *
     * @param [out] time        Indicates HPLMN search timer value in minutes.
     *
     * @returns @ref telux::common::ErrorCode of getHplmnSearchTime i.e. success or suitable error
     *          code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     *
     */
    virtual telux::common::ErrorCode getHplmnSearchTime(uint32_t &time) = 0;

    /**
     * Request radio resource control(RRC) state for WCDMA, LTE and NR5G RATs.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to successfully invoke this API.
     *
     * @param [in] callback    Callback function to retrieve the response of get
     *                         RRC state request.
     *
     * @returns Status of requestRrcState i.e. success or suitable error code
     * @ref telux::common::Status
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::Status requestRrcState(RrcStateCallback callback) = 0;

    /**
     * Configures notification filtering for @ref IServingSystemListener::onSystemInfoChanged,
     * @ref IServingSystemListener::onCallBarringInfoChanged, and
     * @ref IServingSystemListener::onSmsCapabilityChanged.
     *
     * When the SYSTEM_INFO notification type is enabled, all system information changes are
     * reported to registered listeners by default. This API allows clients to restrict
     * notifications to only the selected system information attributes defined by
     * @ref SystemInfoNotificationFilter.
     *
     * After a filter mask is applied, notifications are generated only when one or more of the
     * selected attributes change. For example, notifications may be triggered by service
     * registration state changes, service loss or recovery, RAT transitions, or other configured
     * system information updates.
     *
     * This mechanism helps reduce unnecessary notifications and device wakeups caused by
     * unrelated system information changes. It is particularly useful for power-sensitive
     * applications that are interested only in specific network state transitions.
     *
     * If @ref SystemInfoNotificationFilter::SRV_STATUS,
     * @ref SystemInfoNotificationFilter::SRV_STATUS_TOGGLE, or both are included in the filter
     * mask, @ref ServingSystemInfo updates are delivered through
     * @ref IServingSystemListener::onSystemInfoChanged based on the following rules:
     *
     * - @ref SystemInfoNotificationFilter::SRV_STATUS
     *   - Reports every service status transition within the same RAT, including transitions
     *     between NO_SERVICE, LIMITED_SERVICE, IN_SERVICE, LIMITED_REGIONAL, and POWER_SAVE.
     *   - Reports all service status transitions across RATs, including cases where the service
     *     status remains unchanged but the RAT changes.
     *
     * - @ref SystemInfoNotificationFilter::SRV_STATUS_TOGGLE
     *   - Reports only in-service/out-of-service boundary transitions:
     *       - NO_SERVICE, LIMITED_SERVICE, LIMITED_REGIONAL, or POWER_SAVE -> IN_SERVICE
     *       - IN_SERVICE -> NO_SERVICE, LIMITED_SERVICE, LIMITED_REGIONAL, or POWER_SAVE
     *   - Does not report transitions that do not cross the in-service boundary (for example,
     *     NO_SERVICE -> LIMITED_SERVICE or NO_SERVICE -> LIMITED_REGIONAL or NO_SERVICE ->
     *     POWER_SAVE).
     *
     * - When both filters are enabled, notifications are generated for all service status changes
     *   covered by either filter, including status transitions within the same RAT, and
     *   RAT changes.
     *
     * A filter is applied only when the SYSTEM_INFO notification type has been enabled through
     * @ref IServingSystemManager::registerListener.
     *
     * On platforms with access control enabled, the caller must have
     * TELUX_TEL_SRV_SYSTEM_CONFIG permission to invoke this API successfully.
     *
     * @param[in] mask    Bit mask specifying the @ref SystemInfoNotificationFilter attributes
     *                    that should trigger:
     *                    - @ref IServingSystemListener::onSystemInfoChanged
     *                    - @ref IServingSystemListener::onCallBarringInfoChanged
     *                    - @ref IServingSystemListener::onSmsCapabilityChanged
     *
     *                    A value of @ref FILTER_MASK_NONE clears any previously configured filter
     *                    and restores reporting of all eligible SYSTEM_INFO-related changes.
     *
     * @return @ref telux::common::ErrorCode indicating whether the filter was successfully
     *         applied or the reason for failure.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode setSystemInfoNotificationFilter(
        SystemInfoNotificationFilterMask mask = FILTER_MASK_NONE)
        = 0;

    /**
     * Get the currently configured filter for system information which were registered using
     * @ref IServingSystemManager::registerListener with @ref
     * ServingSystemNotificationType::SYSTEM_INFO.
     *
     * On platforms with access control enabled, the caller needs to have
     * TELUX_TEL_SRV_SYSTEM_READ permission to successfully invoke this API.
     *
     * @param [out] mask       Bit mask representing the set of attributes defined in
     *                         @ref SystemInfoNotificationFilter that are currently configured to
     *                         trigger @ref IServingSystemListener::onSystemInfoChanged,
     *                         @ref IServingSystemListener::onCallBarringInfoChanged and
     *                         @ref IServingSystemListener::onSmsCapabilityChanged
     *                         notifications. An empty mask indicates that filtering is
     *                         disabled and all system information changes are reported.
     *
     * @returns @ref telux::common::ErrorCode of getSystemInfoNotificationFilter i.e. success or
     * suitable error code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getSystemInfoNotificationFilter(
        SystemInfoNotificationFilterMask &mask)
        = 0;

    /**
     * Represents no filtering for system information notifications.
     * When this constant value is passed to
     * @ref IServingSystemManager::setSystemInfoNotificationFilter, all filtering is
     * disabled and notifications are generated for every supported system
     * information change.
     */
    static const uint64_t FILTER_MASK_NONE = 0x00000000;

    /**
     * Register a listener for specific updates from serving system.
     *
     * @param [in] listener     Pointer of IServingSystemListener object that
     *                          processes the notification
     * @param [in] mask         Bit mask representing a set of notifications that needs to be
     *                          registered - @ref ServingSystemNotificationMask
     *                          Notifications under IServingSystemListener that are not listed in
     *                          @ref ServingSystemNotificationType would always be registered by
     *                          default.
     *                          All the notifications will be registered when the client provides
     *                          ALL_NOTIFICATIONS as input. The bits that are not set in the mask
     * are ignored and do not have any effect on registration. To deregister, the API @ref
     * deregisterListener should be used.
     *
     * @note @ref IServingSystemManager::setSystemInfoNotificationFilter to receive
     * notifications only for selected system information changes. The following notification
     * types can be filtered.
     * 1. @ref IServingSystemListener::onSystemInfoChanged
     * 2. @ref IServingSystemListener::onCallBarringInfoChanged
     * 3. @ref IServingSystemListener::onSmsCapabilityChanged
     *
     * @returns Status of registerListener i.e success or suitable status code.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IServingSystemListener> listener,
        ServingSystemNotificationMask mask = ALL_NOTIFICATIONS)
        = 0;

    /**
     * Deregister the previously added listener.
     *
     * @param [in] listener     Previously registered IServingSystemListener that
     *                          needs to be removed
     * @param [in] mask         Bit mask that denotes a set of notifications that needs to be
     *                          de-registered - @ref ServingSystemNotificationMask
     *                          Notifications under IServingSystemListener that are not listed in
     *                          @ref ServingSystemNotificationType will be de-registered only when
     *                          ALL_NOTIFICATIONS is provided as input.
     *                          The bits that are not set in the mask are ignored and does not have
     *                          any effect on de-registration. However, providing an empty mask is
     *                          an invalid operation.
     *                          To register again, the API @ref registerListener should be used.
     *
     * @returns Status of deregisterListener i.e. success or suitable status code
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IServingSystemListener> listener,
        ServingSystemNotificationMask mask = ALL_NOTIFICATIONS)
        = 0;

    /**
     * Represents the set of all notifications defined in @ref ServingSystemNotificationType.
     * When this constant value is provided for registration or deregistration, all notifications
     * will be registered or deregistered.
     */
    static const uint32_t ALL_NOTIFICATIONS = 0xFFFFFFFF;

    /**
     * Destructor of IServingSystemManager
     */
    virtual ~IServingSystemManager() {
    }
};

/**
 * @brief Listener class for getting notifications related to updates in radio access technology
 *        mode preference, service domain preference, serving system information, etc.
 *        Some notifications in this listener could be frequent in nature. When the system is in a
 *        suspended/low power state, those indications will wake the system up. This could result
 *        in increased power consumption by the system. If those notifications are not required in
 *        the suspended/low power state, it is recommended for the client to de-register specific
 *        notifications using the @ref deregisterListener API.
 *
 *        The listener method can be invoked from multiple different threads.
 *        Client needs to make sure that implementation is thread-safe.
 */
class IServingSystemListener : public common::IServiceStatusListener {
 public:
    /**
     * This function is called whenever RAT mode preference is changed.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] preference      @ref RatPreference
     */
    virtual void onRatPreferenceChanged(RatPreference preference) {
    }

    /**
     * This function is called whenever service domain preference is changed.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] preference      @ref ServiceDomainPreference
     */
    virtual void onServiceDomainPreferenceChanged(ServiceDomainPreference preference) {
    }

    /**
     * This function is called whenever voice domain preference changes — from the explicit
     * setVoiceDomainPreference() call.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] preference      @ref VoiceDomainPreference
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual void onVoiceDomainPreferenceChanged(VoiceDomainPreference preference) {
    }

    /**
     * This function is called whenever the Serving System information is changed.
     * Supports only 3GPP RATs.
     *
     * To receive this notification, client needs to register a listener using @ref registerListener
     * API by setting the @ref ServingSystemNotificationType::SYSTEM_INFO bit in the bitmask.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] sysInfo    @ref ServingSystemInfo
     *
     */
    virtual void onSystemInfoChanged(ServingSystemInfo sysInfo) {
    }

    /**
     * This function is called whenever the Dual Connnectivity status is changed on 5G NR.
     *
     * To receive this notification, client needs to register a listener using @ref registerListener
     * API by setting the @ref ServingSystemNotificationType::SYSTEM_INFO bit in the bitmask.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] dcStatus       @ref DcStatus
     *
     */
    virtual void onDcStatusChanged(DcStatus dcStatus) {
    }

    /**
     * This function is called whenever network time information is changed.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] info    Network time information @ref NetworkTimeInfo
     *
     */
    virtual void onNetworkTimeChanged(NetworkTimeInfo info) {
    }

    /**
     * This function is called whenever LTE(SIB16) or NR5G RRC(SIB9) UTC time information is
     * changed.
     *
     * To receive this notification, client needs to register a listener using @ref
     * telux::tel::IServingSystemManager::registerListener API by setting the @ref
     * telux::tel::ServingSystemNotificationType::LTE_SIB16_NETWORK_TIME for LTE or
     * telux::tel::ServingSystemNotificationType::NR5G_RRC_UTC_TIME for NR5G bit in the
     * telux::tel::ServingSystemNotificationMask bitmask.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] radioTech    Time information changed on specified Radio technology.
     *                          @ref telux::tel::RadioTechnology
     * @param [in] info         Network time information. @ref telux::tel::NetworkTimeInfo
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onNetworkTimeChanged(RadioTechnology radioTech, NetworkTimeInfo info) {
    }

    /**
     * This function is called whenever the RF band information changes.
     *
     * To receive this notification, client needs to register a listener using @ref registerListener
     * API by setting the @ref ServingSystemNotificationType::RF_BAND_INFO bit in the bitmask.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @note The active bandwidth information @ref telux::tel::RFBandInfo::bandWidth is not
     * supported for NB1 NTN RAT.
     *
     * @param [in] bandInfo       @ref RFBandInfo
     *
     */
    virtual void onRFBandInfoChanged(RFBandInfo bandInfo) {
    }

    /**
     * This function is called when network registration rejection occurs.
     *
     * To receive this notification, client needs to register a listener using @ref registerListener
     * API by setting the @ref ServingSystemNotificationType::NETWORK_REJ_INFO bit in the bitmask.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] rejectInfo       @ref NetworkRejectInfo
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual void onNetworkRejection(NetworkRejectInfo rejectInfo) {
    }

    /**
     * Notifies registered listeners whenever the call barring information for the currently
     * registered cell of the device changes.
     *
     * To receive this notification, the client needs to register a listener using the
     * @ref registerListener API by setting the @ref ServingSystemNotificationType::SYSTEM_INFO
     * bit in the bitmask.
     *
     * On platforms with access control enabled, the caller needs to have the
     * TELUX_TEL_SRV_SYSTEM_READ permission to receive this notification.
     *
     * @param [in] barringInfo       List of call barring information @ref CallBarringInfo
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual void onCallBarringInfoChanged(std::vector<CallBarringInfo> barringInfo) {
    }

    /**
     * This function is called whenever the SMS capability over currently registered network
     * changes.
     *
     * To receive this notification, client needs to register a listener using @ref registerListener
     * API by setting the @ref ServingSystemNotificationType::SYSTEM_INFO bit in the bitmask.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] smsCapability       SMS capability @ref SmsCapability
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual void onSmsCapabilityChanged(SmsCapability smsCapability) {
    }

    /**
     * This function is called whenever the CS service capabilities of the LTE network changes.
     *
     * To receive this notification, client needs to register a listener using @ref registerListener
     * API by setting the @ref ServingSystemNotificationType::SYSTEM_INFO bit in the bitmask.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] lteCapability       LTE CS capability @ref LteCsCapability
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to
     *         change and could break backwards compatibility.
     */
    virtual void onLteCsCapabilityChanged(LteCsCapability lteCapability) {
    }

    /**
     * This function is called whenever RF band preference changes.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification
     *
     * @note This API is not supported for NB1 NTN RAT.
     *
     * @param [in] prefList    @ref telux::tel::IRFBandList instance
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onRFBandPreferenceChanged(std::shared_ptr<IRFBandList> prefList) {
    }

    /**
     * This function is called whenever radio resource control (RRC) state changes.
     *
     * To receive this notification, client needs to register a listener using @ref registerListener
     * API by setting the @ref ServingSystemNotificationType::ARFCN_INFO bit in the bitmask.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_TEL_SRV_SYSTEM_READ
     * permission to receive this notification.
     *
     * @param [in] rrcState    @ref telux::tel::RrcState instance
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onRrcStateChanged(telux::tel::RrcState rrcState) {
    }

    /**
     * Destructor of IServingSystemListener
     */
    virtual ~IServingSystemListener() {
    }
};

/** @} */ /* end_addtogroup telematics_serving_system */
}  // namespace tel
}  // namespace telux

#endif  // TELUX_TEL_SERVINGSYSTEMMANAGER_HPP
