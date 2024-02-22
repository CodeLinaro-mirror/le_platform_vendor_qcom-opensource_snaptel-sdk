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

#include "CallManagerServerImpl.hpp"
#include "../../../libs/tel/TelDefinesStub.hpp"
#include "SimulationServer.hpp"
#include "../../../libs/tel/Helper.hpp"
#include <telux/common/DeviceConfig.hpp>

#define CALL_MANAGER "ICallManager"
#define MSD_UPDATE_EVENT "msdUpdateRequest"
#define HANGUP_CALL_EVENT "hangupCall"
#define INCOMING_CALL_EVENT "incomingCall"

#define JSON_PATH1 "system-state/tel/ICallManagerStateSlot1.json"
#define JSON_PATH2 "system-state/tel/ICallManagerStateSlot2.json"
#define JSON_PATH3 "api/tel/ICallManagerSlot1.json"
#define JSON_PATH4 "api/tel/ICallManagerSlot2.json"

#define SLOT_1 1
#define SLOT_2 2

using namespace telux::tel;

CallManagerServerImpl::CallManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    readJson();
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

CallManagerServerImpl::~CallManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if(taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status CallManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    if(ecallStateMachine_) {
        ecallStateMachine_ = nullptr;
    }
    calls_.clear();
    return grpc::Status::OK;
}

grpc::Status CallManagerServerImpl::GetInProgressCalls(ServerContext* context,
    const ::google::protobuf::Empty* request, telStub::GetInProgressCallsReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<CallInfo>> calls = calls_;
    for(auto &it : calls) {
        telStub::Call *result = response->add_calls();
        result->set_call_state(static_cast<telStub::CallState>(it->callState));
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"CallState is ", static_cast<int>(it->callState));
        result->set_call_index(it->index);
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"CallIndex is ", static_cast<int>(it->index));
        result->set_call_direction
        (static_cast<telStub::CallDirection_Direction>(it->callDirection));
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"Calldirection is ",
        static_cast<int>(it->callDirection));
        result->set_remote_party_number(it->remotePartyNumber);
        LOG(DEBUG, "CallMgr - ", __FUNCTION__,"remotePartyNumber is ",
        static_cast<std::string>(it->remotePartyNumber));
        result->set_call_end_cause(static_cast<telStub::CallEndCause_Cause>(it->callEndCause));
        result->set_phone_id(it->phoneId);
        result->set_is_multi_party_call(it->isMultiPartyCall);
        result->set_is_mpty(it->isMpty);
    }
    return grpc::Status::OK;
}

grpc::Status CallManagerServerImpl::readJson() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObjSystemStateSlot1_, JSON_PATH1);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH1 );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjSystemStateSlot2_, JSON_PATH2);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH2 );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjApiResponseSlot1_, JSON_PATH3);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH3 );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjApiResponseSlot2_, JSON_PATH4);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH4 );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    //System state response
    jsonObjSystemStateSlot_[SLOT_1] = rootObjSystemStateSlot1_;
    jsonObjSystemStateSlot_[SLOT_2] = rootObjSystemStateSlot2_;
    jsonObjSystemStateFileName_[SLOT_1] = JSON_PATH1;
    jsonObjSystemStateFileName_[SLOT_2] = JSON_PATH2;
    //Api response
    jsonObjApiResponseSlot_[SLOT_1] = rootObjApiResponseSlot1_;
    jsonObjApiResponseSlot_[SLOT_2] = rootObjApiResponseSlot2_;
    jsonObjApiResponseFileName_[SLOT_1] = JSON_PATH3;
    jsonObjApiResponseFileName_[SLOT_2] = JSON_PATH4;
    return grpc::Status::OK;
}

void CallManagerServerImpl::getJsonForSystemData(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjSystemStateFileName_[phoneId];
    rootObj = jsonObjSystemStateSlot_[phoneId];
}

void CallManagerServerImpl::getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjApiResponseFileName_[phoneId];
    rootObj = jsonObjApiResponseSlot_[phoneId];
}

grpc::Status CallManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        rootObj = jsonObjApiResponseSlot_[SLOT_1];
        int cbDelay = rootObj[CALL_MANAGER]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus =
            rootObj[CALL_MANAGER]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);

        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::vector<std::string> filters = {TEL_CALL_FILTER};
            auto &serverEventManager = ServerEventManager::getInstance();
            serverEventManager.registerListener(shared_from_this(), filters);
        }
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::GetServiceStatus(ServerContext* context,
    const google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        rootObj = jsonObjApiResponseSlot_[SLOT_1];
        int cbDelay = rootObj[CALL_MANAGER]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus =
            rootObj[CALL_MANAGER]["IsSubsystemReady"].asString();
        telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::MakeCall(ServerContext* context,
    const telStub::MakeCallRequest* request, telStub::MakeCallReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();

    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "makeCall", status,
            error, cbDelay );

        if(cbDelay == -1) {
            isCallback = false;
        }
        if(addNewCallDetails<telStub::MakeCallRequest>(request)) {
            auto f = std::async(std::launch::async,
            [this]() {
                handleCallMachine();
            }).share();
            taskQ_->add(f);
        } else {
            error = telux::common::ErrorCode::OP_IN_PROGRESS;
            response->set_error(static_cast<commonStub::ErrorCode>(error));
        }
        telStub::Call call_;
        call_.set_call_direction
                (static_cast<telStub::CallDirection_Direction>(callInfo_.callDirection));
        call_.set_remote_party_number(static_cast<std::string>(callInfo_.remotePartyNumber));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        response->set_status(static_cast<commonStub::Status>(status));
        call_.set_call_index(static_cast<int>(callInfo_.index));
        *response->mutable_call() = call_;
        if(error == telux::common::ErrorCode::SUCCESS) {
            response->set_error(static_cast<commonStub::ErrorCode>(error));
        }
    }
    return readStatus;
}

