/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "EcallStateMachine.hpp"

namespace telux {
namespace tel {

Idle::Idle(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("CallConnect",
    EcallStateMachine::StateID::STATE_CALL_CONNECT, parent) {
    changeState(std::make_shared<CallConnect>(parent_));
}

bool Idle::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    return true;
}

CallConnect::CallConnect(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("CallConnect",
       EcallStateMachine::StateID::STATE_CALL_CONNECT, parent) {
}

bool CallConnect::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_ENDED", ecallStateMachine->getRemotePartyNumber());
        (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
    return true;
}

void CallConnect::onEnter() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
    "CALL_DIALING", ecallStateMachine->getRemotePartyNumber());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
    "CALL_ALERTING", ecallStateMachine->getRemotePartyNumber());
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    (ecallStateMachine->getCallservice())->startTimer("T2Timer");
    changeState(std::make_shared<DecodeSendMSD>(parent_));
}

void CallConnect::onExit() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->isNGeCall())) {
        if(ecallStateMachine->isMsdTransmitted() == true) {
            if(!(ecallStateMachine->parseVectortoString("T5FAILED"))) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->sendEvent("T5Timer", "start");
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->startTimer("T5Timer");
            }
        }
    }
}

ModemRedial::ModemRedial(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("ModemRedial",
       EcallStateMachine::StateID::STATE_MODEM_REDIAL, parent) {
}

bool ModemRedial::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    return true;
}
/*TODO: In future release*/
void ModemRedial::onEnter() {
}

void ModemRedial::onExit() {
}

DecodeSendMSD::DecodeSendMSD(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("DecodeSendMSD",
       EcallStateMachine::StateID::STATE_DECODE_SEND_MSD, parent) {
}

bool DecodeSendMSD::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
                    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(ecallStateMachine->parseVectortoString("T5FAILED")) {
        if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
            if(event->name_ == "T5Timer") {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->expiryTimer("T5Timer");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->msdTransmissionStatus("MSD_TRANSMISSION_FAILURE");
                changeState(std::make_shared<CallConversation>(parent_));
            }
        }
    }
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_ENDED", ecallStateMachine->getRemotePartyNumber());
        (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
    return true;
}

void DecodeSendMSD::onEnter() {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->isNGeCall())) {     //CS eCall
        if(ecallStateMachine->isMsdTransmitted() == true) {
            if(ecallStateMachine->eventId_ ==
                static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
                std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
                (ecallStateMachine->getCallservice())->msdTransmissionStatus("START_RECEIVED");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->msdTransmissionStatus(
                    "MSD_TRANSMISSION_STARTED");
                changeState(std::make_shared<CRCCheckonMSD>(parent_));
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->msdTransmissionStatus("MSD_TRANSMISSION_STARTED");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
                "CALL_ACTIVE", ecallStateMachine->getRemotePartyNumber());
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                (ecallStateMachine->getCallservice())->msdTransmissionStatus("START_RECEIVED");
                if(!(ecallStateMachine->parseVectortoString("T5FAILED"))) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    (ecallStateMachine->getCallservice())->sendEvent("T5Timer", "stop");
                    changeState(std::make_shared<CRCCheckonMSD>(parent_));
                }
            }
        } else {  //CS ecall
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
            "CALL_ACTIVE", ecallStateMachine->getRemotePartyNumber());
            changeState(std::make_shared<CallConversation>(parent_));
        }
    } else { //NG eCall
        if(ecallStateMachine->isMsdTransmitted() == true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->msdTransmissionStatus("MSD_TRANSMISSION_STARTED");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
            "CALL_ACTIVE", ecallStateMachine->getRemotePartyNumber());
            changeState(std::make_shared<DecodeMSD>(parent_));
        } else {
            changeState(std::make_shared<CallConversation>(parent_));
        }
    }
}

void DecodeSendMSD::onExit() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
                    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(ecallStateMachine->isMsdTransmitted() == true) { //CS ecall
        if(ecallStateMachine->parseVectortoString("T5FAILED")) {
            //Wait in DecodeSendMSD state till timer expiry event is not recieved
        }
    }
}

