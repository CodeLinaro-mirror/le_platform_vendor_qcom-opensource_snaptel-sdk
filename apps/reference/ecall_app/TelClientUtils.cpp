/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @file    TelClientUtils.cpp
 *
 * @brief   TelClientUtils class provides helper functions to convert various parameters to strings.
 */

#include <iostream>
#include <sstream>

#include "TelClientUtils.hpp"

#define CLIENT_NAME "ECall-Tel-Utils: "

std::string TelClientUtils::callDirectionToString(telux::tel::CallDirection cd) {
    switch(cd) {
        case telux::tel::CallDirection::INCOMING:
            return std::string("Incoming call");
        case telux::tel::CallDirection::OUTGOING:
            return std::string("Outgoing call");
        case telux::tel::CallDirection::NONE:
            return std::string("None");
        default:
            std::cout << CLIENT_NAME << "Unexpected call direction = " << (int)cd << std::endl;
            return std::string("unknown");
    }
}

std::string TelClientUtils::callStateToString(telux::tel::CallState cs) {
    switch(cs) {
        case telux::tel::CallState::CALL_IDLE:
            return std::string("Idle call");
        case telux::tel::CallState::CALL_ACTIVE:
            return std::string("Active call");
        case telux::tel::CallState::CALL_ON_HOLD:
            return std::string("On hold call");
        case telux::tel::CallState::CALL_DIALING:
            return std::string("Outgoing call");
        case telux::tel::CallState::CALL_INCOMING:
            return std::string("Incoming call");
        case telux::tel::CallState::CALL_WAITING:
            return std::string("Waiting call");
        case telux::tel::CallState::CALL_ALERTING:
            return std::string("Alerting call");
        case telux::tel::CallState::CALL_ENDED:
            return std::string("Call ended");
        default:
            std::cout << CLIENT_NAME << "Unexpected CallState = " << (int)cs << std::endl;
            return std::string("unknown");
    }
}

std::string TelClientUtils::eCallMsdTransmissionStatusToString(
                    telux::tel::ECallMsdTransmissionStatus status) {
    switch(status) {
        case telux::tel::ECallMsdTransmissionStatus::SUCCESS:
            return std::string("SUCCESS");
        case telux::tel::ECallMsdTransmissionStatus::FAILURE:
            return std::string("FAILURE");
        case telux::tel::ECallMsdTransmissionStatus::MSD_TRANSMISSION_STARTED:
            return std::string("MSD TRANSMISSION STARTED");
        case telux::tel::ECallMsdTransmissionStatus::NACK_OUT_OF_ORDER:
            return std::string("NACK OUT OF ORDER");
        case telux::tel::ECallMsdTransmissionStatus::ACK_OUT_OF_ORDER:
            return std::string("ACK OUT OF ORDER");
        case telux::tel::ECallMsdTransmissionStatus::START_RECEIVED:
            return std::string("SEND-MSD(START) RECEIVED");
        case telux::tel::ECallMsdTransmissionStatus::LL_ACK_RECEIVED:
            return std::string("LL-ACK RECEIVED");
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_STARTED:
            return std::string("OUTBAND MSD TRANSMISSION STARTED");
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_SUCCESS:
            return std::string("OUTBAND MSD TRANSMISSION SUCCESS");
        case telux::tel::ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_FAILURE:
            return std::string("OUTBAND MSD TRANSMISSION FAILURE");
        default:
            std::stringstream ss;
            ss << "Unknown ECallMsdTransmissionStatus  = " << (int)status;
            return ss.str();
    }
}

