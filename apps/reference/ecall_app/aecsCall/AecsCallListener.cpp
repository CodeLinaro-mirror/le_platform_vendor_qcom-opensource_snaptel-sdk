/* Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

extern "C" {
#include <sys/time.h>
}

#include "AecsCallListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void AecsCallListener::onIncomingCall(std::shared_ptr<telux::tel::ICall> call) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << getCurrentTime() << std::endl;
   PRINT_NOTIFICATION <<  getCallStateString(call->getCallState())
             << " on slot Id: " << call->getPhoneId()
             << " and Phone Number: " << call->getRemotePartyNumber()
             << std::endl;

   if (!call->getCallReason().empty()) {
       PRINT_NOTIFICATION << "Call reason: " << call->getCallReason() << std::endl;
   }
   std::cout << "Enter 2 to answer call" << std::endl;
   std::cout << "Enter 3 to reject call" << std::endl;
}

void AecsCallListener::onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << " Call State: " << getCallStateString(call->getCallState())
                      << "\n Call Index: " << (int)call->getCallIndex()
                      << ", Call Direction: " << (int)call->getCallDirection()
                      << ", Phone Number: " << call->getRemotePartyNumber()
                      << ", Slot Id: " << call->getPhoneId()
                      << std::endl;

   auto &mgr = AecsCallManager::getInstance();
   mgr.setAecsCallDropStatus(false);
   mgr.setAecsCallFailStatus(false);
   int phoneId = call->getPhoneId();
   if (call->getCallState() == telux::tel::CallState::CALL_ENDED) {
       if (call->isAecsCallDrop()) {
           std::cout << std::endl << std::endl;
           PRINT_NOTIFICATION
               << " AECS information: -voice connection is dropped, retry the voice connection"
               << std::endl;
           std::cout << std::endl << std::endl;
           std::cout << "Enter 7 to retry dropped AECS call" << std::endl;
           mgr.setAecsCallDropStatus(true);
       }

       if (call->getRedialState() == telux::tel::RedialState::MODEM_RETRY_END) {
           std::cout << std::endl << std::endl;
           PRINT_NOTIFICATION
               << " AECS information: - voice connection failure, retrying the voice connection"
               << std::endl;
            std::cout << std::endl << std::endl;
            std::cout << "Enter 8 to retry failed AECS call" << std::endl;
            mgr.setAecsCallFailStatus(true);
       }

       if (mgr.isEmergencyMode(phoneId) && (!mgr.getAecsCallDropStatus() &&
           !mgr.getAecsCallFailStatus())) {
           mgr.stopAudioIfNoCalls(phoneId);
           std::cout << std::endl << std::endl;
           std::cout << "Enter 10 to exit emergency mode" << std::endl;
       }

       std::cout << std::endl << std::endl;
       PRINT_NOTIFICATION << getCurrentTime() << " Cause of call termination: "
           << getCallEndCauseString(call->getCallEndCause()) << std::endl;
   } else if (call->getCallState() == telux::tel::CallState::CALL_ACTIVE &&
       mgr.isEmergencyMode(phoneId)) {
       std::cout << std::endl << std::endl;
       PRINT_NOTIFICATION
           << " AECS information: -voice connection established, voice communication in progress"
           << std::endl;
   } else {
       // nothing
   }
}

std::string AecsCallListener::getCallStateString(telux::tel::CallState cs) {
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
         std::cout << "Unexpected CallState = " << (int)cs << std::endl;
         return std::string("unknown");
   }
}

std::string AecsCallListener::getCallEndCauseString(telux::tel::CallEndCause callEndCause) {
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
      case telux::tel::CallEndCause::USER_BUSY:
      case telux::tel::CallEndCause::SIP_BUSY:
         return std::string("Busy");
      case telux::tel::CallEndCause::NO_USER_RESPONDING:
         return std::string("No user responding");
      case telux::tel::CallEndCause::NO_ANSWER_FROM_USER:
         return std::string("No answer from user");
      case telux::tel::CallEndCause::NOT_REACHABLE:
      case telux::tel::CallEndCause::SIP_NOT_REACHABLE:
         return std::string("Not reachable");
      case telux::tel::CallEndCause::CALL_REJECTED:
      case telux::tel::CallEndCause::USER_REJECT:
      case telux::tel::CallEndCause::SIP_USER_REJECTED:
      case telux::tel::CallEndCause::SIP_REQUEST_CANCELLED:
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
      case telux::tel::CallEndCause::EMERGENCY_TEMP_FAILURE:
         return std::string("Emergency temporary failure");
      case telux::tel::CallEndCause::EMERGENCY_PERM_FAILURE:
         return std::string("Emergency permanent failure");
      case telux::tel::CallEndCause::HO_NOT_FEASIBLE:
         return std::string("Hand over not feasible");
      case telux::tel::CallEndCause::LOW_BATTERY:
         return std::string("Low battery");
      case telux::tel::CallEndCause::BLACKLISTED_CALL_ID:
         return std::string("Blacklisted call ID");
      case telux::tel::CallEndCause::CS_RETRY_REQUIRED:
         return std::string("CS retry required");
      case telux::tel::CallEndCause::NETWORK_UNAVAILABLE:
         return std::string("Network unavailable");
      case telux::tel::CallEndCause::FEATURE_UNAVAILABLE:
         return std::string("Feature unavailable");
      case telux::tel::CallEndCause::SIP_ERROR:
         return std::string("SIP error");
      case telux::tel::CallEndCause::MISC:
         return std::string("MISC");
      case telux::tel::CallEndCause::ANSWERED_ELSEWHERE:
         return std::string("Answered elsewhere");
      case telux::tel::CallEndCause::PULL_OUT_OF_SYNC:
         return std::string("Pull out of sync");
      case telux::tel::CallEndCause::CAUSE_CALL_PULLED:
         return std::string("Cause call pulled");
      case telux::tel::CallEndCause::SIP_REDIRECTED:
         return std::string("Redirected");
      case telux::tel::CallEndCause::SIP_BAD_REQUEST:
         return std::string("Bad request");
      case telux::tel::CallEndCause::SIP_FORBIDDEN:
         return std::string("Forbidden");
      case telux::tel::CallEndCause::SIP_NOT_FOUND:
         return std::string("Remote URI not found");
      case telux::tel::CallEndCause::SIP_NOT_SUPPORTED:
         return std::string("Not Supported");
      case telux::tel::CallEndCause::SIP_REQUEST_TIMEOUT:
         return std::string("Request timeout");
      case telux::tel::CallEndCause::SIP_TEMPORARILY_UNAVAILABLE:
         return std::string("Temporary unavailable");
      case telux::tel::CallEndCause::SIP_BAD_ADDRESS:
         return std::string("Bad address");
      case telux::tel::CallEndCause::SIP_NOT_ACCEPTABLE:
         return std::string("Not acceptable");
      case telux::tel::CallEndCause::SIP_SERVER_INTERNAL_ERROR:
         return std::string("Server internal error");
      case telux::tel::CallEndCause::SIP_SERVER_NOT_IMPLEMENTED:
         return std::string("Server not implemented");
      case telux::tel::CallEndCause::SIP_SERVER_BAD_GATEWAY:
         return std::string("Bad gateway");
      case telux::tel::CallEndCause::SIP_SERVICE_UNAVAILABLE:
         return std::string("Service unavailable");
      case telux::tel::CallEndCause::SIP_SERVER_TIMEOUT:
         return std::string("Server timeout");
      case telux::tel::CallEndCause::SIP_SERVER_VERSION_UNSUPPORTED:
         return std::string("Server version unsupported");
      case telux::tel::CallEndCause::SIP_SERVER_MESSAGE_TOOLARGE:
         return std::string("Message too large");
      case telux::tel::CallEndCause::SIP_SERVER_PRECONDITION_FAILURE:
         return std::string("Precondition failure");
      case telux::tel::CallEndCause::SIP_GLOBAL_ERROR:
         return std::string("Global error");
      case telux::tel::CallEndCause::MEDIA_INIT_FAILED:
         return std::string("Media init failed");
      case telux::tel::CallEndCause::MEDIA_NO_DATA:
         return std::string("Media no data");
      case telux::tel::CallEndCause::MEDIA_NOT_ACCEPTABLE:
         return std::string("Media not acceptable");
      case telux::tel::CallEndCause::MEDIA_UNSPECIFIED_ERROR:
         return std::string("Media unspecified error");
      case telux::tel::CallEndCause::HOLD_RESUME_FAILED:
         return std::string("Hold resume failed");
      case telux::tel::CallEndCause::HOLD_RESUME_CANCELED:
         return std::string("Hold resume cancelled");
      case telux::tel::CallEndCause::HOLD_REINVITE_COLLISION:
         return std::string("Hold reinvite collision");
      case telux::tel::CallEndCause::THERMAL_EMERGENCY:
         return std::string("Thermal emergency");
      case telux::tel::CallEndCause::ERROR_UNSPECIFIED:
         return std::string("Error unspecified");
      default:
         std::stringstream ss;
         ss << "Unknown call fail cause = " << (int)callEndCause;
         return ss.str();
   }
}

std::string AecsCallListener::getCurrentTime() {
   timeval tod;
   gettimeofday(&tod, NULL);
   std::stringstream ss;
   time_t tt = tod.tv_sec;
   char buffer[100];
   std::strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&tt));
   char currTime[120];
   snprintf(currTime, 120, "%s.%ld", buffer, tod.tv_usec / 1000);
   return std::string(currTime);
}

AecsCallCommandCallback::AecsCallCommandCallback(std::string commandName)
   : commandName_(commandName) {
}

void AecsCallCommandCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << commandName_ << " operation successful" << std::endl;
   } else {
      PRINT_CB << commandName_ << " operation failed" << std::endl;
   }
   PRINT_CB << commandName_ << " operation - ErrorCode " << (int)error
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}
