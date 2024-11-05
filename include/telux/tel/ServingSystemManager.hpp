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
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       ServingSystemManager.hpp
 *
 * @brief      Serving System Manager class provides the interface to request and set
 *             service domain preference and radio access technology mode preference for
 *             searching and registering (CS/PS domain, RAT and operation mode).
 */

#ifndef SERVINGSYSTEMMANAGER_HPP
#define SERVINGSYSTEMMANAGER_HPP

#include <bitset>
#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

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
   CS_PS,   /**< Circuit-switched and packet-switched */
};

/**
 * Defines service domain
 */
enum class ServiceDomain {
   UNKNOWN = -1,  /**< Unknown, when the information is not available */
   NO_SRV,        /**< No Service */
   CS_ONLY,       /**< Circuit-switched only */
   PS_ONLY,       /**< Packet-switched only */
   CS_PS,         /**< Circuit-switched and packet-switched */
   CAMPED,        /**< Device camped on the network according to its provisioning, but not
                       registered */
};

/**
 * Defines current serving system information
 */
struct ServingSystemInfo {
   RadioTechnology rat;    /**< Current serving RAT */
   ServiceDomain   domain; /**< Current service domain registered on system for the serving RAT */
};

/**
 * Defines RF Bands.
 */
enum class RFBand {
   INVALID = -1,
   BC_0 = 0,
   BC_1 = 1,
   BC_3 = 3,
   BC_4 = 4,
   BC_5 = 5,
   BC_6 = 6,
   BC_7 = 7,
   BC_8 = 8,
   BC_9 = 9,
   BC_10 = 10,
   BC_11 = 11,
   BC_12 = 12,
   BC_13 = 13,
   BC_14 = 14,
   BC_15 = 15,
   BC_16 = 16,
   BC_17 = 17,
   BC_18 = 18,
   BC_19 = 19,
   GSM_450 = 40,
   GSM_480 = 41,
   GSM_750 = 42,
   GSM_850 = 43,
   GSM_900_EXTENDED = 44,
   GSM_900_PRIMARY = 45,
   GSM_900_RAILWAYS = 46,
   GSM_1800 = 47,
   GSM_1900 = 48,
   WCDMA_2100 = 80,
   WCDMA_PCS_1900 = 81,
   WCDMA_DCS_1800 = 82,
   WCDMA_1700_US = 83,
   WCDMA_850 = 84,
   WCDMA_800 = 85,
   WCDMA_2600 = 86,
   WCDMA_900 = 87,
   WCDMA_1700_JAPAN = 88,
   WCDMA_1500_JAPAN = 90,
   WCDMA_850_JAPAN = 91,
   E_UTRA_OPERATING_BAND_1 = 120,
   E_UTRA_OPERATING_BAND_2 = 121,
   E_UTRA_OPERATING_BAND_3 = 122,
   E_UTRA_OPERATING_BAND_4 = 123,
   E_UTRA_OPERATING_BAND_5 = 124,
   E_UTRA_OPERATING_BAND_6 = 125,
   E_UTRA_OPERATING_BAND_7 = 126,
   E_UTRA_OPERATING_BAND_8 = 127,
   E_UTRA_OPERATING_BAND_9 = 128,
   E_UTRA_OPERATING_BAND_10 = 129,
   E_UTRA_OPERATING_BAND_11 = 130,
   E_UTRA_OPERATING_BAND_12 = 131,
   E_UTRA_OPERATING_BAND_13 = 132,
   E_UTRA_OPERATING_BAND_14 = 133,
   E_UTRA_OPERATING_BAND_17 = 134,
   E_UTRA_OPERATING_BAND_33 = 135,
   E_UTRA_OPERATING_BAND_34 = 136,
   E_UTRA_OPERATING_BAND_35 = 137,
   E_UTRA_OPERATING_BAND_36 = 138,
   E_UTRA_OPERATING_BAND_37 = 139,
   E_UTRA_OPERATING_BAND_38 = 140,
   E_UTRA_OPERATING_BAND_39 = 141,
   E_UTRA_OPERATING_BAND_40 = 142,
   E_UTRA_OPERATING_BAND_18 = 143,
   E_UTRA_OPERATING_BAND_19 = 144,
   E_UTRA_OPERATING_BAND_20 = 145,
   E_UTRA_OPERATING_BAND_21 = 146,
   E_UTRA_OPERATING_BAND_24 = 147,
   E_UTRA_OPERATING_BAND_25 = 148,
   E_UTRA_OPERATING_BAND_41 = 149,
   E_UTRA_OPERATING_BAND_42 = 150,
   E_UTRA_OPERATING_BAND_43 = 151,
   E_UTRA_OPERATING_BAND_23 = 152,
   E_UTRA_OPERATING_BAND_26 = 153,
   E_UTRA_OPERATING_BAND_32 = 154,
   E_UTRA_OPERATING_BAND_125 = 155,
   E_UTRA_OPERATING_BAND_126 = 156,
   E_UTRA_OPERATING_BAND_127 = 157,
   E_UTRA_OPERATING_BAND_28 = 158,
   E_UTRA_OPERATING_BAND_29 = 159,
   E_UTRA_OPERATING_BAND_30 = 160,
   E_UTRA_OPERATING_BAND_66 = 161,
   E_UTRA_OPERATING_BAND_250 = 162,
   E_UTRA_OPERATING_BAND_46 = 163,
   E_UTRA_OPERATING_BAND_27 = 164,
   E_UTRA_OPERATING_BAND_31 = 165,
   E_UTRA_OPERATING_BAND_71 = 166,
   E_UTRA_OPERATING_BAND_47 = 167,
   E_UTRA_OPERATING_BAND_48 = 168,
   E_UTRA_OPERATING_BAND_67 = 169,
   E_UTRA_OPERATING_BAND_68 = 170,
   E_UTRA_OPERATING_BAND_49 = 171,
   E_UTRA_OPERATING_BAND_85 = 172,
   E_UTRA_OPERATING_BAND_72 = 173,
   E_UTRA_OPERATING_BAND_73 = 174,
   E_UTRA_OPERATING_BAND_86 = 175,
   E_UTRA_OPERATING_BAND_53 = 176,
   E_UTRA_OPERATING_BAND_87 = 177,
   E_UTRA_OPERATING_BAND_88 = 178,
   E_UTRA_OPERATING_BAND_70 = 179,
   TDSCDMA_BAND_A = 200,
   TDSCDMA_BAND_B = 201,
   TDSCDMA_BAND_C = 202,
   TDSCDMA_BAND_D = 203,
   TDSCDMA_BAND_E = 204,
   TDSCDMA_BAND_F = 205,
   NR5G_BAND_1 = 250,
   NR5G_BAND_2 = 251,
   NR5G_BAND_3 = 252,
   NR5G_BAND_5 = 253,
   NR5G_BAND_7 = 254,
   NR5G_BAND_8 = 255,
   NR5G_BAND_20 = 256,
   NR5G_BAND_28 = 257,
   NR5G_BAND_38 = 258,
   NR5G_BAND_41 = 259,
   NR5G_BAND_50 = 260,
   NR5G_BAND_51 = 261,
   NR5G_BAND_66 = 262,
   NR5G_BAND_70 = 263,
   NR5G_BAND_71 = 264,
   NR5G_BAND_74 = 265,
   NR5G_BAND_75 = 266,
   NR5G_BAND_76 = 267,
   NR5G_BAND_77 = 268,
   NR5G_BAND_78 = 269,
   NR5G_BAND_79 = 270,
   NR5G_BAND_80 = 271,
   NR5G_BAND_81 = 272,
   NR5G_BAND_82 = 273,
   NR5G_BAND_83 = 274,
   NR5G_BAND_84 = 275,
   NR5G_BAND_85 = 276,
   NR5G_BAND_257 = 277,
   NR5G_BAND_258 = 278,
   NR5G_BAND_259 = 279,
   NR5G_BAND_260 = 280,
   NR5G_BAND_261 = 281,
   NR5G_BAND_12 = 282,
   NR5G_BAND_25 = 283,
   NR5G_BAND_34 = 284,
   NR5G_BAND_39 = 285,
   NR5G_BAND_40 = 286,
   NR5G_BAND_65 = 287,
   NR5G_BAND_86 = 288,
   NR5G_BAND_48 = 289,
   NR5G_BAND_14 = 290,
   NR5G_BAND_13 = 291,
   NR5G_BAND_18 = 292,
   NR5G_BAND_26 = 293,
   NR5G_BAND_30 = 294,
   NR5G_BAND_29 = 295,
   NR5G_BAND_53 = 296,
   NR5G_BAND_46 = 297,
   NR5G_BAND_91 = 298,
   NR5G_BAND_92 = 299,
   NR5G_BAND_93 = 300,
   NR5G_BAND_94 = 301
};