void CallManagerServerImpl::handleCallMachine() {
    if(callInfo_.callDirection == telux::tel::CallDirection::OUTGOING) {
        if(calls_.size() == 1) {
            changeCallState(callInfo_.phoneId , "CALL_DIALING", callInfo_.remotePartyNumber);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            changeCallState(callInfo_.phoneId ,"CALL_ALERTING", callInfo_.remotePartyNumber);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            changeCallState(callInfo_.phoneId ,"CALL_ACTIVE", callInfo_.remotePartyNumber);
        } else {
            changeCallState(callInfo_.phoneId ,"CALL_DIALING", callInfo_.remotePartyNumber);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            changeCallState(callInfo_.phoneId ,"CALL_ALERTING", callInfo_.remotePartyNumber);
            CallInfo info = callInfo_;
            changeCallStateofActiveCalls(info);
        }
    }
}

grpc::Status CallManagerServerImpl::Answer(ServerContext* context,
    const telStub::AnswerRequest* request, telStub::AnswerReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        int callIndex = request->call_index();
        telux::common::ErrorCode error;
        telux::common::Status status;
        bool isCallback = true;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "answer", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            if(info->callState == telux::tel::CallState::CALL_INCOMING) {
                changeCallState(info->phoneId ,"CALL_ACTIVE", info->remotePartyNumber);
            } else if(info->callState == telux::tel::CallState::CALL_WAITING){
                changeCallStateofActiveCalls(*info);
            }
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

void CallManagerServerImpl::changeCallStateofActiveCalls(CallInfo info) {
    std::shared_ptr<CallInfo> newCall;
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if ((*callIterator)->index != info.index) {
            if((*callIterator)->callState == telux::tel::CallState::CALL_ACTIVE) {
                changeCallState((*callIterator)->phoneId, "CALL_HOLD",
                (*callIterator)->remotePartyNumber);
            }
        } else {
            newCall = (*callIterator);
        }
    }
    changeCallState(newCall->phoneId, "CALL_ACTIVE", newCall->remotePartyNumber);
}