std::string TelClientUtils::callEndCauseToString(telux::tel::CallEndCause callEndCause) {
    switch(callEndCause) {
        case telux::tel::CallEndCause::UNOBTAINABLE_NUMBER:
            return std::string("Unobtainable number");
        case telux::tel::CallEndCause::NO_ROUTE_TO_DESTINATION:
            return std::string("No route to destination");
        case telux::tel::CallEndCause::CHANNEL_UNACCEPTABLE:
            return std::string("Channel unacceptable");
        case telux::tel::CallEndCause::OPERATOR_DETERMINED_BARRING:
            return std::string("Operator determined barring");
        case telux::tel::CallEndCause::NORMAL:
            return std::string("Normal");
        case telux::tel::CallEndCause::BUSY:
            return std::string("Busy");
        case telux::tel::CallEndCause::NO_USER_RESPONDING:
            return std::string("No user responding");
        case telux::tel::CallEndCause::NO_ANSWER_FROM_USER:
            return std::string("No answer from user");
        case telux::tel::CallEndCause::CALL_REJECTED:
            return std::string("Call rejected");
        case telux::tel::CallEndCause::NUMBER_CHANGED:
            return std::string("Number changed");
        case telux::tel::CallEndCause::PREEMPTION:
            return std::string("Preemption");
        case telux::tel::CallEndCause::DESTINATION_OUT_OF_ORDER:
            return std::string("Destination out of order");
        case telux::tel::CallEndCause::INVALID_NUMBER_FORMAT:
            return std::string("Invalid number format");
        case telux::tel::CallEndCause::FACILITY_REJECTED:
            return std::string("Facility rejected");
        case telux::tel::CallEndCause::RESP_TO_STATUS_ENQUIRY:
            return std::string("Resp to status enquiry");
        case telux::tel::CallEndCause::NORMAL_UNSPECIFIED:
            return std::string("Normal unspecified");
        case telux::tel::CallEndCause::CONGESTION:
            return std::string("Congestion");
        case telux::tel::CallEndCause::NETWORK_OUT_OF_ORDER:
            return std::string("Network out of order");
        case telux::tel::CallEndCause::TEMPORARY_FAILURE:
            return std::string("Temporary failure");
        case telux::tel::CallEndCause::SWITCHING_EQUIPMENT_CONGESTION:
            return std::string("Switching equipment congestion");
        case telux::tel::CallEndCause::ACCESS_INFORMATION_DISCARDED:
            return std::string("Access information discarded");
        case telux::tel::CallEndCause::REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE:
            return std::string("Requested circuit or channel not available");
        case telux::tel::CallEndCause::RESOURCES_UNAVAILABLE_OR_UNSPECIFIED:
            return std::string("Resources unavailable or unspecified");
        case telux::tel::CallEndCause::QOS_UNAVAILABLE:
            return std::string("QOS unavailable");
        case telux::tel::CallEndCause::REQUESTED_FACILITY_NOT_SUBSCRIBED:
            return std::string("Requested facility not subscribed");
        case telux::tel::CallEndCause::INCOMING_CALLS_BARRED_WITHIN_CUG:
            return std::string("Incoming calls barred within CUG");
        case telux::tel::CallEndCause::BEARER_CAPABILITY_NOT_AUTHORIZED:
            return std::string("Bearer capability not authorized");
        case telux::tel::CallEndCause::BEARER_CAPABILITY_UNAVAILABLE:
            return std::string("Bearer capability unavailable");
        case telux::tel::CallEndCause::SERVICE_OPTION_NOT_AVAILABLE:
            return std::string("Service option not available");
        case telux::tel::CallEndCause::BEARER_SERVICE_NOT_IMPLEMENTED:
            return std::string("Bearer service not implemented");
        case telux::tel::CallEndCause::ACM_LIMIT_EXCEEDED:
            return std::string("Acm limit exceeded");
        case telux::tel::CallEndCause::REQUESTED_FACILITY_NOT_IMPLEMENTED:
            return std::string("Requested facility not implemented");
        case telux::tel::CallEndCause::ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE:
            return std::string("Only digital information bearer availablE");
        case telux::tel::CallEndCause::SERVICE_OR_OPTION_NOT_IMPLEMENTED:
            return std::string("Service or option not implemented");
        case telux::tel::CallEndCause::INVALID_TRANSACTION_IDENTIFIER:
            return std::string("Invalid transaction identifier");
        case telux::tel::CallEndCause::USER_NOT_MEMBER_OF_CUG:
            return std::string("User not member of CUG");
        case telux::tel::CallEndCause::INCOMPATIBLE_DESTINATION:
            return std::string("Incompatible destination");
        case telux::tel::CallEndCause::INVALID_TRANSIT_NW_SELECTION:
            return std::string("Invalid transit nw selection");
        case telux::tel::CallEndCause::SEMANTICALLY_INCORRECT_MESSAGE:
            return std::string("Semantically incorrect message");
        case telux::tel::CallEndCause::INVALID_MANDATORY_INFORMATION:
            return std::string("Invalid mandatory information");
        case telux::tel::CallEndCause::MESSAGE_TYPE_NON_IMPLEMENTED:
            return std::string("Message type non implemented");
        case telux::tel::CallEndCause::MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
            return std::string("Message type not compatible with protocol state");
        case telux::tel::CallEndCause::INFORMATION_ELEMENT_NON_EXISTENT:
            return std::string("Information element non existent");
        case telux::tel::CallEndCause::CONDITIONAL_IE_ERROR:
            return std::string("Conditional ie error");
        case telux::tel::CallEndCause::MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
            return std::string("Message not compatible with protocol state");
        case telux::tel::CallEndCause::RECOVERY_ON_TIMER_EXPIRED:
            return std::string("Recovery on timer expired");
        case telux::tel::CallEndCause::PROTOCOL_ERROR_UNSPECIFIED:
            return std::string("Protocol error unspecified");
        case telux::tel::CallEndCause::INTERWORKING_UNSPECIFIED:
            return std::string("Interworking unspecified");
        case telux::tel::CallEndCause::CALL_BARRED:
            return std::string("Call barred");
        case telux::tel::CallEndCause::FDN_BLOCKED:
            return std::string("FDN blocked");
        case telux::tel::CallEndCause::IMSI_UNKNOWN_IN_VLR:
            return std::string("IMSI unknown in VLR");
        case telux::tel::CallEndCause::IMEI_NOT_ACCEPTED:
            return std::string("IMEI not accepted");
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_USSD:
            return std::string("Dial modified to USSD");
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_SS:
            return std::string("Dial modified to SS");
        case telux::tel::CallEndCause::DIAL_MODIFIED_TO_DIAL:
            return std::string("Dial modified to dial");
        case telux::tel::CallEndCause::CDMA_LOCKED_UNTIL_POWER_CYCLE:
            return std::string("CDMA locked until power cycle");
        case telux::tel::CallEndCause::CDMA_DROP:
            return std::string("CDMA drop");
        case telux::tel::CallEndCause::CDMA_INTERCEPT:
            return std::string("CDMA intercept");
        case telux::tel::CallEndCause::CDMA_REORDER:
            return std::string("CDMA reorder");
        case telux::tel::CallEndCause::CDMA_SO_REJECT:
            return std::string("CDMA SO reject");
        case telux::tel::CallEndCause::CDMA_RETRY_ORDER:
            return std::string("CDMA retry order");
        case telux::tel::CallEndCause::CDMA_ACCESS_FAILURE:
            return std::string("CDMA access failure");
        case telux::tel::CallEndCause::CDMA_PREEMPTED:
            return std::string("CDMA preempted");
        case telux::tel::CallEndCause::CDMA_NOT_EMERGENCY:
            return std::string("CDMA not emergency");
        case telux::tel::CallEndCause::CDMA_ACCESS_BLOCKED:
            return std::string("CDMA access blocked");
        case telux::tel::CallEndCause::ERROR_UNSPECIFIED:
            return std::string("Error unspecified");
        default:
            std::stringstream ss;
            ss << "Unknown call fail cause = " << (int)callEndCause;
            return ss.str();
    }
}

