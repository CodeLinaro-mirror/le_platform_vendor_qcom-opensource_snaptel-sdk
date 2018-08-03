/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

/**
 * @file       DataDefines.hpp
 * @brief      DataDefines provides the enumerations required for Connection Manager
 *
 * @note       Eval: This is a new API and is being evaluated. It is subject to
 *             change and could break backwards compatibility.
 */

#ifndef DATADEFINES_HPP
#define DATADEFINES_HPP

#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */
/**
 * Preferred IP family for the call
 */
enum class IpFamilyType {
   IP_FAMILY_TYPE_UNKNOWN = -1,
   IP_FAMILY_TYPE_V4 = 0x04,    // IPv4 call
   IP_FAMILY_TYPE_V6 = 0x06,    // IPv6 call
   IP_FAMILY_TYPE_V4V6 = 0x0A,  // IPv4 and IPv6 call
};

/**
 * Technology Preference
 */
enum class TechPreference {
   TECH_PREFERENCE_3GPP,  /**< UMTS, LTE */
   TECH_PREFERENCE_3GPP2, /**< CDMA */
   TECH_PREFERENCE_ANY,   /**< ANY (3GPP or 3GPP2)  */
};

/**
 * Authentication protocol type to be used for PDP context.
 */
enum class AuthProtocolType {
   AUTH_TYPE_NONE = 0,
   AUTH_TYPE_PAP = 1,  /**< Password Authentication Protocol */
   AUTH_TYPE_CHAP = 2, /**< Challenge Handshake Authentication Protocol */
   AUTH_TYPE_PAP_CHAP = 3,
};

/**
 * Profile Parameters used for profile creation, query and modify
 */
struct ProfileParams {
   std::string profileName;                                        /**< Profile Name */
   std::string apn;                                                /**< APN name */
   std::string userName;                                           /**< APN user name (if any) */
   std::string password;                                           /**< APN password (if any) */
   TechPreference techPref = TechPreference::TECH_PREFERENCE_3GPP; /**< Technology preference,
                                     default is TechPreference::TECH_PREFERENCE_3GPP */
   AuthProtocolType authType = AuthProtocolType::AUTH_TYPE_NONE; /**< Authentication protocol type,
                                     default is AuthProtocolType::AUTH_TYPE_NONE */
   IpFamilyType ipFamilyType
      = IpFamilyType::IP_FAMILY_TYPE_UNKNOWN; /**< Preferred IP family for the call,
                                                   default is
                                                   IpFamilyType::IP_FAMILY_TYPE_UNKNOWN */
};

/**
 * Data transfer statistics structure.
 */
struct DataCallStats {
   unsigned long packetsTx = 0;          /**< Number of packets transmitted */
   unsigned long packetsRx = 0;          /**< Number of packets received */
   long long bytesTx = 0;                /**< Number of bytes transmitted */
   long long bytesRx = 0;                /**< Number of bytes received */
   unsigned long packetsDroppedTx = 0;   /**< Number of transmit packets dropped */
   unsigned long packetsDroppedRx = 0;   /**< Number of receive packets dropped */
};

/**
 * Data bit rate in kbps
 */
struct DataChannelRate {
   int txRate = 0;    /**< Current transfer rate */
   int rxRate = 0;    /**< Current receiver rate */
   int maxTxRate = 0; /**< Max transfer rate */
   int maxRxRate = 0; /**< Max receiver rate */
};

/**
 * DATA event status
 */
enum class DataCallStatus {
   INVALID = 0x00,    /**<  Invalid  */
   NET_CONNECTED,     /**< Call is connected */
   NET_NO_NET,        /**< Call is disconnected */
   NET_IDLE,          /**< Call is in idle state */
   NET_CONNECTING,    /**< Call is in connecting state */
   NET_DISCONNECTING, /**< Call is in disconnecting state */
   NET_RECONFIGURED,  /**< Interface is reconfigured, IP Address got changed */
   NET_NEWADDR,       /**< A new IP address was added on an existing call */
   NET_DELADDR,       /**< An IP address was removed from the existing interface */
};

/**
 * IP address information structure
 */
struct IpAddrInfo {
   std::string ifAddress;           /**< Interface IP address. */
   unsigned int ifMask = 0;         /**< Subnet mask.          */
   std::string gwAddress;           /**< Gateway IP address.   */
   unsigned int gwMask = 0;         /**< Subnet mask.          */
   std::string primaryDnsAddress;   /**< Primary DNS address.  */
   std::string secondaryDnsAddress; /**< Secondary DNS address.*/
};

/**
 * Bearer technology types (returned with getCurrentBearerTech).
 */