CRCCheckonMSD::CRCCheckonMSD(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("CRCCheckonMSD",
       EcallStateMachine::StateID::STATE_CRC_CHECK_ON_MSD, parent) {
}

bool CRCCheckonMSD::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
        if(event->name_ == "T7Timer") {
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            (ecallStateMachine->getCallservice())->expiryTimer("T7Timer");
            (ecallStateMachine->getCallservice())->msdTransmissionStatus("MSD_TRANSMISSION_FAILURE");
            changeState(std::make_shared<CallConversation>(parent_));
        }
    }
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_ENDED", ecallStateMachine->getRemotePartyNumber());
        (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
    return true;
}

void CRCCheckonMSD::onEnter() {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->parseVectortoString("T7FAILED"))) {
        if(ecallStateMachine->eventId_ ==
            static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
                std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
                (ecallStateMachine->getCallservice())->sendEvent("T7Timer", "start");
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                changeState(std::make_shared<DecodeMSD>(parent_));
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->sendEvent("T7Timer", "start");
            changeState(std::make_shared<DecodeMSD>(parent_));
        }
    } else {
        (ecallStateMachine->getCallservice())->startTimer("T7Timer");
    }

}

void CRCCheckonMSD::onExit() {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->parseVectortoString("T7FAILED"))) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        (ecallStateMachine->getCallservice())->sendEvent("T7Timer", "stop");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        (ecallStateMachine->getCallservice())->msdTransmissionStatus("LL_ACK_RECEIVED");
    }
}

DecodeMSD::DecodeMSD(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("DecodeMSD",
       EcallStateMachine::StateID::STATE_DECODE_MSD, parent) {
    LOG(DEBUG, __FUNCTION__);
}

bool DecodeMSD::onEvent(std::shared_ptr<telux::common::Event> event) {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
                    = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->isNGeCall())) {     //CS eCall
        LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
        if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
            if(event->name_ == "T6Timer") {
                (ecallStateMachine->getCallservice())->expiryTimer("T6Timer");
                (ecallStateMachine->getCallservice())->msdTransmissionStatus
                    ("MSD_TRANSMISSION_FAILURE");
                changeState(std::make_shared<CallConversation>(parent_));
            }
        }
    }
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
        || event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
        "CALL_ENDED", ecallStateMachine->getRemotePartyNumber());
        (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
        changeState(std::make_shared<PSAPCallback>(parent_));
    }
    return true;
}

void DecodeMSD::onEnter() {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
    if(!(ecallStateMachine->isNGeCall())) {     //CS eCall
        if(!(ecallStateMachine->parseVectortoString("T6FAILED"))) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->sendEvent("T6Timer", "start");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->msdTransmissionStatus("MSD_TRANSMISSION_SUCCESS");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->sendEvent("T6Timer", "stop");
            if(ecallStateMachine->eventId_ ==
                static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
                ecallStateMachine->updateInProgress_ = false;
            }
            changeState(std::make_shared<CallConversation>(parent_));
        } else {
            (ecallStateMachine->getCallservice())->startTimer("T6Timer");
        }
    } else { //NG eCall
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        (ecallStateMachine->getCallservice())->msdTransmissionStatus("OUTBAND_MSD_TRANSMISSION_SUCCESS");
        if(ecallStateMachine->eventId_ ==
            static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
            ecallStateMachine->updateInProgress_ = false;
        }
        changeState(std::make_shared<CallConversation>(parent_));
    }
}

void DecodeMSD::onExit() {
    LOG(DEBUG, __FUNCTION__);
}

PSAPCallback::PSAPCallback(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("PSAPCallback",
       EcallStateMachine::StateID::STATE_PSAP_CALLBACK, parent) {
}

bool PSAPCallback::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
        if(event->name_ == "T9Timer") {
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            (ecallStateMachine->getCallservice())->expiryTimer("T9Timer");
            ecallStateMachine->stop();
            /**TODO: Add T10 timer if eCallOperating = ECALL_ONLY*/
        }
    }
    return true;
}

void PSAPCallback::onEnter() {
    std::shared_ptr<EcallStateMachine> ecallStateMachine
        = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        (ecallStateMachine->getCallservice())->startTimer("T9Timer");
}

void PSAPCallback::onExit() {
    LOG(DEBUG, __FUNCTION__);
}

CallConversation::CallConversation(std::weak_ptr<BaseStateMachine> parent)
   : BaseState("CallConversation",
       EcallStateMachine::StateID::STATE_CALL_CONVERSATION, parent) {
}

