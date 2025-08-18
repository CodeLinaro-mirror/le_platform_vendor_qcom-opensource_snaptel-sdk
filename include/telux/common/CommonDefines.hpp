/*
 *  Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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
 * @file       CommonDefines.hpp
 * @brief      Contains enumerations and variables used across telephony, location, data subsystems.
 *             Also defines interface for command callback for asynchronous calls.
 *
 */

#ifndef COMMONDEFINES_HPP
#define COMMONDEFINES_HPP

#include <functional>

/**
 * Specifies the slot id where the Uicc card is inserted
 */
typedef enum {
   INVALID_SLOT_ID = -1,
   DEFAULT_SLOT_ID =  1,
   SLOT_ID_1 = DEFAULT_SLOT_ID,
   SLOT_ID_2 = 2,
   MAX_SLOT_ID = SLOT_ID_2,
}SlotId;

namespace telux {
namespace common {

/** @addtogroup telematics_common
 * @{ */
/**
 * Defines all the status codes that all Telematics SDK APIs can return
 */
enum class Status {
   SUCCESS,        /**< API processing is successful, returned parameters are valid*/
   FAILED,         /**< API processing failure.*/
   NOCONNECTION,   /**< Connection to Socket server has not been established*/
   NOSUBSCRIPTION, /**< Subscription not available */
   INVALIDPARAM,   /**< Input parameters are invalid*/
   INVALIDSTATE,   /**< Invalid State*/
   NOTREADY,       /**< Subsystem is not ready*/
   NOTALLOWED,     /**< Operation not allowed*/
   NOTIMPLEMENTED, /**< Functionality not implemented*/
   CONNECTIONLOST, /**< Connection to Socket server lost*/
   EXPIRED,        /**< Expired */
   ALREADY,        /**< Already registered handler */
   NOSUCH,         /**< No such object */
   NOTSUPPORTED,   /**< Not supported on target platform */
   NOMEMORY,       /**< Not sufficient memory to process the request */
};

/**
 * Generic Error code for each API responses
 */
enum class ErrorCode {
   SUCCESS = 0,                          /**< No error */
   RADIO_NOT_AVAILABLE = 1,              /**< If radio did not start or is resetting */
   GENERIC_FAILURE = 2,                  /**< Generic Failure */
   PASSWORD_INCORRECT = 3,               /**< For PIN/PIN2 methods only */
   SIM_PIN2 = 4,                         /**< Operation requires SIM PIN2 to be entered */
   SIM_PUK2 = 5,                         /**< Operation requires SIM PIN2 to be entered */
   REQUEST_NOT_SUPPORTED = 6,            /**< Not Supported request */
   CANCELLED = 7,                        /**< Cancelled */
   OP_NOT_ALLOWED_DURING_VOICE_CALL = 8, /**< Data operation are not allowed during voice
                                              call on a Class C GPRS device */
   OP_NOT_ALLOWED_BEFORE_REG_TO_NW = 9,  /**< Data operation are not allowed before device
                                            registers in network */
   SMS_SEND_FAIL_RETRY = 10,             /**< Fail to send SMS and need retry */
   SIM_ABSENT = 11,                      /**< Fail to set the location where CDMA subscription
                                              shall be retrieved because of SIM or RUIM
                                              are absent */
   SUBSCRIPTION_NOT_AVAILABLE = 12,      /**< Fail to find CDMA subscription from specified
                                              location */
   MODE_NOT_SUPPORTED = 13,              /**< Hardware does not support preferred network type */
   FDN_CHECK_FAILURE = 14,               /**< Command failed because recipient is not on FDN list */
   ILLEGAL_SIM_OR_ME = 15,               /**< Network selection failed due to
                                              illegal SIM or ME */
   MISSING_RESOURCE = 16,                /**< No logical channel available */
   NO_SUCH_ELEMENT = 17,                 /**< Application not found on SIM */
   DIAL_MODIFIED_TO_USSD = 18,           /**< DIAL request modified to USSD */
   DIAL_MODIFIED_TO_SS = 19,             /**< DIAL request modified to SS */
   DIAL_MODIFIED_TO_DIAL = 20,           /**< DIAL request modified to DIAL with different
                                              data */
   USSD_MODIFIED_TO_DIAL = 21,           /**< USSD request modified to DIAL */
   USSD_MODIFIED_TO_SS = 22,             /**< USSD request modified to SS */
   USSD_MODIFIED_TO_USSD = 23,           /**< USSD request modified to different USSD
                                              request */
   SS_MODIFIED_TO_DIAL = 24,             /**< SS request modified to DIAL */
   SS_MODIFIED_TO_USSD = 25,             /**< SS request modified to USSD */
   SUBSCRIPTION_NOT_SUPPORTED = 26,      /**< Subscription not supported */
   SS_MODIFIED_TO_SS = 27,               /**< SS request modified to different SS request */
   LCE_NOT_SUPPORTED = 36,               /**< LCE service not supported */
   NO_MEMORY = 37,                       /**< Not sufficient memory to process the request */
   INTERNAL_ERR = 38,                    /**< Hit unexpected vendor internal error scenario */
   SYSTEM_ERR = 39,                      /**< Hit platform or system error */
   MODEM_ERR = 40,                       /**< Hit unexpected modem error */
   INVALID_STATE = 41,                   /**< Unexpected request for the current state */
   NO_RESOURCES = 42,                    /**< Not sufficient resource to process the request */
   SIM_ERR = 43,                         /**< Received error from SIM card */
   INVALID_ARGUMENTS = 44,               /**< Received invalid arguments in request */
   INVALID_SIM_STATE = 45,               /**< Cannot process the request in current SIM state */
   INVALID_MODEM_STATE = 46,             /**< Cannot process the request in current Modem state */
   INVALID_CALL_ID = 47,                 /**< Received invalid call id in request */
   NO_SMS_TO_ACK = 48,                   /**< ACK received when there is no SMS to ack */
   NETWORK_ERR = 49,                     /**< Received error from network */
   REQUEST_RATE_LIMITED = 50,            /**< Operation denied due to overly-frequent requests */
   SIM_BUSY = 51,                        /**< SIM is busy */
   SIM_FULL = 52,                        /**< The target EF is full */
   NETWORK_REJECT = 53,                  /**< Request is rejected by network */
   OPERATION_NOT_ALLOWED = 54,           /**< Not allowed the request now */
   EMPTY_RECORD = 55,                    /**< The request record is empty */
   INVALID_SMS_FORMAT = 56,              /**< Invalid SMS format */
   ENCODING_ERR = 57,                    /**< Message not encoded properly */
   INVALID_SMSC_ADDRESS = 58,            /**< SMSC address specified is invalid */
   NO_SUCH_ENTRY = 59,                   /**< No such entry present to perform the request */
   NETWORK_NOT_READY = 60,               /**< Network is not ready to perform the request */
   NOT_PROVISIONED = 61,                 /**< Device does not have this value provisioned */
   NO_SUBSCRIPTION = 62,                 /**< Device does not have subscription */
   NO_NETWORK_FOUND = 63,                /**< Network cannot be found */
   DEVICE_IN_USE = 64,                   /**< Operation cannot be performed because the device
                                              is currently in use */
   ABORTED = 65,                         /**< Operation aborted */
   INCOMPATIBLE_STATE = 90,              /**< Operation cannot be performed because the device
                                          is in incompatible state */
   NO_EFFECT = 101,                      /**< Given request had to no effect */
   DEVICE_NOT_READY = 102,               /**< Device not ready */
   MISSING_ARGUMENTS = 103,              /**< Missing one or more arguments */