grpc::Status CallManagerServerImpl::MakeECall(ServerContext* context,
    const telStub::MakeECallRequest* request, telStub::MakeECallReply* response) {
    telux::common::ErrorCode error;
    telux::common::Status status;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    bool isCallback = true;
    int cbDelay;
    updateInProgress_ = false;
    int phoneId = request->phone_id();
    int makeEcallApiType = static_cast<int>(request->api());
    std::string input = "";
    if(makeEcallApiType == makeECallWithMsd) {
        input = "makeECallWithMsd";
    } else if(makeEcallApiType == makeTpsECallOverCSWithMsd) {
        input = "makeTpsECallOverCSWithMsd";
    } else if(makeEcallApiType == makeTpsECallOverIMS) {
        input = "makeTpsECallOverIMS";
    } else if(makeEcallApiType == makeECallWithRawMsd) {
        input = "makeECallWithRawMsd";
    } else if(makeEcallApiType == makeTpsECallOverCSWithRawMsd) {
        input = "makeTpsECallOverCSWithRawMsd";
    } else if(makeEcallApiType == makeECallWithoutMsd) {
        input = "makeECallWithoutMsd";
    } else if(makeEcallApiType == makeTpsECallOverCSWithoutMsd) {
        input = "makeTpsECallOverCSWithoutMsd";
    } else {
        input = "makeECallWithMsd";
    }
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
            CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, input, status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        // add ecall data to server cache if a new call.
        if(addNewCallDetails<telStub::MakeECallRequest>(request)) {
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            telStub::Call call_;
            call_.set_call_direction
                    (static_cast<telStub::CallDirection_Direction>(callInfo_.callDirection));
            call_.set_remote_party_number(static_cast<std::string>(callInfo_.remotePartyNumber));
            response->set_iscallback(isCallback);
            response->set_delay(cbDelay);
            response->set_status(static_cast<commonStub::Status>(status));
            call_.set_call_index(static_cast<int>(callInfo_.index));
            *response->mutable_call() = call_;
            auto f = std::async(std::launch::async,
                [this]() {
                    handleStateMachine(callInfo_.phoneId);
                }).share();
            taskQ_->add(f);
        } else {
            // Send a negative response if eCall is already in progress.
            response->set_error(commonStub::ErrorCode::OP_IN_PROGRESS);
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::SetConfig(ServerContext* context,
    const telStub::SetConfigRequest* request,  telStub::SetConfigReply* response) {
    telux::tel::EcallConfig config = {};
    telux::common::ErrorCode error;
    telux::common::Status status;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(SLOT_1, jsonfilename, rootObj);
        if (request->is_mute_rx_audio_valid()) {
                config.muteRxAudio = request->mute_rx_audio();
                rootObj[CALL_MANAGER]["eCallConfig"]["muteRxAudio"]
                    = config.muteRxAudio;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_num_type_valid()) {
                config.numType = static_cast<telux::tel::ECallNumType>(request->num_type());
                rootObj[CALL_MANAGER]["eCallConfig"]["numType"] =
                static_cast<int>(config.numType);
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_overridden_num_valid()) {
                config.overriddenNum = request->overridden_num();
                rootObj[CALL_MANAGER]["eCallConfig"]["overriddenNum"]
                    = config.overriddenNum;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_use_canned_msd_valid()) {
                config.useCannedMsd = request->use_canned_msd();
                rootObj[CALL_MANAGER]["eCallConfig"]["useCannedMsd"]
                    = config.useCannedMsd;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_gnss_update_interval_valid()) {
                config.gnssUpdateInterval = request->gnss_update_interval();
                rootObj[CALL_MANAGER]["eCallConfig"]["gnssUpdateInterval"] =
                config.gnssUpdateInterval;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_t2_timer_valid()) {
                config.t2Timer = request->t2_timer();
                LOG(INFO, __FUNCTION__, " t2 timer value is : ", config.t2Timer);
                rootObj[CALL_MANAGER]["eCallConfig"]["T2Timer"] = config.t2Timer;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_t7_timer_valid()) {
                config.t7Timer = request->t7_timer();
                rootObj[CALL_MANAGER]["eCallConfig"]["T7Timer"] = config.t7Timer;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_t9_timer_valid()) {
                config.t9Timer = request->t9_timer();
                rootObj[CALL_MANAGER]["eCallConfig"]["T9Timer"] = config.t9Timer;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        if (request->is_msd_version_valid()) {
                config.msdVersion = request->msd_version();
                rootObj[CALL_MANAGER]["eCallConfig"]["msdVersion"]
                    = config.msdVersion;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[SLOT_1] = rootObj;
        }
        getJsonForApiResponseSlot(SLOT_1, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "setECallConfig", status,
            error, cbDelay );
        response->set_status(static_cast<commonStub::Status>(status));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::GetConfig(ServerContext* context,
    const google::protobuf::Empty* request, telStub::GetConfigResponse* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::ErrorCode error;
    telux::common::Status status;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForSystemData(SLOT_1, jsonfilename, rootObj);
        response->set_is_mute_rx_audio_valid(true);
        response->set_mute_rx_audio(
            rootObj[CALL_MANAGER]["eCallConfig"]["muteRxAudio"].asBool());
        response->set_is_num_type_valid(true);
        response->set_num_type(static_cast<telStub::ECallNumType>(
                rootObj[CALL_MANAGER]["eCallConfig"]["numType"].asInt()));
        response->set_is_overridden_num_valid(true);
        response->set_overridden_num(
            rootObj[CALL_MANAGER]["eCallConfig"]["overriddenNum"].asString());
        response->set_is_use_canned_msd_valid(true);
        response->set_use_canned_msd(
            rootObj[CALL_MANAGER]["eCallConfig"]["useCannedMsd"].asBool());
        response->set_is_gnss_update_interval_valid(true);
        response->set_gnss_update_interval(
            rootObj[CALL_MANAGER]["eCallConfig"]["gnssUpdateInterval"].asInt());
        response->set_is_t2_timer_valid(true);
        response->set_t2_timer(
            rootObj[CALL_MANAGER]["eCallConfig"]["T2Timer"].asInt());
        response->set_is_t7_timer_valid(true);
        response->set_t7_timer(
            rootObj[CALL_MANAGER]["eCallConfig"]["T7Timer"].asInt());
        response->set_is_t9_timer_valid(true);
        response->set_t9_timer(
            rootObj[CALL_MANAGER]["eCallConfig"]["T9Timer"].asInt());
        response->set_is_msd_version_valid(true);
        response->set_msd_version(
            rootObj[CALL_MANAGER]["eCallConfig"]["msdVersion"].asInt());
        getJsonForApiResponseSlot(SLOT_1, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "getECallConfig", status,
            error, cbDelay );
        response->set_status(static_cast<commonStub::Status>(status));
        LOG(DEBUG, __FUNCTION__, "Status is ", static_cast<int>(status));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::UpdateEcallHlapTimer(ServerContext* context,
    const telStub::UpdateEcallHlapTimerRequest* request,
    telStub::UpdateEcallHlapTimerResponse* response) {

    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    int phoneId = request->phone_id();
    ::telStub::HlapTimerType type = request->type();
    int time = request->time_duration();

    if(readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        if(type == ::telStub::HlapTimerType::T10_TIMER) {
            rootObj[CALL_MANAGER]["eCallHlapTimer"]["t10"] = time;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
        }
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "updateEcallHlapTimer", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RequestEcallHlapTimer(ServerContext* context,
    const telStub::RequestEcallHlapTimerRequest* request,
    telStub::RequestEcallHlapTimerReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    int phoneId = request->phone_id();
    ::telStub::HlapTimerType type = request->type();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "requestEcallHlapTimer", status,
            error, cbDelay );
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        if(type == ::telStub::HlapTimerType::T10_TIMER) {
            int timeDuration = rootObj[CALL_MANAGER]["eCallHlapTimer"]["t10"].asInt();
            response->set_time_duration(timeDuration);
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RequestECallHlapTimerStatus(ServerContext* context,
    const telStub::RequestECallHlapTimerStatusRequest* request,
    telStub::RequestECallHlapTimerStatusReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        telux::common::ErrorCode error;
        telux::common::Status status;
        bool isCallback = true;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER,
            "requestECallHlapTimerStatus", status, error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }

        telStub::ECallHlapTimerStatus hlapTimerStatus;
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        hlapTimerStatus.set_t2(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T2Timer"].asInt())));
        hlapTimerStatus.set_t5(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T5Timer"].asInt())));
        hlapTimerStatus.set_t6(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T6Timer"].asInt())));
        hlapTimerStatus.set_t7(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T7Timer"].asInt())));
        hlapTimerStatus.set_t9(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T9Timer"].asInt())));
        hlapTimerStatus.set_t10(static_cast<telStub::HlapTimerStatus_Status>
            ((rootObj[CALL_MANAGER]["ecallHlapTimerStatus"]["T10Timer"].asInt())));
        *response->mutable_hlap_timer_status() = hlapTimerStatus;

        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::ExitEcbm(ServerContext* context,
    const telStub::RequestEcbmRequest* request,
    telStub::RequestEcbmReply* response) {
    grpc::Status readStatus = readJson();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    int phoneId = request->phone_id();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "exitEcbm", status,
            error, cbDelay );
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        ::telStub::EcbMode_Mode mode =
        static_cast<::telStub::EcbMode_Mode>(
            rootObj[CALL_MANAGER]["ecbm"]["ecbMode"].asInt());
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        if(rootObj[CALL_MANAGER]["ecbm"]["ecbMode"].asInt() == 0) {
            response->set_error(commonStub::ErrorCode::INVALID_ARGUMENTS);
        }
        response->set_ecbmode(mode);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RequestEcbm(ServerContext* context,
    const telStub::RequestEcbmRequest* request,
    telStub::RequestEcbmReply* response) {
    grpc::Status readStatus = readJson();
    telux::common::ErrorCode error;
    telux::common::Status status;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    std::string jsonfilename = "";
    Json::Value rootObj;
    bool isCallback = true;
    int cbDelay;
    int phoneId = request->phone_id();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "requestEcbm", status,
            error, cbDelay );
        getJsonForSystemData(phoneId, jsonfilename, rootObj);
        ::telStub::EcbMode_Mode mode =
        static_cast<::telStub::EcbMode_Mode>(rootObj[CALL_MANAGER]["ecbm"]\
            ["ecbMode"].asInt());
        response->set_ecbmode(mode);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_delay(cbDelay);
    }
    return readStatus;
}

bool CallManagerServerImpl::match(std::shared_ptr<CallInfo> call, CallInfo callToCompare) {
    logCallDetails();
    return ((callToCompare.remotePartyNumber == call->remotePartyNumber)
        && (callToCompare.phoneId == call->phoneId));
}

void CallManagerServerImpl::logCallDetails() {
    LOG(DEBUG, __FUNCTION__, " SlotId = ", static_cast<int>(callInfo_.phoneId),
        " Call Info: remotePartyNumber = ", callInfo_.remotePartyNumber,
        ", callIndex = ", callInfo_.index,
        ", callDirection = ", static_cast<int>(callInfo_.callDirection),
        ", callState = ", static_cast<int>(callInfo_.callState));
}

std::shared_ptr<CallInfo> CallManagerServerImpl::findMatchingCall(int slotId, int callIndex) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);
    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        return match(call, slotId, callIndex);
    });

    if (iter != std::end(calls_)) {
        LOG(DEBUG, __FUNCTION__, " found matched call");
        return *iter;
    } else {
        LOG(DEBUG, __FUNCTION__, " no matched call");
        return nullptr;
    }
}

bool CallManagerServerImpl::match(std::shared_ptr<CallInfo> call, int slotId, int callIndex) {
    logCallDetails();
    return ((call->index == callIndex) && (call->phoneId == slotId));
}

bool CallManagerServerImpl::findMatchingCall(CallInfo callToCompare) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);

    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        return match(call, callToCompare);
    });

    if (iter != std::end(calls_)) {
        LOG(DEBUG, __FUNCTION__, " found matched call");
        return true;
    } else {
        LOG(DEBUG, __FUNCTION__, " no matched call");
        return false;
    }
}

void CallManagerServerImpl::hangupWaitingOrBackgroundCalls(int phoneId) {
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if((*callIterator)->phoneId == phoneId) {
            if(((*callIterator)->callState == telux::tel::CallState::CALL_ACTIVE)
            ||((*callIterator)->callState == telux::tel::CallState::CALL_INCOMING)) {
            changeCallState((*callIterator)->phoneId, "CALL_ENDED",
            (*callIterator)->remotePartyNumber);
        }
        }
    }
}

void CallManagerServerImpl::resumeBackgroundCalls(int phoneId) {
    bool foundCall = false;
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if((*callIterator)->phoneId == phoneId) {
            if((*callIterator)->callState == telux::tel::CallState::CALL_WAITING) {
                changeCallState((*callIterator)->phoneId, "CALL_ACTIVE",
                (*callIterator)->remotePartyNumber);
                foundCall = true;
                break;
            }
        }
    }
    if(!foundCall) {
        for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
            ++callIterator) {
            if((*callIterator)->callState == telux::tel::CallState::CALL_ON_HOLD) {
                changeCallState((*callIterator)->phoneId, "CALL_ACTIVE",
                (*callIterator)->remotePartyNumber);
                break;
            }
        }
    }

}