bool CallConversation::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event ", event->name_, " while in ", name_);
    if(event->id_ == static_cast<int>(EcallStateMachine::EventID::ON_TIMER_EXPIRY)) {
        if(event->name_ == "T2Timer") {
            std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
            (ecallStateMachine->getCallservice())->expiryTimer("T2Timer");
            (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
            "CALL_ENDED", ecallStateMachine->getRemotePartyNumber());
            changeState(std::make_shared<PSAPCallback>(parent_));
        }
    } else if(event->id_ == static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER)
            || event->id_ ==
                static_cast<int>(EcallStateMachine::EventID::HANGUP_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
           (ecallStateMachine->getCallservice())->changeCallState(ecallStateMachine->getPhoneId(),
           "CALL_ENDED", ecallStateMachine->getRemotePartyNumber());
           (ecallStateMachine->getCallservice())->sendEvent("T2Timer", "stop");
           changeState(std::make_shared<PSAPCallback>(parent_));
    } else if(event->id_ ==
        static_cast<int>(EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP)) {
        std::shared_ptr<EcallStateMachine> ecallStateMachine
                = std::dynamic_pointer_cast<EcallStateMachine>(parent_.lock());
        ecallStateMachine->eventId_ =  event->id_;
        if(event->name_ == "CSeCall") {
            ecallStateMachine->updateInProgress_ = true;
            changeState(std::make_shared<DecodeSendMSD>(parent_));
        } else {
            ecallStateMachine->updateInProgress_ = true;
            (ecallStateMachine->getCallservice())->msdTransmissionStatus("MSD_TRANSMISSION_STARTED");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            (ecallStateMachine->getCallservice())->msdTransmissionStatus("OUTBAND_MSD_TRANSMISSION_SUCCESS");
            ecallStateMachine->updateInProgress_ = false;
        }
    }
    return true;
}

void CallConversation::onEnter() {
}

void CallConversation::onExit() {
    LOG(DEBUG, __FUNCTION__);
}


std::shared_ptr<CallManagerServerImpl> EcallStateMachine::getCallservice() const {
    return callservice_.lock();
}

EcallStateMachine::EcallStateMachine(std::shared_ptr<CallManagerServerImpl> callservice,
    std::vector<std::string> result, bool isMsdTransmitted, bool isNGeCall, int phoneId
    , std::string remotePartyNumber, bool updateInProgress)
   : BaseStateMachine("CallSubSystemStateMachine")
   , callservice_(callservice)
   , result_(result)
   , isMsdTransmitted_(isMsdTransmitted)
   , isNGeCall_(isNGeCall)
   , phoneId_(phoneId)
   , remotePartyNumber_(remotePartyNumber)
   , updateInProgress_(updateInProgress) {
}

bool EcallStateMachine::onEvent(std::shared_ptr<telux::common::Event> event) {
    LOG(DEBUG, "Received event: ", event->name_);
    currentState_->onEvent(event);
    return true;
}

bool EcallStateMachine::isMsdTransmitted() {
    return isMsdTransmitted_;
}

bool EcallStateMachine::isNGeCall() {
    return isNGeCall_;
}

int EcallStateMachine::getPhoneId() {
    return phoneId_;
}

bool EcallStateMachine::isEcallMSDUpdateInProgress() {
    return updateInProgress_;
}

std::string EcallStateMachine::getRemotePartyNumber() {
    return remotePartyNumber_;
}

void EcallStateMachine::start() {
    // Call the base class method to get it running
    LOG(DEBUG, __FUNCTION__);
    BaseStateMachine::start();
    //TODO: if(configration = modem redial) -> different state
    // Move to CallConnect
    changeState(std::make_shared<CallConnect>(shared_from_this()));
}

void EcallStateMachine::stop() {
    BaseStateMachine::stop();
}

bool EcallStateMachine::parseVectortoString(std::string compareTimer) {
    int size = result_.size();
    int i = 0;
    while(i < size) {
        if (compareTimer == result_[i]) {
            return true;
        }
        i++;
    }
    return false;
}

std::shared_ptr<telux::common::Event> EcallStateMachine::createTelEvent(
    EventID id, std::string timer) {
    return std::make_shared<TelEvent>(id, timer);
}

}  // namespace data
}  // namespace telux
