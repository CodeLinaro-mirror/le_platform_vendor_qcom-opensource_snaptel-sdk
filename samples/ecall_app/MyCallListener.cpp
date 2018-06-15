/*
 *  Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <vector>

#include "MyCallListener.hpp"

#define print_notification std::cout << std::endl << "\033[1;35mNotification: \033[0m"

using namespace telux::tel;
using namespace telux::common;

void MyCallListener::onIncomingCall(std::shared_ptr<ICall> call) {
   std::cout << std::endl << std::endl;
   print_notification << getCurrentTime() << "Answer incoming call" << std::endl;
   std::string user_string;
   std::cout << " Enter 7 to answer call " << std::endl;
}

void MyCallListener::onCallInfoChange(std::shared_ptr<ICall> call) {
   print_notification << "\n onCallInfoChange: "
                      << " Call State: " << callStateToString(call->getCallState())
                      << ", Call Index: " << (int)call->getCallIndex()
                      << ", Call Direction: " << callDirectionToString(call->getCallDirection())
                      << ", Phone Number: " << call->getRemotePartyNumber() << std::endl;
   if(call->getCallState() == CallState::CALL_ENDED) {
      print_notification << getCurrentTime() << "  Cause of call termination: "
                         << callEndCauseToString(call->getCallEndCause()) << std::endl;
   }
}

std::string MyCallListener::callDirectionToString(CallDirection cd) {
   switch(cd) {
      case CallDirection::INCOMING:
         return std::string("Incoming call");
      case CallDirection::OUTGOING:
         return std::string("Outgoing call");
      case CallDirection::NONE:
         return std::string("none");
      default:
         std::cout << "Unexpected call direction = " << (int)cd << std::endl;
         return std::string("unknown");
   }
}

std::string MyCallListener::callStateToString(CallState cs) {
   switch(cs) {
      case CallState::CALL_IDLE:
         return std::string("Idle call");
      case CallState::CALL_ACTIVE:
         return std::string("Active call");
      case CallState::CALL_ON_HOLD:
         return std::string("On hold call");
      case CallState::CALL_DIALING:
         return std::string("Outgoing call");
      case CallState::CALL_INCOMING:
         return std::string("Incoming call");
      case CallState::CALL_WAITING:
         return std::string("Waiting call");
      case CallState::CALL_ALERTING:
         return std::string("Alerting call");
      case CallState::CALL_ENDED:
         return std::string("Call ended");
      default:
         std::cout << "Unexpected CallState = " << (int)cs << std::endl;
         return std::string("unknown");
   }
}

void MyCallListener::onECallMsdTransmissionStatus(int phoneId, ErrorCode errorCode) {
   if(errorCode == ErrorCode::SUCCESS) {
      print_notification << "onECallMsdTransmissionStatus is Success" << std::endl;
   } else {
      print_notification
         << "onECallMsdTransmissionStatus failed with error code: " << static_cast<int>(errorCode)
         << std::endl;
   }
}

std::string MyCallListener::eCallMsdTransmissionStatusToString(telux::tel::ECallMsdTransmissionStatus status) {
   switch(status) {
      case telux::tel::ECallMsdTransmissionStatus::SUCCESS:
         return std::string("SUCCESS ");
      case telux::tel::ECallMsdTransmissionStatus::FAILURE:
         return std::string("FAILURE");
      case telux::tel::ECallMsdTransmissionStatus::MSD_TRANSMISSION_STARTED:
         return std::string("MSD TRANSMISSION STARTED");
      case telux::tel::ECallMsdTransmissionStatus::NACK_OUT_OF_ORDER:
         return std::string("NACK OUT OF ORDER");
      case telux::tel::ECallMsdTransmissionStatus::ACK_OUT_OF_ORDER:
         return std::string("ACK OUT OF ORDER");
      default:
         std::stringstream ss;
         ss << "Unknown ECallMsdTransmissionStatus  = " << (int)status;
         return ss.str();
   }
}

void MyCallListener::onECallMsdTransmissionStatus(int phoneId,
                  telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus) {
   print_notification << "ECallMsdTransmission  Status: " <<
         eCallMsdTransmissionStatusToString(msdTransmissionStatus) << std::endl;
}

std::string MyCallListener::callEndCauseToString(CallEndCause causeCode) {
   switch(causeCode) {
      case CallEndCause::UNOBTAINABLE_NUMBER:
         return std::string("Unobtainable number");
      case CallEndCause::NO_ROUTE_TO_DESTINATION:
         return std::string("No route to destination");
      case CallEndCause::CHANNEL_UNACCEPTABLE:
         return std::string("Channel unacceptable");
      case CallEndCause::OPERATOR_DETERMINED_BARRING:
         return std::string("Operator determined barring");
      case CallEndCause::NORMAL:
         return std::string("Normal");
      case CallEndCause::BUSY:
         return std::string("Busy");
      case CallEndCause::NO_USER_RESPONDING:
         return std::string("No user responding");
      case CallEndCause::NO_ANSWER_FROM_USER:
         return std::string("No answer from user");
      case CallEndCause::CALL_REJECTED:
         return std::string("Call rejected");
      case CallEndCause::NUMBER_CHANGED:
         return std::string("Number changed");
      case CallEndCause::PREEMPTION:
         return std::string("Preemption");
      case CallEndCause::DESTINATION_OUT_OF_ORDER:
         return std::string("Destination out of order");
      case CallEndCause::INVALID_NUMBER_FORMAT:
         return std::string("Invalid number format");
      case CallEndCause::FACILITY_REJECTED:
         return std::string("Facility rejected");
      case CallEndCause::RESP_TO_STATUS_ENQUIRY:
         return std::string("Resp to status enquiry");
      case CallEndCause::NORMAL_UNSPECIFIED:
         return std::string("Normal unspecified");
      case CallEndCause::CONGESTION:
         return std::string("Congestion");
      case CallEndCause::NETWORK_OUT_OF_ORDER:
         return std::string("Network out of order");
      case CallEndCause::TEMPORARY_FAILURE:
         return std::string("Temporary failure");
      case CallEndCause::SWITCHING_EQUIPMENT_CONGESTION:
         return std::string("Switching equipment congestion");
      case CallEndCause::ACCESS_INFORMATION_DISCARDED:
         return std::string("Access information discarded");
      case CallEndCause::REQUESTED_CIRCUIT_OR_CHANNEL_NOT_AVAILABLE:
         return std::string("Requested circuit or channel not available");
      case CallEndCause::RESOURCES_UNAVAILABLE_OR_UNSPECIFIED:
         return std::string("Resources unavailable or unspecified");
      case CallEndCause::QOS_UNAVAILABLE:
         return std::string("QOS unavailable");
      case CallEndCause::REQUESTED_FACILITY_NOT_SUBSCRIBED:
         return std::string("Requested facility not subscribed");
      case CallEndCause::INCOMING_CALLS_BARRED_WITHIN_CUG:
         return std::string("Incoming calls barred within CUG");
      case CallEndCause::BEARER_CAPABILITY_NOT_AUTHORIZED:
         return std::string("Bearer capability not authorized");
      case CallEndCause::BEARER_CAPABILITY_UNAVAILABLE:
         return std::string("Bearer capability unavailable");
      case CallEndCause::SERVICE_OPTION_NOT_AVAILABLE:
         return std::string("Service option not available");
      case CallEndCause::BEARER_SERVICE_NOT_IMPLEMENTED:
         return std::string("Bearer service not implemented");
      case CallEndCause::ACM_LIMIT_EXCEEDED:
         return std::string("Acm limit exceeded");
      case CallEndCause::REQUESTED_FACILITY_NOT_IMPLEMENTED:
         return std::string("Requested facility not implemented");
      case CallEndCause::ONLY_DIGITAL_INFORMATION_BEARER_AVAILABLE:
         return std::string("Only digital information bearer availablE");
      case CallEndCause::SERVICE_OR_OPTION_NOT_IMPLEMENTED:
         return std::string("Service or option not implemented");
      case CallEndCause::INVALID_TRANSACTION_IDENTIFIER:
         return std::string("Invalid transaction identifier");
      case CallEndCause::USER_NOT_MEMBER_OF_CUG:
         return std::string("User not member of CUG");
      case CallEndCause::INCOMPATIBLE_DESTINATION:
         return std::string("Incompatible destination");
      case CallEndCause::INVALID_TRANSIT_NW_SELECTION:
         return std::string("Invalid transit nw selection");
      case CallEndCause::SEMANTICALLY_INCORRECT_MESSAGE:
         return std::string("Semantically incorrect message");
      case CallEndCause::INVALID_MANDATORY_INFORMATION:
         return std::string("Invalid mandatory information");
      case CallEndCause::MESSAGE_TYPE_NON_IMPLEMENTED:
         return std::string("Message type non implemented");
      case CallEndCause::MESSAGE_TYPE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
         return std::string("Message type not compatible with protocol state");
      case CallEndCause::INFORMATION_ELEMENT_NON_EXISTENT:
         return std::string("Information element non existent");
      case CallEndCause::CONDITIONAL_IE_ERROR:
         return std::string("Conditional ie error");
      case CallEndCause::MESSAGE_NOT_COMPATIBLE_WITH_PROTOCOL_STATE:
         return std::string("Message not compatible with protocol state");
      case CallEndCause::RECOVERY_ON_TIMER_EXPIRED:
         return std::string("Recovery on timer expired");
      case CallEndCause::PROTOCOL_ERROR_UNSPECIFIED:
         return std::string("Protocol error unspecified");
      case CallEndCause::INTERWORKING_UNSPECIFIED:
         return std::string("Interworking unspecified");
      case CallEndCause::CALL_BARRED:
         return std::string("Call barred");
      case CallEndCause::FDN_BLOCKED:
         return std::string("FDN blocked");
      case CallEndCause::IMSI_UNKNOWN_IN_VLR:
         return std::string("IMSI unknown in VLR");
      case CallEndCause::IMEI_NOT_ACCEPTED:
         return std::string("IMEI not accepted");
      case CallEndCause::DIAL_MODIFIED_TO_USSD:
         return std::string("Dial modified to USSD");
      case CallEndCause::DIAL_MODIFIED_TO_SS:
         return std::string("Dial modified to SS");
      case CallEndCause::DIAL_MODIFIED_TO_DIAL:
         return std::string("Dial modified to dial");
      case CallEndCause::CDMA_LOCKED_UNTIL_POWER_CYCLE:
         return std::string("CDMA locked until power cycle");
      case CallEndCause::CDMA_DROP:
         return std::string("CDMA drop");
      case CallEndCause::CDMA_INTERCEPT:
         return std::string("CDMA intercept");
      case CallEndCause::CDMA_REORDER:
         return std::string("CDMA reorder");
      case CallEndCause::CDMA_SO_REJECT:
         return std::string("CDMA SO reject");
      case CallEndCause::CDMA_RETRY_ORDER:
         return std::string("CDMA retry order");
      case CallEndCause::CDMA_ACCESS_FAILURE:
         return std::string("CDMA access failure");
      case CallEndCause::CDMA_PREEMPTED:
         return std::string("CDMA preempted");
      case CallEndCause::CDMA_NOT_EMERGENCY:
         return std::string("CDMA not emergency");
      case CallEndCause::CDMA_ACCESS_BLOCKED:
         return std::string("CDMA access blocked");
      case CallEndCause::ERROR_UNSPECIFIED:
         return std::string("Error unspecified");
      default:
         std::stringstream ss;
         ss << "Unknown call fail cause = " << (int)causeCode;
         return ss.str();
   }
}

std::string MyCallListener::getCurrentTime() {
   timeval tod;
   gettimeofday(&tod, NULL);
   std::stringstream ss;
   time_t tt = tod.tv_sec;
   char buffer[100];
   std::strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&tt));
   char currTime[120];
   sprintf(currTime, "%s.%ld", buffer, tod.tv_usec / 1000);
   return std::string(currTime);
}