grpc::Status CallManagerServerImpl::HangupForegroundResumeBackground(ServerContext* context,
    const telStub::HangupForegroundResumeBackgroundRequest* request,
    telStub::HangupForegroundResumeBackgroundReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        telux::common::ErrorCode error;
        telux::common::Status status;
        bool isCallback = true;
        int cbDelay;
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER,
            "hangupWaitingOrBackground", status, error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        hangupWaitingOrBackgroundCalls(phoneId);
        resumeBackgroundCalls(phoneId);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

void CallManagerServerImpl::resumeCall(int phoneId, int callIndex) {
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if(((*callIterator)->phoneId == phoneId) && ((*callIterator)->index )) {
            if((*callIterator)->callState == telux::tel::CallState::CALL_ON_HOLD) {
                changeCallState((*callIterator)->phoneId, "CALL_ACTIVE",
                (*callIterator)->remotePartyNumber);
                break;
            }
        }
    }
}

grpc::Status CallManagerServerImpl::Resume(ServerContext* context,
    const telStub::ResumeRequest* request,
    telStub::ResumeReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "resume", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        resumeCall(phoneId, callIndex);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::Hold(ServerContext* context,
    const telStub::HoldRequest* request,
    telStub::HoldReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "hold", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        holdCall(phoneId, callIndex);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

void CallManagerServerImpl::holdCall(int phoneId, int callIndex) {
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if(((*callIterator)->phoneId == phoneId) && ((*callIterator)->index )) {
            if((*callIterator)->callState == telux::tel::CallState::CALL_ACTIVE) {
                changeCallState((*callIterator)->phoneId, "CALL_HOLD",
                (*callIterator)->remotePartyNumber);
                break;
            }
        }
    }
}

void CallManagerServerImpl::swapCalls(int callHoldIndex, int phoneIndex,
    int callActivateIndex) {
    for(auto callIterator = std::begin(calls_); callIterator != std::end(calls_);
        ++callIterator) {
        if(((*callIterator)->phoneId == phoneIndex)
            && ((*callIterator)->index  == callHoldIndex)) {
            if((*callIterator)->callState == telux::tel::CallState::CALL_ON_HOLD) {
                changeCallState((*callIterator)->phoneId, "CALL_ACTIVE",
                (*callIterator)->remotePartyNumber);
            }
        }
        if(((*callIterator)->phoneId == phoneIndex)
            && ((*callIterator)->index  == callActivateIndex)) {
            if((*callIterator)->callState == telux::tel::CallState::CALL_ACTIVE) {
                changeCallState((*callIterator)->phoneId, "CALL_HOLD",
                (*callIterator)->remotePartyNumber);
            }
        }

    }
}

grpc::Status CallManagerServerImpl::Swap(ServerContext* context,
    const telStub::SwapRequest* request,
    telStub::SwapReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int callHoldIndex = request->call_to_hold_index();
    int phoneIndex = request->phone_id();
    int callActivateIndex = request->call_to_activate_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(SLOT_1, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "swap", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        swapCalls(callHoldIndex, phoneIndex, callActivateIndex);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::HangupWaitingOrBackground(ServerContext* context,
    const telStub::HangupWaitingOrBackgroundRequest* request,
    telStub::HangupWaitingOrBackgroundReply* response) {
    int phoneId = request->phone_id();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "hangupWaitingOrBackground",
            status, error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        hangupWaitingOrBackgroundCalls(phoneId);
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::Hangup(ServerContext* context,
    const telStub::HangupRequest* request, telStub::HangupReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "hangup", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            if(info->iseCall) { //emergency call
                ecallStateMachine_->onEvent(
                ecallStateMachine_->createTelEvent(
                telux::tel::EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER,
                ""));
            } else {  //Voice call
                changeCallState(info->phoneId, "CALL_ENDED", info->remotePartyNumber);
            }
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::Reject(ServerContext* context,
    const telStub::RejectRequest* request, telStub::RejectReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "reject", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            changeCallState(info->phoneId , "CALL_ENDED", info->remotePartyNumber);
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

grpc::Status CallManagerServerImpl::RejectWithSMS(ServerContext* context,
    const telStub::RejectWithSMSRequest* request, telStub::RejectWithSMSReply* response) {
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int callIndex = request->call_index();
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, "rejectSms", status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
        if(info != nullptr) {
            changeCallState(info->phoneId , "CALL_ENDED", info->remotePartyNumber);
            response->set_status(static_cast<commonStub::Status>(status));
            response->set_iscallback(isCallback);
            response->set_error(static_cast<commonStub::ErrorCode>(error));
            response->set_delay(cbDelay);
        }
    }
    return readStatus;
}

void CallManagerServerImpl::handleHangupRequest(std::string eventParams) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    int phoneId;
    int callIndex;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(ERROR, __FUNCTION__, "CallId not passed");
        return;
    } else {
        try {
            callIndex = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
    std::shared_ptr<CallInfo> info = findMatchingCall(phoneId, callIndex);
    if(info != nullptr) {
        if(info->iseCall) {
            ecallStateMachine_->onEvent(
            ecallStateMachine_->createTelEvent(
                telux::tel::EcallStateMachine::EventID::HANGUP_REQUEST_FROM_USER, ""));
            //Clear call cache in server
           std::shared_ptr<CallInfo> call =
            findCallAndUpdateCallState
                (getRemotePartyNumber(phoneId), telux::tel::CallState::CALL_ENDED);
        } else {
            changeCallState(info->phoneId, "CALL_ENDED", info->remotePartyNumber);
        }
    } else {
        LOG(ERROR, __FUNCTION__, " Matching call not found ");
        return;

    }
}

grpc::Status CallManagerServerImpl::UpdateECallMsd(ServerContext* context,
    const telStub::UpdateECallMsdRequest* request, telStub::UpdateECallMsdResponse* response) {
    telux::common::ErrorCode error;
    telux::common::Status status;
    bool isCallback = true;
    int cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    int phoneId = request->phone_id();
    int updateECallMsdApiType = static_cast<int>(request->api());
    std::string input = "";
    if(updateECallMsdApiType == updateEcallMsd) {
        input = "updateECallMsd";
    } else if(updateECallMsdApiType == updateECallRawMsd) {
        input = "updateECallRawMsd";
    } else {
        input = "updateECallMsd";
    }
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
        CommonUtils::getValues(jsonObjApiResponse, CALL_MANAGER, input, status,
            error, cbDelay );
        if(cbDelay == -1) {
            isCallback = false;
        }
        response->set_status(static_cast<commonStub::Status>(status));
        response->set_iscallback(isCallback);
        response->set_error(static_cast<commonStub::ErrorCode>(error));
        response->set_delay(cbDelay);
    }
    return readStatus;
}

void CallManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == "tel_call") {
        onEventUpdate(message.event());
    }
}

void CallManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__,"String is ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__,"String is ", token );
    if ( MSD_UPDATE_EVENT == token) {
        handleMsdUpdateRequest(event);
    } else if(HANGUP_CALL_EVENT == token) {
         handleHangupRequest(event);
    } else if("incomingCall" == token) {
        handleIncomingCallRequest(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void CallManagerServerImpl::triggerMsdPullrequestEvent(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::MsdPullRequestEvent msdPullRequestEvent;
    ::eventService::EventResponse anyResponse;

    msdPullRequestEvent.set_phone_id(phoneId);
    anyResponse.set_filter("tel_call");
    anyResponse.mutable_any()->PackFrom(msdPullRequestEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
}

void CallManagerServerImpl::handleIncomingCallRequest(std::string eventParams) {
    int phoneId;
    std::string dialNumber;
    /* Fetch the slotId */
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    if(phoneId == SLOT_2) {
        if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
            LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
            return;
        }
    }
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Fetch the dialnumber */
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "MT call is considered to be originating from PSAP");
    } else {
        dialNumber = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched dialNumber is: ", dialNumber);
    //Update call cache for new MT Voice call
    CallInfo callInfo;
    callInfo.phoneId = phoneId;
    callInfo.index = calls_.size() + 1;
    callInfo.callDirection = telux::tel::CallDirection::INCOMING;
    if(callInfo.index > 1) { //MO or MT call already exist then callState = WAITING
        callInfo.callState = telux::tel::CallState::CALL_WAITING;
    } else { //No MO or MT call already exist then callState = INCOMING
        callInfo.callState = telux::tel::CallState::CALL_INCOMING;
    }
    callInfo.remotePartyNumber = dialNumber;
    callInfo.isMsdTransmitted = false;
    callInfo.isMultiPartyCall = false;
    callInfo.isMpty = true;
    callInfo_ = callInfo;
    logCallDetails();
    auto call = std::make_shared<CallInfo>(callInfo);
    if(!findMatchingCall(callInfo)) {
        calls_.emplace_back(call);
    } else {
        LOG(DEBUG, __FUNCTION__, "DialNumber is already in progress: ", dialNumber);
        return;
    }
    auto f = std::async(std::launch::async, [this, callInfo]() {
            this->triggerIncomingCallEvent(callInfo);
        }).share();
    taskQ_->add(f);
}

void CallManagerServerImpl::triggerIncomingCallEvent(CallInfo callInfo ) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::Call callEvent;
    ::eventService::EventResponse anyResponse;

    callEvent.set_call_state(static_cast<telStub::CallState>(callInfo.callState));
    callEvent.set_call_index(callInfo.index);
    callEvent.set_call_direction(
    static_cast<telStub::CallDirection_Direction>(callInfo.callDirection));
    callEvent.set_remote_party_number(callInfo.remotePartyNumber);
    callEvent.set_phone_id(callInfo.phoneId);
    callEvent.set_is_multi_party_call(callInfo.isMultiPartyCall);
    callEvent.set_is_mpty(callInfo.isMpty);
    anyResponse.set_filter("tel_call");
    anyResponse.mutable_any()->PackFrom(callEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void CallManagerServerImpl::handleMsdUpdateRequest(std::string eventParams) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    int phoneId;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
            return;
        }
    }

    if(!updateInProgress_) {
        auto f = std::async(std::launch::async, [this, phoneId]() {
            this->triggerMsdPullrequestEvent(phoneId);
        }).share();
        taskQ_->add(f);
        if(ecallStateMachine_ != nullptr) {
            std::vector<std::string> input = parseUserInput();
            if((input[0] == "SUCCESS")
                && (ecallStateMachine_->getCurrentState() ==
                telux::tel::EcallStateMachine::StateID::STATE_CALL_CONVERSATION)) {
                updateInProgress_ = true;
                bool isNGeCall = getUserConfiguredeCallRat();
                if(isNGeCall) {
                    ecallStateMachine_->onEvent(
                    ecallStateMachine_->createTelEvent(
                    telux::tel::EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP, "NGeCall"));
                } else {
                    ecallStateMachine_->onEvent(
                    ecallStateMachine_->createTelEvent(
                    telux::tel::EcallStateMachine::EventID::MSD_PULL_REQUEST_FROM_PSAP, "CSeCall"));
                }
            } else {
                LOG(ERROR, __FUNCTION__, "Incorrect JSON configuration ");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, "The state machine is not yet initialised ");
        }
    }
}

std::vector<std::string> CallManagerServerImpl::parseUserInput() {
    std::vector<std::string> parsedString = {};
    std::string jsonObjApiResponseFileName = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(callInfo_.phoneId, jsonObjApiResponseFileName, rootObj);
        std::string input =
            rootObj[CALL_MANAGER]["configureFailureForRegulatoryECall"].asString();
        LOG(DEBUG, __FUNCTION__, "Input is ", input);
        int size = input.size();
        int i = 1;
        while (i <= size ) {
           int j = 1;
           LOG(DEBUG, __FUNCTION__, "parsed string is", input);
           std::string out = fetchNextToken(input, " ");
           j = out.size();
           LOG(DEBUG, __FUNCTION__, "J is ", j);
           parsedString.push_back(out);
           LOG(DEBUG, __FUNCTION__, "parsed string is", out);
           i = i+j+1;
           LOG(DEBUG, __FUNCTION__, "I  is ", i);
        }
    }
    return parsedString;
}

