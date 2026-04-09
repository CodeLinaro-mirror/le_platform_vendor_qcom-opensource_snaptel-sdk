/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <iostream>
#include <sstream>

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

    std::cout << "Enter 2 to answer call" << std::endl;
    std::cout << "Enter 3 to reject call" << std::endl;
}

void AecsCallListener::onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) {
    int phoneId = call->getPhoneId();
    std::string number = call->getRemotePartyNumber();
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << " Call State: " << getCallStateString(call->getCallState())
                      << "\n Call Index: " << (int)call->getCallIndex()
                      << ", Call Direction: " << (int)call->getCallDirection()
                      << ", Phone Number: " << number
                      << ", Slot Id: " << phoneId
                      << std::endl;

    auto &mgr = AecsCallManager::getInstance();
    mgr.setAecsCallDropStatus(false);
    mgr.setAecsCallFailStatus(false);
    if (call->getCallState() == telux::tel::CallState::CALL_ENDED) {
        // Retry if it is call drop or call origination fail or call failed due
        // to reasons other than Radio off, user or network end the call.
        if (!isAecsCallFailReason(call->getCallEndCause())) {
            mgr.stopAudioIfNoCalls(phoneId);

            // Stop any running retry loop
            stopRetryLoop();

            // Not a drop/failure termination → close any leftover drop window.
            dropWindowArmed_ = false;

        } else if (call->isAecsCallDrop() &&
            (call->getCallDirection() == telux::tel::CallDirection::OUTGOING)) {
            mgr.setAecsCallDropStatus(true);
            // Use OEM/user config for drops
            int intervalSec = mgr.getAecsOemRetryInterval();
            int durationSec = mgr.getAecsOemRetryDuration();

            if (!retryInProgress_.load()) {
                startUnifiedRetry(RetryMode::Drop, phoneId, number, intervalSec, durationSec);
            } else if (retryMode_ != RetryMode::Drop) {
                // Switch running loop to Drop: stop & restart with Drop parameters
                stopRetryLoop();
                startUnifiedRetry(RetryMode::Drop, phoneId, number, intervalSec, durationSec);
            } else {
                // Already retrying -> push next attempt to (END + interval)
                scheduleNextRetryFromNow();
            }
        } else if ((call->getRedialState() == telux::tel::RedialState::MODEM_RETRY_END) &&
            (call->getCallDirection() == telux::tel::CallDirection::OUTGOING)) {
            mgr.setAecsCallFailStatus(true);
            int intervalSec = mgr.getAecsRetryInterval();
            int durationSec = mgr.getAecsRetryDuration();

            if (!retryInProgress_.load()) {
                startUnifiedRetry(RetryMode::Fail, phoneId, number, intervalSec, durationSec);
            } else if (retryMode_ != RetryMode::Fail) {
               // Switch running loop to Fail: stop & restart with Fail parameters
               stopRetryLoop();
               startUnifiedRetry(RetryMode::Fail, phoneId, number, intervalSec, durationSec);
            } else {
                // Already retrying -> push next attempt to (END + interval)
                scheduleNextRetryFromNow();
            }
        } else {
             // retry for AECS call failed reasons
            mgr.setAecsCallFailStatus(true);
            int intervalSec = mgr.getAecsRetryInterval();
            int durationSec = mgr.getAecsRetryDuration();

            if (!retryInProgress_.load()) {
                startUnifiedRetry(RetryMode::Fail, phoneId, number, intervalSec, durationSec);
            } else if (retryMode_ != RetryMode::Fail) {
               // Switch running loop to Fail: stop & restart with Fail parameters
               stopRetryLoop();
               startUnifiedRetry(RetryMode::Fail, phoneId, number, intervalSec, durationSec);
            } else {
                // Already retrying -> push next attempt to (END + interval)
                scheduleNextRetryFromNow();
            }
        }

        if (mgr.isEmergencyMode(phoneId) && (!mgr.getAecsCallDropStatus() &&
            !mgr.getAecsCallFailStatus())) {
            mgr.stopAudioIfNoCalls(phoneId);

            // Stop any running retry loop
            stopRetryLoop();
            // Not a drop/failure termination → close any leftover drop window.
            dropWindowArmed_ = false;
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

        // Stop any running retry loop
        stopRetryLoop();

    } else {
        // nothing
    }
}

bool AecsCallListener::isAecsCallFailReason(telux::tel::CallEndCause endCause) {
    return ((endCause != telux::tel::CallEndCause::RADIO_OFF) &&
        /*
         * TODO: Add below once eCALL Logging Enhancement (SDK Enhancement) is enabled
         * (endCause != telux::tel::CallEndCause::CLIENT_END)
         */
        (endCause != telux::tel::CallEndCause::NORMAL) &&
        (endCause != telux::tel::CallEndCause::ERROR_UNSPECIFIED));
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

std::string AecsCallListener::getCurrentTime() {
   timeval tod;
   gettimeofday(&tod, NULL);
   std::stringstream ss;
   time_t tt = tod.tv_sec;
   char buffer[100];
   std::strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&tt));
   char currTime[120];
   snprintf(currTime, 120, "%s.%lld", buffer, (long long)tod.tv_usec / 1000);
   return std::string(currTime);
}

AecsCallCommandCallback::AecsCallCommandCallback(std::string commandName)
    : commandName_(commandName) {
}

void AecsCallCommandCallback::commandResponse(telux::common::ErrorCode error) {
    std::cout << std::endl << std::endl;
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << commandName_ << " operation successful" << std::endl;
    } else {
        PRINT_CB << commandName_ << " operation failed" << std::endl;
    }
    PRINT_CB << commandName_ << " operation - ErrorCode " << (int)error
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

