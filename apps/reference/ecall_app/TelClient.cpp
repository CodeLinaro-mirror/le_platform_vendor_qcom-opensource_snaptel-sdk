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
 * @file    TelClient.cpp
 *
 * @brief   TelClient class provides methods to trigger an eCall, update MSD, answer/hangup a call.
 *          It manages the telephony subsystem using Telematics-SDK APIs.
 */

#include <iostream>

#include <telux/tel/PhoneFactory.hpp>

#include "TelClient.hpp"
#include "TelClientUtils.hpp"
#include "Utils.hpp"

#define CLIENT_NAME "ECall-Tel-Client: "

TelClient::TelClient()
    : answerCommandCallback_(nullptr)
    , hangupCommandCallback_(nullptr)
    , updateMsdCommandCallback_(nullptr)
    , callMgr_(nullptr)
    , eCall_(nullptr)
    , eCallInprogress_(false) {
}

TelClient::~TelClient() {
    eCallInprogress_ = false;
}

// Initialize the telephony subsystem
telux::common::Status TelClient::init() {

    answerCommandCallback_ = std::make_shared<AnswerCommandCallback>();
    answerCommandCallback_->eCallTelClient_ = shared_from_this();
    hangupCommandCallback_ = std::make_shared<HangupCommandCallback>();
    updateMsdCommandCallback_ = std::make_shared<UpdateMsdCommandCallback>();

    // Get Phone Manager from PhoneFactory
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    auto phoneManager = phoneFactory.getPhoneManager();
    if(phoneManager) {
        auto defaultPhone = phoneManager->getPhone();

        //  Wait for the telephony subsystem to be ready
        bool isReady = phoneManager->isSubsystemReady();
        if(!isReady) {
            std::cout << CLIENT_NAME
                      << " Telephony subsystem is not ready, waiting for it to be ready.."
                     << std::endl;
            std::future<bool> f = phoneManager->onSubsystemReady();
            isReady = f.get();
            if(isReady) {
                std::cout << CLIENT_NAME << "Telephony subsystem is ready" << std::endl;
            } else {
                std::cout << CLIENT_NAME << "Unable to initialize Telephony subSystem" << std::endl;
                return telux::common::Status::FAILED;
            }
        } else {
            std::cout << CLIENT_NAME << "Telephony subsystem is ready" << std::endl;
        }
    } else {
       std::cout << CLIENT_NAME << " Phone Manager is NULL, failed to initialize subsystem"
                 << std::endl;
       return telux::common::Status::FAILED;
    }
    // Get Call Manager from PhoneFactory
    callMgr_ = phoneFactory.getCallManager();
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Failed to get Call Manager" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->registerListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << " Failed to register a Call listener" << std::endl;
    }

    return telux::common::Status::SUCCESS;
}

// Indicates whether an eCall is in progress
bool TelClient::isECallInProgress() {
    std::unique_lock<std::mutex> lock(mutex_);
    return eCallInprogress_;
}

void TelClient::setECallProgressState(bool state) {
    std::unique_lock<std::mutex> lock(mutex_);
    eCallInprogress_ = state;
}

// Callback invoked when an incoming call is received
void TelClient::onIncomingCall(std::shared_ptr<ICall> call) {
    std::cout << CLIENT_NAME << std::endl << "Received an incoming call" << std::endl;
    std::cout << CLIENT_NAME << "\n Incoming CallInfo: "
                      << " Call State: " << TelClientUtils::callStateToString(call->getCallState())
                      << "\n Call Index: " << (int)call->getCallIndex()
                      << ", Call Direction: " <<
                                    TelClientUtils::callDirectionToString(call->getCallDirection())
                      << ", Phone Number: " << call->getRemotePartyNumber() << std::endl;
}