enum class DataBearerTechnology {
   UNKNOWN, /**< Unknown bearer. */
   // CDMA related data bearer technologies
   CDMA_1X,                /**< 1X technology. */
   EVDO_REV0,              /**< CDMA Rev 0. */
   EVDO_REVA,              /**< CDMA Rev A. */
   EVDO_REVB,              /**< CDMA Rev B. */
   EHRPD,                  /**< EHRPD. */
   FMC,                    /**< Fixed mobile convergence. */
   HRPD,                   /**< HRPD */
   BEARER_TECH_3GPP2_WLAN, /**< IWLAN */

   // UMTS related data bearer technologies
   WCDMA,                 /**< WCDMA. */
   GPRS,                  /**< GPRS. */
   HSDPA,                 /**< HSDPA. */
   HSUPA,                 /**< HSUPA. */
   EDGE,                  /**< EDGE. */
   LTE,                   /**< LTE. */
   HSDPA_PLUS,            /**< HSDPA+. */
   DC_HSDPA_PLUS,         /**< DC HSDPA+. */
   HSPA,                  /**< HSPA */
   BEARER_TECH_64_QAM,    /**< 64 QAM. */
   TDSCDMA,               /**< TD-SCDMA. */
   GSM,                   /**< GSM */
   BEARER_TECH_3GPP_WLAN, /**< IWLAN */
};

/**
 * Data call terminated due to reason type.
 */
enum class EndReasonType {
   CE_UNKNOWN = 0xFF,
   CE_MOBILE_IP = 0x01,
   CE_INTERNAL = 0x02,
   CE_CALL_MANAGER_DEFINED = 0x03,
   CE_3GPP_SPEC_DEFINED = 0x06,
   CE_PPP = 0x07,
   CE_EHRPD = 0x08,
   CE_IPV6 = 0x09,
};

enum class MobileIpReasonCode {
   /*Mobile IP Call End reasons*/
   CE_MIP_FA_ERR_REASON_UNSPECIFIED = 64,
   CE_MIP_FA_ERR_ADMINISTRATIVELY_PROHIBITED = 65,
   CE_MIP_FA_ERR_INSUFFICIENT_RESOURCES = 66,
   CE_MIP_FA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE = 67,
   CE_MIP_FA_ERR_HA_AUTHENTICATION_FAILURE = 68,
   CE_MIP_FA_ERR_REQUESTED_LIFETIME_TOO_LONG = 69,
   CE_MIP_FA_ERR_MALFORMED_REQUEST = 70,
   CE_MIP_FA_ERR_MALFORMED_REPLY = 71,
   CE_MIP_FA_ERR_ENCAPSULATION_UNAVAILABLE = 72,
   CE_MIP_FA_ERR_VJHC_UNAVAILABLE = 73,
   CE_MIP_FA_ERR_REVERSE_TUNNEL_UNAVAILABLE = 74,
   CE_MIP_FA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET = 75,
   CE_MIP_FA_ERR_DELIVERY_STYLE_NOT_SUPPORTED = 79,
   CE_MIP_FA_ERR_MISSING_NAI = 97,
   CE_MIP_FA_ERR_MISSING_HA = 98,
   CE_MIP_FA_ERR_MISSING_HOME_ADDR = 99,
   CE_MIP_FA_ERR_UNKNOWN_CHALLENGE = 104,
   CE_MIP_FA_ERR_MISSING_CHALLENGE = 105,
   CE_MIP_FA_ERR_STALE_CHALLENGE = 106,
   CE_MIP_HA_ERR_REASON_UNSPECIFIED = 128,
   CE_MIP_HA_ERR_ADMINISTRATIVELY_PROHIBITED = 129,
   CE_MIP_HA_ERR_INSUFFICIENT_RESOURCES = 130,
   CE_MIP_HA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE = 131,
   CE_MIP_HA_ERR_FA_AUTHENTICATION_FAILURE = 132,
   CE_MIP_HA_ERR_REGISTRATION_ID_MISMATCH = 133,
   CE_MIP_HA_ERR_MALFORMED_REQUEST = 134,
   CE_MIP_HA_ERR_UNKNOWN_HA_ADDR = 136,
   CE_MIP_HA_ERR_REVERSE_TUNNEL_UNAVAILABLE = 137,
   CE_MIP_HA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET = 138,
   CE_MIP_HA_ERR_ENCAPSULATION_UNAVAILABLE = 139,
   CE_MIP_ERR_REASON_UNKNOWN = 65535,
};