/**
 * Defines RF Bandwidth Information.
 */

enum class RFBandWidth {
   INVALID_BANDWIDTH = -1,    /**<  Invalid Value */
   LTE_BW_NRB_6 = 0,          /**<  LTE 1.4 */
   LTE_BW_NRB_15 = 1,         /**<  LTE 3 */
   LTE_BW_NRB_25 = 2,         /**<  LTE 5  */
   LTE_BW_NRB_50 = 3,         /**<  LTE 10 */
   LTE_BW_NRB_75 = 4,         /**<  LTE 15 */
   LTE_BW_NRB_100 = 5,        /**<  LTE 20 */
   NR5G_BW_NRB_5 = 6,         /**<  NR5G 5 */
   NR5G_BW_NRB_10 = 7,        /**<  NR5G 10 */
   NR5G_BW_NRB_15 = 8,        /**<  NR5G 15 */
   NR5G_BW_NRB_20 = 9,        /**<  NR5G 20 */
   NR5G_BW_NRB_25 = 10,       /**<  NR5G 25 */
   NR5G_BW_NRB_30 = 11,       /**<  NR5G 30 */
   NR5G_BW_NRB_40 = 12,       /**<  NR5G 40 */
   NR5G_BW_NRB_50 = 13,       /**<  NR5G 50 */
   NR5G_BW_NRB_60 = 14,       /**<  NR5G 60 */
   NR5G_BW_NRB_80 = 15,       /**<  NR5G 80 */
   NR5G_BW_NRB_90 = 16,       /**<  NR5G 90 */
   NR5G_BW_NRB_100 = 17,      /**<  NR5G 100 */
   NR5G_BW_NRB_200 = 18,      /**<  NR5G 200 */
   NR5G_BW_NRB_400 = 19,      /**<  NR5G 400 */
   GSM_BW_NRB_2 = 20,         /**<  GSM  0.2 */
   TDSCDMA_BW_NRB_2 = 21,     /**<  TDSCDMA 1.6 */
   WCDMA_BW_NRB_5 = 22,       /**<  WCDMA 5 */
   WCDMA_BW_NRB_10 = 23,      /**<  WCDMA 10 */
   NR5G_BW_NRB_70 = 24        /**<  NR5G 70 */
};