std::string CallManagerServerImpl::fetchNextToken(std::string& inputString, std::string delimiter) {
    unsigned int position = 0;
    std::string token;
    if ((position = inputString.find(delimiter)) != std::string::npos) {
        token = inputString.substr(0, position);
        inputString.erase(0, position + delimiter.length());
    }
    return token;
}

telux::common::Status CallManagerServerImpl::handleStateMachine(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::string> input = parseUserInput();
    bool isNGeCall = getUserConfiguredeCallRat();
    std::string remotePartyNumber = getRemotePartyNumber(phoneId);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    ecallStateMachine_ = std::make_shared<telux::tel::EcallStateMachine>(shared_from_this(), input,
        callInfo_.isMsdTransmitted, isNGeCall, phoneId, remotePartyNumber);
    if(!ecallStateMachine_) {
        return telux::common::Status::NOMEMORY;
    } else {
        ecallStateMachine_->start();
    }
    return telux::common::Status::SUCCESS;
}

std::string CallManagerServerImpl::getRemotePartyNumber(int phoneId) {
    LOG(DEBUG, __FUNCTION__, "PhoneId ", phoneId);
    std::string jsonObjFileName = "";
    Json::Value rootObj;
    readJson();
    getJsonForSystemData(phoneId, jsonObjFileName, rootObj);
    std::string input = rootObj[CALL_MANAGER]["eCallConfig"]["overriddenNum"].asString();
    LOG(DEBUG, __FUNCTION__, " Remote party number is ", input);
    return input;
}