   PIN_PERM_BLOCKED = 201,               /**< PIN is permanently blocked. The SIM is unusable. */
   PIN_BLOCKED = 202,                    /**< PIN is blocked. Unblock operation must be issued. */
   MALFORMED_MSG = 1001,                 /**< Message was not formulated correctly
                                              by the control point or the message was corrupted
                                              during transmission */
   INTERNAL = 1003,                      /**< Internal error*/
   CLIENT_IDS_EXHAUSTED = 1005,          /**< Client IDs exhausted */
   UNABORTABLE_TRANSACTION = 1006,       /**< The specified transaction could not be aborted */
   INVALID_CLIENT_ID = 1007,             /**< Could not find client's request */
   NO_THRESHOLDS = 1008,                 /**< No thresholds specified in enable signal strength */
   INVALID_HANDLE = 1009,                /**< Invalid client handle was received */
   INVALID_PROFILE = 1010,               /**< Invalid profile index specified */
   INVALID_PINID = 1011,                 /**< PIN in the request is invalid. */
   INCORRECT_PIN = 1012,                 /**< PIN in the request is incorrect. */
   CALL_FAILED = 1014,                   /**< Call origination failed in the lower layers */
   OUT_OF_CALL = 1015,                   /**< Request issued when packet data session
                                              disconnected */
   MISSING_ARG = 1017,                   /**< TLV was missing in the request. */
   ARG_TOO_LONG = 1019,                  /**< Path in the request was too long. */
   INVALID_TX_ID = 1022,                 /**< The transaction ID supplied in the request
                                              does not match any pending transaction i.e. either
                                              the transaction was not received or it is already
                                              executed by the device */
   OP_NETWORK_UNSUPPORTED = 1024,        /**< Selected operation is not supported by the network */
   OP_DEVICE_UNSUPPORTED = 1025,         /**< Operation is not supported by device or SIM card */
   NO_FREE_PROFILE = 1027,               /**< Maximum number of profiles are stored in the device
                                              and there is no more storage available to create
                                              a new profile */
   INVALID_PDP_TYPE = 1028,              /**< PDP type specified is not supported */
   INVALID_TECH_PREF = 1029,             /**< Invalid technology preference */
   INVALID_PROFILE_TYPE = 1030,          /**< Invalid profile type is specified */
   INVALID_SERVICE_TYPE = 1031,          /**< Invalid service type */
   INVALID_REGISTER_ACTION = 1032,       /**< Invalid register action value specified in request */
   INVALID_PS_ATTACH_ACTION = 1033,      /**< Invalid PS attach action value specified in request */
   AUTHENTICATION_FAILED = 1034,         /**< Authentication error. */
   SIM_NOT_INITIALIZED = 1037,           /**< PIN is not yet initialized because the SIM
                                              initialization has not finished. Try the PIN
                                              operation later. */
   MAX_QOS_REQUESTS_IN_USE = 1038,       /**< Maximum QoS requests in use */
   INCORRECT_FLOW_FILTER = 1039,         /**< Incorrect flow filter  */
   NETWORK_QOS_UNAWARE = 1040,           /**< Network QoS unaware */
   INVALID_ID = 1041,                    /**< Invalid call ID was sent in the request */
   REQUESTED_NUM_UNSUPPORTED = 1042,     /**< Requested message ID is not supported by the
                                              currently running software */
   INTERFACE_NOT_FOUND = 1043,           /**< Cannot retrieve the FMC interface */
   FLOW_SUSPENDED = 1044,                /**< Flow suspended */
   INVALID_DATA_FORMAT = 1045,           /**< Invalid data format */
   GENERAL = 1046,                       /**< General error */
   UNKNOWN = 1047,                       /**< Unknown error */
   INVALID_ARG = 1048,                   /**< Parameters passed as input were invalid */
   INVALID_INDEX = 1049,                 /**< MIP profile index is not within the valid range */
   NO_ENTRY = 1050,                      /**< No message exists at the specified memory storage
                                              designation */
   DEVICE_STORAGE_FULL = 1051,           /**< Memory storage specified in the request is full */
   CAUSE_CODE = 1054,                    /**< There was an error in the request */
   MESSAGE_NOT_SENT = 1055,              /**< Message could not be sent */
   MESSAGE_DELIVERY_FAILURE = 1056,      /**<  Message could not be delivered */
   INVALID_MESSAGE_ID = 1057,            /**< Message ID specified for the message is invalid */
   ENCODING = 1058,                      /**< Message is not encoded properly */
   AUTHENTICATION_LOCK = 1059,           /**< Maximum number of authentication failures
                                              has been reached */
   INVALID_TRANSITION = 1060,            /**< Selected operating mode transition from the current
                                              operating mode is invalid */
   NOT_A_MCAST_IFACE = 1061,             /**< Not a MCAST interface */
   MAX_MCAST_REQUESTS_IN_USE = 1062,     /**< MCAST request in use */
   INVALID_MCAST_HANDLE = 1063,          /**< An invalid MCAST handle */
   INVALID_IP_FAMILY_PREF = 1064,        /**< IP family preference is invalid */
   SESSION_INACTIVE = 1065,              /**< Session inactive */
   SESSION_INVALID = 1066,               /**< Session not valid */
   SESSION_OWNERSHIP = 1067,             /**< Session ownership error */
   INSUFFICIENT_RESOURCES = 1068,        /**< Response is longer than the maximum supported size */
   DISABLED = 1069,                      /**< Disabled */
   INVALID_OPERATION = 1070,             /**< Device is not expecting the request. */
   INVALID_QMI_CMD = 1071,               /**< Invalid QMI command */
   TPDU_TYPE = 1072,                     /**< Message in memory contains a TPDU type that cannot
                                              be read */
   SMSC_ADDR = 1073,                     /**< SMSC address specified is invalid */
   INFO_UNAVAILABLE = 1074,              /**< Information is not available */
   SEGMENT_TOO_LONG = 1075,              /**< PRL segment size is too large */
   SEGMENT_ORDER = 1076,                 /**< PRL segment order is incorrect */
   BUNDLING_NOT_SUPPORTED = 1077,        /**< Bundling not supported */
   OP_PARTIAL_FAILURE = 1078,            /**< Some personalization codes were set but
                                              an error prevented */
   POLICY_MISMATCH = 1079,               /**< Network policy does not match a valid NAT */
   SIM_FILE_NOT_FOUND = 1080,            /**< File is not present on the card. */
   EXTENDED_INTERNAL = 1081,             /**< Error from the the DS profile module,
                                              the extended error */
   ACCESS_DENIED = 1082,                 /**< Access to the requested file is denied. This can
                                              occur when there is an attempt to access a
                                              PIN-protected file. */
   HARDWARE_RESTRICTED = 1083,           /**< Selected operating mode is invalid with the current
                                              wireless disable setting */
   ACK_NOT_SENT = 1084,                  /**< ACK could not be sent */
   INJECT_TIMEOUT = 1085,                /**< Inject timeout */
   FDN_RESTRICT = 1091,                  /**< FDN restriction */
   SUPS_FAILURE_CAUSE = 1092,            /**< Indicates supplementary services failure
                                              information; */
   NO_RADIO = 1093,                      /**< Radio is not available */
   NOT_SUPPORTED = 1094,                 /**< Operation is not supported */
   CARD_CALL_CONTROL_FAILED = 1096,      /**< SIM/R-UIM call control failed */
   NETWORK_ABORTED = 1097,               /**< Operation was released abruptly by the network */
   MSG_BLOCKED = 1098,                   /**< Message blocked */
   INVALID_SESSION_TYPE = 1100,          /**< Invalid session type */
   INVALID_PB_TYPE = 1101,               /**< Invalid Phone Book type */
   NO_SIM = 1102,                        /**< Action is being performed on a SIM that is not
                                              initialized. */
   PB_NOT_READY = 1103,                  /**< Phone Book not ready */
   PIN_RESTRICTION = 1104,               /**< PIN restriction */
   PIN2_RESTRICTION = 1105,              /**< PIN2 restriction */
   PUK_RESTRICTION = 1106,               /**< PUK restriction */
   PUK2_RESTRICTION = 1107,              /**< PUK2 restriction */
   PB_ACCESS_RESTRICTED = 1108,          /**< Phone Book access restricted */
   PB_DELETE_IN_PROG = 1109,             /**< Phone Book delete in progress */
   PB_TEXT_TOO_LONG = 1110,              /**< Phone Book text too long */
   PB_NUMBER_TOO_LONG = 1111,            /**< Phone Book number too long */
   PB_HIDDEN_KEY_RESTRICTION = 1112,     /**< Phone Book hidden key restriction */
   PB_NOT_AVAILABLE = 1113,              /**< Phone Book not available */
   DEVICE_MEMORY_ERROR = 1114,           /**< Device memory error */
   NO_PERMISSION = 1115,                 /**< No permission */
   TOO_SOON = 1116,                      /**< Too soon */
   TIME_NOT_ACQUIRED = 1117,             /**< Time not acquired */
   OP_IN_PROGRESS = 1118,                /**< Operation is in progress */
   // WDS extended error codes with offset 2000
   DS_PROFILE_REG_RESULT_FAIL = 2001,                   /**< General failure */
   DS_PROFILE_REG_RESULT_ERR_INVAL_HNDL = 2002,         /**< Request contains an invalid
                                                             profile handle */
   DS_PROFILE_REG_RESULT_ERR_INVAL_OP = 2003,           /**< Invalid operation was
                                                             requested */
   DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_TYPE = 2004, /**< Request contains an
                                                             invalid technology type */
   DS_PROFILE_REG_RESULT_ERR_INVAL_PROFILE_NUM = 2005,  /**< Request contains an invalid
                                                             profile number */
   DS_PROFILE_REG_RESULT_ERR_INVAL_IDENT = 2006,        /**< Request contains an invalid
                                                             profile identifier */
   DS_PROFILE_REG_RESULT_ERR_INVAL = 2007,              /**< Request contains an invalid
                                                             argument other than profile
                                                             number and profile identifier
                                                             received */
   DS_PROFILE_REG_RESULT_ERR_LIB_NOT_INITED = 2008,     /**< Profile registry has not been
                                                             initialized yet */
   DS_PROFILE_REG_RESULT_ERR_LEN_INVALID = 2009,        /**< Request contains a parameter
                                                             with invalid length */
   DS_PROFILE_REG_RESULT_LIST_END = 2010,               /**< End of the profile list was
                                                             reached while searching for
                                                             the requested profile */
   DS_PROFILE_REG_RESULT_ERR_INVAL_SUBS_ID = 2011,      /**< Request contains an invalid
                                                             subscription identifier */
   DS_PROFILE_REG_INVAL_PROFILE_FAMILY = 2012,          /**< Request contains an invalid
                                                             profile family*/
   DS_PROFILE_REG_PROFILE_VERSION_MISMATCH = 2013,      /**< Version mismatch */
   REG_RESULT_ERR_OUT_OF_MEMORY = 2014,                 /**< Out of memory */
   DS_PROFILE_REG_RESULT_ERR_FILE_ACCESS = 2015,        /**< File access error */
   DS_PROFILE_REG_RESULT_ERR_EOF = 2016,                /**< End of field */
   REG_RESULT_ERR_VALID_FLAG_NOT_SET = 2017,            /**< A valid flag is not set */
   REG_RESULT_ERR_OUT_OF_PROFILES = 2018,               /**< Out of profiles */
   REG_RESULT_NO_EMERGENCY_PDN_SUPPORT = 2019,          /**< No emergency PDN support */

