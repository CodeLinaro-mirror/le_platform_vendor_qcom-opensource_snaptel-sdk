/*
 *  Copyright (c) 2018,2020 The Linux Foundation. All rights reserved.
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

#include <chrono>
#include <iostream>
#include <sstream>

extern "C" {
#include <sys/time.h>
}

#include <telux/tel/PhoneFactory.hpp>

#include "MyCallListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void MyCallListener::onIncomingCall(std::shared_ptr<telux::tel::ICall> call) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << getCurrentTime() << std::endl;
   std::cout <<  getCallStateString(call->getCallState())
             << " on slot Id: " << call->getPhoneId() << std::endl;
   std::cout << "Enter 2 to answer call" << std::endl;
   std::cout << "Enter 3 to reject call" << std::endl;
   std::cout << "Enter 4 to reject call with an SMS message" << std::endl;
}

void MyCallListener::onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << " Call State: " << getCallStateString(call->getCallState())
                      << "\n Call Index: " << (int)call->getCallIndex()
                      << ", Call Direction: " << (int)call->getCallDirection()
                      << ", Phone Number: " << call->getRemotePartyNumber()
                      << ", Slot Id: " << call->getPhoneId() << std::endl;
   if(call->getCallState() == telux::tel::CallState::CALL_ENDED) {
       int phoneId = call->getPhoneId();
        AudioClient& audioClient = AudioClient::getInstance();
        if (audioClient.isReady()) {
            int numCalls = getCallsOnSlot(static_cast<SlotId>(phoneId));
            std::cout << "In progress call for slotID : " << phoneId
                << " are : " << numCalls << std::endl;
            if (numCalls < 1) {
                audioClient.stopVoiceSession(static_cast<SlotId>(phoneId));
            }
        }
      PRINT_NOTIFICATION << getCurrentTime() << " Cause of call termination: "
                         << getCallEndCauseString(call->getCallEndCause()) << std::endl;
   }
}

void MyCallListener::onECallMsdTransmissionStatus(int phoneId, telux::common::ErrorCode errorCode) {
   if(errorCode == telux::common::ErrorCode::SUCCESS) {
      PRINT_NOTIFICATION << "OnECallMsdTransmissionStatus is success" << std::endl;
   } else {
      PRINT_NOTIFICATION
         << "OnECallMsdTransmissionStatus failed, code: " << static_cast<int>(errorCode)
         << std::endl;
   }
}

std::string MyCallListener::getCallStateString(telux::tel::CallState cs) {
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

std::string MyCallListener::getCallEndCauseString(telux::tel::CallEndCause callEndCause) {
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

std::string MyCallListener::getCurrentTime() {
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

void MyDialCallback::makeCallResponse(telux::common::ErrorCode error,
                                      std::shared_ptr<telux::tel::ICall> call) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "makeCall response ErrorCode: " << int(error)
                      << ", description: " << Utils::getErrorCodeAsString(error)
                      << ", slot id: " << call->getPhoneId() << std::endl;
   callObj_ = call;
}

std::shared_ptr<telux::tel::ICall> MyDialCallback::getCallObj() {
   return callObj_;
}

void MyDialCallback::waitForResponse(int seconds) {
   std::cout << __FUNCTION__ << " : " << seconds << std::endl;
   std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

MyCallCommandCallback::MyCallCommandCallback(std::string commandName)
   : commandName_(commandName) {
}

void MyCallCommandCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_NOTIFICATION << commandName_ << " operation successful" << std::endl;
   } else {
      PRINT_NOTIFICATION << commandName_ << " operation failed" << std::endl;
   }
   PRINT_NOTIFICATION << commandName_ << " operation - ErrorCode " << (int)error
                      << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

int MyCallListener::getCallsOnSlot(SlotId slotId) {
    int numCalls = 0;
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    auto callManager = phoneFactory.getCallManager();
    if (callManager) {
        std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
          = callManager->getInProgressCalls();
        for(auto callIterator = std::begin(inProgressCalls);
            callIterator != std::end(inProgressCalls); ++callIterator) {
            if (slotId == static_cast<SlotId>((*callIterator)->getPhoneId())) {
                numCalls++;
            }
        }
    } else {
        std::cout << "ERROR - CallManager is NULL, failed to get in progress calls on slot Id:"
                  << static_cast<int>(slotId) << std::endl;
    }
    return numCalls;
}
