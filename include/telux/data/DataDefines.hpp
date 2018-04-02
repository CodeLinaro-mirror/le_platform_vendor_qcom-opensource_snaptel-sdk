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

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */
/**
 * Preferred IP family for the call
 */
enum class IpFamilyType {
   IP_FAMILY_TYPE_UNKNOWN = -1,
   IP_FAMILY_TYPE_V4 = 0,    // IPv4 call
   IP_FAMILY_TYPE_V6 = 2,    // IPv6 call
   IP_FAMILY_TYPE_V4V6 = 3,  // IPv4 and IPv6 call
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
   int bytesReceived = 0;
   int bytesTransmitted = 0;
   int packetsReceived = 0;
   int packetsTransmitted = 0;
   int packetsDroppedOnReceive = 0;
   int packetsDroppedOnTransmit = 0;
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
   CALL_STATUS_INVALID = 0x00,    /**<  Invalid  */
   CALL_STATUS_NET_CONNECTED,     /**< Call is connected */
   CALL_STATUS_NET_NO_NET,        /**< Call is disconnected */
   CALL_STATUS_NET_IDLE,          /**< Call is in idle state */
   CALL_STATUS_NET_CONNECTING,    /**< Call is in connecting state */
   CALL_STATUS_NET_DISCONNECTING, /**< Call is in disconnecting state */
   CALL_STATUS_NET_RECONFIGURED,  /**< Interface is reconfigured, IP Address got changed */
   CALL_STATUS_NET_NEWADDR,       /**< A new IP address was added on an existing call */
   CALL_STATUS_NET_DELADDR,       /**< An IP address was removed from the existing interface */
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
   BEARER_TECH_UNKNOWN, /**< Unknown bearer. */
   // CDMA related data bearer technologies
   BEARER_TECH_CDMA_1X,    /**< 1X technology. */
   BEARER_TECH_EVDO_REV0,  /**< CDMA Rev 0. */
   BEARER_TECH_EVDO_REVA,  /**< CDMA Rev A. */
   BEARER_TECH_EVDO_REVB,  /**< CDMA Rev B. */
   BEARER_TECH_EHRPD,      /**< EHRPD. */
   BEARER_TECH_FMC,        /**< Fixed mobile convergence. */
   BEARER_TECH_HRPD,       /**< HRPD */
   BEARER_TECH_3GPP2_WLAN, /**< IWLAN */

   // UMTS related data bearer technologies
   BEARER_TECH_WCDMA,         /**< WCDMA. */
   BEARER_TECH_GPRS,          /**< GPRS. */
   BEARER_TECH_HSDPA,         /**< HSDPA. */
   BEARER_TECH_HSUPA,         /**< HSUPA. */
   BEARER_TECH_EDGE,          /**< EDGE. */
   BEARER_TECH_LTE,           /**< LTE. */
   BEARER_TECH_HSDPA_PLUS,    /**< HSDPA+. */
   BEARER_TECH_DC_HSDPA_PLUS, /**< DC HSDPA+. */
   BEARER_TECH_HSPA,          /**< HSPA */
   BEARER_TECH_64_QAM,        /**< 64 QAM. */
   BEARER_TECH_TDSCDMA,       /**< TD-SCDMA. */
   BEARER_TECH_GSM,           /**< GSM */
   BEARER_TECH_3GPP_WLAN,     /**< IWLAN */
   BEARER_TECH_MAX,
};

/**
 * Data call terminated due to reason type.
 */
enum class DataCallFailType {
   CALL_FAIL_TYPE_UNKNOWN,
   CALL_FAIL_TYPE_MOBILE_IP,
   CALL_FAIL_TYPE_INTERNAL,
   CALL_FAIL_TYPE_CALL_MANAGER_DEFINED,
   CALL_FAIL_TYPE_3GPP_SPEC_DEFINED,
   CALL_FAIL_TYPE_PPP,
   CALL_FAIL_TYPE_EHRPD,
   CALL_FAIL_TYPE_IPV6,
};
struct DataCallFailReason {
   DataCallFailType reasonType
      = DataCallFailType::CALL_FAIL_TYPE_UNKNOWN; /**< Data call terminated due to reason type,
              default is CALL_FAIL_TYPE_UNKNOWN */
   int reasonCode = 0;                            /**< Reason Code corresponding to reason type*/
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif  // DATADEFINES_HPP