   /*cv2x specific error codes*/
   V2X_ERR_EXCEED_MAX                     = 3000,       /**< Exceed max allowed number */
   V2X_ERR_V2X_DISABLED                   = 3001,       /**< V2x mode was not enabled*/
   V2X_ERR_UNKNOWN_SERVICE_ID             = 3002,       /**< The service id unknown*/
   V2X_ERR_SRV_ID_L2_ADDRS_NOT_COMPATIBLE = 3003,       /**< The service Id mismatch with L2 addr*/
   V2X_ERR_PORT_UNAVAIL                   = 3004,       /**< The port was occupied by others*/

   // WDS extended error codes without offset
   DS_PROFILE_3GPP_INVAL_PROFILE_FAMILY = 4097,           /**< Request contains an invalid
                                                               3GPP profile family */
   DS_PROFILE_3GPP_ACCESS_ERR = 4098,                     /**< Error was encountered while
                                                               accessing the 3GPP profiles*/
   DS_PROFILE_3GPP_CONTEXT_NOT_DEFINED = 4099,            /**< Specified 3GPP profile does
                                                               not have a valid context*/
   DS_PROFILE_3GPP_VALID_FLAG_NOT_SET = 4100,             /**< Specified 3GPP profile is
                                                               marked invalid */
   DS_PROFILE_3GPP_READ_ONLY_FLAG_SET = 4101,             /**< Specified 3GPP profile is
                                                               marked read-only */
   DS_PROFILE_3GPP_ERR_OUT_OF_PROFILES = 4102,            /**< Creation of a new 3GPP profile
                                                               failed because the limit of
                                                               16 profiles has already been
                                                               reached*/
   DS_PROFILE_3GPP2_ERR_INVALID_IDENT_FOR_PROFILE = 4353, /**< Invalid profile
                                                               identifier was received as
                                                               part of the 3GPP2 profile
                                                               modification request */
   DS_PROFILE_3GPP2_ERR_OUT_OF_PROFILE = 4354,            /**< Creation of a new 3GPP2 profile
                                                               failed because the limit has already
                                                               been reached*/

