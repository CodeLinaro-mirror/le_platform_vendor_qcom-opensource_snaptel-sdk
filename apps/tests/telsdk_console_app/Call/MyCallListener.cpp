/*
 *  Copyright (c) 2018,2020-2021 The Linux Foundation. All rights reserved.
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

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

extern "C" {
#include <sys/time.h>
}

#include <telux/tel/PhoneFactory.hpp>

#include "MyCallListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MyCallListener::onIncomingCall(std::shared_ptr<telux::tel::ICall> call) {
    std::cout << std::endl << std::endl;
    std::string rttMode = getRttModeString(call->getRttMode());
    PRINT_NOTIFICATION << Utils::getCurrentTime() << std::endl;
    PRINT_NOTIFICATION << getCallStateString(call->getCallState())
                       << (rttMode == "FULL" ? " real time text call" : " normal voice call")
                       << " on slot Id: " << call->getPhoneId() << std::endl;
    if (!call->getCallReason().empty()) {
        PRINT_NOTIFICATION << "Call reason: " << call->getCallReason() << std::endl;
    }
    std::cout << "Enter 2 to answer call" << std::endl;
    std::cout << "Enter 3 to reject call" << std::endl;
}

void MyCallListener::onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION
        << " Call State: " << getCallStateString(call->getCallState())
        << "\n Call Index: " << (int)call->getCallIndex()
        << ", Call Direction: " << (int)call->getCallDirection()
        << ", Call Type: " << getCallTypeString(call->getCallType())
        << ", Network Mode: " << getNetworkModeString(call->getNetworkMode())
        << ", Phone Number: " << call->getRemotePartyNumber() << ", Slot Id: " << call->getPhoneId()
        << ", RTT mode of the call: " << getRttModeString(call->getRttMode())
        << ", Local capability of call: " << getRttModeString(call->getLocalRttCapability())
        << ", Peer capability of call: " << getRttModeString(call->getPeerRttCapability())
        << std::endl;
    if (!call->getCallReason().empty()) {
        PRINT_NOTIFICATION << "Call reason: " << call->getCallReason() << std::endl;
    }

    if (call->getCallState() == telux::tel::CallState::CALL_ENDED) {
        int phoneId                                     = call->getPhoneId();
        static std::shared_ptr<AudioClient> audioClient = AudioClient::getInstance();
        if (audioClient->isReady()) {
            int numCalls = getCallsOnSlot(static_cast<SlotId>(phoneId));
            std::cout << "In progress call for slotID : " << phoneId << " are : " << numCalls
                      << std::endl;
            if (numCalls < 1) {
                audioClient->stopVoiceSession(static_cast<SlotId>(phoneId));
            }
        }
        PRINT_NOTIFICATION
            << Utils::getCurrentTime()
            << " Cause of call termination: " << getCallEndCauseString(call->getCallEndCause())
            << ((call->getSipErrorCode() > 0) ? " and Sip error code: " : "")
            << ((call->getSipErrorCode() > 0) ? std::to_string(call->getSipErrorCode()) : "")
            << ((call->getDetailedCauseCode() > 0) ? " and detailed cause code: " : "")
            << ((call->getDetailedCauseCode() > 0) ? std::to_string(call->getDetailedCauseCode())
                                                   : "")
            << std::endl;
    }
}

void MyCallListener::onRingbackTone(bool isAlerting, int phoneId) {
    PRINT_NOTIFICATION << "onRingbackTone: " << (isAlerting == true ? "Start" : "Stop")
                       << " playing ringback tone on slot " << phoneId << std::endl;
}

// Notify CallManager subsystem restart to user
void MyCallListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat = "";
    switch (status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat = " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " Call onServiceStatusChange" << stat << "\n";
}

void MyCallListener::onModifyCallRequest(telux::tel::RttMode rttMode, int callId, int phoneId) {
    PRINT_NOTIFICATION
        << "onModifyCallRequest: "
        << (rttMode == telux::tel::RttMode::FULL ? " upgrade normal voice call to RTT call "
                                                 : " downgrade RTT call to normal voice call")
        << " on slot " << phoneId << " for callIndex " << callId << std::endl;
}

void MyCallListener::onRttMessage(int phoneId, std::string text) {
    PRINT_NOTIFICATION << "RTT message is " << text << " on slot " << phoneId << std::endl;
}

std::string MyCallListener::getCallStateString(telux::tel::CallState cs) {
    switch (cs) {
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

std::string MyCallListener::getCallTypeString(telux::tel::CallType type) {
    switch (type) {
        case telux::tel::CallType::VOICE_CALL:
            return std::string("Voice call");
        case telux::tel::CallType::VOICE_IP_CALL:
            return std::string("Voice IP call");
        case telux::tel::CallType::EMERGENCY_CALL:
            return std::string("Emergency call");
        case telux::tel::CallType::EMERGENCY_IP_CALL:
            return std::string("Emergency IP call");
        default:
            return std::string("unknown");
    }
}

std::string MyCallListener::getNetworkModeString(telux::tel::NetworkMode mode) {
    switch (mode) {
        case telux::tel::NetworkMode::GSM:
            return std::string("GSM mode");
        case telux::tel::NetworkMode::WCDMA:
            return std::string("WCDMA mode");
        case telux::tel::NetworkMode::LTE:
            return std::string("LTE mode");
        case telux::tel::NetworkMode::NR5G:
            return std::string("NR5G mode");
        default:
            return std::string("unknown");
    }
}

std::string MyCallListener::getRttModeString(telux::tel::RttMode mode) {
    switch (mode) {
        case telux::tel::RttMode::DISABLED:
            return std::string("DISABLED");
        case telux::tel::RttMode::FULL:
            return std::string("FULL");
        case telux::tel::RttMode::UNKNOWN:
        default:
            return std::string("UNKNOWN");
    }
}

std::string MyCallListener::getCallEndCauseString(telux::tel::CallEndCause callEndCause) {
    switch (callEndCause) {
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
        case telux::tel::CallEndCause::RADIO_OFF:
            return std::string("Radio off");
        case telux::tel::CallEndCause::OUT_OF_SERVICE:
            return std::string("Out of service");
        case telux::tel::CallEndCause::NO_VALID_SIM:
            return std::string("No valid sim");
        case telux::tel::CallEndCause::RADIO_INTERNAL_ERROR:
            return std::string("Radio internal error");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT:
            return std::string("Network response timeout");
        case telux::tel::CallEndCause::NETWORK_REJECT:
            return std::string("Network reject");
        case telux::tel::CallEndCause::RADIO_ACCESS_FAILURE:
            return std::string("Radio access failure");
        case telux::tel::CallEndCause::RADIO_LINK_FAILURE:
            return std::string("Radio link failure");
        case telux::tel::CallEndCause::RADIO_LINK_LOST:
            return std::string("Radio link lost");
        case telux::tel::CallEndCause::RADIO_UPLINK_FAILURE:
            return std::string("Radio uplink failure");
        case telux::tel::CallEndCause::RADIO_SETUP_FAILURE:
            return std::string("Radio setup failure");
        case telux::tel::CallEndCause::RADIO_RELEASE_NORMAL:
            return std::string("Radio release normal");
        case telux::tel::CallEndCause::RADIO_RELEASE_ABNORMAL:
            return std::string("Radio release abnormal");
        case telux::tel::CallEndCause::ACCESS_CLASS_BLOCKED:
            return std::string("Access class barring");
        case telux::tel::CallEndCause::NETWORK_DETACH:
            return std::string("Network detach");
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
        case telux::tel::CallEndCause::CLIENT_END:
            return std::string("Client End");
        case telux::tel::CallEndCause::INCOM_REJ:
            return std::string("Incom Rej");
        case telux::tel::CallEndCause::NO_GATEWAY_SRV:
            return std::string("No Gateway Srv");
        case telux::tel::CallEndCause::NO_FULL_SRV:
            return std::string("No Full Srv");
        case telux::tel::CallEndCause::CDMA_MAX_ACCESS_PROBE:
            return std::string("Cdma Max Access Probe");
        case telux::tel::CallEndCause::CDMA_PSIST_N:
            return std::string("Cdma Psist N");
        case telux::tel::CallEndCause::USSD_BUSY:
            return std::string("USSD Busy");
        case telux::tel::CallEndCause::REJECTED_BY_USER:
            return std::string("Rejected By User");
        case telux::tel::CallEndCause::NORMAL_CALL_CLEARING:
            return std::string("Normal Call Clearing");
        case telux::tel::CallEndCause::NORMAL_CALL_RINGBACK_TIMEOUT:
            return std::string("Normal Call Ringback Timeout");
        case telux::tel::CallEndCause::UIM_NOT_PRESENT:
            return std::string("Uim Not Present");
        case telux::tel::CallEndCause::INCOMPATIBLE:
            return std::string("Incompatible");
        case telux::tel::CallEndCause::ALREADY_IN_TC:
            return std::string("Already In Tc");
        case telux::tel::CallEndCause::USER_CALL_ORIG_DURING_GPS:
            return std::string("User Call Orig During Gps");
        case telux::tel::CallEndCause::USER_CALL_ORIG_DURING_SMS:
            return std::string("User Call Orig During Sms");
        case telux::tel::CallEndCause::USER_CALL_ORIG_DURING_DATA:
            return std::string("User Call Orig During Data");
        case telux::tel::CallEndCause::TRM_REQ_FAIL:
            return std::string("Trm Req Fail");
        case telux::tel::CallEndCause::CALL_CANNOT_BE_IDENTIFIED:
            return std::string("Call Cannot Be Identified");
        case telux::tel::CallEndCause::INCORRECT_SEMANTICS_IN_MESSAGE:
            return std::string("Incorrect Semantics In Message");
        case telux::tel::CallEndCause::MANDATORY_INFORMATION_INVALID:
            return std::string("Mandatory Information Invalid");
        case telux::tel::CallEndCause::WRONG_STATE:
            return std::string("Wrong State");
        case telux::tel::CallEndCause::INVALID_USER_DATA:
            return std::string("Invalid User Data");
        case telux::tel::CallEndCause::CNM_MM_REL_PENDING:
            return std::string("Cnm Mm Rel Pending");
        case telux::tel::CallEndCause::ACCESS_STRATUM_REJ_LOW_LEVEL_FAIL:
            return std::string("Access Stratum Rej Low Level Fail");
        case telux::tel::CallEndCause::ACCESS_STRATUM_REJ_LOW_LEVEL_FAIL_REDIAL_NOT_ALLOWED:
            return std::string("Access Stratum Rej Low Level Fail Redial Not Allowed");
        case telux::tel::CallEndCause::ACCESS_STRATUM_REJ_LOW_LEVEL_IMMED_RETRY:
            return std::string("Access Stratum Rej Low Level Immed Retry");
        case telux::tel::CallEndCause::ACCESS_STRATUM_REJ_ABORT_RADIO_UNAVAILABLE:
            return std::string("Access Stratum Rej Abort Radio Unavailable");
        case telux::tel::CallEndCause::CCS_NOT_SUPPORTED_BY_BS:
            return std::string("Ccs Not Supported By Bs");
        case telux::tel::CallEndCause::REJECTED_BY_BS:
            return std::string("Rejected By Bs");
        case telux::tel::CallEndCause::ACC_FAIL_REJ_ORD:
            return std::string("Acc Fail Rej Ord");
        case telux::tel::CallEndCause::ACC_FAIL_RETRY_ORD:
            return std::string("Acc Fail Retry Ord");
        case telux::tel::CallEndCause::UNKNOWN_SUBSCRIBER:
            return std::string("Unknown Subscriber");
        case telux::tel::CallEndCause::ILLEGAL_SUBSCRIBER:
            return std::string("Illegal Subscriber");
        case telux::tel::CallEndCause::BEARER_SERVICE_NOT_PROVISIONED:
            return std::string("Bearer Service Not Provisioned");
        case telux::tel::CallEndCause::TELE_SERVICE_NOT_PROVISIONED:
            return std::string("Tele Service Not Provisioned");
        case telux::tel::CallEndCause::ILLEGAL_EQUIPMENT:
            return std::string("Illegal Equipment");
        case telux::tel::CallEndCause::ILLEGAL_SS_OPERATION:
            return std::string("Illegal Ss Operation");
        case telux::tel::CallEndCause::SS_ERROR_STATUS:
            return std::string("Ss Error Status");
        case telux::tel::CallEndCause::SS_NOT_AVAILABLE:
            return std::string("Ss Not Available");
        case telux::tel::CallEndCause::SS_SUBSCRIPTION_VIOLATION:
            return std::string("Ss Subscription Violation");
        case telux::tel::CallEndCause::SS_INCOMPATIBILITY:
            return std::string("Ss Incompatibility");
        case telux::tel::CallEndCause::FACILITY_NOT_SUPPORTED:
            return std::string("Facility Not Supported");
        case telux::tel::CallEndCause::ABSENT_SUBSCRIBER:
            return std::string("Absent Subscriber");
        case telux::tel::CallEndCause::SHORT_TERM_DENIAL:
            return std::string("Short Term Denial");
        case telux::tel::CallEndCause::LONG_TERM_DENIAL:
            return std::string("Long Term Denial");
        case telux::tel::CallEndCause::SYSTEM_FAILURE:
            return std::string("System Failure");
        case telux::tel::CallEndCause::IMSI_UNKNOWN_IN_HLR:
            return std::string("Imsi Unknown In Hlr");
        case telux::tel::CallEndCause::ILLEGAL_MS:
            return std::string("Illegal Ms");
        case telux::tel::CallEndCause::ILLEGAL_ME:
            return std::string("Illegal Me");
        case telux::tel::CallEndCause::PLMN_NOT_ALLOWED:
            return std::string("Plmn Not Allowed");
        case telux::tel::CallEndCause::LOCATION_AREA_NOT_ALLOWED:
            return std::string("Location Area Not Allowed");
        case telux::tel::CallEndCause::ROAMING_NOT_ALLOWED_IN_THIS_LOCATION_AREA:
            return std::string("Roaming Not Allowed In This Location Area");
        case telux::tel::CallEndCause::NO_SUITABLE_CELLS_IN_LOCATION_AREA:
            return std::string("No Suitable Cells In Location Area");
        case telux::tel::CallEndCause::NETWORK_FAILURE:
            return std::string("Network Failure");
        case telux::tel::CallEndCause::MAC_FAILURE:
            return std::string("Mac Failure");
        case telux::tel::CallEndCause::SYNCH_FAILURE:
            return std::string("Synch Failure");
        case telux::tel::CallEndCause::GSM_AUTHENTICATION_UNACCEPTABLE:
            return std::string("Gsm Authentication Unacceptable");
        case telux::tel::CallEndCause::SERVICE_NOT_SUBSCRIBED:
            return std::string("Service Not Subscribed");
        case telux::tel::CallEndCause::ABORT_MSG_RECEIVED:
            return std::string("Abort Msg Received");
        case telux::tel::CallEndCause::SERVICE_OPTION_NOT_SUPPORTED:
            return std::string("Service Option Not Supported");
        case telux::tel::CallEndCause::AS_REJ_LRRC_CONN_EST_FAILURE_CONN_REJECT:
            return std::string("As Rej Lrrc Conn Est Failure Conn Reject");
        case telux::tel::CallEndCause::EMM_REJ_SERVICE_REQ_FAILURE_LTE_NW_REJECT:
            return std::string("Emm Rej Service Req Failure Lte Nw Reject");
        case telux::tel::CallEndCause::EMM_REJ_SERVICE_REQ_FAILURE_CS_DOMAIN_NOT_AVAILABLE:
            return std::string("Emm Rej Service Req Failure Cs Domain Not Available");
        case telux::tel::CallEndCause::EMM_REJ:
            return std::string("Emm Rej");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_FROM_BS:
            return std::string("Network Resp Timeout From Bs");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T42:
            return std::string("Network Resp Timeout T42");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T40:
            return std::string("Network Resp Timeout T40");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T50:
            return std::string("Network Resp Timeout T50");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T51:
            return std::string("Network Resp Timeout T51");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_BAD_FL:
            return std::string("Network Resp Timeout Bad Fl");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T41:
            return std::string("Network Resp Timeout T41");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T3230:
            return std::string("Network Resp Timeout T3230");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T303:
            return std::string("Network Resp Timeout T303");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_MT_CSFB:
            return std::string("Network Resp Timeout Mt Csfb");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T3417_EXT:
            return std::string("Network Resp Timeout T3417 Ext");
        case telux::tel::CallEndCause::NETWORK_RESP_TIMEOUT_T3417:
            return std::string("Network Resp Timeout T3417");
        case telux::tel::CallEndCause::RADIO_ACCESS_FAILURE_REJ_RR_RANDOM:
            return std::string("Radio Access Failure Rej Rr Random");
        case telux::tel::CallEndCause::RADIO_ACCESS_ESR_FAILURE:
            return std::string("Radio Access Esr Failure");
        case telux::tel::CallEndCause::RADIO_ACCESS_CS_ACQ_FAILURE:
            return std::string("Radio Access Cs Acq Failure");
        case telux::tel::CallEndCause::ACCESS_BARRED:
            return std::string("Access Barred");
        case telux::tel::CallEndCause::SSAC_REJECT:
            return std::string("Ssac Reject");
        case telux::tel::CallEndCause::RADIO_RELEASE_NORMAL_REJ_RR_REL:
            return std::string("Radio Release Normal Rej Rr Rel");
        case telux::tel::CallEndCause::RADIO_RELEASE_NORMAL_REJ_RRC_REL:
            return std::string("Radio Release Normal Rej Rrc Rel");
        case telux::tel::CallEndCause::RADIO_RELEASE_NORMAL_OOS_DURING_CRE:
            return std::string("Radio Release Normal Oos During Cre");
        case telux::tel::CallEndCause::RADIO_RELEASE_ABNORMAL_CLOSE_SESSION_IND:
            return std::string("Radio Release Abnormal Close Session Ind");
        case telux::tel::CallEndCause::RADIO_RELEASE_ABNORMAL_OPEN_SESSION_FAILURE:
            return std::string("Radio Release Abnormal Open Session Failure");
        case telux::tel::CallEndCause::RADIO_RELEASE_ABNORMAL_CRE_FAILURE:
            return std::string("Radio Release Abnormal Cre Failure");
        case telux::tel::CallEndCause::RADIO_RELEASE_ABNORMAL_SIB_READ_ERROR:
            return std::string("Radio Release Abnormal Sib Read Error");
        case telux::tel::CallEndCause::RADIO_RELEASE_ABNORMAL_ABORTED_IRAT_SUCCESS:
            return std::string("Radio Release Abnormal Aborted Irat Success");
        case telux::tel::CallEndCause::RADIO_UPLINK_FAILURE_TXN:
            return std::string("Radio Uplink Failure Txn");
        case telux::tel::CallEndCause::RADIO_UPLINK_FAILURE_HO:
            return std::string("Radio Uplink Failure Ho");
        case telux::tel::CallEndCause::RADIO_UPLINK_FAILURE_CTRL_NOT_CONN:
            return std::string("Radio Uplink Failure Ctrl Not Conn");
        case telux::tel::CallEndCause::RADIO_LINK_FAILURE_UL_DATA_CNF:
            return std::string("Radio Link Failure Ul Data Cnf");
        case telux::tel::CallEndCause::RADIO_LINK_FAILURE_EST_FAILURE:
            return std::string("Radio Link Failure Est Failure");
        case telux::tel::CallEndCause::RADIO_LINK_FAILURE_CONN_REL_RLF:
            return std::string("Radio Link Failure Conn Rel Rlf");
        case telux::tel::CallEndCause::RADIO_LINK_FAILURE_REJ:
            return std::string("Radio Link Failure Rej");
        case telux::tel::CallEndCause::RADIO_LINK_FAILURE_DURING_CC_DISCONNECT:
            return std::string("Radio Link Failure During Cc Disconnect");
        case telux::tel::CallEndCause::RADIO_SETUP_FAILURE_REJ:
            return std::string("Radio Setup Failure Rej");
        case telux::tel::CallEndCause::RADIO_SETUP_FAILURE_ABORTED:
            return std::string("Radio Setup Failure Aborted");
        case telux::tel::CallEndCause::RADIO_SETUP_FAILURE_CELL_RESEL:
            return std::string("Radio Setup Failure Cell Resel");
        case telux::tel::CallEndCause::RADIO_SETUP_FAILURE_CONFIG_FAILURE:
            return std::string("Radio Setup Failure Config Failure");
        case telux::tel::CallEndCause::RADIO_SETUP_FAILURE_TIMER_EXPIRED:
            return std::string("Radio Setup Failure Timer Expired");
        case telux::tel::CallEndCause::RADIO_SETUP_FAILURE_SI_FAILURE:
            return std::string("Radio Setup Failure Si Failure");
        case telux::tel::CallEndCause::NETWORK_DETACH_WITH_OUT_REATTACH:
            return std::string("Network Detach With Out Reattach");
        case telux::tel::CallEndCause::PDN_DISCONNECTED:
            return std::string("Pdn Disconnected");
        case telux::tel::CallEndCause::CSFB_FAILURE_CALL_REL_NW_REL_ODR:
            return std::string("1xcsfb failure call rel nw rel odr");
        case telux::tel::CallEndCause::CSFB_FAILURE_CALL_REL_REG_REJ:
            return std::string("1xcsfb failure call rel reg rej");
        case telux::tel::CallEndCause::CSFB_FAILURE_RETRY_EXHAUST:
            return std::string("1xcsfb failure retry exhaust");
        case telux::tel::CallEndCause::CSFB_FAILURE_USER_CALL_END:
            return std::string("1xcsfb failure user call end");
        case telux::tel::CallEndCause::CSFB_FAILURE_SRCH_TT_FAIL:
            return std::string("1xcsfb failure srch tt fail");
        case telux::tel::CallEndCause::CSFB_FAILURE_TCH_INIT_FAIL:
            return std::string("1xcsfb failure tch init fail");
        case telux::tel::CallEndCause::CSFB_FAIL_ACQ_FAIL:
            return std::string("1xcsfb fail acq fail");
        case telux::tel::CallEndCause::CSFB_FAIL_CALL_REL_INTERCEPT_ORDER:
            return std::string("1xcsfb fail call rel intercept order");
        case telux::tel::CallEndCause::CSFB_FAIL_CALL_REL_NORMAL:
            return std::string("1xcsfb fail call rel normal");
        case telux::tel::CallEndCause::CSFB_FAIL_CALL_REL_OTASP_SPC_ERR:
            return std::string("1xcsfb fail call rel otasp spc err");
        case telux::tel::CallEndCause::CSFB_FAIL_CALL_REL_REL_ORDER:
            return std::string("1xcsfb fail call rel rel order");
        case telux::tel::CallEndCause::CSFB_FAIL_CALL_REL_REORDER:
            return std::string("1xcsfb fail call rel reorder");
        case telux::tel::CallEndCause::CSFB_FAIL_CALL_REL_SO_REJ:
            return std::string("1xcsfb fail call rel so rej");
        case telux::tel::CallEndCause::CSFB_HARD_FAILURE:
            return std::string("1xcsfb hard failure");
        case telux::tel::CallEndCause::CSFB_HO_FAILURE:
            return std::string("1xcsfb ho failure");
        case telux::tel::CallEndCause::CSFB_MSG_IGNORE:
            return std::string("1xcsfb msg ignore");
        case telux::tel::CallEndCause::CSFB_MSG_INVAILD:
            return std::string("1xcsfb msg invaild");
        case telux::tel::CallEndCause::CSFB_SOFT_FAILURE:
            return std::string("1xcsfb soft failure");
        case telux::tel::CallEndCause::ACCESS_BLOCK:
            return std::string("Access block");
        case telux::tel::CallEndCause::ACC_IN_PROG:
            return std::string("Acc in prog");
        case telux::tel::CallEndCause::ACTIVATION:
            return std::string("Activation");
        case telux::tel::CallEndCause::ADDRESS_INCOMPLETE:
            return std::string("Address incomplete");
        case telux::tel::CallEndCause::ALERT_STOP:
            return std::string("Alert stop");
        case telux::tel::CallEndCause::ALTERNATE_EMERGENCY_CALL:
            return std::string("Alternate emergency call");
        case telux::tel::CallEndCause::ALTERNATE_SERVICE:
            return std::string("Alternate service");
        case telux::tel::CallEndCause::AMBIGUOUS:
            return std::string("Ambiguous");
        case telux::tel::CallEndCause::AS_REJ_LRRC_CONN_EST_FAILURE_NOT_CAMPED:
            return std::string("As rej lrrc conn est failure not camped");
        case telux::tel::CallEndCause::AS_REJ_LRRC_CONN_EST_SUCCESS:
            return std::string("As rej lrrc conn est success");
        case telux::tel::CallEndCause::BAD_EXTENSION:
            return std::string("Bad extension");
        case telux::tel::CallEndCause::BAD_GATEWAY:
            return std::string("Bad gateway");
        case telux::tel::CallEndCause::BAD_REQ_WAIT_INVITE:
            return std::string("Bad req wait invite");
        case telux::tel::CallEndCause::BAD_REQ_WAIT_REINVITE:
            return std::string("Bad req wait reinvite");
        case telux::tel::CallEndCause::BUSY_EVERYWHERE:
            return std::string("Busy everywhere");
        case telux::tel::CallEndCause::CALL_COMPLETED_ELSEWHERE:
            return std::string("Call completed elsewhere");
        case telux::tel::CallEndCause::CALL_DEFLECTED:
            return std::string("Call deflected");
        case telux::tel::CallEndCause::CALL_OR_TRANS_DOES_NOT_EXIST:
            return std::string("Call or trans does not exist");
        case telux::tel::CallEndCause::CALL_PULLED:
            return std::string("Call pulled");
        case telux::tel::CallEndCause::CALL_PULL_OUT_OF_SYNC:
            return std::string("Call pull out of sync");
        case telux::tel::CallEndCause::CCBS_NOT_POSSIBLE:
            return std::string("Ccbs not possible");
        case telux::tel::CallEndCause::CCBS_POSSIBLE:
            return std::string("Ccbs possible");
        case telux::tel::CallEndCause::CLIR_NOT_SUBSCRIBED:
            return std::string("Clir not subscribed");
        case telux::tel::CallEndCause::CODEC_ERROR:
            return std::string("Codec error");
        case telux::tel::CallEndCause::CSFB_NOT_FEASIBLE_IN_ROAM_CS_NW:
            return std::string("Csfb not feasible in roam cs nw");
        case telux::tel::CallEndCause::CS_HARD_FAILURE:
            return std::string("Cs hard failure");
        case telux::tel::CallEndCause::CUG_CALL_FAILURE_UNSPECIFIED:
            return std::string("Cug call failure unspecified");
        case telux::tel::CallEndCause::CUG_INDEX_INCOMPATIBLE:
            return std::string("Cug index incompatible");
        case telux::tel::CallEndCause::DATA_CONNECTION_LOST:
            return std::string("Data connection lost");
        case telux::tel::CallEndCause::DATA_MISSING:
            return std::string("Data missing");
        case telux::tel::CallEndCause::DEAD_BATTERY:
            return std::string("Dead battery");
        case telux::tel::CallEndCause::DEFLECTION_TO_SERVED_SUBSCRIBER:
            return std::string("Deflection to served subscriber");
        case telux::tel::CallEndCause::DOES_NOT_EXIST_ANYWHERE:
            return std::string("Does not exist anywhere");
        case telux::tel::CallEndCause::DRVCC_END_CALL:
            return std::string("Drvcc end call");
        case telux::tel::CallEndCause::DRVCC_IN_PROG:
            return std::string("Drvcc in prog");
        case telux::tel::CallEndCause::EXTENSION_REQUIRED:
            return std::string("Extension required");
        case telux::tel::CallEndCause::FALLBACK_TO_CS:
            return std::string("Fallback to cs");
        case telux::tel::CallEndCause::GONE:
            return std::string("Gone");
        case telux::tel::CallEndCause::INCOMING_REJ_CAUSE_1X_COLLISION:
            return std::string("Incoming rej cause 1x collision");
        case telux::tel::CallEndCause::INCOMING_REJ_CAUSE_CALL_ONGOING_CB_ENABLED:
            return std::string("Incoming rej cause call ongoing cb enabled");
        case telux::tel::CallEndCause::INCOMING_REJ_CAUSE_CALL_ONGOING_CW_DISABLED:
            return std::string("Incoming rej cause call ongoing cw disabled");
        case telux::tel::CallEndCause::INCOMING_REJ_CAUSE_CALL_ON_OTHER_SUB:
            return std::string("Incoming rej cause call on other sub");
        case telux::tel::CallEndCause::INCOM_REJ_CAUSE_UI_NOT_READY:
            return std::string("Incom rej cause ui not ready");
        case telux::tel::CallEndCause::INTERVAL_TOO_BRIEF:
            return std::string("Interval too brief");
        case telux::tel::CallEndCause::INVALID_DEFLECTED_TO_NUMBER:
            return std::string("Invalid deflected to number");
        case telux::tel::CallEndCause::INVALID_REMOTE_URI:
            return std::string("Invalid remote uri");
        case telux::tel::CallEndCause::IS707B_MAX_ACC:
            return std::string("Is707b max acc");
        case telux::tel::CallEndCause::LOOP_DETECTED:
            return std::string("Loop detected");
        case telux::tel::CallEndCause::MC_ABORT:
            return std::string("Mc abort");
        case telux::tel::CallEndCause::MERGED_TO_CONFERENCE:
            return std::string("Merged to conference");
        case telux::tel::CallEndCause::MESSAGE_TOO_LARGE:
            return std::string("Message too large");
        case telux::tel::CallEndCause::METHOD_NOT_ALLOWED:
            return std::string("Method not allowed");
        case telux::tel::CallEndCause::MOVED_PERMANENTLY:
            return std::string("Moved permanently");
        case telux::tel::CallEndCause::MOVED_TEMPORARILY:
            return std::string("Moved temporarily");
        case telux::tel::CallEndCause::MPTY_PARTICIPANTS_EXCEEDED:
            return std::string("Mpty participants exceeded");
        case telux::tel::CallEndCause::MULTIPLE_CHOICES:
            return std::string("Multiple choices");
        case telux::tel::CallEndCause::NEGATIVE_PWD_CHECK:
            return std::string("Negative pwd check");
        case telux::tel::CallEndCause::NETWORK_NO_RESP_HOLD_FAIL:
            return std::string("Network no resp hold fail");
        case telux::tel::CallEndCause::NETWORK_NO_RESP_TIME_OUT:
            return std::string("Network no resp time out");
        case telux::tel::CallEndCause::NOT_ACCEPTABLE:
            return std::string("Not acceptable");
        case telux::tel::CallEndCause::NOT_ACCEPTABLE_GLOBAL:
            return std::string("Not acceptable global");
        case telux::tel::CallEndCause::NOT_ACCEPTABLE_HERE:
            return std::string("Not acceptable here");
        case telux::tel::CallEndCause::NOT_IMPLEMENTED:
            return std::string("Not implemented");
        case telux::tel::CallEndCause::NO_CDMA_SRV:
            return std::string("No cdma srv");
        case telux::tel::CallEndCause::NO_CELL_AVAILABLE:
            return std::string("No cell available");
        case telux::tel::CallEndCause::NO_CUG_SELECTION:
            return std::string("No cug selection");
        case telux::tel::CallEndCause::NO_NETWORK_RESP:
            return std::string("No network resp");
        case telux::tel::CallEndCause::NO_RESOURCES:
            return std::string("No resources");
        case telux::tel::CallEndCause::NUM_OF_PWD_ATTEMPTS_VIOLATION:
            return std::string("Num of pwd attempts violation");
        case telux::tel::CallEndCause::OTASP_SPC_ERR:
            return std::string("Otasp spc err");
        case telux::tel::CallEndCause::OUTGOING_CALLS_BARRED_WITHIN_CUG:
            return std::string("Outgoing calls barred within cug");
        case telux::tel::CallEndCause::PAYMENT_REQUIRED:
            return std::string("Payment required");
        case telux::tel::CallEndCause::POSITION_METHOD_FAILURE:
            return std::string("Position method failure");
        case telux::tel::CallEndCause::PRECONDITION_FAILURE:
            return std::string("Precondition failure");
        case telux::tel::CallEndCause::PROXY_AUTHENTICATION_REQUIRED:
            return std::string("Proxy authentication required");
        case telux::tel::CallEndCause::PWD_REGISTRATION_FAILURE:
            return std::string("Pwd registration failure");
        case telux::tel::CallEndCause::REDIR_OR_HANDOFF:
            return std::string("Redir or handoff");
        case telux::tel::CallEndCause::REG_RESTORATION:
            return std::string("Reg restoration");
        case telux::tel::CallEndCause::REMOTE_UNSUPP_MEDIA_TYPE:
            return std::string("Remote unsupp media type");
        case telux::tel::CallEndCause::REQUEST_ENTITY_TOO_LARGE:
            return std::string("Request entity too large");
        case telux::tel::CallEndCause::REQUEST_PENDING:
            return std::string("Request pending");
        case telux::tel::CallEndCause::REQUEST_TERMINATED:
            return std::string("Request terminated");
        case telux::tel::CallEndCause::REQUEST_URI_TOO_LARGE:
            return std::string("Request uri too large");
        case telux::tel::CallEndCause::RESOURCES_NOT_AVAILABLE:
            return std::string("Resources not available");
        case telux::tel::CallEndCause::RRC_CONN_REL_NO_MT_SETUP:
            return std::string("Rrc conn rel no mt setup");
        case telux::tel::CallEndCause::RTP_FAILURE:
            return std::string("Rtp failure");
        case telux::tel::CallEndCause::RTP_RTCP_TIMEOUT:
            return std::string("Rtp rtcp timeout");
        case telux::tel::CallEndCause::SERVER_INTERNAL_ERROR:
            return std::string("Server internal error");
        case telux::tel::CallEndCause::SERVER_TIME_OUT:
            return std::string("Server time out");
        case telux::tel::CallEndCause::SERVER_UNAVAILABLE:
            return std::string("Server unavailable");
        case telux::tel::CallEndCause::SESS_DESCR_NOT_ACCEPTABLE:
            return std::string("Sess descr not acceptable");
        case telux::tel::CallEndCause::SIP_403_FORBIDDEN:
            return std::string("Sip 403 forbidden");
        case telux::tel::CallEndCause::SIP_503_SERVER_UNAVAILABLE:
            return std::string("Sip 503 server unavailable");
        case telux::tel::CallEndCause::SPECIAL_SERVICE_CODE:
            return std::string("Special service code");
        case telux::tel::CallEndCause::SRVCC_END_CALL:
            return std::string("Srvcc end call");
        case telux::tel::CallEndCause::SRV_INIT_FAIL:
            return std::string("Srv init fail");
        case telux::tel::CallEndCause::TOO_MANY_HOPS:
            return std::string("Too many hops");
        case telux::tel::CallEndCause::UNAUTHORIZED:
            return std::string("Unauthorized");
        case telux::tel::CallEndCause::UNDECIPHERABLE:
            return std::string("Undecipherable");
        case telux::tel::CallEndCause::UNEXPECTED_DATA_VALUE:
            return std::string("Unexpected data value");
        case telux::tel::CallEndCause::UNKNOWN_ALPHABET:
            return std::string("Unknown alphabet");
        case telux::tel::CallEndCause::UNKNOWN_CUG_INDEX:
            return std::string("Unknown cug index");
        case telux::tel::CallEndCause::UNSUPPORTED_SDP:
            return std::string("Unsupported sdp");
        case telux::tel::CallEndCause::UNSUPPORTED_URI_SCHEME:
            return std::string("Unsupported uri scheme");
        case telux::tel::CallEndCause::UNWANTED_CALL:
            return std::string("Unwanted call");
        case telux::tel::CallEndCause::UPGRADE_DOWNGRADE_CANCELLED:
            return std::string("Upgrade downgrade cancelled");
        case telux::tel::CallEndCause::UPGRADE_DOWNGRADE_FAILED:
            return std::string("Upgrade downgrade failed");
        case telux::tel::CallEndCause::UPGRADE_DOWNGRADE_REJ:
            return std::string("Upgrade downgrade rej");
        case telux::tel::CallEndCause::USE_PROXY:
            return std::string("Use proxy");
        case telux::tel::CallEndCause::VERSION_NOT_SUPPORTED:
            return std::string("Version not supported");
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
            return std::string("Not found");
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
        case telux::tel::CallEndCause::SIP_ALTERNATE_EMERGENCY_CALL:
            return std::string("Emergency call");
        case telux::tel::CallEndCause::NO_CSFB_IN_CS_ROAM:
            return std::string("No cs fallback in roaming network");
        case telux::tel::CallEndCause::SRV_NOT_REGISTERED:
            return std::string("Service no registered");
        case telux::tel::CallEndCause::CALL_TYPE_NOT_ALLOWED:
            return std::string("Call type is not allowed");
        case telux::tel::CallEndCause::EMRG_CALL_ONGOING:
            return std::string("Emergency call ongoing");
        case telux::tel::CallEndCause::CALL_SETUP_ONGOING:
            return std::string("Call setup ongoing");
        case telux::tel::CallEndCause::MAX_CALL_LIMIT_REACHED:
            return std::string("Maximum call limit reached");
        case telux::tel::CallEndCause::UNSUPPORTED_SIP_HDRS:
            return std::string("Unsupported sip header");
        case telux::tel::CallEndCause::CALL_TRANSFER_ONGOING:
            return std::string("Call transfer ongoing");
        case telux::tel::CallEndCause::PRACK_TIMEOUT:
            return std::string("Memory failure");
        case telux::tel::CallEndCause::QOS_FAILURE:
            return std::string("Lack of dedicated barrier");
        case telux::tel::CallEndCause::ONGOING_HANDOVER:
            return std::string("Handover ongoing");
        case telux::tel::CallEndCause::VT_WITH_TTY_NOT_ALLOWED:
            return std::string("VT and TTY not supported together");
        case telux::tel::CallEndCause::CALL_UPGRADE_ONGOING:
            return std::string("Call upgrade is ongoing");
        case telux::tel::CallEndCause::CONFERENCE_WITH_TTY_NOT_ALLOWED:
            return std::string("Conference with TTY not allowed");
        case telux::tel::CallEndCause::CALL_CONFERENCE_ONGOING:
            return std::string("Call conference ongoing");
        case telux::tel::CallEndCause::VT_WITH_AVPF_NOT_ALLOWED:
            return std::string("VT with AVPF not allowed");
        case telux::tel::CallEndCause::ENCRYPTION_CALL_ONGOING:
            return std::string("Encryption call is ongoing");
        case telux::tel::CallEndCause::CALL_ONGOING_CW_DISABLED:
            return std::string("Call waiting disabled for incoming call");
        case telux::tel::CallEndCause::CALL_ON_OTHER_SUB:
            return std::string("Call on other subscription");
        case telux::tel::CallEndCause::ONE_X_COLLISION:
            return std::string("CDMA collision");
        case telux::tel::CallEndCause::UI_NOT_READY:
            return std::string("UI is not reay for incomg call");
        case telux::tel::CallEndCause::CS_CALL_ONGOING:
            return std::string("CS call is ongoing");
        case telux::tel::CallEndCause::REJECTED_ELSEWHERE:
            return std::string("One of the devices rejected the call");
        case telux::tel::CallEndCause::USER_REJECTED_SESSION_MODIFICATION:
            return std::string("Session modification is rejected");
        case telux::tel::CallEndCause::USER_CANCELLED_SESSION_MODIFICATION:
            return std::string("Session modification is cancelled");
        case telux::tel::CallEndCause::SESSION_MODIFICATION_FAILED:
            return std::string("Session modification is failed");
        case telux::tel::CallEndCause::SIP_UNAUTHORIZED:
            return std::string("Unauthorized");
        case telux::tel::CallEndCause::SIP_PAYMENT_REQUIRED:
            return std::string("Payment required");
        case telux::tel::CallEndCause::SIP_METHOD_NOT_ALLOWED:
            return std::string("Method not allowed");
        case telux::tel::CallEndCause::SIP_PROXY_AUTHENTICATION_REQUIRED:
            return std::string("Proxy authentication required");
        case telux::tel::CallEndCause::SIP_REQUEST_ENTITY_TOO_LARGE:
            return std::string("Request entity too large");
        case telux::tel::CallEndCause::SIP_REQUEST_URI_TOO_LARGE:
            return std::string("Request URI too large");
        case telux::tel::CallEndCause::SIP_EXTENSION_REQUIRED:
            return std::string("Extension requied");
        case telux::tel::CallEndCause::SIP_INTERVAL_TOO_BRIEF:
            return std::string("Interval too brief");
        case telux::tel::CallEndCause::SIP_CALL_OR_TRANS_DOES_NOT_EXIST:
            return std::string("Call/Transcation does not exist");
        case telux::tel::CallEndCause::SIP_LOOP_DETECTED:
            return std::string("Loop detected");
        case telux::tel::CallEndCause::SIP_TOO_MANY_HOPS:
            return std::string("Too many hops");
        case telux::tel::CallEndCause::SIP_AMBIGUOUS:
            return std::string("Ambiguous");
        case telux::tel::CallEndCause::SIP_REQUEST_PENDING:
            return std::string("Request pending");
        case telux::tel::CallEndCause::SIP_UNDECIPHERABLE:
            return std::string("Undecipherable");
        case telux::tel::CallEndCause::RETRY_ON_IMS_WITHOUT_RTT:
            return std::string("Retry call by disabling RTT");
        case telux::tel::CallEndCause::MAX_PS_CALLS:
            return std::string("Maximum PS calls exceeded");
        case telux::tel::CallEndCause::SIP_MULTIPLE_CHOICES:
            return std::string("Multiple choices");
        case telux::tel::CallEndCause::SIP_MOVED_PERMANENTLY:
            return std::string("Moved permanently");
        case telux::tel::CallEndCause::SIP_MOVED_TEMPORARILY:
            return std::string("Moved temporarily");
        case telux::tel::CallEndCause::SIP_USE_PROXY:
            return std::string("Use proxy");
        case telux::tel::CallEndCause::SIP_ALTERNATE_SERVICE:
            return std::string("Alternative service");
        case telux::tel::CallEndCause::SIP_UNSUPPORTED_URI_SCHEME:
            return std::string("Unsupported URI scheme");
        case telux::tel::CallEndCause::SIP_REMOTE_UNSUPP_MEDIA_TYPE:
            return std::string("Unsupported media type");
        case telux::tel::CallEndCause::SIP_BAD_EXTENSION:
            return std::string("Bad extension");
        case telux::tel::CallEndCause::DSDA_CONCURRENT_CALL_NOT_POSSIBLE:
            return std::string("Concurrent call is not possible");
        case telux::tel::CallEndCause::EPSFB_FAILURE:
            return std::string("EPS fallback failure");
        case telux::tel::CallEndCause::TWAIT_EXPIRED:
            return std::string("Twait timer expired");
        case telux::tel::CallEndCause::TCP_CONNECTION_REQ:
            return std::string("TCP connection rejected");
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

void MyDialCallback::makeCallResponse(
    telux::common::ErrorCode error, std::shared_ptr<telux::tel::ICall> call) {
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
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_NOTIFICATION << commandName_ << " operation successful" << std::endl;
    } else {
        PRINT_NOTIFICATION << commandName_ << " operation failed" << std::endl;
    }
    PRINT_NOTIFICATION << commandName_ << " operation - ErrorCode " << (int)error
                       << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

void MyHangupCallback::hangupFgResumeBgResponse(telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << " Hangup foreground resume background request executed successfully \n";
    } else {
        PRINT_CB << " Hangup foreground resume background request failed with error: "
                 << Utils::getErrorCodeAsString(error) << "\n";
    }
}

void MyHangupCallback::hangupWaitingOrBgResponse(telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << " Hangup waiting or background request executed successfully \n";
    } else {
        PRINT_CB << " Hangup waiting or background request request failed with error: "
                 << Utils::getErrorCodeAsString(error) << "\n";
    }
}

void MyRttMessageCallback::sendRttMessageResponse(telux::common::ErrorCode error) {
    std::cout << "\n";
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << " Send RTT data request is successful \n";
    } else {
        PRINT_CB << "Send RTT data request request failed with error: "
                 << Utils::getErrorCodeAsString(error) << "\n";
    }
}

int MyCallListener::getCallsOnSlot(SlotId slotId) {
    int numCalls = 0;
    std::promise<ServiceStatus> callMgrprom;
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    auto callManager
        = phoneFactory.getCallManager([&](ServiceStatus status) { callMgrprom.set_value(status); });
    if (!callManager) {
        std::cout << "ERROR - Failed to get CallManager instance \n";
        return false;
    }
    ServiceStatus callMgrsubSystemStatus = callMgrprom.get_future().get();
    if (callMgrsubSystemStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
            = callManager->getInProgressCalls();
        for (auto callIterator = std::begin(inProgressCalls);
             callIterator != std::end(inProgressCalls); ++callIterator) {
            if (slotId == static_cast<SlotId>((*callIterator)->getPhoneId())) {
                numCalls++;
            }
        }
    } else {
        std::cout << "ERROR - CallManager subsystem is not ready"
                  << ", failed to get in progress calls on slot Id:" << static_cast<int>(slotId)
                  << std::endl;
    }
    return numCalls;
}