std::string TelClientUtils::eCallHlapTimerStatusToString(telux::tel::HlapTimerStatus status) {
    switch(status) {
        case telux::tel::HlapTimerStatus::INACTIVE:
            return std::string("INACTIVE");
        case telux::tel::HlapTimerStatus::ACTIVE:
            return std::string("ACTIVE");
        case telux::tel::HlapTimerStatus::UNKNOWN:
            return std::string("UNKNOWN");
        default:
            std::stringstream ss;
            ss << "Unknown HlapTimerStatus  = " << (int)status;
            return ss.str();
    }
}

std::string TelClientUtils::eCallHlapTimerEventToString(telux::tel::HlapTimerEvent event) {
    switch(event) {
        case telux::tel::HlapTimerEvent::STARTED:
            return std::string("STARTED");
        case telux::tel::HlapTimerEvent::STOPPED:
            return std::string("STOPPED");
        case telux::tel::HlapTimerEvent::EXPIRED:
            return std::string("EXPIRED");
        case telux::tel::HlapTimerEvent::UNKNOWN:
            return std::string("UNKNOWN");
        case telux::tel::HlapTimerEvent::UNCHANGED:
            return std::string("UNCHANGED");
        default:
            std::stringstream ss;
            ss << "Unknown HlapTimerEvent  = " << (int)event;
            return ss.str();
    }
}