enum class InternalReasonCode {
   /*Internal Error Call End reasons*/
   CE_INTERNAL_ERROR = 201,
   CE_CALL_ENDED = 202,
   CE_INTERNAL_UNKNOWN_CAUSE_CODE = 203,
   CE_UNKNOWN_CAUSE_CODE = 204,
   CE_CLOSE_IN_PROGRESS = 205,
   CE_NW_INITIATED_TERMINATION = 206,
   CE_APP_PREEMPTED = 207,
   CE_ERR_PDN_IPV4_CALL_DISALLOWED = 208,
   CE_ERR_PDN_IPV4_CALL_THROTTLED = 209,
   CE_ERR_PDN_IPV6_CALL_DISALLOWED = 210,
   CE_ERR_PDN_IPV6_CALL_THROTTLED = 211,
   CE_UNPREFERRED_RAT = 214,
   CE_APN_DISABLED = 220,
   CE_MAX_V4_CONNECTIONS = 228,
   CE_MAX_V6_CONNECTIONS = 229,
   CE_APN_MISMATCH = 230,
   CE_IP_VERSION_MISMATCH = 231,
   CE_DUN_CALL_DISALLOWED = 232,
   CE_INVALID_PROFILE = 233,
   CE_INTERNAL_EPC_NONEPC_TRANSITION = 234,
};

enum class CallManagerReasonCode {
   /*CM defined Call End reasons*/
   CE_CDMA_LOCK = 500,
   CE_INTERCEPT = 501,
   CE_REORDER = 502,
   CE_REL_SO_REJ = 503,
   CE_INCOM_CALL = 504,
   CE_ALERT_STOP = 505,
   CE_ACTIVATION = 506,
   CE_MAX_ACCESS_PROBE = 507,
   CE_CCS_NOT_SUPPORTED_BY_BS = 508,
   CE_NO_RESPONSE_FROM_BS = 509,
   CE_REJECTED_BY_BS = 510,
   CE_INCOMPATIBLE = 511,
   CE_ALREADY_IN_TC = 512,
   CE_USER_CALL_ORIG_DURING_GPS = 513,
   CE_USER_CALL_ORIG_DURING_SMS = 514,
   CE_NO_CDMA_SRV = 515,
   CE_CONF_FAILED = 1000,
   CE_INCOM_REJ = 1001,
   CE_NO_GW_SRV = 1002,
   CE_NO_GPRS_CONTEXT = 1003,
   CE_ILLEGAL_MS = 1004,
   CE_ILLEGAL_ME = 1005,
   CE_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED = 1006,
   CE_GPRS_SERVICES_NOT_ALLOWED = 1007,
   CE_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK = 1008,
   CE_IMPLICITLY_DETACHED = 1009,
   CE_PLMN_NOT_ALLOWED = 1010,
   CE_LA_NOT_ALLOWED = 1011,
   CE_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN = 1012,
   CE_PDP_DUPLICATE = 1013,
   CE_UE_RAT_CHANGE = 1014,
   CE_CONGESTION = 1015,
   CE_NO_PDP_CONTEXT_ACTIVATED = 1016,
   CE_ACCESS_CLASS_DSAC_REJECTION = 1017,
   CE_CD_GEN_OR_BUSY = 1500,
   CE_CD_BILL_OR_AUTH = 1501,
   CE_CHG_HDR = 1502,
   CE_EXIT_HDR = 1503,
   CE_HDR_NO_SESSION = 1504,
   CE_HDR_ORIG_DURING_GPS_FIX = 1505,
   CE_HDR_CS_TIMEOUT = 1506,
   CE_HDR_RELEASED_BY_CM = 1507,
   CE_CLIENT_END = 2000,
   CE_NO_SRV = 2001,
   CE_FADE = 2002,
   CE_REL_NORMAL = 2003,
   CE_ACC_IN_PROG = 2004,
   CE_ACC_FAIL = 2005,
   CE_REDIR_OR_HANDOFF = 2006,
   CE_UNKNOWN = -1,
};

