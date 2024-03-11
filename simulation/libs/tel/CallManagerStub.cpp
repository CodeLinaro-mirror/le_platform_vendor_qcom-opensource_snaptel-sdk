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

#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/ECallDefines.hpp>
#include "CallManagerStub.hpp"

using namespace telux::common;
using namespace telux::tel;
using namespace std;

CallManagerStub::CallManagerStub(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<DialerService>();
    noOfSlots_ = 1;                            // Defaulting to 1 for Single SIM.
    if (telux::common::DeviceConfig::isMultiSimSupported()) {  // For DSDA slot count is 2.
        noOfSlots_ = 2;
    }
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

void CallManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    if (reqstatus.ok()) {
        telux::common::ServiceStatus cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        int cbDelay = static_cast<int>(response.delay());
        if(cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            listenerMgr_ = std::make_shared<telux::common::ListenerManager<ICallListener>>();
            if(!listenerMgr_) {
                LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
                cbStatus = telux::common::ServiceStatus::SERVICE_FAILED;
            }
        }
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, cbStatus, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(cbStatus);
            }).share();
            taskQ_->add(f);
        }
    }
}

CallManagerStub::~CallManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    calls_.clear();
    cleanup();
}

void CallManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus CallManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    grpc::Status status = stub_->GetServiceStatus(&context, request, &response);
    if (!status.ok()) {
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }
    telux::common::ServiceStatus serviceStatus =
    static_cast<telux::common::ServiceStatus>(response.service_status());
    return serviceStatus;
}