bool CallManagerServerImpl::getUserConfiguredeCallRat() {
    LOG(DEBUG, __FUNCTION__);
    std::string jsonObjFileName = "";
    Json::Value rootObj;
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        getJsonForApiResponseSlot(callInfo_.phoneId, jsonObjFileName, rootObj);
        std::string input =
                rootObj[CALL_MANAGER]["eCallType"].asString();
        if(input == "NGeCall") {
            LOG(DEBUG, __FUNCTION__, "NG ecall is configured");
            return true;
        }
    }
    LOG(DEBUG, __FUNCTION__, "CS ecall is configured");
    return false;
}

void CallManagerServerImpl::updateEcallHlapTimer(std::string timer,
    telux::tel::HlapTimerStatus status) {
    LOG(DEBUG, __FUNCTION__," Timer ", timer, " Timer status ", static_cast<int>(status));
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    grpc::Status readStatus = readJson();
    if (!readStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed!");
        return;
    }
    getJsonForSystemData(callInfo_.phoneId, jsonObjApiResponseFileName, rootObj);
    rootObj[CALL_MANAGER]["ecallHlapTimerStatus"][timer] = static_cast<int>(status);
    JsonParser::writeToJsonFile(rootObj, jsonObjApiResponseFileName);
    jsonObjSystemStateSlot_[callInfo_.phoneId] = rootObj;
}