// Callback invoked when a call status changes
void TelClient::onCallInfoChange(std::shared_ptr<ICall> call) {
    std::cout << CLIENT_NAME << "\n CallInfoChange: "
                      << " Call State: " << TelClientUtils::callStateToString(call->getCallState())
                      << "\n Call Index: " << (int)call->getCallIndex()
                      << ", Call Direction: " <<
                                    TelClientUtils::callDirectionToString(call->getCallDirection())
                      << ", Phone Number: " << call->getRemotePartyNumber() << std::endl;
    if(call->getCallState() == telux::tel::CallState::CALL_ENDED) {
        std::cout << CLIENT_NAME << "  Cause of call termination: "
                      << TelClientUtils::callEndCauseToString(call->getCallEndCause()) << std::endl;
        if(eCall_ != nullptr) {
            if(eCall_->getCallIndex() == call->getCallIndex()) {
                if(callListener_) {
                    callListener_->onCallDisconnect();
                    callListener_=nullptr;
                }
                setECallProgressState(false);
                eCall_=nullptr;
            }
        }
    }
}

// Callback to notify MSD transmission status
void TelClient::onECallMsdTransmissionStatus(int phoneId,
                                                   telux::common::ErrorCode errorCode) {
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "MSD is transmitted Successfully" << std::endl;
    } else {
        std::cout << CLIENT_NAME << "MSD transmission failed with error code: "
                << static_cast<int>(errorCode) << " : " << Utils::getErrorCodeAsString(errorCode)
                << std::endl;
    }
}

// Callback to notify MSD transmission status
void TelClient::onECallMsdTransmissionStatus(
    int phoneId, telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus) {
    std::cout << CLIENT_NAME << "ECallMsdTransmission  Status: "
                      << TelClientUtils::eCallMsdTransmissionStatusToString(msdTransmissionStatus)
                      << std::endl;
}

// Callback to notify eCall HLAP timers status
void TelClient::onECallHlapTimerEvent(int phoneId, ECallHlapTimerEvents timerEvents) {
    std::string infoStr = "\n";
    std::cout << CLIENT_NAME << " eCall HLAP Timer event on phoneId: " << phoneId << std::endl;
    if((timerEvents.t2 != HlapTimerEvent::UNCHANGED) &&
       (timerEvents.t2 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T2 HLAP Timer event : " +
                   TelClientUtils::eCallHlapTimerEventToString(timerEvents.t2) + "\n");
    }
    if((timerEvents.t5 != HlapTimerEvent::UNCHANGED) &&
       (timerEvents.t5 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T5 HLAP Timer event : " +
                   TelClientUtils::eCallHlapTimerEventToString(timerEvents.t5) + "\n");
    }
    if((timerEvents.t6 != HlapTimerEvent::UNCHANGED) &&
       (timerEvents.t6 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T6 HLAP Timer event : " +
                   TelClientUtils::eCallHlapTimerEventToString(timerEvents.t6) + "\n");
    }
    if((timerEvents.t7 != HlapTimerEvent::UNCHANGED) &&
       (timerEvents.t7 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T7 HLAP Timer event : " +
                   TelClientUtils::eCallHlapTimerEventToString(timerEvents.t7) + "\n");
    }
    if((timerEvents.t9 != HlapTimerEvent::UNCHANGED) &&
       (timerEvents.t9 != HlapTimerEvent::UNKNOWN)) {
        infoStr.append("T9 HLAP Timer event : " +
                   TelClientUtils::eCallHlapTimerEventToString(timerEvents.t9) + "\n");
    }
    std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Callback which provides response to makeECall
void TelClient::makeCallResponse(telux::common::ErrorCode errorCode,
                                      std::shared_ptr<telux::tel::ICall> call) {
    std::string infoStr = "";
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        infoStr.append("Call is successful,call index - " + static_cast<int>(call->getCallIndex()));
        eCall_ = call;
    } else {
        infoStr.append("Call failed with error code: " + std::to_string(static_cast<int>(errorCode))
                     + ":" + Utils::getErrorCodeAsString(errorCode));
        if(callListener_) {
            callListener_->onCallDisconnect();
            callListener_=nullptr;
        }
        setECallProgressState(false);
   }
   std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Callback which provides response to updateECallMsd command
void TelClient::UpdateMsdCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
    if(errorCode != telux::common::ErrorCode::SUCCESS) {
        std::string infoStr = "";
        infoStr.append("Update MSD failed with error code: "
                     + std::to_string(static_cast<int>(errorCode)) + ":"
                     + Utils::getErrorCodeAsString(errorCode));
        std::cout << CLIENT_NAME << infoStr << std::endl;
    }
}