/**
 * Defines information of RF bands.
 */
struct RFBandInfo {
   RFBand band;            /**< Currently active band */
   uint32_t channel;       /**< Currently active channel */
   RFBandWidth bandWidth;  /**< Bandwidth information */
};

/**
 * Defines the radio access technology mode preference.
 */
enum RatPrefType {
   PREF_CDMA_1X,   /**< CDMA_1X */
   PREF_CDMA_EVDO, /**< CDMA_EVDO */
   PREF_GSM,       /**< GSM */
   PREF_WCDMA,     /**< WCDMA */
   PREF_LTE,       /**< LTE */
   PREF_TDSCDMA,   /**< TDSCDMA */
   PREF_NR5G       /**< NR5G */
};

/**
 * Defines ENDC(E-UTRAN New Radio-Dual Connectivity) Availability status on 5G NR
 */
enum class EndcAvailability {
   UNKNOWN = -1,   /**< Status unknown */
   AVAILABLE,      /**< ENDC is Available */
   UNAVAILABLE,    /**< ENDC is not Available */
};

/**
 * Defines DCNR(Dual Connectivity with NR) Restriction status on 5G NR
 */
enum class DcnrRestriction {
   UNKNOWN = -1,    /**< Status unknown */
   RESTRICTED,      /**< DCNR is Rescticted */
   UNRESTRICTED,    /**< DCNR is not Restricted */
};

/**
 * Defines Dual Connectivity status
 */
struct DcStatus {
   EndcAvailability endcAvailability;     /**< ENDC availability */
   DcnrRestriction  dcnrRestriction;      /**< DCNR restriction */
};

/**
 * Defines Network time information
 */
struct NetworkTimeInfo {
   uint16_t year;         /**< Year. */
   uint8_t month;         /**< Month. 1 is January and 12 is December. */
   uint8_t day;           /**< Day. Range: 1 to 31.  */
   uint8_t hour;          /**< Hour. Range: 0 to 23. */
   uint8_t minute;        /**< Minute. Range: 0 to 59. */
   uint8_t second;        /**< Second. Range: 0 to 59. */
   uint8_t dayOfWeek;     /**< Day of the week. 0 is Monday and 6 is Sunday. */
   int8_t timeZone;       /**< Offset between UTC and local time in units of 15 minutes (signed
                               value). Actual value = field value * 15 minutes. */
   uint8_t dstAdj;        /**< Daylight saving adjustment in hours to obtain local time.
                               Possible values: 0, 1, and 2.*/
   std::string nitzTime;  /**< Network Identity and Time Zone(NITZ) information in the form
                               "yyyy/mm/dd,hh:mm:ss(+/-)tzh:tzm,dt */
};

/**
 * Defines network registration reject information
 */
