/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
 * @file      PhoneDefines.hpp
 * @brief     PhoneDefines contains enumerations and variables used for
 *            telephony subsystems.
 */
#ifndef PHONEDEFINES_HPP
#define PHONEDEFINES_HPP

#include <string>
#include <memory>

#define DEFAULT_PHONE_ID 1
#define INVALID_PHONE_ID -1

#define DEFAULT_SLOT_ID 1
#define INVALID_SLOT_ID -1

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
   CALL_IDLE = -1, /**< idle call, default state of a newly created call object */
   CALL_ACTIVE,    /**< active call*/
   CALL_ON_HOLD,   /**< on hold call */
   CALL_DIALING,   /**< out going call, in dialing state and not yet connected,
                        MO Call only */
   CALL_INCOMING,  /**< incoming call, not yet answered  */
   CALL_WAITING,   /**< waiting call*/
   CALL_ALERTING,  /**< alerting call, MO Call only */
   CALL_ENDED,     /**<  call ended / disconnected */
};

/**
 * Defines the radio state
 */
enum class RadioState {
   RADIO_STATE_OFF,         /**< Radio is explicitly powered off */
   RADIO_STATE_UNAVAILABLE, /**< Radio unavailable (eg, resetting or not booted) */
   RADIO_STATE_ON,          /**< Radio is on */
};

/**
 * Defines the service states
 */
enum class ServiceState {
   EMERGENCY_ONLY, /**< Only emergency calls allowed */
   IN_SERVICE,     /**< Normal operation, device is registered with a carrier and online */
   OUT_OF_SERVICE, /**< Device is not registered with any carrier */
   RADIO_OFF,      /**< Device radio is off - Airplane mode for example */
};

/**
 * Reason for the recently terminated call (either normally ended or failed)
 */
enum class CallEndCause {
   UNOBTAINABLE_NUMBER = 1,
   NO_ROUTE_TO_DESTINATION = 3,
   CHANNEL_UNACCEPTABLE = 6,
   OPERATOR_DETERMINED_BARRING = 8,
   NORMAL = 16,
   BUSY = 17,
   NO_USER_RESPONDING = 18,
   NO_ANSWER_FROM_USER = 19,
   CALL_REJECTED = 21,
   NUMBER_CHANGED = 22,
   PREEMPTION = 25,
   DESTINATION_OUT_OF_ORDER = 27,
   INVALID_NUMBER_FORMAT = 28,
   FACILITY_REJECTED = 29,
   RESP_TO_STATUS_ENQUIRY = 30,
   NORMAL_UNSPECIFIED = 31,
   CONGESTION = 34,
   NETWORK_OUT_OF_ORDER = 38,
   TEMPORARY_FAILURE = 41,
   SWITCHING_EQUIPMENT_CONGESTION = 42,
   ACCESS_INFORMATION_DISCARDED = 43,
   REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE = 44,
   RESOURCES_UNAVAILABLE_OR_UNSPECIFIED = 47,
   QOS_UNAVAILABLE = 49,
   REQUESTED_FACILITY_NOT_SUBSCRIBED = 50,
   INCOMING_CALLS_BARRED_WITHIN_CUG = 55,
   BEARER_CAPABILITY_NOT_AUTHORIZED = 57,
   BEARER_CAPABILITY_UNAVAILABLE = 58,
   SERVICE_OPTION_NOT_AVAILABLE = 63,
   BEARER_SERVICE_NOT_IMPLEMENTED = 65,
   ACM_LIMIT_EXCEEDED = 68,
   REQUESTED_FACILITY_NOT_IMPLEMENTED = 69,
   ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE = 70,
   SERVICE_OR_OPTION_NOT_IMPLEMENTED = 79,
   INVALID_TRANSACTION_IDENTIFIER = 81,
   USER_NOT_MEMBER_OF_CUG = 87,
   INCOMPATIBLE_DESTINATION = 88,
   INVALID_TRANSIT_NW_SELECTION = 91,
   SEMANTICALLY_INCORRECT_MESSAGE = 95,
   INVALID_MANDATORY_INFORMATION = 96,
   MESSAGE_TYPE_NON_IMPLEMENTED = 97,
   MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 98,
   INFORMATION_ELEMENT_NON_EXISTENT = 99,
   CONDITIONAL_IE_ERROR = 100,
   MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE = 101,
   RECOVERY_ON_TIMER_EXPIRED = 102,
   PROTOCOL_ERROR_UNSPECIFIED = 111,
   INTERWORKING_UNSPECIFIED = 127,
   CALL_BARRED = 240,
   FDN_BLOCKED = 241,
   IMSI_UNKNOWN_IN_VLR = 242,
   IMEI_NOT_ACCEPTED = 243,
   DIAL_MODIFIED_TO_USSD = 244,
   DIAL_MODIFIED_TO_SS = 245,
   DIAL_MODIFIED_TO_DIAL = 246,
   CDMA_LOCKED_UNTIL_POWER_CYCLE = 1000,
   CDMA_DROP = 1001,
   CDMA_INTERCEPT = 1002,
   CDMA_REORDER = 1003,
   CDMA_SO_REJECT = 1004,
   CDMA_RETRY_ORDER = 1005,
   CDMA_ACCESS_FAILURE = 1006,
   CDMA_PREEMPTED = 1007,
   CDMA_NOT_EMERGENCY = 1008,
   CDMA_ACCESS_BLOCKED = 1009,
   ERROR_UNSPECIFIED = 0xffff,
};
/** @} */ /* end_addtogroup telematics_call */

}  // End of namespace tel

}  // End of namespace telux

#endif  // PHONEDEFINES_HPP