enum class SpecReasonCode {
   /*3GPP spec defined Call End reasons*/
   CE_OPERATOR_DETERMINED_BARRING = 8,
   CE_LLC_SNDCP_FAILURE = 25,
   CE_INSUFFICIENT_RESOURCES = 26,
   CE_UNKNOWN_APN = 27,
   CE_UNKNOWN_PDP = 28,
   CE_AUTH_FAILED = 29,
   CE_GGSN_REJECT = 30,
   CE_ACTIVATION_REJECT = 31,
   CE_OPTION_NOT_SUPPORTED = 32,
   CE_OPTION_UNSUBSCRIBED = 33,
   CE_OPTION_TEMP_OOO = 34,
   CE_NSAPI_ALREADY_USED = 35,
   CE_REGULAR_DEACTIVATION = 36,
   CE_QOS_NOT_ACCEPTED = 37,
   CE_NETWORK_FAILURE = 38,
   CE_UMTS_REACTIVATION_REQ = 39,
   CE_FEATURE_NOT_SUPPORTED = 40,
   CE_TFT_SEMANTIC_ERROR = 41,
   CE_TFT_SYNTAX_ERROR = 42,
   CE_UNKNOWN_PDP_CONTEXT = 43,
   CE_FILTER_SEMANTIC_ERROR = 44,
   CE_FILTER_SYNTAX_ERROR = 45,
   CE_PDP_WITHOUT_ACTIVE_TFT = 46,
   CE_IP_V4_ONLY_ALLOWED = 50,
   CE_IP_V6_ONLY_ALLOWED = 51,
   CE_SINGLE_ADDR_BEARER_ONLY = 52,
   CE_INVALID_TRANSACTION_ID = 81,
   CE_MESSAGE_INCORRECT_SEMANTIC = 95,
   CE_INVALID_MANDATORY_INFO = 96,
   CE_MESSAGE_TYPE_UNSUPPORTED = 97,
   CE_MSG_TYPE_NONCOMPATIBLE_STATE = 98,
   CE_UNKNOWN_INFO_ELEMENT = 99,
   CE_CONDITIONAL_IE_ERROR = 100,
   CE_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE = 101,
   CE_PROTOCOL_ERROR = 111,
   CE_APN_TYPE_CONFLICT = 112,
   CE_UNKNOWN = -1,
};

enum class PPPReasonCode {
   /*Enumeration for the PPP verbose call end reason*/
   CE_PPP_TIMEOUT = 1,
   CE_PPP_AUTH_FAILURE = 2,
   CE_PPP_OPTION_MISMATCH = 3,
   CE_PPP_PAP_FAILURE = 31,
   CE_PPP_CHAP_FAILURE = 32,
   CE_PPP_UNKNOWN = -1,
};

enum class EHRPDReasonCode {
   /* Enumeration for the EHRPD verbose call end reason */
   CE_EHRPD_SUBS_LIMITED_TO_V4 = 1,
   CE_EHRPD_SUBS_LIMITED_TO_V6 = 2,
   CE_EHRPD_VSNCP_TIMEOUT = 4,
   CE_EHRPD_VSNCP_FAILURE = 5,
   CE_EHRPD_VSNCP_3GPP2I_GEN_ERROR = 6,
   CE_EHRPD_VSNCP_3GPP2I_UNAUTH_APN = 7,
   CE_EHRPD_VSNCP_3GPP2I_PDN_LIMIT_EXCEED = 8,
   CE_EHRPD_VSNCP_3GPP2I_NO_PDN_GW = 9,
   CE_EHRPD_VSNCP_3GPP2I_PDN_GW_UNREACH = 10,
   CE_EHRPD_VSNCP_3GPP2I_PDN_GW_REJ = 11,
   CE_EHRPD_VSNCP_3GPP2I_INSUFF_PARAM = 12,
   CE_EHRPD_VSNCP_3GPP2I_RESOURCE_UNAVAIL = 13,
   CE_EHRPD_VSNCP_3GPP2I_ADMIN_PROHIBIT = 14,
   CE_EHRPD_VSNCP_3GPP2I_PDN_ID_IN_USE = 15,
   CE_EHRPD_VSNCP_3GPP2I_SUBSCR_LIMITATION = 16,
   CE_EHRPD_VSNCP_3GPP2I_PDN_EXISTS_FOR_THIS_APN = 17,
   CE_EHRPD_UNKNOWN = -1,
};

enum class Ipv6ReasonCode {
   /*IPV6 defined Call End reasons*/
   CE_PREFIX_UNAVAILABLE = 1,
   CE_IPV6_ERR_HRPD_IPV6_DISABLED = 2,
   CE_IPV6_DISABLED = 3,
};

struct DataCallEndReason {
   EndReasonType type = EndReasonType::CE_UNKNOWN;
   /**< Data call terminated due to reason type, default is CE_UNKNOWN */
   union {
      MobileIpReasonCode IpCode;
      InternalReasonCode internalCode;
      CallManagerReasonCode cmCode;
      SpecReasonCode specCode;
      PPPReasonCode pppCode;
      EHRPDReasonCode ehrpdCode;
      Ipv6ReasonCode ipv6Code;
   };
   /**< Reason Code corresponding to reason type*/
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif  // DATADEFINES_HPP