struct NetworkRejectInfo {
    ServingSystemInfo rejectSrvInfo; /**< Serving system information where the registration is
                                          rejected.*/
    uint8_t rejectCause;             /**< Reject cause values as specified in 3GPP TS 24.008,
                                          3GPP TS 24.301 and 3GPP TS 24.501. */
    std::string mcc;                 /**< Mobile Country Code for rejection*/
    std::string mnc;                 /**< Mobile Network Code for rejection*/
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
   SYSTEM_INFO,      /* Represents @ref onSystemInfoChanged() and @ref onDcStatusChanged() */
   RF_BAND_INFO,     /* Represents @ref onRFBandInfoChanged */
   NETWORK_REJ_INFO  /* Represents @ref onNetworkRejection */
};

/**
 * Bit mask that denotes a set of notifications defined in @ref ServingSystemNotificationType
 */
using ServingSystemNotificationMask = std::bitset<32>;

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
 * @brief Serving System Manager class provides the API to request and set
 *        service domain preference and RAT preference.
 */

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
using RFBandInfoCallback
   = std::function<void(RFBandInfo bandInfo, telux::common::ErrorCode error)>;
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
   virtual telux::common::Status setRatPreference(RatPreference ratPref,
                                                  common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Request for preferred radio access technology mode.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
    * permission to invoke this API successfully.
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
   virtual telux::common::Status setServiceDomainPreference(ServiceDomainPreference serviceDomain,
                                                            common::ResponseCallback callback
                                                            = nullptr)
      = 0;

   /**
    * Request for Service Domain Preference asynchronously.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
    * permission to invoke this API successfully.
    *
    * @param [in] callback    Callback function to get the response of request
    *                         service domain preference.
    *
    * @returns Status of requestServiceDomainPreference i.e. success or suitable
    *          error code.
    */
   virtual telux::common::Status
      requestServiceDomainPreference(ServiceDomainPreferenceCallback callback)
      = 0;

   /**
    * Get the Serving system information. Supports only 3GPP RATs.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
    * permission to invoke this API successfully.
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
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
    * permission to invoke this API successfully.
    *
    * @returns @ref DcStatus
    */
   virtual telux::tel::DcStatus getDcStatus() = 0;

   /**
    * Get network time information asynchronously.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
    * permission to invoke this API successfully.
    *
    * @param [in] callback    Callback function to get the response of get
    *                         network time information request.
    *
    * @returns Status of requestNetworkTime i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestNetworkTime(NetworkTimeResponseCallback callback) = 0;

   /**
    * Get the information about the band that the device is currently using.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
    * permission to invoke this API successfully.
    *
    * @param [in] callback    Callback function to get the response of get
    *                         RF band information request.
    *
    * @returns Status of requestRFBandInfo i.e. success or suitable error code.
    *
    */
   virtual telux::common::Status requestRFBandInfo(RFBandInfoCallback callback) = 0;

   /**
    * Register a listener for specific updates from serving system.
    *
    * @param [in] listener     Pointer of IServingSystemListener object that
    *                          processes the notification
    * @param [in] mask         Bit mask representing a set of notifications that needs to be
    *                          registered - @ref ServingSystemNotificationMask
    *                          Notifications under IServingSystemListener that are not listed in
    *                          in @ref ServingSystemNotificationType would always be registered by
    *                          default.
    *                          All the notifications will be registered when the client provides
    *                          ALL_NOTIFICATIONS as input. The bits that are not set in the mask are
    *                          ignored and do not have any effect on registration.
    *                          To deregister, the API @ref deregisterListener should be used.
    *
    * @returns Status of registerListener i.e success or suitable status code.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<IServingSystemListener> listener,
        ServingSystemNotificationMask mask = ALL_NOTIFICATIONS) = 0;

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
    * @returns Status of removeListener i.e. success or suitable status code
    */
   virtual telux::common::Status deregisterListener(std::weak_ptr<IServingSystemListener> listener,
        ServingSystemNotificationMask mask = ALL_NOTIFICATIONS) = 0;

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
class IServingSystemListener : public common::IServiceStatusListener{
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
    * This function is called whenever the RF band information changes.
    *
    * To receive this notification, client needs to register a listener using @ref registerListener
    * API by setting the @ref ServingSystemNotificationType::RF_BAND_INFO bit in the bitmask.
    *
    * On platforms with Access control enabled, Caller needs to have TELUX_TEL_SRV_SYSTEM_READ
    * permission to receive this notification.
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
    * Destructor of IServingSystemListener
    */
   virtual ~IServingSystemListener() {
   }
};

/** @} */ /* end_addtogroup telematics_serving_system */
}
}

#endif