   // Transport error codes
   INTERNAL_ERROR = -1,        /**< Internal error */
   SERVICE_ERROR = -2,         /**< Service error */
   TIMEOUT_ERROR = -3,         /**< Timeout error */
   EXTENDED_ERROR = -4,        /**< Extended error */
   PORT_NOT_OPEN_ERROR = -5,   /**< Port not open */
   MEMCOPY_ERROR = -13,        /**< Memory copy error */
   INVALID_TRANSACTION = -14,  /**< Invalid transaction */
   ALLOCATION_FAILURE = -15,   /**< Allocation failure */
   TRANSPORT_ERROR = -16,      /**< Transport error */
   PARAM_ERROR = -17,          /**< Parameter error */
   INVALID_CLIENT = -18,       /**< Invalid client */
   FRAMEWORK_NOT_READY = -19,  /**< Framework not ready */
   INVALID_SIGNAL = -20,       /**< Invalid signal */
   TRANSPORT_BUSY_ERROR = -21, /**< Transport busy error */

   // SDK Error codes
   SUBSYSTEM_UNAVAILABLE = 5000, /**< Underlying service currently unavailable */
   OPERATION_TIMEOUT = 5001,     /**< Timeout error */
   ROLLBACK_FAILED = 5002,       /**< Rollback to initial state failed */