void CallManagerServerImpl::startTimer(std::string timer) {
    LOG(DEBUG, __FUNCTION__,"Start timer ", timer);
    updateEcallHlapTimer(timer, telux::tel::HlapTimerStatus::ACTIVE);
    if((timer == "T2Timer") || (timer == "T5Timer") || (timer == "T6Timer")
        || (timer == "T7Timer") || (timer == "T9Timer") || (timer == "T10Timer")) {
        startTimers(timer);
        auto f = std::async(std::launch::async, [this, timer]() {
            this->triggerCallInfoChangeEvent(timer, telux::tel::HlapTimerEvent::STARTED);
        }).share();
        taskQ_->add(f);
    } else {
       LOG(ERROR, __FUNCTION__,"Invalid timer ", timer);
    }
}

void CallManagerServerImpl::changeCallState(int phoneId, std::string callstate,
    std::string remotepartyNumber) {
    //Update call state at server
    auto f = std::async(std::launch::async, [this, phoneId, callstate, remotepartyNumber ]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        this->triggerCallStateChangeEvent(phoneId, callstate, remotepartyNumber);
    }).share();
    taskQ_->add(f);
}

void CallManagerServerImpl::msdTransmissionStatus(std::string msdtransmision ) {
    auto f = std::async(std::launch::async, [this, msdtransmision ]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            this->triggerCallInfoChangeEvent(msdtransmision, telux::tel::HlapTimerEvent::UNCHANGED);
        }).share();
    taskQ_->add(f);
}

void CallManagerServerImpl::startTimers(std::string timer) {
    Json::Value rootObj;
    std::string jsonfilename = "";
    grpc::Status readStatus = readJson();
    if(readStatus.ok()) {
        updateEcallHlapTimer(timer, telux::tel::HlapTimerStatus::ACTIVE);
        getJsonForSystemData(callInfo_.phoneId, jsonfilename, rootObj);
        LOG(DEBUG, __FUNCTION__,"Timer is ", timer, " Phone id is ", callInfo_.phoneId);
        int delay;
        if((timer == "T5Timer") || (timer == "T6Timer")) {
            delay = 5000; //timer expiry is set as per eCall specification to 5 secs
        } else {
            delay = rootObj[CALL_MANAGER]["eCallConfig"][timer].asInt();
        }
        auto f = std::async(std::launch::async, [this, delay, timer ]() {
            LOG(DEBUG, __FUNCTION__,"Delay is", delay);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                this->triggerTimerExpiry(timer);
        }).share();
        taskQ_->add(f);
        if(timer == "T9") {
            // Reset the state machine so that again eCall can be triggered.
            // T9 Timer will remain active as per the JSON configured timer and PSAP call will be
            // handled.
            if(ecallStateMachine_) {
                if(!(ecallStateMachine_->parseVectortoString("T9FAILED"))) {
                    ecallStateMachine_->stop();
                }
            }
        }
    }
}

