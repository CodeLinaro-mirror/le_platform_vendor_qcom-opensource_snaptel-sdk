/*
 *  Copyright (c) 2017-2018, 2020-2021 The Linux Foundation. All rights reserved.
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
 * @file      PhoneDefines.hpp
 * @brief     PhoneDefines contains enumerations and variables used for
 *            telephony subsystems.
 */
#ifndef TELUX_TEL_PHONEDEFINES_HPP
#define TELUX_TEL_PHONEDEFINES_HPP

#include <array>
#include <bitset>
#include <memory>
#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>

#define DEFAULT_PHONE_ID 1
#define INVALID_PHONE_ID -1
#define THRESHOLD_LIST_MAX 10

namespace telux {

namespace tel {

/** @addtogroup telematics_call
 * @{ */

/**
 * Defines type of call like incoming, outgoing and none.
 */
enum class CallDirection {
  INCOMING,
  OUTGOING,
  NONE,
};

/**
 * Defines the states a call can be in
 */
enum class CallState {
  CALL_IDLE =
      -1,        /**< idle call, default state of a newly created call object */
  CALL_ACTIVE,   /**< active call*/
  CALL_ON_HOLD,  /**< on hold call */
  CALL_DIALING,  /**< out going call, in dialing state and not yet connected,
                      MO Call only */
  CALL_INCOMING, /**< incoming call, not yet answered  */
  CALL_WAITING,  /**< waiting call*/
  CALL_ALERTING, /**< alerting call, MO Call only */
  CALL_ENDED,    /**<  call ended / disconnected */
};

/**
 * Defines call type
 */
enum class CallType {
  UNKNOWN = -1,   /**< Unknown; information is not available */
  VOICE_CALL,     /**< Normal voice call or TPS eCall */
  VOICE_IP_CALL,  /**< Normal Voice over IP (VoIP) call or TPS eCall over IP */
  EMERGENCY_CALL, /**< Non-automotive emergency call, automotive eCall or
                     NGeCall */
  EMERGENCY_IP_CALL, /**< Non-automotive emergency Voice over IP (VoIP) call */
};

/**
 * Reason for the recently terminated call (either normally ended or failed)
 */
enum class CallEndCause {
  UNOBTAINABLE_NUMBER = 1,         /**< Unassigned(unallocated) number */
  NO_ROUTE_TO_DESTINATION = 3,     /**< No route  to destination */
  CHANNEL_UNACCEPTABLE = 6,        /**< Channel unacceptable */
  OPERATOR_DETERMINED_BARRING = 8, /**< Operator determined barring */
  NORMAL = 16,                     /**< Normal call barring */
  BUSY = 17,                       /**< User busy */
  NO_USER_RESPONDING = 18,         /**< No user responding */
  NO_ANSWER_FROM_USER = 19,        /**< User alerting, no answer */
  NOT_REACHABLE = 20,              /**< Not reachable */
  CALL_REJECTED = 21,              /**< Call rejected */
  NUMBER_CHANGED = 22,             /**< Number changed */
  PREEMPTION = 25,                 /**< Pre-emption */
  DESTINATION_OUT_OF_ORDER = 27,   /**< Destination out of order */
  INVALID_NUMBER_FORMAT = 28,  /**< Invalid number format (incomplete number) */
  FACILITY_REJECTED = 29,      /**< Facility rejected */
  RESP_TO_STATUS_ENQUIRY = 30, /**< Response to STATUS ENQUIRY */
  NORMAL_UNSPECIFIED = 31,     /**< Normal, unspecified */
  CONGESTION = 34,             /**< No circuit/channel available */
  NETWORK_OUT_OF_ORDER = 38,   /**< Network out of order */
  TEMPORARY_FAILURE = 41,      /**< Temporary failure */
  SWITCHING_EQUIPMENT_CONGESTION = 42, /**< Switching equipment congestion */
  ACCESS_INFORMATION_DISCARDED = 43,   /**< Access information discarded */
  REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE = 44,
  /**< Requested circuit/channel not available */
  RESOURCES_UNAVAILABLE_OR_UNSPECIFIED =
      47,               /**< Resource unavailable, unspecified */
  QOS_UNAVAILABLE = 49, /**< Quality of service unavailable */
  REQUESTED_FACILITY_NOT_SUBSCRIBED =
      50, /**< Requested facility not subscribed */
  INCOMING_CALLS_BARRED_WITHIN_CUG =
      55, /**< Incoming calls barred within the CUG */
  BEARER_CAPABILITY_NOT_AUTHORIZED =
      57, /**< Bearer capability not authorized */
  BEARER_CAPABILITY_UNAVAILABLE =
      58, /**< Bearer capability not presently available */
  SERVICE_OPTION_NOT_AVAILABLE =
      63, /**< Service or option not available, unspecified */
  BEARER_SERVICE_NOT_IMPLEMENTED = 65, /**< Bearer service not implemented */
  ACM_LIMIT_EXCEEDED = 68, /**< ACM equal to or greater than ACMmax */
  REQUESTED_FACILITY_NOT_IMPLEMENTED =
      69, /**< Requested facility not implemented */
  ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE = 70,
  /**< Only restricted digital information
       bearer capability is available */
  SERVICE_OR_OPTION_NOT_IMPLEMENTED =
      79, /**< Service or option not implemented, unspecified */
  INVALID_TRANSACTION_IDENTIFIER =
      81,                        /**< Invalid transaction identifier value */
  USER_NOT_MEMBER_OF_CUG = 87,   /**< User not member of CUG */
  INCOMPATIBLE_DESTINATION = 88, /**< Incompatible destination */
  INVALID_TRANSIT_NW_SELECTION = 91,   /**< Invalid transit network selection */
  SEMANTICALLY_INCORRECT_MESSAGE = 95, /**< Semantically incorrect message */
  INVALID_MANDATORY_INFORMATION = 96,  /**< Invalid mandatory information */
  MESSAGE_TYPE_NON_IMPLEMENTED =
      97, /**< Message type non-existent or not implemented */
  MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 98,
  /**< Message type not compatible with protocol state */
  INFORMATION_ELEMENT_NON_EXISTENT = 99, /**< Information element non-existent
                                            or not implemented */
  CONDITIONAL_IE_ERROR = 100,            /**< Conditional IE error */
  MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 101,
  /**< Message not compatible with protocol state */
  RECOVERY_ON_TIMER_EXPIRED = 102,  /**< Recovery on timer expiry */
  PROTOCOL_ERROR_UNSPECIFIED = 111, /**< Protocol error, unspecified */
  INTERWORKING_UNSPECIFIED = 127,   /**< Interworking, unspecified */
  CALL_BARRED = 240,                /**< Call barred */
  FDN_BLOCKED = 241,                /**< FDN blocked */
  IMSI_UNKNOWN_IN_VLR = 242,        /**< Incorrect IMSI */
  IMEI_NOT_ACCEPTED = 243,          /**< IMEI not accepted */
  DIAL_MODIFIED_TO_USSD = 244,      /**< DIAL request modified to USSD */
  DIAL_MODIFIED_TO_SS = 245,        /**< DIAL request modified to SS */
  DIAL_MODIFIED_TO_DIAL =
      246,         /**< DIAL request modified to DIAL with different data */
  RADIO_OFF = 247, /**< Radio is OFF */
  OUT_OF_SERVICE = 248,         /**< No cellular coverage */
  NO_VALID_SIM = 249,           /**< No valid SIM is present */
  RADIO_INTERNAL_ERROR = 250,   /**< Internal error at Modem */
  NETWORK_RESP_TIMEOUT = 251,   /**< No response from network */
  NETWORK_REJECT = 252,         /**< Explicit network reject */
  RADIO_ACCESS_FAILURE = 253,   /**< RRC connection failure. Eg.RACH */
  RADIO_LINK_FAILURE = 254,     /**< Radio Link Failure */
  RADIO_LINK_LOST = 255,        /**< Radio link lost due to poor coverage */
  RADIO_UPLINK_FAILURE = 256,   /**< Radio uplink failure */
  RADIO_SETUP_FAILURE = 257,    /**< RRC connection setup failure */
  RADIO_RELEASE_NORMAL = 258,   /**< RRC connection release, normal */
  RADIO_RELEASE_ABNORMAL = 259, /**< RRC connection release, abnormal */
  ACCESS_CLASS_BLOCKED = 260,   /**< Access class barring */
  NETWORK_DETACH = 261,         /**< Explicit network detach */
  EMERGENCY_TEMP_FAILURE = 325, /**< Emergency redial temporary failure */
  EMERGENCY_PERM_FAILURE = 326, /**< Emergency redial permanent failure */
  HO_NOT_FEASIBLE = 382,        /**< Hand over not feasible */
  USER_BUSY = 501,              /**< User busy */
  USER_REJECT = 502,            /**< User reject */
  LOW_BATTERY = 503,            /**< Battery is low */
  BLACKLISTED_CALL_ID = 504,    /**< Blacklisted caller id */
  CS_RETRY_REQUIRED = 505, /**< Retry CS call, VoLTE service can't be provided
                              by the network or remote end */
  CDMA_LOCKED_UNTIL_POWER_CYCLE =
      1000,              /**< MS is locked until next power cycle */
  CDMA_DROP = 1001,      /**< Drop call */
  CDMA_INTERCEPT = 1002, /**< INTERCEPT order received, MS state idle entered */
  CDMA_REORDER = 1003,   /**< MS has been redirected, call is cancelled */
  CDMA_SO_REJECT = 1004, /**< Service option rejection */
  CDMA_RETRY_ORDER =
      1005, /**< Requested service is rejected, retry delay is set */
  CDMA_ACCESS_FAILURE = 1006, /**< Unable to obtain access to the CDMA system */
  CDMA_PREEMPTED = 1007,      /**< Not a preempted call */
  CDMA_NOT_EMERGENCY = 1008,  /**< For non-emergency number dialed
                                   during emergency callback mode */
  CDMA_ACCESS_BLOCKED = 1009, /**< CDMA network access probes blocked */
  NETWORK_UNAVAILABLE = 1010, /**< Network unavailable */
  FEATURE_UNAVAILABLE = 1011, /**< Feature unavailable */
  SIP_ERROR = 1012,           /**< SIP internal error */
  MISC = 1013,                /**< SIP miscellaneous error */
  ANSWERED_ELSEWHERE =
      1014, /**< MT call has ended due to a release from the network
                 because the call was answered elsewhere */
  PULL_OUT_OF_SYNC = 1015,  /**< MultiEndpoint - call pull request has failed */
  CAUSE_CALL_PULLED = 1016, /**< MultiEndpoint - call has been pulled from
                               primary to secondary */
  SIP_REDIRECTED = 2001,    /**< Request is redirected */
  SIP_BAD_REQUEST = 2002,   /**< Bad request */
  SIP_FORBIDDEN = 2003,     /**< Forbidden */
  SIP_NOT_FOUND = 2004,     /**< Remote URI not found */
  SIP_NOT_SUPPORTED = 2005, /**< Not supported */
  SIP_REQUEST_TIMEOUT = 2006,             /**< Request timed out */
  SIP_TEMPORARILY_UNAVAILABLE = 2007,     /**< Temporarily unavailable */
  SIP_BAD_ADDRESS = 2008,                 /**< Address incomplete */
  SIP_BUSY = 2009,                        /**< User busy */
  SIP_REQUEST_CANCELLED = 2010,           /**< Request(call) rejected */
  SIP_NOT_ACCEPTABLE = 2011,              /**< Not acceptable */
  SIP_NOT_REACHABLE = 2012,               /**< Not reachable */
  SIP_SERVER_INTERNAL_ERROR = 2013,       /**< Server internal error */
  SIP_SERVER_NOT_IMPLEMENTED = 2014,      /**< Server not implemented */
  SIP_SERVER_BAD_GATEWAY = 2015,          /**< Server bad gateway */
  SIP_SERVICE_UNAVAILABLE = 2016,         /**< Service unavailable */
  SIP_SERVER_TIMEOUT = 2017,              /**< Server time out */
  SIP_SERVER_VERSION_UNSUPPORTED = 2018,  /**< Server version not supported */
  SIP_SERVER_MESSAGE_TOOLARGE = 2019,     /**< Server message is too large */
  SIP_SERVER_PRECONDITION_FAILURE = 2020, /**< Server pre-condition failure */
  SIP_USER_REJECTED = 2021,               /**< User(call) rejected */
  SIP_GLOBAL_ERROR = 2022,                /**< Global error */
  MEDIA_INIT_FAILED = 3001, /**< Media resource initialization failure */
  MEDIA_NO_DATA =
      3002, /**< RTP timeout(no audio/video traffic in the session) */
  MEDIA_NOT_ACCEPTABLE = 3003,         /**< Media is not supported */
  MEDIA_UNSPECIFIED_ERROR = 3004,      /**< Media unspecified error */
  HOLD_RESUME_FAILED = 3005,           /**< Resume failed for hold call */
  HOLD_RESUME_CANCELED = 3006,         /**< Resume cancelled for hold call */
  HOLD_REINVITE_COLLISION = 3007,      /**< Reinvite collision for hold call */
  SIP_ALTERNATE_EMERGENCY_CALL = 3008, /**< Alternate emergency call */
  NO_CSFB_IN_CS_ROAM = 3009,     /**< CS fallback in roaming not allowed */
  SRV_NOT_REGISTERED = 3010,     /**< Service not registered */
  CALL_TYPE_NOT_ALLOWED = 3011,  /**< Call type not allowed */
  EMRG_CALL_ONGOING = 3012,      /**< Emergency call is in progress */
  CALL_SETUP_ONGOING = 3013,     /**< Call setup is in progress */
  MAX_CALL_LIMIT_REACHED = 3014, /**< Maximum call limit reached */
  UNSUPPORTED_SIP_HDRS = 3015,   /**< Unsupported sip header */
  CALL_TRANSFER_ONGOING = 3016,  /**< Call transfer is in progress */
  PRACK_TIMEOUT = 3017,    /**< Memory allocation failure or RTP open failure */
  QOS_FAILURE = 3018,      /**< Call failed due to lack of dedicated bearer */
  ONGOING_HANDOVER = 3019, /**< Call rejected due to pending handover */
  VT_WITH_TTY_NOT_ALLOWED = 3020, /**< TTY and VT are not supported together */
  CALL_UPGRADE_ONGOING = 3021,    /**< Upgrade request is in progress */
  CONFERENCE_WITH_TTY_NOT_ALLOWED = 3022, /**< Call from conference server
                                             received when TTY is ON */
  CALL_CONFERENCE_ONGOING = 3023,         /**< Conference call is ongoing */
  VT_WITH_AVPF_NOT_ALLOWED = 3024,        /**< VT call with AVPF */
  ENCRYPTION_CALL_ONGOING =
      3025, /**< Encrypted call could not coexist with other calls */
  CALL_ONGOING_CW_DISABLED =
      3026,                 /**< Call waiting disabled during incoming call */
  CALL_ON_OTHER_SUB = 3027, /**< Active call on other subscription */
  ONE_X_COLLISION = 3028,   /**< CDMA collision */
  UI_NOT_READY = 3029,      /**< UI is not ready during the incoming call */
  CS_CALL_ONGOING = 3030, /**< CS call ongoing when incoming call is received */
  REJECTED_ELSEWHERE = 3031, /**< One of the devices (interconnected endpoints)
                                  rejected the call */
  USER_REJECTED_SESSION_MODIFICATION = 3032, /**< Upgrade/downgrade rejected */
  USER_CANCELLED_SESSION_MODIFICATION =
      3033,                           /**< Upgrade/downgrade cancelled */
  SESSION_MODIFICATION_FAILED = 3034, /**< Upgrade/downgrade failed */
  SIP_UNAUTHORIZED = 3035,            /**< Unauthorized */
  SIP_PAYMENT_REQUIRED = 3036,        /**< Payment required */
  SIP_METHOD_NOT_ALLOWED =
      3037, /**< Method requested in the address line was not allowed
                 for the address identified by the request-URI */
  SIP_PROXY_AUTHENTICATION_REQUIRED =
      3038, /**< Client must first authenticate with a proxy */
  SIP_REQUEST_ENTITY_TOO_LARGE =
      3039, /**< Request entity body is larger than what the server
                 is willing to process */
  SIP_REQUEST_URI_TOO_LARGE =
      3040, /**< Server is refusing to service because the request-URI
                 is longer than the server willing to interpret */
  SIP_EXTENSION_REQUIRED =
      3041, /**< Extension to process a request is not listed in the
                 supported header field in the request */
  SIP_INTERVAL_TOO_BRIEF = 3042, /**< Expiration time of the resource refreshed
                                    by the request is too short */
  SIP_CALL_OR_TRANS_DOES_NOT_EXIST =
      3043,                 /**< Request received by a UAS does not match any
                                 existing dialog or transaction */
  SIP_LOOP_DETECTED = 3044, /**< Server detected a loop */
  SIP_TOO_MANY_HOPS =
      3045,             /**< Request received has Max-Forwards header field at 0
                         */
  SIP_AMBIGUOUS = 3046, /**< Requested URI was ambiguous */
  SIP_REQUEST_PENDING = 3047, /**< Request was received by a UAS that had a
                                 pending request within the same dialog */
  SIP_UNDECIPHERABLE = 3048,  /**< Request has an encrypted MIME body for which
                                 the  recipient does not possess an appropriate
                                 decryption  key */
  RETRY_ON_IMS_WITHOUT_RTT =
      3049,            /**< Call should be tried on IMS with RTT disabled */
  MAX_PS_CALLS = 3050, /**< Maximum PS calls exceeded */
  SIP_MULTIPLE_CHOICES = 3051,         /**< Multiple choices */
  SIP_MOVED_PERMANENTLY = 3052,        /**< Moved permanently */
  SIP_MOVED_TEMPORARILY = 3053,        /**< Moved temporarily */
  SIP_USE_PROXY = 3054,                /**< Use proxy */
  SIP_ALTERNATE_SERVICE = 3055,        /**< Alternate service */
  SIP_UNSUPPORTED_URI_SCHEME = 3056,   /**< Unsupported URI scheme */
  SIP_REMOTE_UNSUPP_MEDIA_TYPE = 3057, /**< Unsupported media type */
  SIP_BAD_EXTENSION = 3058,            /**< Bad extension */
  DSDA_CONCURRENT_CALL_NOT_POSSIBLE =
      3059,             /**< Concurrent call is not possible */
  EPSFB_FAILURE = 3060, /**< Call ended due to evolved packet system fallback
                             (EPSFB) failure */
  TWAIT_EXPIRED = 3061, /**< Call ended due to twait timer expired */
  TCP_CONNECTION_REQ = 3062,           /**< Call ended due to TCP connection */
  THERMAL_EMERGENCY = 3100,            /**< Thermal emergency */
  CLIENT_END = 6000,                   /**< Client end */
  INCOM_REJ = 6001,                    /**< Incoming call rejection */
  NO_GATEWAY_SRV = 6002,               /**< No gateway srv */
  NO_FULL_SRV = 6003,                  /**< No full srv */
  CDMA_MAX_ACCESS_PROBE = 6004,        /**< CDMA max access probe */
  CDMA_PSIST_N = 6005,                 /**< CDMA persistence test failure */
  USSD_BUSY = 6006,                    /**< USSD busy*/
  REJECTED_BY_USER = 6007,             /**< Rejected by user */
  NORMAL_CALL_CLEARING = 6008,         /**< Normal call clearing */
  NORMAL_CALL_RINGBACK_TIMEOUT = 6009, /**< Normal call ringback timeout */
  UIM_NOT_PRESENT = 6010,              /**< Uim not present */
  INCOMPATIBLE = 6011,                 /**< Incompatible */
  ALREADY_IN_TC = 6012,                /**< Already in tc */
  USER_CALL_ORIG_DURING_GPS = 6013,    /**< User call orig during gps */
  USER_CALL_ORIG_DURING_SMS = 6014,    /**< User call orig during sms */
  USER_CALL_ORIG_DURING_DATA = 6015,   /**< User call orig during data */
  TRM_REQ_FAIL = 6016,                 /**< Trm req fail */
  CALL_CANNOT_BE_IDENTIFIED = 6017,    /**< Call cannot be identified */
  INCORRECT_SEMANTICS_IN_MESSAGE = 6018, /**< Incorrect semantics in message */
  MANDATORY_INFORMATION_INVALID = 6019,  /**< Mandatory information invalid */
  WRONG_STATE = 6020,                    /**< Wrong state */
  INVALID_USER_DATA = 6021,              /**< Invalid user data */
  CNM_MM_REL_PENDING = 6022,             /**< Cnm mm rel pending */
  ACCESS_STRATUM_REJ_LOW_LEVEL_FAIL =
      6023, /**< Access stratum rej low level fail */
  ACCESS_STRATUM_REJ_LOW_LEVEL_FAIL_REDIAL_NOT_ALLOWED =
      6024, /**< Access stratum rej low level
                 fail redial not allowed */
  ACCESS_STRATUM_REJ_LOW_LEVEL_IMMED_RETRY = 6025, /**< Access stratum rej low
                                                      level immed retry */
  ACCESS_STRATUM_REJ_ABORT_RADIO_UNAVAILABLE =
      6026,                              /**< Access stratum rej abort radio
                                              unavailable */
  CCS_NOT_SUPPORTED_BY_BS = 6027,        /**< Ccs not supported by bs */
  REJECTED_BY_BS = 6028,                 /**< Rejected by bs */
  ACC_FAIL_REJ_ORD = 6029,               /**< Acc fail rej ord */
  ACC_FAIL_RETRY_ORD = 6030,             /**< Acc fail retry ord */
  UNKNOWN_SUBSCRIBER = 6031,             /**< Unknown subscriber */
  ILLEGAL_SUBSCRIBER = 6032,             /**< Illegal subscriber */
  BEARER_SERVICE_NOT_PROVISIONED = 6033, /**< Bearer service not provisioned */
  TELE_SERVICE_NOT_PROVISIONED = 6034,   /**< Tele service not provisioned */
  ILLEGAL_EQUIPMENT = 6035,              /**< Illegal equipment */
  ILLEGAL_SS_OPERATION = 6036,           /**< Illegal ss operation */
  SS_ERROR_STATUS = 6037,                /**< Ss error status */
  SS_NOT_AVAILABLE = 6038,               /**< Ss not available */
  SS_SUBSCRIPTION_VIOLATION = 6039,      /**< Ss subscription violation */
  SS_INCOMPATIBILITY = 6040,             /**< Ss incompatibility */
  FACILITY_NOT_SUPPORTED = 6041,         /**< Facility not supported */
  ABSENT_SUBSCRIBER = 6042,              /**< Absent subscriber */
  SHORT_TERM_DENIAL = 6043,              /**< Short term denial */
  LONG_TERM_DENIAL = 6044,               /**< Long term denial */
  SYSTEM_FAILURE = 6045,                 /**< System failure */
  IMSI_UNKNOWN_IN_HLR = 6046,            /**< Imsi unknown in hlr */
  ILLEGAL_MS = 6047,                     /**< Illegal ms */
  ILLEGAL_ME = 6048,                     /**< Illegal me */
  PLMN_NOT_ALLOWED = 6049,               /**< Plmn not allowed */
  LOCATION_AREA_NOT_ALLOWED = 6050,      /**< Location area not allowed */
  ROAMING_NOT_ALLOWED_IN_THIS_LOCATION_AREA = 6051, /**< Roaming not allowed in
                                                       this location area */
  NO_SUITABLE_CELLS_IN_LOCATION_AREA =
      6052,               /**< No suitable cells in location area */
  NETWORK_FAILURE = 6053, /**< Network failure */
  MAC_FAILURE = 6054,     /**< Mac failure */
  SYNCH_FAILURE = 6055,   /**< Synch failure */
  GSM_AUTHENTICATION_UNACCEPTABLE =
      6056,                            /**< Gsm authentication unacceptable */
  SERVICE_NOT_SUBSCRIBED = 6057,       /**< Service not subscribed */
  ABORT_MSG_RECEIVED = 6058,           /**< Abort msg received */
  SERVICE_OPTION_NOT_SUPPORTED = 6059, /**< Service option not supported */
  AS_REJ_LRRC_CONN_EST_FAILURE_CONN_REJECT = 6060,  /**< As rej lrrc conn est
                                                       failure  conn reject */
  EMM_REJ_SERVICE_REQ_FAILURE_LTE_NW_REJECT = 6061, /**< Emm rej service req
                                                       failure lte nw reject */
  EMM_REJ_SERVICE_REQ_FAILURE_CS_DOMAIN_NOT_AVAILABLE =
      6062,                              /**< Emm rej service req failure
                                              cs domain not available */
  EMM_REJ = 6063,                        /**< Emm rej */
  NETWORK_RESP_TIMEOUT_FROM_BS = 6064,   /**< Network resp timeout from bs */
  NETWORK_RESP_TIMEOUT_T42 = 6065,       /**< Network resp timeout t42 */
  NETWORK_RESP_TIMEOUT_T40 = 6066,       /**< Network resp timeout t40 */
  NETWORK_RESP_TIMEOUT_T50 = 6067,       /**< Network resp timeout t50 */
  NETWORK_RESP_TIMEOUT_T51 = 6068,       /**< Network resp timeout t51 */
  NETWORK_RESP_TIMEOUT_BAD_FL = 6069,    /**< Network resp timeout bad fl */
  NETWORK_RESP_TIMEOUT_T41 = 6070,       /**< Network resp timeout t41 */
  NETWORK_RESP_TIMEOUT_T3230 = 6071,     /**< Network resp timeout t3230 */
  NETWORK_RESP_TIMEOUT_T303 = 6072,      /**< Network resp timeout t303 */
  NETWORK_RESP_TIMEOUT_MT_CSFB = 6073,   /**< Network resp timeout mt csfb */
  NETWORK_RESP_TIMEOUT_T3417_EXT = 6074, /**< Network resp timeout t3417 ext */
  NETWORK_RESP_TIMEOUT_T3417 = 6075,     /**< Network resp timeout t3417 */
  RADIO_ACCESS_FAILURE_REJ_RR_RANDOM =
      6076,                           /**< Radio access failure rej rr random */
  RADIO_ACCESS_ESR_FAILURE = 6077,    /**< Radio access esr failure */
  RADIO_ACCESS_CS_ACQ_FAILURE = 6078, /**< Radio access cs acq failure */
  ACCESS_BARRED = 6079,               /**< Access barred */
  SSAC_REJECT = 6080,                 /**< Ssac reject */
  RADIO_RELEASE_NORMAL_REJ_RR_REL =
      6081, /**< Radio release normal rej rr rel */
  RADIO_RELEASE_NORMAL_REJ_RRC_REL =
      6082, /**< Radio release normal rej rrc rel */
  RADIO_RELEASE_NORMAL_OOS_DURING_CRE =
      6083, /**< Radio release normal oos during cre */
  RADIO_RELEASE_ABNORMAL_CLOSE_SESSION_IND =
      6084, /**< Radio release abnormal close session ind */
  RADIO_RELEASE_ABNORMAL_OPEN_SESSION_FAILURE =
      6085, /**< Radio release abnormal open session
                 failure */
  RADIO_RELEASE_ABNORMAL_CRE_FAILURE =
      6086, /**< Radio release abnormal cre failure */
  RADIO_RELEASE_ABNORMAL_SIB_READ_ERROR =
      6087, /**< Radio release abnormal sib read error */
  RADIO_RELEASE_ABNORMAL_ABORTED_IRAT_SUCCESS =
      6088,                        /**< Radio release abnormal aborted irat
                                        success */
  RADIO_UPLINK_FAILURE_TXN = 6089, /**< Radio uplink failure txn */
  RADIO_UPLINK_FAILURE_HO = 6090,  /**< Radio uplink failure ho */
  RADIO_UPLINK_FAILURE_CTRL_NOT_CONN =
      6091, /**< Radio uplink failure ctrl not conn */
  RADIO_LINK_FAILURE_UL_DATA_CNF = 6092, /**< Radio link failure ul data cnf */
  RADIO_LINK_FAILURE_EST_FAILURE = 6093, /**< Radio link failure est failure */
  RADIO_LINK_FAILURE_CONN_REL_RLF =
      6094,                      /**< Radio link failure conn rel rlf */
  RADIO_LINK_FAILURE_REJ = 6095, /**< Radio link failure rej */
  RADIO_LINK_FAILURE_DURING_CC_DISCONNECT =
      6096, /**< Radio link failure during cc disconnect */
  RADIO_SETUP_FAILURE_REJ = 6097,        /**< Radio setup failure rej */
  RADIO_SETUP_FAILURE_ABORTED = 6098,    /**< Radio setup failure aborted */
  RADIO_SETUP_FAILURE_CELL_RESEL = 6099, /**< Radio setup failure cell resel */
  RADIO_SETUP_FAILURE_CONFIG_FAILURE =
      6100, /**< Radio setup failure config failure */
  RADIO_SETUP_FAILURE_TIMER_EXPIRED =
      6101, /**< Radio setup failure timer expired */
  RADIO_SETUP_FAILURE_SI_FAILURE = 6102, /**< Radio setup failure si failure */
  NETWORK_DETACH_WITH_OUT_REATTACH =
      6103,                /**< Network detach with out reattach */
  PDN_DISCONNECTED = 6104, /**< Pdn disconnected */
  CSFB_FAILURE_CALL_REL_NW_REL_ODR =
      6105, /**< 1xcsfb failure call rel nw rel odr */
  CSFB_FAILURE_CALL_REL_REG_REJ = 6106, /**< 1xcsfb failure call rel reg rej */
  CSFB_FAILURE_RETRY_EXHAUST = 6107,    /**< 1xcsfb failure retry exhaust */
  CSFB_FAILURE_USER_CALL_END = 6108,    /**< 1xcsfb failure user call end */
  CSFB_FAILURE_SRCH_TT_FAIL = 6109,     /**< 1xcsfb failure srch tt fail */
  CSFB_FAILURE_TCH_INIT_FAIL = 6110,    /**< 1xcsfb failure tch init fail */
  CSFB_FAIL_ACQ_FAIL = 6111,            /**< 1xcsfb fail acq fail */
  CSFB_FAIL_CALL_REL_INTERCEPT_ORDER =
      6112,                         /**< 1xcsfb fail call rel intercept order */
  CSFB_FAIL_CALL_REL_NORMAL = 6113, /**< 1xcsfb fail call rel normal */
  CSFB_FAIL_CALL_REL_OTASP_SPC_ERR =
      6114, /**< 1xcsfb fail call rel otasp spc err */
  CSFB_FAIL_CALL_REL_REL_ORDER = 6115, /**< 1xcsfb fail call rel rel order */
  CSFB_FAIL_CALL_REL_REORDER = 6116,   /**< 1xcsfb fail call rel reorder */
  CSFB_FAIL_CALL_REL_SO_REJ = 6117,    /**< 1xcsfb fail call rel so rej */
  CSFB_HARD_FAILURE = 6118,            /**< 1xcsfb hard failure */
  CSFB_HO_FAILURE = 6119,              /**< 1xcsfb ho failure */
  CSFB_MSG_IGNORE = 6120,              /**< 1xcsfb msg ignore */
  CSFB_MSG_INVAILD = 6121,             /**< 1xcsfb msg invaild */
  CSFB_SOFT_FAILURE = 6122,            /**< 1xcsfb soft failure */
  ACCESS_BLOCK = 6123,                 /**< Access block */
  ACC_IN_PROG = 6124,                  /**< Acc in prog */
  ACTIVATION = 6125,                   /**< Activation */
  ADDRESS_INCOMPLETE = 6126,           /**< Address incomplete */
  ALERT_STOP = 6127,                   /**< Alert stop */
  ALTERNATE_EMERGENCY_CALL = 6128,     /**< Alternate emergency call */
  ALTERNATE_SERVICE = 6129,            /**< Alternate service */
  AMBIGUOUS = 6130,                    /**< Ambiguous */
  AS_REJ_LRRC_CONN_EST_FAILURE_NOT_CAMPED =
      6131,                            /**< Conn est failure not camped */
  AS_REJ_LRRC_CONN_EST_SUCCESS = 6132, /**< As rej lrrc conn est success */
  BAD_EXTENSION = 6133,                /**< Bad extension */
  BAD_GATEWAY = 6134,                  /**< Bad gateway */
  BAD_REQ_WAIT_INVITE = 6135,          /**< Bad req wait invite */
  BAD_REQ_WAIT_REINVITE = 6136,        /**< Bad req wait reinvite */
  BUSY_EVERYWHERE = 6137,              /**< Busy everywhere */
  CALL_COMPLETED_ELSEWHERE = 6138,     /**< Call completed elsewhere */
  CALL_DEFLECTED = 6139,               /**< Call deflected */
  CALL_OR_TRANS_DOES_NOT_EXIST = 6140, /**< Call or trans does not exist */
  CALL_PULLED = 6141,                  /**< Call pulled */
  CALL_PULL_OUT_OF_SYNC = 6142,        /**< Call pull out of sync */
  CCBS_NOT_POSSIBLE = 6143,            /**< Ccbs not possible */
  CCBS_POSSIBLE = 6144,                /**< Ccbs possible */
  CLIR_NOT_SUBSCRIBED = 6145,          /**< Clir not subscribed */
  CODEC_ERROR = 6146,                  /**< Codec error */
  CSFB_NOT_FEASIBLE_IN_ROAM_CS_NW =
      6147,                            /**< Csfb not feasible in roam cs nw */
  CS_HARD_FAILURE = 6148,              /**< Cs hard failure */
  CUG_CALL_FAILURE_UNSPECIFIED = 6149, /**< Cug call failure unspecified */
  CUG_INDEX_INCOMPATIBLE = 6150,       /**< Cug index incompatible */
  DATA_CONNECTION_LOST = 6151,         /**< Data connection lost */
  DATA_MISSING = 6153,                 /**< Data missing */
  DEAD_BATTERY = 6154,                 /**< Dead battery */
  DEFLECTION_TO_SERVED_SUBSCRIBER =
      6155,                       /**< Deflection to served subscriber */
  DOES_NOT_EXIST_ANYWHERE = 6156, /**< Does not exist anywhere */
  DRVCC_END_CALL = 6157,          /**< Drvcc end call */
  DRVCC_IN_PROG = 6158,           /**< Drvcc in prog */
  EXTENSION_REQUIRED = 6159,      /**< Extension required */
  FALLBACK_TO_CS = 6160,          /**< Fallback to cs */
  GONE = 6161,                    /**< Gone */
  INCOMING_REJ_CAUSE_1X_COLLISION =
      6162, /**< Incoming rej cause 1x collision */
  INCOMING_REJ_CAUSE_CALL_ONGOING_CB_ENABLED =
      6163, /**< Incoming call ongoing cb enabled */
  INCOMING_REJ_CAUSE_CALL_ONGOING_CW_DISABLED =
      6164, /**< Incoming call ongoing cw disabled */
  INCOMING_REJ_CAUSE_CALL_ON_OTHER_SUB =
      6165, /**< Incoming rej cause call on other sub */
  INCOM_REJ_CAUSE_UI_NOT_READY = 6166,  /**< Incom rej cause ui not ready */
  INTERVAL_TOO_BRIEF = 6167,            /**< Interval too brief */
  INVALID_DEFLECTED_TO_NUMBER = 6168,   /**< Invalid deflected to number */
  INVALID_REMOTE_URI = 6169,            /**< Invalid remote uri */
  IS707B_MAX_ACC = 6170,                /**< Is707b max acc */
  LOOP_DETECTED = 6171,                 /**< Loop detected */
  MC_ABORT = 6172,                      /**< Mc abort */
  MERGED_TO_CONFERENCE = 6173,          /**< Merged to conference */
  MESSAGE_TOO_LARGE = 6174,             /**< Message too large */
  METHOD_NOT_ALLOWED = 6175,            /**< Method not allowed */
  MOVED_PERMANENTLY = 6176,             /**< Moved permanently */
  MOVED_TEMPORARILY = 6177,             /**< Moved temporarily */
  MPTY_PARTICIPANTS_EXCEEDED = 6178,    /**< Mpty participants exceeded */
  MULTIPLE_CHOICES = 6179,              /**< Multiple choices */
  NEGATIVE_PWD_CHECK = 6180,            /**< Negative pwd check */
  NETWORK_NO_RESP_HOLD_FAIL = 6181,     /**< Network no resp hold fail */
  NETWORK_NO_RESP_TIME_OUT = 6182,      /**< Network no resp time out */
  NOT_ACCEPTABLE = 6183,                /**< Not acceptable */
  NOT_ACCEPTABLE_GLOBAL = 6184,         /**< Not acceptable global */
  NOT_ACCEPTABLE_HERE = 6185,           /**< Not acceptable here */
  NOT_IMPLEMENTED = 6186,               /**< Not implemented */
  NO_CDMA_SRV = 6187,                   /**< No cdma srv */
  NO_CELL_AVAILABLE = 6188,             /**< No cell available */
  NO_CUG_SELECTION = 6189,              /**< No cug selection */
  NO_NETWORK_RESP = 6190,               /**< No network resp */
  NO_RESOURCES = 6191,                  /**< No resources */
  NUM_OF_PWD_ATTEMPTS_VIOLATION = 6192, /**< Num of pwd attempts violation */
  OTASP_SPC_ERR = 6193,                 /**< Otasp spc err */
  OUTGOING_CALLS_BARRED_WITHIN_CUG =
      6194,                             /**< Outgoing calls barred within cug */
  PAYMENT_REQUIRED = 6195,              /**< Payment required */
  POSITION_METHOD_FAILURE = 6196,       /**< Position method failure */
  PRECONDITION_FAILURE = 6197,          /**< Precondition failure */
  PROXY_AUTHENTICATION_REQUIRED = 6198, /**< Proxy authentication required */
  PWD_REGISTRATION_FAILURE = 6199,      /**< Pwd registration failure */
  REDIR_OR_HANDOFF = 6200,              /**< Redir or handoff */
  REG_RESTORATION = 6201,               /**< Reg restoration */
  REMOTE_UNSUPP_MEDIA_TYPE = 6202,      /**< Remote unsupp media type */
  REQUEST_ENTITY_TOO_LARGE = 6203,      /**< Request entity too large */
  REQUEST_PENDING = 6204,               /**< Request pending */
  REQUEST_TERMINATED = 6205,            /**< Request terminated */
  REQUEST_URI_TOO_LARGE = 6206,         /**< Request uri too large */
  RESOURCES_NOT_AVAILABLE = 6207,       /**< Resources not available */
  RRC_CONN_REL_NO_MT_SETUP = 6208,      /**< Rrc conn rel no mt setup */
  RTP_FAILURE = 6209,                   /**< Rtp failure */
  RTP_RTCP_TIMEOUT = 6210,              /**< Rtp rtcp timeout */
  SERVER_INTERNAL_ERROR = 6211,         /**< Server internal error */
  SERVER_TIME_OUT = 6212,               /**< Server time out */
  SERVER_UNAVAILABLE = 6213,            /**< Server unavailable */
  SESS_DESCR_NOT_ACCEPTABLE = 6214,     /**< Sess descr not acceptable */
  SIP_403_FORBIDDEN = 6215,             /**< Sip 403 forbidden */
  SIP_503_SERVER_UNAVAILABLE = 6216,    /**< Sip 503 server unavailable */
  SPECIAL_SERVICE_CODE = 6217,          /**< Special service code */
  SRVCC_END_CALL = 6218,                /**< Srvcc end call */
  SRV_INIT_FAIL = 6219,                 /**< Srv init fail */
  TOO_MANY_HOPS = 6220,                 /**< Too many hops */
  UNAUTHORIZED = 6221,                  /**< Unauthorized */
  UNDECIPHERABLE = 6222,                /**< Undecipherable */
  UNEXPECTED_DATA_VALUE = 6223,         /**< Unexpected data value */
  UNKNOWN_ALPHABET = 6224,              /**< Unknown alphabet */
  UNKNOWN_CUG_INDEX = 6225,             /**< Unknown cug index */
  UNSUPPORTED_SDP = 6226,               /**< Unsupported sdp */
  UNSUPPORTED_URI_SCHEME = 6227,        /**< Unsupported uri scheme */
  UNWANTED_CALL = 6228,                 /**< Unwanted call */
  UPGRADE_DOWNGRADE_CANCELLED = 6229,   /**< Upgrade downgrade cancelled */
  UPGRADE_DOWNGRADE_FAILED = 6230,      /**< Upgrade downgrade failed */
  UPGRADE_DOWNGRADE_REJ = 6231,         /**< Upgrade downgrade rej */
  USE_PROXY = 6232,                     /**< Use proxy */
  VERSION_NOT_SUPPORTED = 6233,         /**< Version not supported */