// Callback which provides response to answer command
void TelClient::AnswerCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
    std::string infoStr = "";
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        infoStr.append(" Answer Call is successful");
    } else {
        infoStr.append(" Answer call failed with error code: "
                + std::to_string(static_cast<int>(errorCode)) + ":"
                + Utils::getErrorCodeAsString(errorCode));
        if(eCallTelClient_) {
            if(eCallTelClient_->callListener_) {
                eCallTelClient_->callListener_->onCallDisconnect();
                eCallTelClient_->callListener_=nullptr;
            }
            eCallTelClient_->setECallProgressState(false);
            eCallTelClient_->eCall_=nullptr;
        }
    }
    std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Callback which provides response to hangup command
void TelClient::HangupCommandCallback::commandResponse(telux::common::ErrorCode errorCode) {
    std::string infoStr = "";
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        infoStr.append(" Hangup is successful");
    } else {
        infoStr.append(" Hangup failed with error code: "
                    + std::to_string(static_cast<int>(errorCode)) + ":"
                    + Utils::getErrorCodeAsString(errorCode));
    }
    std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Callback which provides response to HLAP timer status request
void TelClient::hlapTimerStatusResponse(telux::common::ErrorCode error, int phoneId,
    ECallHlapTimerStatus timersStatus) {
    if(error != telux::common::ErrorCode::SUCCESS) {
        std::cout << CLIENT_NAME << "Get HLAP timers status failed with error code: "
            << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    }
    std::string infoStr = "eCall HLAP Timers status on phoneId - " +
                           std::to_string(static_cast<int>(phoneId)) + "\n";
    infoStr.append("T2 HLAP Timer Status : " +
                   TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t2) + "\n");
    infoStr.append("T5 HLAP Timer Status : " +
                   TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t5) + "\n");
    infoStr.append("T6 HLAP Timer Status : " +
                   TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t6) + "\n");
    infoStr.append("T7 HLAP Timer Status : " +
                   TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t7) + "\n");
    infoStr.append("T9 HLAP Timer Status : " +
                   TelClientUtils::eCallHlapTimerStatusToString(timersStatus.t9) + "\n");
    std::cout << CLIENT_NAME << infoStr << std::endl;
}

// Initiate a standard eCall procedure(eg.112)
telux::common::Status TelClient::startECall(int phoneId, ECallMsdData msdData,
                                ECallCategory category, ECallVariant variant, bool transmitMsd,
                                std::shared_ptr<CallStatusListener> callListener) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to initiate an eCall"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    setECallProgressState(true);
    // Initiate an eCall
    telux::common::Status status = telux::common::Status::FAILED;
    if(transmitMsd) {
        status = callMgr_->makeECall(phoneId, msdData, (int)category, (int)variant,
                                     shared_from_this());
    } else {
        status = callMgr_->makeECall(phoneId, (int)category, (int)variant,
                                     std::bind(&TelClient::makeCallResponse, this,
                                     std::placeholders::_1, std::placeholders::_2));
    }
    if(status == telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Request to make an ECall is sent successfully" << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Request to make an ECall failed!" << std::endl;
        setECallProgressState(false);
        return telux::common::Status::FAILED;
    }
    callListener_ = callListener;
    return telux::common::Status::SUCCESS;
}

// Initiate a voice eCall procedure to the specified phone number
telux::common::Status TelClient::startECall(int phoneId, ECallMsdData msdData,
                                ECallCategory category, const std::string dialNumber,
                                bool transmitMsd, std::shared_ptr<CallStatusListener> callListener){
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to initiate an eCall"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    setECallProgressState(true);
    // Initiate voice eCall
    telux::common::Status status = telux::common::Status::FAILED;
    if(transmitMsd) {
        status = callMgr_->makeECall(phoneId, dialNumber, msdData, (int)category,
                                            shared_from_this());
    } else {
        status = callMgr_->makeECall(phoneId, dialNumber, (int)category,
                                     std::bind(&TelClient::makeCallResponse, this,
                                     std::placeholders::_1, std::placeholders::_2));
    }
    if(status == telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Request to make a Voice ECall is sent successfully"
            << std::endl;
    } else {
        std::cout << CLIENT_NAME << "Request to make a Voice ECall failed!" << std::endl;
        setECallProgressState(false);
        return telux::common::Status::FAILED;
    }
    callListener_ = callListener;
    return telux::common::Status::SUCCESS;
}