telux::common::Status CallManagerStub::registerListener(std::shared_ptr<ICallListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_CALL_FILTER};
        std::vector<std::weak_ptr<ICallListener>> applisteners;
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 1) {
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::removeListener(std::shared_ptr<ICallListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ICallListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_CALL_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeCall(int phoneId, const std::string &dialNumber,
    std::shared_ptr<IMakeCallCallback> callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeCallRequest request =
        createRequest<::telStub::MakeCallRequest>(phoneId, dialNumber, false, makeVoiceCall);
    ::telStub::MakeCallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeCall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());

        CallInfo callInfo;
        callInfo.remotePartyNumber = static_cast<std::string>(response.call().remote_party_number());
        callInfo.index = static_cast<int>(response.call().call_index());
        callInfo.callDirection = CallDirection::OUTGOING;
        callInfo.callState = telux::tel::CallState::CALL_IDLE;
        auto info = std::make_shared<CallStub>(phoneId, callInfo);
        logCallDetails(info);
        if (status == telux::common::Status::SUCCESS ) {
            if(callback) {
                auto f = std::async(std::launch::async,
                [this, error, info, callback, cbDelay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback->makeCallResponse(error, info);
                }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    const ECallMsdData &eCallMsdData, int category, std::shared_ptr<IMakeCallCallback> callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, dialNumber, true,
        makeTpsECallOverCSWithMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());

    CallInfo callInfo;
    callInfo.remotePartyNumber = static_cast<std::string>(response.call().remote_party_number());
    callInfo.index = static_cast<int>(response.call().call_index());
    callInfo.callDirection = CallDirection::OUTGOING;
    callInfo.callState = telux::tel::CallState::CALL_IDLE;
    callInfo.transmitMsd = true;
    auto info = std::make_shared<CallStub>(phoneId, callInfo);
    logCallDetails(info);
    if (status == telux::common::Status::SUCCESS ) {
       if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, info, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback->makeCallResponse(error, info);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const ECallMsdData &eCallMsdData,
    int category, int variant, std::shared_ptr<IMakeCallCallback> callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, "", true,
        makeECallWithMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());

        CallInfo callInfo;
        callInfo.remotePartyNumber = static_cast<std::string>(response.call().remote_party_number());
        callInfo.index = static_cast<int>(response.call().call_index());
        callInfo.callDirection = CallDirection::OUTGOING;
        callInfo.callState = telux::tel::CallState::CALL_IDLE;
        callInfo.transmitMsd = true;
        auto info = std::make_shared<CallStub>(phoneId, callInfo);
        logCallDetails(info);
        if (status == telux::common::Status::SUCCESS ) {
        if(callback) {
                auto f = std::async(std::launch::async,
                [this, error, info, callback, cbDelay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback->makeCallResponse(error, info);
                }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

void CallManagerStub::logCallDetails(std::shared_ptr<ICall> info) {
    LOG(DEBUG, __FUNCTION__,
        " Call Info: remotePartyNumber = ", info->getRemotePartyNumber(),
        ", callIndex = ", info->getCallIndex(),
        ", callDirection = ", static_cast<int>(info->getCallDirection()),
        ", callState = ", static_cast<int>(info->getCallState()));
}

void CallManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::ECallInfoEvent>()) {
        ::telStub::ECallInfoEvent callevent;
        event.UnpackTo(&callevent);
        handleEcallEvent(callevent);
    } else if(event.Is<::telStub::MsdPullRequestEvent>()) {
        ::telStub::MsdPullRequestEvent callevent;
        event.UnpackTo(&callevent);
        handleMsdUpdateRequest(callevent);
    } else if(event.Is<::telStub::CallStateChangeEvent>()) {
        ::telStub::CallStateChangeEvent callevent;
        event.UnpackTo(&callevent);
        handleCallInfoChanged(callevent);
    } else if(event.Is<::telStub::Call>()) {
        ::telStub::Call callevent;
        event.UnpackTo(&callevent);
        handleIncomingCall(callevent);
    } else {
        LOG(DEBUG, __FUNCTION__, "No handling required for other events");
    }
}

void CallManagerStub::handleIncomingCall(::telStub::Call event) {
    CallInfo callInfo;
    int phoneId = event.phone_id();
    callInfo.callState = static_cast<telux::tel::CallState>(event.call_state());
    callInfo.index = event.call_index();
    callInfo.callDirection = static_cast<telux::tel::CallDirection>(event.call_direction());
    callInfo.remotePartyNumber = event.remote_party_number();
    callInfo.isMultiPartyCall = event.is_multi_party_call();
    callInfo.isMpty = event.is_mpty();
    auto call = std::make_shared<CallStub>(phoneId, callInfo);
    logCallDetails(call);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onIncomingCall(call);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::handleMsdUpdateRequest(::telStub::MsdPullRequestEvent event) {
    int phoneId = event.phone_id();
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->OnMsdUpdateRequest(phoneId);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::handleCallInfoChanged(::telStub::CallStateChangeEvent event) {
    int callIndex = event.call_index();
    std::string remotePartyNumber = event.remote_party_number();
    std::string action = event.callstate();
    int phoneId = event.phone_id();

    LOG(DEBUG, __FUNCTION__, "The fetched callIndex is: ", callIndex);

    telux::tel::CallState callState = Helper::getCallState(action);

    LOG(DEBUG, __FUNCTION__, "The fetched callState is: ", action);

    getInProgressCalls();
    CallInfo callInfo;
    callInfo.remotePartyNumber = remotePartyNumber;
    callInfo.index = callIndex;
    callInfo.callDirection = CallDirection::OUTGOING;
    callInfo.callState = callState;
    auto info = std::make_shared<CallStub>(phoneId, callInfo);
    logCallDetails(info);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onCallInfoChange(info);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::handleEcallEvent(::telStub::ECallInfoEvent event) {

    int slotId = event.phone_id();
    HlapTimerEvent action = static_cast<HlapTimerEvent>(event.action());
    std::string input = event.timer();

    ECallHlapTimerEvents timersStatus;
    timersStatus.t2 = HlapTimerEvent::UNCHANGED;
    timersStatus.t5 = HlapTimerEvent::UNCHANGED;
    timersStatus.t6 = HlapTimerEvent::UNCHANGED;
    timersStatus.t7 = HlapTimerEvent::UNCHANGED;
    timersStatus.t9 = HlapTimerEvent::UNCHANGED;
    timersStatus.t10 = HlapTimerEvent::UNCHANGED;
    if(input == "T2Timer") {
        timersStatus.t2 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T5Timer") {
        timersStatus.t5 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T6Timer") {
        timersStatus.t6 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T7Timer") {
        timersStatus.t7 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T9Timer") {
        timersStatus.t9 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "T10Timer") {
        timersStatus.t10 = action;
        invokeECallHlapTimerEventlisteners(slotId, timersStatus);
    } else if(input == "START_RECEIVED") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::START_RECEIVED);
    } else if(input == "MSD_TRANSMISSION_STARTED") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::MSD_TRANSMISSION_STARTED);
    } else if(input == "LL_ACK_RECEIVED") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::LL_ACK_RECEIVED);
    } else if(input == "MSD_TRANSMISSION_SUCCESS") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::SUCCESS);
        invokeECallMsdTransmissionStatuslisteners(slotId, telux::common::ErrorCode::SUCCESS);
    } else if(input == "MSD_TRANSMISSION_FAILURE") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::FAILURE);
        invokeECallMsdTransmissionStatuslisteners(slotId,
            telux::common::ErrorCode::GENERIC_FAILURE);
    } else if(input == "OUTBAND_MSD_TRANSMISSION_STARTED") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_STARTED);
    } else if(input == "OUTBAND_MSD_TRANSMISSION_SUCCESS") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_SUCCESS);
        invokeECallMsdTransmissionStatuslisteners(slotId, telux::common::ErrorCode::SUCCESS);
    } else if(input == "OUTBAND_MSD_TRANSMISSION_FAILURE") {
        invokeECallMsdTransmissionStatuslisteners(slotId,
            ECallMsdTransmissionStatus::OUTBAND_MSD_TRANSMISSION_FAILURE);
        invokeECallMsdTransmissionStatuslisteners(slotId,
            telux::common::ErrorCode::GENERIC_FAILURE);
    }
    else {
        LOG(ERROR, __FUNCTION__, "No supported event ");
    }
}