  ERROR_UNSPECIFIED = 0xffff, /**< Error unspecified */
};

/**
 * Defines the real time text (RTT) mode of a call.
 */
enum class RttMode {
  UNKNOWN = -1, /*< RTT mode data is unknown */
  DISABLED = 0, /*< RTT mode is not enabled */
  FULL = 1,     /*< RTT call being used by both the parties*/
};

/** @} */ /* end_addtogroup telematics_call */

/** @addtogroup telematics_phone
 * @{ */

/**
 * Defines the radio state
 */
enum class RadioState {
  RADIO_STATE_OFF = 0, /**< Radio is explicitly powered off */
  RADIO_STATE_UNAVAILABLE =
      1,               /**< Radio unavailable (eg, resetting or not booted) */
  RADIO_STATE_ON = 10, /**< Radio is on */
};

/**
 * Defines the service states
 *
 * @deprecated Use requestVoiceServiceState() API or  to know the status of
 * phone
 */
enum class ServiceState {
  EMERGENCY_ONLY, /**< Only emergency calls allowed */
  IN_SERVICE,     /**< Normal operation, device is registered with a carrier and
                       online */
  OUT_OF_SERVICE, /**< Device is not registered with any carrier */
  RADIO_OFF,      /**< Device radio is off - Airplane mode for example */
};

/**
 * Defines all available radio access technologies
 */
enum class RadioTechnology {
  RADIO_TECH_UNKNOWN,  /**< Network type is unknown */
  RADIO_TECH_GPRS,     /**< Network type is GPRS */
  RADIO_TECH_EDGE,     /**< Network type is EDGE */
  RADIO_TECH_UMTS,     /**< Network type is UMTS */
  RADIO_TECH_IS95A,    /**< Network type is IS95A */
  RADIO_TECH_IS95B,    /**< Network type is IS95B */
  RADIO_TECH_1xRTT,    /**< Network type is 1xRTT */
  RADIO_TECH_EVDO_0,   /**< Network type is EVDO revision 0 */
  RADIO_TECH_EVDO_A,   /**< Network type is EVDO revision A */
  RADIO_TECH_HSDPA,    /**< Network type is HSDPA */
  RADIO_TECH_HSUPA,    /**< Network type is HSUPA */
  RADIO_TECH_HSPA,     /**< Network type is HSPA */
  RADIO_TECH_EVDO_B,   /**< Network type is EVDO revision B*/
  RADIO_TECH_EHRPD,    /**< Network type is eHRPD */
  RADIO_TECH_LTE,      /**< Network type is LTE */
  RADIO_TECH_HSPAP,    /**< Network type is HSPA+ */
  RADIO_TECH_GSM,      /**< Network type is GSM, Only supports voice */
  RADIO_TECH_TD_SCDMA, /**< Network type is TD SCDMA */
  RADIO_TECH_IWLAN,    /**< Network type is TD IWLAN */
  RADIO_TECH_LTE_CA,   /**< Network type is LTE CA */
  RADIO_TECH_NR5G,     /**< Network type is NR5G */
  RADIO_TECH_NB1_NTN,  /**< Network type is NB-IoT(NB1) Non Terrestrial
                          Network(NTN) */
};

/**
 * Defines network types.
 */
enum class NetworkMode {
    UNKNOWN = -1, /**< Network mode is unknown */
    GSM,          /**< Network mode is GSM */
    WCDMA,        /**< Network mode is WCDMA */
    LTE,          /**< Network mode is LTE */
    NR5G,         /**< Network mode is NR5G SA and NSA */
    NR5G_SA,      /**< Network mode is NR5G SA */
    NR5G_NSA,     /**< Network mode is NR5G NSA */
    NB1_NTN,      /**< Network mode is NB-IoT(NB1) Non Terrestrial Network(NTN) */
};

/**
 * Defines all available RAT capabilities for each subscription
 */
enum class RATCapability {
  AMPS,    /**< AMPS mode */
  CDMA,    /**< CDMA mode */
  HDR,     /**< HDR mode */
  GSM,     /**< GSM mode */
  WCDMA,   /**< WCDMA mode */
  LTE,     /**< LTE mode */
  TDS,     /**< TD-SCDMA mode */
  NR5G,    /**< NR5G NSA mode */
  NR5GSA,  /**< NR5G SA mode */
  NB1_NTN, /**< NB-IoT(NB1) Non Terrestrial Network(NTN) mode */
};

using RATCapabilitiesMask = std::bitset<16>;

/**
 * Defines all voice support available on device
 */
enum class VoiceServiceTechnology {
  VOICE_TECH_GW_CSFB,
  VOICE_TECH_1x_CSFB,
  VOICE_TECH_VOLTE,
};

using VoiceServiceTechnologiesMask = std::bitset<16>;

/**
 * Structure contains slotID and RAT capabilities corresponding to slot.
 */
struct SimRatCapability {
  int slotId; /**< Number of slotid's */
  RATCapabilitiesMask
      capabilities; /**< This field indicates RATCapabilitiesMask */
};

/**
 * For Device max subcription capability
 */
using DeviceRatCapability = SimRatCapability;

/**
 * Structure contains information about device capability.
 */
struct CellularCapabilityInfo {
  VoiceServiceTechnologiesMask
      voiceServiceTechs; /**< Indicates voice support capabilities */
  int simCount;          /**< The maximum number of SIMs that can be supported
                            simultaneously */
  int maxActiveSims; /**< The maximum number of SIMs that can be simultaneously
                        active. If this number is less than numberofSims, it
                        implies that any combination of the SIMs can be active
                        and the remaining can be in standby. */
  std::vector<SimRatCapability>
      simRatCapabilities; /**< A Sim inserted in a slot allows for certain rat
                             capabilities. And the UE's HW allows for certain
                             rat capabilities. This field lists the intersection
                             of capabilities allowed by the Sim and the HW. The
                             capabilities are indexed based on slotId. */
  std::vector<DeviceRatCapability>
      deviceRatCapability; /**< This field lists the Rat capabilities supported
                              by the HW on a given Sim slot. The capabilities
                              are indexed based on slotId. */
};

/**
 * Defines operating modes of the device.
 */
enum class OperatingMode {
  ONLINE = 0,           /**< Online mode */
  AIRPLANE,             /**< Low Power mode i.e temporarily disabled RF */
  FACTORY_TEST,         /**< Special mode for manufacturer use*/
  OFFLINE,              /**< Device has deactivated RF and partially shutdown */
  RESETTING,            /**< Device is in process of power cycling */
  SHUTTING_DOWN,        /**< Device is in process of shutting down */
  PERSISTENT_LOW_POWER, /**< Persists low power mode even on reset*/
};

/**
 * Emergency callback mode
 */
enum class EcbMode {
  NORMAL = 0, /**< Device is not in emergency callback mode(ECBM) */
  EMERGENCY,  /**< Device is in emergency callback mode(ECBM) */
};

/**
 * Defines the radio SignalStrength types for delta or threshold.
 * @deprecated Use SignalStrengthMeasurementType with RadioTechnology.
 */
enum class RadioSignalStrengthType {
  GSM_RSSI,   /**< GSM received signal strength indicator.*/
  WCDMA_RSSI, /**< WCDMA received signal strength indicator.*/
  LTE_RSSI,   /**< LTE received signal strength indicator.*/
  LTE_SNR,    /**< LTE signal-to-noise ratio.*/
  LTE_RSRQ,   /**< LTE reference signal received quality.*/
  LTE_RSRP,   /**< LTE reference signal received power.*/
  NR5G_SNR,   /**< NR5G signal-to-noise ratio.*/
  NR5G_RSRP,  /**< NR5G reference signal received power.*/
  NR5G_RSRQ,  /**< NR5G reference signal received quality.*/
};

/**
 * Defines the SignalStrength configuration parameters.
 * @deprecated Use SignalStrengthConfigExType.
 */
enum class SignalStrengthConfigType {
  DELTA = 1,     /**< Signal strength delta provided. */
  THRESHOLD = 2, /**< Signal strength threshold provided. */
};

/**
 * Defines different configuration types to configure the signal strength
 * notification. Each value represents a corresponding bit for the
 * SignalStrengthConfigMask bitset.
 */
enum SignalStrengthConfigExType {
  DELTA = 1,         /**< Signal strength delta provided. */
  THRESHOLD = 2,     /**< Signal strength threshold list provided. */
  HYSTERESIS_DB = 3, /**< Signal strength hysteresis delta provided. */
};

/**
 * 8-bit mask that denotes which signal strength config type is used for the
 * signal strength configuration.
 */
using SignalStrengthConfigMask = std::bitset<8>;

/**
 * Defines different signal strength measurement types.
 */
enum class SignalStrengthMeasurementType {
  RSSI, /**< Received signal strength indicator. */
  ECIO, /**< Energy per chip to interference power ratio. */
  SINR, /**< Signal-to-interference-plus-noise ratio. */
  IO,   /**< Interference power ratio. */
  RSRQ, /**< Reference signal received quality. */
  RSRP, /**< Reference signal received power. */
  SNR,  /**< Signal-to-noise ratio. */
  RSCP, /**< Received signal code power. */
};

/**
 * Defines the SignalStrength threshold parameters.
 * @deprecated Use the thresholdList field from SignalStrengthConfigData.
 */
struct SignalStrengthThreshold {
  int32_t lowerRangeThreshold; /**< Lower threshold for the selected radio
                                  technology. */
  int32_t upperRangeThreshold; /**< Upper threshold for the selected radio
                                  technology. */
};

/**
 * Defines the SignalStrength notification configuration parameters and their
 * corresponding values.
 * @deprecated Use SignalStrengthConfigEx.
 */
struct SignalStrengthConfig {
  SignalStrengthConfigType
      configType; /**< Signal strength configuration type. */
  RadioSignalStrengthType ratSigType; /**< Radio signal strength type. */