// Update the MSD data
telux::common::Status TelClient::updateECallMSD(int phoneId, ECallMsdData msdData) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to send MSD update request"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->updateECallMsd(phoneId, msdData, updateMsdCommandCallback_);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send MSD update request!" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Answer an incoming call
telux::common::Status TelClient::answer(int phoneId, std::shared_ptr<CallStatusListener> callListener) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed answer call" << std::endl;
        return telux::common::Status::FAILED;
    }
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICall>> callList = callMgr_->getInProgressCalls();
    // Fetch the list of in progress calls from CallManager and accept the incoming call.
    for(auto callIterator = std::begin(callList); callIterator != std::end(callList)
                        ; ++callIterator) {
        if(((*callIterator)->getCallState() == telux::tel::CallState::CALL_INCOMING) &&
                            (phoneId == (*callIterator)->getPhoneId())) {
            spCall = *callIterator;
            break;
        }
    }
    if(spCall) {
        eCall_ = spCall;
        setECallProgressState(true);
        telux::common::Status status = spCall->answer(answerCommandCallback_);
        if(status != telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Failed to accept call " << std::endl;
            setECallProgressState(false);
            eCall_=nullptr;
            return telux::common::Status::FAILED;
        }
    } else {
        std::cout << CLIENT_NAME << "No incoming call found to accept " << std::endl;
        return telux::common::Status::FAILED;
    }
    callListener_ = callListener;
    return telux::common::Status::SUCCESS;
}

// Hangup an ongoing call
telux::common::Status TelClient::hangup(int phoneId, int callIndex) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to hangup call" << std::endl;
        return telux::common::Status::FAILED;
    }
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    // If callIndex is not provided, iterate through the call list in the application and hangup if
    // only one call is Active or on Hold. If callIndex is provided, hangup the corresponding call.
    std::vector<std::shared_ptr<telux::tel::ICall>> callList = callMgr_->getInProgressCalls();
    int numOfCalls = 0;
    for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
                    ++callIterator) {
        if(phoneId == (*callIterator)->getPhoneId()) {
            telux::tel::CallState callState = (*callIterator)->getCallState();
            if((callState != telux::tel::CallState::CALL_ENDED) &&
               ((callIndex == -1) || (callIndex == (*callIterator)->getCallIndex()))) {
                spCall = *callIterator;
                numOfCalls++;
                break;
            }
        }
    }
    if(spCall && (numOfCalls == 1)) {
        telux::common::Status status = spCall->hangup(hangupCommandCallback_);
        if(status != telux::common::Status::SUCCESS) {
            std::cout << CLIENT_NAME << "Failed to hangup call " << std::endl;
            return telux::common::Status::FAILED;
        }
    } else {
        std::cout << CLIENT_NAME << "No relevant call found to hangup" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

// Dump the list of current calls
telux::common::Status TelClient::getCurrentCalls() {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to get current calls" << std::endl;
        return telux::common::Status::FAILED;
    }
    std::vector<std::shared_ptr<telux::tel::ICall>> calls = callMgr_->getInProgressCalls();
    for(auto callIterator = std::begin(calls); callIterator != std::end(calls); ++callIterator) {
        std::cout << " Call Index: " << static_cast<int>((*callIterator)->getCallIndex())
                  << ", Phone ID: " << static_cast<int>((*callIterator)->getPhoneId())
                  << ", Call State: "
                      << TelClientUtils::callStateToString((*callIterator)->getCallState())
                  << ", Call Direction: "
                      << TelClientUtils::callDirectionToString((*callIterator)->getCallDirection())
                  << ", Phone Number: " << (*callIterator)->getRemotePartyNumber() << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

// Get eCall HLAP timers status
telux::common::Status TelClient::requestECallHlapTimerStatus(int phoneId) {
    if(!callMgr_) {
        std::cout << CLIENT_NAME << "Invalid Call Manager, Failed to request for HLAP timers status"
            << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = callMgr_->requestECallHlapTimerStatus(phoneId,
                            std::bind(&TelClient::hlapTimerStatusResponse, this,
                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request for HLAP timers status" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}