void AecsDialCallback::makeCallResponse(telux::common::ErrorCode error,
    std::shared_ptr<telux::tel::ICall> call) {
    std::cout << std::endl << std::endl;
    std::cout << "makeAecsCall response ErrorCode: " << int(error)
            << ", description: " << Utils::getErrorCodeAsString(error);
    if (call) {
        std::cout << ", slot id: " << call->getPhoneId() << std::endl;
    } else {
        std::cout << ", call object is null" << std::endl;
    }
}

AecsCallListener::~AecsCallListener() {
    stopRetryLoop();
}

void AecsCallListener::stopRetryLoop() {
    // Signal workers to exit and wake them
    {
        std::lock_guard<std::mutex> lk(retryMutex_);
        retryStop_.store(true);
        retryScheduled_ = false;
    }
    retryCv_.notify_all();
    if (retryThread_.joinable()) retryThread_.join();
    retryInProgress_.store(false);
    retryMode_ = RetryMode::None;
}

void AecsCallListener::scheduleNextRetryFromNow() {
    // Schedule only if still within the retry window; the worker also re-checks.
    std::lock_guard<std::mutex> lk(retryMutex_);
    auto now = std::chrono::steady_clock::now();
    auto proposedRetry = now + std::chrono::seconds(retryIntervalSec_);

    // Only reschedule if the new time is sooner or no retry is scheduled
    if (!retryScheduled_ || proposedRetry < nextRetryAt_) {
        nextRetryAt_ = proposedRetry;
        retryScheduled_ = true;
        retryCv_.notify_all();
    }
}

void AecsCallListener::startUnifiedRetry(RetryMode mode, int phoneId, const std::string &number,
    int intervalSec, int durationSec) {
    auto now = std::chrono::steady_clock::now();

    // For AECS "Drop" retries, preserve the original window if it was already started.
    if (mode == RetryMode::Drop) {
        if (!dropWindowArmed_) {
            // First drop in this session: arm the window.
            dropWindowDeadline_ = now + std::chrono::seconds(durationSec);
            dropWindowArmed_ = true;
        } else {
            // Subsequent drops: use the remaining window.
            auto remaining = std::chrono::duration_cast<std::chrono::seconds>
                (dropWindowDeadline_ - now).count();
            if (remaining <= 0) {
                std::cout << "AECS drop retry window already expired. Skipping retries."
                    << std::endl;
                return;
            }
            durationSec = static_cast<int>(remaining);
        }
    } else {
        // Failure mode has its own window semantics; do not carry over the drop window.
        dropWindowArmed_ = false;
    }
    retryMode_ = mode;
    retryPhoneId_ = phoneId;
    retryNumber_ = number;
    retryIntervalSec_ = intervalSec;
    retryDurationSec_ = durationSec;
    retryStart_ = now;
    retryStop_.store(false);
    retryInProgress_.store(true);

    // schedule the FIRST retry from THIS call end moment
    {
       std::lock_guard<std::mutex> lk(retryMutex_);
       nextRetryAt_ = std::chrono::steady_clock::now() + std::chrono::seconds(retryIntervalSec_);
       retryScheduled_ = true;
    }

    std::cout << "Starting AECS retry (mode="
              << (mode == RetryMode::Drop ? "Drop" : "Fail")
              << ") interval=" << (retryIntervalSec_ / SEC_PER_MIN)
              << " min, duration=" << (retryDurationSec_ / SEC_PER_MIN) << " min..."
              << std::endl;

    retryThread_ = std::thread([this] {
        auto &mgr = AecsCallManager::getInstance();
        while (!retryStop_.load()) {
            // Check window expiry
            auto now = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(now - retryStart_).count();
            if (elapsed >= retryDurationSec_) {
                std::cout << "AECS " << (retryMode_ == RetryMode::Drop ? "drop" : "failure")
                          << " retry window expired. Stopping retries." << std::endl;
                if (retryMode_ == RetryMode::Drop) {
                    // Drop window is finished; disarm it.
                    dropWindowArmed_ = false;
                }
                break;
            }

            std::unique_lock<std::mutex> lk(retryMutex_);
            retryCv_.wait(lk, [this] {
            return retryStop_.load() || retryScheduled_;
        });
        if (retryStop_.load()) break;

        // Copy and clear the schedule
        auto due = nextRetryAt_;
        retryScheduled_ = false;

        // Wait until 'due' (or stop), then originate
        while (!retryStop_.load()) {
            auto now2 = std::chrono::steady_clock::now();
            if (now2 >= due) break;
            retryCv_.wait_for(lk, (due - now2), [this]{ return retryStop_.load(); });
            if (retryStop_.load()) break;
        }
        if (retryStop_.load()) break;
        lk.unlock();

        // Re-check window just before origination
        now = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - retryStart_).count();
        if (elapsed >= retryDurationSec_) {
            std::cout << "AECS " << (retryMode_ == RetryMode::Drop ? "drop" : "failure")
                  << " retry window expired. Stopping retries." << std::endl;
             break;
        }

        std::cout << std::endl << std::endl;
            PRINT_NOTIFICATION << " AECS information: - "
                << (retryMode_ == RetryMode::Drop
                     ? "voice connection is dropped, retry the voice connection"
                     : "voice connection failure, retrying the voice connection")
                << std::endl;

        telux::common::Status st =
            mgr.getCallManager()->makeAecsCall(retryPhoneId_, retryNumber_,
                    AecsDialCallback::makeCallResponse);
        std::cout << "AECS " << (retryMode_ == RetryMode::Drop ? "drop" : "failure")
                   << " retry requested, status = " << (int)st << std::endl;
        }
    });
}