void CallManagerStub::invokeECallMsdTransmissionStatuslisteners(int phoneId,
    telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus ) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onECallMsdTransmissionStatus(phoneId, msdTransmissionStatus);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::invokeECallMsdTransmissionStatuslisteners(int phoneId,
    telux::common::ErrorCode errorCode ) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onECallMsdTransmissionStatus(phoneId, errorCode);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void CallManagerStub::invokeECallHlapTimerEventlisteners(int phoneId,
    ECallHlapTimerEvents timersStatus) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICallListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onECallHlapTimerEvent(phoneId, timersStatus);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    const std::vector<uint8_t> &msdPdu, CustomSipHeader header, MakeCallCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, dialNumber, true,
        makeTpsECallOverIMS);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());

    CallInfo callInfo;
    callInfo.remotePartyNumber = static_cast<std::string>(response.call().remote_party_number());
    callInfo.index = static_cast<int>(response.call().call_index());
    callInfo.callDirection = CallDirection::OUTGOING;
    callInfo.callState = telux::tel::CallState::CALL_IDLE;
    callInfo.transmitMsd = true;
    auto info = std::make_shared<CallStub>(phoneId, callInfo);
    logCallDetails(info);
    if (status == telux::common::Status::SUCCESS ) {
       if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, info, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error, info);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::vector<uint8_t> &msdPdu,
    int category, int variant, MakeCallCallback callback) {
	LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, "", true,
        makeECallWithRawMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());

        CallInfo callInfo;
        callInfo.remotePartyNumber = static_cast<std::string>(response.call().remote_party_number());
        callInfo.index = static_cast<int>(response.call().call_index());
        callInfo.callDirection = CallDirection::OUTGOING;
        callInfo.callState = telux::tel::CallState::CALL_IDLE;
        callInfo.transmitMsd = true;
        auto info = std::make_shared<CallStub>(phoneId, callInfo);
        logCallDetails(info);
        if (status == telux::common::Status::SUCCESS ) {
        if(callback) {
                auto f = std::async(std::launch::async,
                [this, error, info, callback, cbDelay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback(error, info);
                }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    const std::vector<uint8_t> &msdPdu, int category, MakeCallCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, dialNumber, true,
        makeTpsECallOverCSWithRawMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());

    CallInfo callInfo;
    callInfo.remotePartyNumber = static_cast<std::string>(response.call().remote_party_number());
    callInfo.index = static_cast<int>(response.call().call_index());
    callInfo.callDirection = CallDirection::OUTGOING;
    callInfo.callState = telux::tel::CallState::CALL_IDLE;
    callInfo.transmitMsd = true;
    auto info = std::make_shared<CallStub>(phoneId, callInfo);
    logCallDetails(info);
    if (status == telux::common::Status::SUCCESS ) {
       if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, info, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error, info);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, int category, int variant,
    MakeCallCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeECallRequest request =
        createRequest<::telStub::MakeECallRequest>(phoneId, "", false,
        makeECallWithoutMsd);
    ::telStub::MakeECallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeECall(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());

        CallInfo callInfo;
        callInfo.remotePartyNumber = static_cast<std::string>(response.call().remote_party_number());
        callInfo.index = static_cast<int>(response.call().call_index());
        callInfo.callDirection = CallDirection::OUTGOING;
        callInfo.callState = telux::tel::CallState::CALL_IDLE;
        callInfo.transmitMsd = false;
        auto info = std::make_shared<CallStub>(phoneId, callInfo);
        logCallDetails(info);
        if (status == telux::common::Status::SUCCESS ) {
            if(callback) {
                auto f = std::async(std::launch::async,
                [this, error, info, callback, cbDelay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback(error, info);
                }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::makeECall(int phoneId, const std::string dialNumber,
    int category, MakeCallCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::MakeCallRequest request =
        createRequest<::telStub::MakeCallRequest>(phoneId, dialNumber, false,
        makeTpsECallOverCSWithoutMsd);
    ::telStub::MakeCallReply response;
    ClientContext context;

    grpc::Status reqstatus = stub_->MakeCall(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int cbDelay = static_cast<int>(response.delay());

    CallInfo callInfo;
    callInfo.remotePartyNumber = static_cast<std::string>(response.call().remote_party_number());
    callInfo.index = static_cast<int>(response.call().call_index());
    callInfo.callDirection = CallDirection::OUTGOING;
    callInfo.callState = telux::tel::CallState::CALL_IDLE;
    auto info = std::make_shared<CallStub>(phoneId, callInfo);
    logCallDetails(info);
    if (status == telux::common::Status::SUCCESS ) {
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, info, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error, info);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::updateECallMsd(int phoneId, const ECallMsdData &eCallMsd,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {

    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::UpdateECallMsdRequest request;
    ::telStub::UpdateECallMsdResponse response;
    ClientContext context;

    request.set_phone_id(phoneId);
    request.set_api(updateEcallMsd);

    grpc::Status reqstatus = stub_->UpdateECallMsd(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback->commandResponse(error);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::updateECallMsd(int phoneId,
    const std::vector<uint8_t> &msdPdu, telux::common::ResponseCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::UpdateECallMsdRequest request;
    ::telStub::UpdateECallMsdResponse response;
    ClientContext context;

    request.set_phone_id(phoneId);
    request.set_api(updateECallRawMsd);

    grpc::Status reqstatus = stub_->UpdateECallMsd(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::requestECallHlapTimerStatus(int phoneId,
    ECallHlapTimerStatusCallback callback) {
    LOG(DEBUG, "CallManager - ", __FUNCTION__);

    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::RequestECallHlapTimerStatusRequest request;
    ::telStub::RequestECallHlapTimerStatusReply response;
    ClientContext context;

    request.set_phone_id(phoneId);

    grpc::Status reqstatus = stub_->RequestECallHlapTimerStatus(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        ECallHlapTimerStatus timersStatus;
        timersStatus.t2 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t2());
        timersStatus.t5 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t5());
        timersStatus.t6 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t6());
        timersStatus.t7 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t7());
        timersStatus.t9 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t9());
        timersStatus.t10 = static_cast<telux::tel::HlapTimerStatus>((
            response.hlap_timer_status()).t10());
        int cbDelay = static_cast<int>(response.delay());
        if(callback) {
            auto f = std::async(std::launch::async,
            [this, error, phoneId, timersStatus, callback, cbDelay]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error, phoneId, timersStatus);
            }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

std::vector<std::shared_ptr<ICall>> CallManagerStub::getInProgressCalls() {
    LOG(DEBUG, "CallMgr - ", __FUNCTION__);

    ::telStub::GetInProgressCallsReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    grpc::Status reqstatus = stub_->GetInProgressCalls(&context, request, &response);
    if (reqstatus.ok()) {
        std::lock_guard<std::mutex> lock(callManagerMutex_);
        calls_.clear();
        for (int i = 0; i < response.calls_size(); i++) {
            CallInfo callInfo;
            callInfo.callState = static_cast<telux::tel::CallState>(
                response.calls(i).call_state());
            callInfo.index  =  static_cast<int>(response.calls(i).call_index());
            callInfo.callDirection  =  static_cast<telux::tel::CallDirection>(
                response.calls(i).call_direction());
            callInfo.remotePartyNumber = static_cast<std::string>(
                response.calls(i).remote_party_number());
            callInfo.callEndCause = static_cast<telux::tel::CallEndCause>(
                response.calls(i).call_end_cause());
            int phoneId = response.calls(i).phone_id();
            callInfo.isMultiPartyCall = response.calls(i).is_multi_party_call();
            callInfo.isMpty = response.calls(i).is_mpty();
            auto Info = std::make_shared<CallStub>(phoneId, callInfo);
            calls_.emplace_back(Info);
        }
    }
    //Update local cache
    std::vector<std::shared_ptr<ICall>> iCalls(calls_.begin(), calls_.end());
    return iCalls;
}

telux::common::Status CallManagerStub::conference(std::shared_ptr<ICall> call1,
    std::shared_ptr<ICall> call2,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status CallManagerStub::swap(std::shared_ptr<ICall> callToHold,
    std::shared_ptr<ICall> callToActivate,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "CallMgr - ", __FUNCTION__);
    if ((callToHold == nullptr) || (callToActivate == nullptr)) {
        LOG(ERROR, "Unable to initiate swap operation as null call objects are passed");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    if (callToHold->getPhoneId() != callToActivate->getPhoneId()) {
        LOG(ERROR, "Unable to initiate swap operation as phoneId for both calls are different");
        return telux::common::Status::INVALIDPARAM;
    }
    ::telStub::SwapRequest request;
    ::telStub::SwapReply response;
    ClientContext context;

    if ((callToHold->getCallState() == CallState::CALL_ON_HOLD)
         && (callToActivate->getCallState() == CallState::CALL_ACTIVE)) {
        request.set_call_to_hold_index(callToHold->getCallIndex());
        request.set_phone_id(callToHold->getPhoneId());
        request.set_call_to_activate_index(callToActivate->getCallIndex());
    }
    else if ((callToHold->getCallState() == CallState::CALL_ACTIVE)
        && (callToActivate->getCallState() == CallState::CALL_ON_HOLD)) {
        request.set_call_to_hold_index(callToActivate->getCallIndex());
        request.set_phone_id(callToActivate->getPhoneId());
        request.set_call_to_activate_index(callToHold->getCallIndex());
    } else {
        LOG(ERROR, "Unable to initiate swap calls due to calls in wrong state");
        return telux::common::Status::INVALIDSTATE;
    }

    grpc::Status reqstatus = stub_->Swap(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());

        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            if(callback) {
                auto f = std::async(std::launch::async,
                    [this, error, callback, delay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback->commandResponse(error);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::hangupForegroundResumeBackground(int phoneId,
    common::ResponseCallback callback ) {
    LOG(DEBUG, __FUNCTION__, " SlotId: ", phoneId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::HangupForegroundResumeBackgroundRequest request;
    ::telStub::HangupForegroundResumeBackgroundReply response;
    ClientContext context;
    request.set_phone_id(phoneId);
    grpc::Status reqstatus = stub_->HangupForegroundResumeBackground(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            if(callback) {
                auto f = std::async(std::launch::async,
                    [this, error, callback, delay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback(error);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::hangupWaitingOrBackground(int phoneId,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " SlotId: ", phoneId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::HangupWaitingOrBackgroundRequest request;
    ::telStub::HangupWaitingOrBackgroundReply response;
    ClientContext context;
    request.set_phone_id(phoneId);
    grpc::Status reqstatus = stub_->HangupWaitingOrBackground(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        if(callback) {
                auto f = std::async(std::launch::async,
                    [this, error, callback, delay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                        callback(error);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::requestEcbm(int phoneId, EcbmStatusCallback callback) {
   LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::RequestEcbmRequest request;
    ::telStub::RequestEcbmReply response;
    ClientContext context;
    request.set_phone_id(phoneId);

    grpc::Status reqstatus = stub_->RequestEcbm(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        telux::tel::EcbMode ecbMode = static_cast<telux::tel::EcbMode>(response.ecbmode());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
            if(callback) {
                auto f = std::async(std::launch::async,
                    [this, error , callback, ecbMode, cbDelay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                        callback(ecbMode, error);
                    }).share();
                taskQ_->add(f);
            }
        }
    }
    return status;
}

telux::common::Status CallManagerStub::exitEcbm(int phoneId, common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::ExitEcbmRequest request;
    ::telStub::ExitEcbmReply response;
    ClientContext context;
    request.set_phone_id(phoneId);
    grpc::Status reqstatus = stub_->ExitEcbm(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
            auto f = std::async(std::launch::async,
                [this, error, cbDelay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback(error);
                }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::requestNetworkDeregistration(int phoneId,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }

    ::telStub::RequestNetworkDeregistrationRequest request;
    ::telStub::RequestNetworkDeregistrationReply response;
    ClientContext context;
    request.set_phone_id(phoneId);
    grpc::Status reqstatus = stub_->RequestNetworkDeregistration(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        int cbDelay = static_cast<int>(response.delay());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
            auto f = std::async(std::launch::async,
                [this, error, cbDelay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback(error);
                }).share();
            taskQ_->add(f);
        }
    }
    return status;
}

telux::common::Status CallManagerStub::updateEcallHlapTimer(int phoneId, HlapTimerType type,
    uint32_t timeDuration, common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if(type == HlapTimerType::T10_TIMER) {
        ::telStub::UpdateEcallHlapTimerRequest request;
        ::telStub::UpdateEcallHlapTimerResponse response;
        ClientContext context;
        request.set_phone_id(phoneId);
        request.set_type(static_cast<::telStub::HlapTimerType>(type));
        request.set_time_duration(timeDuration);
        grpc::Status reqstatus = stub_->UpdateEcallHlapTimer(&context, request, &response);
        if (reqstatus.ok()) {
            telux::common::ErrorCode error =
                static_cast<telux::common::ErrorCode>(response.error());
            status = static_cast<telux::common::Status>(response.status());
            int cbDelay = static_cast<int>(response.delay());
            bool isCallbackNeeded = static_cast<bool>(response.iscallback());

            if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
                auto f = std::async(std::launch::async,
                    [this, error , callback, cbDelay]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                        callback(error);
                    }).share();
                taskQ_->add(f);
            }
        }
    } else {
        status = telux::common::Status::NOTSUPPORTED;
    }
    return status;
}

telux::common::Status CallManagerStub::requestEcallHlapTimer(int phoneId, HlapTimerType type,
    ECallHlapTimerCallback callback) {
    LOG(DEBUG, __FUNCTION__, " phoneId:", phoneId);
    if(phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(ERROR, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;

    if(type == HlapTimerType::T10_TIMER) {
        ::telStub::RequestEcallHlapTimerRequest request;
        ::telStub::RequestEcallHlapTimerReply response;
        ClientContext context;

        request.set_phone_id(phoneId);
        request.set_type(static_cast<::telStub::HlapTimerType>(type));
        grpc::Status reqstatus = stub_->RequestEcallHlapTimer(&context, request, &response);
        if (reqstatus.ok()) {
            telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
            status = static_cast<telux::common::Status>(response.status());
            int cbDelay = static_cast<int>(response.delay());
            bool isCallbackNeeded = static_cast<bool>(response.iscallback());
            int timeDuration = static_cast<int>(response.time_duration());
            if ((status == telux::common::Status::SUCCESS ) && (isCallbackNeeded)) {
                if(callback) {
                    auto f = std::async(std::launch::async,
                    [this, error , callback, cbDelay, timeDuration]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                        callback(error, timeDuration);
                    }).share();
                    taskQ_->add(f);
                }
            }
        }
    } else {
        status = telux::common::Status::NOTSUPPORTED;
    }
    return status;
}

telux::common::Status CallManagerStub::setECallConfig(EcallConfig config) {
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetConfigRequest request;
    ::telStub::SetConfigReply response;
    ClientContext context;

    auto validityMask = config.configValidityMask;
    LOG(INFO, __FUNCTION__, " configValidityMask: ", validityMask.to_string());
    if(validityMask.test(ECALL_CONFIG_MUTE_RX_AUDIO)) {
        request.set_mute_rx_audio(static_cast<bool>(config.muteRxAudio));
        request.set_is_mute_rx_audio_valid(static_cast<bool>(true));
    }
    if(validityMask.test(ECALL_CONFIG_NUM_TYPE)) {
        request.set_is_num_type_valid(static_cast<bool>(true));
        if(config.numType == ECallNumType::DEFAULT) {
            request.set_num_type(static_cast<::telStub::ECallNumType>(
                telux::tel::ECallNumType::DEFAULT));
        } else {
            request.set_num_type(static_cast<::telStub::ECallNumType>(
                telux::tel::ECallNumType::OVERRIDDEN));
        }
    }
    if(validityMask.test(ECALL_CONFIG_OVERRIDDEN_NUM)) {
        request.set_is_overridden_num_valid(static_cast<bool>(true));
        request.set_overridden_num(config.overriddenNum);
    }
    if(validityMask.test(ECALL_CONFIG_USE_CANNED_MSD)) {
        request.set_is_use_canned_msd_valid(static_cast<bool>(true));
        request.set_use_canned_msd(config.useCannedMsd);
    }
    if(validityMask.test(ECALL_CONFIG_GNSS_UPDATE_INTERVAL)) {
        request.set_is_gnss_update_interval_valid(static_cast<bool>(true));
        request.set_gnss_update_interval(config.gnssUpdateInterval);
    }
    if(validityMask.test(ECALL_CONFIG_T2_TIMER)) {
        request.set_is_t2_timer_valid(static_cast<bool>(true));
        LOG(INFO, __FUNCTION__, " t2 timer value is : ", config.t2Timer);
        request.set_t2_timer(config.t2Timer);
    }
    if(validityMask.test(ECALL_CONFIG_T7_TIMER)) {
        request.set_is_t7_timer_valid(static_cast<bool>(true));
        request.set_t7_timer(config.t7Timer);
    }
    if(validityMask.test(ECALL_CONFIG_T9_TIMER)) {
        request.set_is_t9_timer_valid(static_cast<bool>(true));
        request.set_t9_timer(config.t9Timer);
    }
    if(validityMask.test(ECALL_CONFIG_MSD_VERSION)) {
        request.set_is_msd_version_valid(static_cast<bool>(true));
        request.set_msd_version(config.msdVersion);
    }
    grpc::Status reqstatus = stub_->SetConfig(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
    }
    return status;
}

telux::common::Status CallManagerStub::getECallConfig(EcallConfig &config) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Call Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    const ::google::protobuf::Empty request;
    ::telStub::GetConfigResponse response;
    ClientContext context;
    EcallConfigValidity validityMask;
    validityMask.reset();
    config = {};
    grpc::Status reqstatus = stub_->GetConfig(&context, request, &response);
    telux::common::Status status = telux::common::Status::FAILED;
    if (reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        if(response.is_mute_rx_audio_valid() == true) {
            validityMask.set(ECALL_CONFIG_MUTE_RX_AUDIO);
            config.muteRxAudio = response.mute_rx_audio();
        }
        if(response.is_num_type_valid() == true) {
            validityMask.set(ECALL_CONFIG_NUM_TYPE);
            config.numType = static_cast<telux::tel::ECallNumType>(response.num_type());
        }
        if(response.is_overridden_num_valid() == true) {
                validityMask.set(ECALL_CONFIG_OVERRIDDEN_NUM);
                config.overriddenNum = response.overridden_num();
        }
        if(response.is_use_canned_msd_valid() == true) {
            validityMask.set(ECALL_CONFIG_USE_CANNED_MSD);
            config.useCannedMsd = response.use_canned_msd();
        }
        if(response.is_gnss_update_interval_valid() == true) {
            validityMask.set(ECALL_CONFIG_GNSS_UPDATE_INTERVAL);
            config.gnssUpdateInterval = response.gnss_update_interval();
        }
        if(response.is_t2_timer_valid() == true) {
            validityMask.set(ECALL_CONFIG_T2_TIMER);
            config.t2Timer = response.t2_timer();
        }
        if(response.is_t7_timer_valid() == true) {
            validityMask.set(ECALL_CONFIG_T7_TIMER);
            config.t7Timer = response.t7_timer();
        }
        if(response.is_t9_timer_valid() == true) {
            validityMask.set(ECALL_CONFIG_T9_TIMER);
            config.t9Timer = response.t9_timer();
        }
        if(response.is_msd_version_valid() == true) {
            validityMask.set(ECALL_CONFIG_MSD_VERSION);
            config.msdVersion = response.msd_version();
        }
        config.configValidityMask = validityMask;
        LOG(INFO, __FUNCTION__, " configValidityMask: ", validityMask.to_string());
    }
    return status;
}