   /*Audio specific error codes*/
   OPERATION_NOT_PERMITTED = 6001,              /**< Operation not permitted.*/
   NO_SUCH_FILE_OR_DIRECTORY = 6002,            /**< No such file or directory.*/
   NO_SUCH_PROCESS = 6003,                      /**< No such process.*/
   INTERRUPTED_SYSTEM_CALL = 6004,              /**< Interrupted system call.*/
   INPUT_OUTPUT_ERROR = 6005,                   /**< Input/output error.*/
   NO_SUCH_DEVICE_OR_ADDRESS = 6006,            /**< No such device or address.*/
   ARGUMENT_LIST_TOO_LONG = 6007,               /**< Argument list too long.*/
   EXEC_FORMAT_ERROR = 6008,                    /**< Exec format error. */
   BAD_FILE_NUMBER = 6009,                      /**< Bad file number. */
   NO_CHILD_PROCESSES = 6010,                   /**< No child processes.*/
   RESOURCE_TEMPORARILY_UNAVAILABLE = 6011,     /**< Resource temporarily unavailable.*/
   CANNOT_ALLOCATE_MEMORY = 6012,               /**< Cannot allocate memory.*/
   PERMISSION_DENIED = 6013,                    /**< Permission denied. */
   BAD_ADDRESS = 6014,                          /**< Bad address.*/
   BLOCK_DEVICE_REQUIRED = 6015,                /**< Block device required.*/
   DEVICE_OR_RESOURCE_BUSY = 6016,              /**< Device or resource busy.*/
   FILE_EXISTS = 6017,                          /**< File exists.*/
   INVALID_CROSS_DEVICE_LINK = 6018,            /**< Invalid cross-device link.*/
   NO_SUCH_DEVICE = 6019,                       /**< No such device. */
   NOT_A_DIRECTORY = 6020,                      /**< Not a directory.*/
   IS_A_DIRECTORY = 6021,                       /**< Is a directory.*/
   INVALID_ARGUMENT = 6022,                     /**< Invalid argument.*/
   TOO_MANY_OPEN_FILES_IN_SYSTEM = 6023,        /**< Too many open files in system.*/
   TOO_MANY_OPEN_FILES = 6024,                  /**< Too many open files.*/
   NOT_A_TTY = 6025,                            /**< Not a teletypewriter.*/
   TEXT_FILE_BUSY = 6026,                       /**< Text file busy.*/
   FILE_TOO_LARGE = 6027,                       /**< File too large.*/
   NO_SPACE_LEFT_ON_DEVICE = 6028,              /**< No space left on device.*/
   ILLEGAL_SEEK = 6029,                         /**< Illegal seek.*/
   READ_ONLY_FILE_SYSTEM = 6030,                /**< Read-only file system.*/
   TOO_MANY_LINKS = 6031,                       /**< Too many links.*/
   BROKEN_PIPE = 6032,                          /**< Broken pipe.*/
   MATH_ARGUMENT_OUT_OF_DOMAIN = 6033,          /**< Math argument out of domain of func.*/
   MATH_RESULT_NOT_REPRESENTABLE = 6034,        /**< Math result not representable.*/
   RESOURCE_DEADLOCK_WOULD_OCCUR = 6035,        /**< Resource deadlock would occur.*/
   FILE_NAME_TOO_LONG = 6036,                   /**< File name too long.*/
   NO_LOCK_AVAIL = 6037,                        /**< No lock available.*/
   FUNCTION_NOT_IMPLEMENTED = 6038,             /**< Function not implemented.*/
   DIRECTORY_NOT_EMPTY = 6039,                  /**< Directory not empty.*/
   TOO_MANY_LEVELS_OF_SYMBOLIC_LINKS = 6040,    /**< Too many levels of symbolic links.*/
   NO_MESSAGE_OF_DESIRED_TYPE = 6042,           /**< No message of desired type.*/
   IDENTIFIER_REMOVED = 6043,                   /**< Identifier removed.*/
   CHANNEL_NUMBER_OUT_OF_RANGE = 6044,          /**< Channel number out of range.*/
   LEVEL_2_NOT_SYNCHRONIZED = 6045,             /**< Level 2 not synchronized.*/
   LEVEL_3_HALTED = 6046,                       /**< Level 3 halted.*/
   LEVEL_3_RESET = 6047,                        /**< Level 3 reset.*/
   LINK_NUMBER_OUT_OF_RANGE = 6048,             /**< Link number out of range.*/
   PROTOCOL_DRIVER_NOT_ATTACHED = 6049,         /**< Protocol driver not attached.*/
   NO_CSI_STRUCTURE_AVAILABLE = 6050,           /**< No CSI structure available.*/
   LEVEL_2_HALTED = 6051,                       /**< Level 2 halted.*/
   INVALID_EXCHANGE = 6052,                     /**< Invalid exchange.*/
   INVALID_REQUEST_DESCRIPTOR = 6053,           /**< Invalid request descriptor.*/
   EXCHANGE_FULL = 6054,                        /**< Exchange full.*/
   NO_ANODE = 6055,                             /**< No anode.*/
   INVALID_REQUEST_CODE = 6056,                 /**< Invalid request code.*/
   INVALID_SLOT = 6057,                         /**< Invalid slot.*/
   BAD_FONT_FILE_FORMAT = 6059,                 /**< Bad font file format.*/
   DEVICE_NOT_A_STREAM = 6060,                  /**< Device not a stream. */
   NO_DATA_AVAILABLE = 6061,                    /**< No data available. */
   TIMER_EXPIRED = 6062,                        /**< Timer expired. */
   OUT_OF_STREAMS_RESOURCES = 6063,             /**< Out of streams resources. */
   MACHINE_IS_NOT_ON_THE_NETWORK = 6064,        /**< Machine is not on the network.*/
   PACKAGE_NOT_INSTALLED = 6065,                /**< Package not installed.*/
   OBJECT_IS_REMOTE = 6066,                     /**< Object is remote.*/
   LINK_HAS_BEEN_SEVERED = 6067,                /**< Link has been severed.*/
   ADVERTISE_ERROR = 6068,                      /**< Advertise error.*/
   SRM_MOUNT_ERROR = 6069,                      /**< Srmount error.*/
   COMMUNICATION_ERROR_ON_SEND = 6070,          /**< Communication error on send.*/
   PROTOCOL_ERROR = 6071,                       /**< Protocol error.*/
   MULTIHOP_ATTEMPTED = 6072,                   /**< Multihop attempted.*/
   RFS_SPECIFIC_ERROR = 6073,                   /**< RFS specific error.*/
   BAD_MESSAGE = 6074,                          /**< Bad message.*/
   VALUE_TOO_LARGE_FOR_DEFINED_DATA_TYPE = 6075, /**< Value too large for defined data
                                                      type.*/
   NAME_NOT_UNIQUE_ON_NETWORK = 6076,           /**< Name not unique on network.*/
   FILE_DESCRIPTOR_IN_BAD_STATE = 6077,         /**< File descriptor in bad state.*/
   REMOTE_ADDRESS_CHANGED = 6078,               /**< Remote address changed.*/
   CANNOT_ACCESS_A_NEEDED_SHARED_LIBRARY = 6079, /**< Cannot access a needed shared
                                                      library.*/
   ACCESSING_A_CORRUPTED_SHARED_LIBRARY = 6080,  /**< Accessing a corrupted shared
                                                      library.*/
   LIB_SECTION_CORRUPTED = 6081,                /**< .lib section in a.out corrupted.*/
   TOO_MANY_SHARED_LIBRARIES = 6082,            /**< Too many shared libraries.*/
   CANNOT_EXEC_A_SHARED_LIBRARY_DIRECTLY = 6083, /**< Cannot exec a shared library
                                                      directly.*/
   ILLEGAL_BYTE_SEQUENCE = 6084,                /**< Illegal byte sequence.*/
   INTERRUPTED_SYSTEM_CALL_RESTART = 6085,      /**< Interrupted system call should be
                                                     restarted.*/
   STREAMS_PIPE_ERROR = 6086,                   /**< Streams pipe error.*/
   TOO_MANY_USERS = 6087,                       /**< Too many users.*/
   SOCKET_OPERATION_ON_NON_SOCKET = 6088,       /**< Socket operation on non-socket.*/
   DESTINATION_ADDRESS_REQUIRED = 6089,         /**< Destination address required.*/
   MESSAGE_TOO_LONG = 6090,                     /**< Message too long.*/
   PROTOCOL_WRONG_TYPE_FOR_SOCKET = 6091,       /**< Protocol wrong type for socket.*/
   PROTOCOL_NOT_AVAILABLE = 6092,               /**< Protocol not available.*/
   PROTOCOL_NOT_SUPPORTED = 6093,               /**< Protocol not supported.*/
   SOCKET_TYPE_NOT_SUPPORTED = 6094,            /**< Socket type not supported.*/
   OPERATION_NOT_SUPPORTED = 6095,              /**< Operation not supported
                                                     (on socket).*/
   PROTOCOL_FAMILY_NOT_SUPPORTED = 6096,        /**< Protocol family not supported.*/
   ADDRESS_FAMILY_NOT_SUPPORTED = 6097,         /**< Address family not supported by
                                                     protocol.*/
   ADDRESS_ALREADY_IN_USE = 6098,               /**< Address already in use.*/
   CANNOT_ASSIGN_REQUESTED_ADDRESS = 6099,      /**< Cannot assign requested
                                                     address.*/
   NETWORK_IS_DOWN = 6100,                      /**< Network is down.*/
   NETWORK_IS_UNREACHABLE = 6101,               /**< Network is unreachable.*/
   NETWORK_DROPPED_CONNECTION_ON_RESET = 6102,  /**< Network dropped connection on
                                                     reset.*/
   SOFTWARE_CAUSED_CONNECTION_ABORT = 6103,     /**< Software caused connection
                                                     abort.*/
   CONNECTION_RESET_BY_PEER = 6104,             /**< Connection reset by peer.*/
   NO_BUFFER_SPACE_AVAILABLE = 6105,            /**< No buffer space available.*/
   SOCKET_IS_ALREADY_CONNECTED = 6106,          /**< Socket is already connected.*/
   SOCKET_IS_NOT_CONNECTED = 6107,              /**< Socket is not connected.*/
   CANNOT_SEND_AFTER_TRANSPORT_ENDPOINT_SHUTDOWN = 6108, /**< Cannot send after transport
                                                              endpoint shutdown.*/
   TOO_MANY_REFERENCES_CANNOT_SPLICE = 6109,    /**< Too many references: cannot
                                                     splice.*/
   CONNECTION_TIMED_OUT = 6110,                 /**< Connection timed out.*/
   CONNECTION_REFUSED = 6111,                   /**< Connection refused.*/
   HOST_IS_DOWN = 6112,                         /**< Host is down.*/
   NO_ROUTE_TO_HOST = 6113,                     /**< No route to host.*/
   OPERATION_ALREADY_IN_PROGRESS = 6114,        /**< Operation already in progress.*/
   OPERATION_NOW_IN_PROGRESS = 6115,            /**< Operation now in progress.*/
   STALE_FILE_HANDLE = 6116,                    /**< Stale file handle.*/
   STRUCTURE_NEEDS_CLEANING = 6117,             /**< Structure needs cleaning.*/
   NOT_A_XENIX_NAMED_TYPE_FILE = 6118,          /**< Not a XENIX named type file.*/
   NO_XENIX_SEMAPHORES_AVAILABLE = 6119,        /**< No XENIX semaphores available.*/
   IS_A_NAMED_TYPE_FILE = 6120,                 /**< Is a named type file.*/
   REMOTE_IO_ERROR = 6121,                      /**< Remote I/O error.*/
   DISK_QUOTA_EXCEEDED = 6122,                  /**< Disk quota exceeded.*/
   NO_MEDIUM_FOUND = 6123,                      /**< No medium found.*/
   WRONG_MEDIUM_TYPE = 6124,                    /**< Wrong medium type.*/
   OPERATION_CANCELED = 6125,                   /**< Operation canceled.*/
   REQUIRED_KEY_NOT_AVAILABLE = 6126,           /**< Required key not available.*/
   KEY_HAS_EXPIRED = 6127,                      /**< Key has expired.*/
   KEY_HAS_BEEN_REVOKED = 6128,                 /**< Key has been revoked.*/
   KEY_WAS_REJECTED_BY_SERVICE = 6129,          /**< Key was rejected by service.*/
   OWNER_DIED = 6130,                           /**< Owner died.*/
   STATE_NOT_RECOVERABLE = 6131,                /**< State not recoverable.*/
   OPERATION_NOT_POSSIBLE_DUE_TO_RFKILL = 6132, /**< Operation not possible due to
                                                     RF-kill.*/
   MEMORY_PAGE_HAS_HARDWARE_ERROR = 6133,       /**< Memory page has hardware error.*/
};

/**
 * @brief Service status.
 */
enum class ServiceStatus {
    SERVICE_UNAVAILABLE,
    SERVICE_AVAILABLE,
    SERVICE_FAILED,
};

/**
 * This applies in system architectures where the modem is attached to an External Application
 * Processor(EAP). The operations associated with the ProcType can be performed by SDK either
 * on EAP or the modem's Internal Application Processor(IAP). This type specifies where the
 * operation is carried out.
 */
enum class ProcType {
    LOCAL_PROC = 0, /**< Perform the operation on the processor where the API is invoked.*/
    REMOTE_PROC,    /**< Perform the operation on the application processor other than where the API
                    is invoked. */
};

/**
 * Base command callback class is responsible for single shot asynchronous callback.
 * This callback will be invoked only once when the operation succeeds or fails.
 */
class ICommandCallback {
public:
   virtual ~ICommandCallback() {
   }
};

/**
 * @brief General command response callback for most of the requests, client needs to implement
 * this interface to get single shot response.
 *
 * The methods in callback can be invoked from multiple different threads. The implementation
 * should be thread safe.
 */
class ICommandResponseCallback : public ICommandCallback {
public:
   /**
    * This function is called with the response to the command operation.
    *
    * @param [in] error - @ref ErrorCode
    */
   virtual void commandResponse(ErrorCode error) = 0;

   virtual ~ICommandResponseCallback() {
   }
};
/** @} */ /* end_addtogroup telematics_common */

/**
 * @brief General response callback for most of the requests, client needs to implement
 * this function to get the asynchronous response.
 *
 * The methods in callback can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 *  @param [in] errorCode  @ref ErrorCode
 */
using ResponseCallback = std::function<void(telux::common::ErrorCode errorCode)>;

/**
 * This API is invoked when the initialization of an object completes.
 *
 * @param[in] status - @ref Service status
 *
 */
using InitResponseCb = std::function<void(telux::common::ServiceStatus status)>;


class IServiceStatusListener {
public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(ServiceStatus status) {}

    virtual ~IServiceStatusListener() {}
};

}  // End of namespace common
}  // End of namespace telux

#endif