  /**< Signal strength data. */
  union {
    uint16_t delta;                    /**< Signal strength delta. */
    SignalStrengthThreshold threshold; /**< Signal strength threshold. */
  };
};

/**
 *  Represents Operator information
 */
struct PlmnInfo {
  std::string longName;  /**< Represents long Name for Network */
  std::string shortName; /**< Represents short Name for Network */
  std::string
      plmn; /**< Represents PLMN code for Network, consists of a MCC and MNC. */
  telux::common::BoolValue
      isHome; /**< Represents whether the network is the home network, default
                 state is STATE_UNKNOWN */
};

/**
 * Defines the signal strength configuration data parameters.
 */
struct SignalStrengthConfigData {
  SignalStrengthMeasurementType
      sigMeasType; /**< Signal strength measurement type. */
  /**< Signal strength data. */
  union {
    uint16_t delta; /**< Signal strength delta. */
    struct {
      std::array<int32_t, THRESHOLD_LIST_MAX> thresholdList;
      /**< Signal strength threshold list. */
      uint16_t hysteresisDb =
          0; /**< (Optional) Signal strength hysteresis delta; note hysteresis
                db is not mandatory but hystersis db requires that the threshold
                list is specified. */
    };
  };
};

/**
 * Defines the signal strength notification configuration parameters.
 */
struct SignalStrengthConfigEx {
  SignalStrengthConfigMask
      configTypeMask; /**< Signal strength configuration mask. Both delta and
                         threshold can't be sent in single request. Hysteresis
                         db is applicable only when threshold is configured. */
  RadioTechnology radioTech; /**< Radio technology. */
  std::vector<SignalStrengthConfigData>
      sigConfigData; /**< Signal strength data. */
};

/** @} */ /* end_addtogroup telematics_phone */

} // End of namespace tel

} // End of namespace telux

#endif // TELUX_TEL_PHONEDEFINES_HPP