void CallManagerServerImpl::triggerTimerExpiry(std::string timer) {
    LOG(DEBUG, __FUNCTION__);
    if((timer == "T2Timer") || (timer == "T5Timer") || (timer == "T6Timer") || (timer == "T7Timer")
        || (timer == "T10Timer")) {
        updateEcallHlapTimer(timer, telux::tel::HlapTimerStatus::INACTIVE);
        ecallStateMachine_->onEvent(
        ecallStateMachine_->createTelEvent(telux::tel::EcallStateMachine::EventID::ON_TIMER_EXPIRY,
        timer));
    } else if (timer == "T9Timer") {
        updateEcallHlapTimer(timer, telux::tel::HlapTimerStatus::INACTIVE);
        expiryTimer(timer);
    } else {
      LOG(ERROR, __FUNCTION__, "Invalid timer");
    }
}

void CallManagerServerImpl::expiryTimer(std::string timer) {
    LOG(DEBUG, __FUNCTION__,"timer is ", timer);
    updateEcallHlapTimer(timer, telux::tel::HlapTimerStatus::INACTIVE);
    auto f = std::async(std::launch::async, [this, timer]() {
            this->triggerCallInfoChangeEvent(timer, telux::tel::HlapTimerEvent::EXPIRED);
    }).share();
    taskQ_->add(f);
}

void CallManagerServerImpl::sendEvent(std::string timer, std::string status ) {
    std::string value = timer + status;
    if(status == "start") {
        updateEcallHlapTimer(timer, telux::tel::HlapTimerStatus::ACTIVE);
        auto f = std::async(std::launch::async, [this, timer, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            this->triggerCallInfoChangeEvent(timer, telux::tel::HlapTimerEvent::STARTED);
        }).share();
        taskQ_->add(f);
    } else if (status == "stop" ) {
        updateEcallHlapTimer(timer, telux::tel::HlapTimerStatus::INACTIVE);
        auto f = std::async(std::launch::async, [this, timer, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            this->triggerCallInfoChangeEvent(timer, telux::tel::HlapTimerEvent::STOPPED);
        }).share();
        taskQ_->add(f);
    }  else {}
}

void CallManagerServerImpl::triggerCallInfoChangeEvent(std::string timer,
    telux::tel::HlapTimerEvent action ) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = 1;
    ::telStub::ECallInfoEvent eCallInfoEvent;
    ::eventService::EventResponse anyResponse;
    eCallInfoEvent.set_timer(timer);
    eCallInfoEvent.set_action(static_cast<telStub::HlapTimerEvent>(action));
    eCallInfoEvent.set_phone_id(slotId);
    anyResponse.set_filter("tel_call");
    anyResponse.mutable_any()->PackFrom(eCallInfoEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void CallManagerServerImpl::triggerCallStateChangeEvent(int phoneId, std::string action,
    std::string remotepartyNumber) {
    LOG(DEBUG, __FUNCTION__);
    telux::tel::CallState state = Helper::getCallState(action);
    std::shared_ptr<CallInfo> call = findCallAndUpdateCallState(remotepartyNumber, state);
    int callIndex = call->index;
    std::string remotePartyNumber = call->remotePartyNumber;

    ::telStub::CallStateChangeEvent callStateChangeEvent;
    ::eventService::EventResponse anyResponse;

    callStateChangeEvent.set_callstate(action);
    callStateChangeEvent.set_call_index(callIndex);
    callStateChangeEvent.set_phone_id(phoneId);
    callStateChangeEvent.set_remote_party_number(remotePartyNumber);
    anyResponse.set_filter("tel_call");
    anyResponse.mutable_any()->PackFrom(callStateChangeEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);

    if(state == telux::tel::CallState::CALL_ENDED ) {
        //Clear call cache in server
        auto f = std::async(std::launch::async, [this, callIndex]() {
         std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            findAndRemoveMatchingCall(callIndex);
        }).share();
        taskQ_->add(f);
    }
}

std::shared_ptr<CallInfo> CallManagerServerImpl::findCallAndUpdateCallState(
    std::string remotePartyNumber, telux::tel::CallState action) {
    LOG(DEBUG, __FUNCTION__,"Remote party number is ",remotePartyNumber,
        "Call state is ", static_cast<int>(action) );
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);

    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        return find(call, remotePartyNumber, action);
    });

    if (iter != std::end(calls_)) {
        LOG(DEBUG, __FUNCTION__, " found matched call");
        (*iter)->callState = action;
        if(action == telux::tel::CallState::CALL_ENDED) {
            (*iter)->callEndCause = telux::tel::CallEndCause::NORMAL;
        }
        return *iter;
    } else {
        return nullptr;
    }
}

bool CallManagerServerImpl::find(std::shared_ptr<CallInfo> call, std::string remotePartyNumber,
    telux::tel::CallState action) {
    if(call->remotePartyNumber == remotePartyNumber) {
        return true;
    } else {
        return false;
    }
}

bool CallManagerServerImpl::findAndRemoveMatchingCall(int callIndex) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<CallInfo>>::iterator iter;
    std::lock_guard<std::mutex> lock(callManagerMutex_);

    iter = std::find_if(std::begin(calls_), std::end(calls_), [=](std::shared_ptr<CallInfo> call) {
        if(call->index == callIndex ) {
            return true;
        } else {
            return false;
        }
    });
    if (iter != std::end(calls_)) {
        calls_.erase(iter);
        LOG(DEBUG, __FUNCTION__, " found matched call");
        return true;
    } else {
        return false;
    }
}