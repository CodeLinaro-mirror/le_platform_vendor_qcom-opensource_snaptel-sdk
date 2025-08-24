/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "ApSimProfileManagerServerImpl.hpp"

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include <telux/tel/ApSimProfileManager.hpp>
#include <telux/common/DeviceConfig.hpp>

#define JSON_PATH1 "api/tel/IApSimProfileManagerSlot1.json"
#define JSON_PATH2 "api/tel/IApSimProfileManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/IApSimProfileManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/IApSimProfileManagerStateSlot2.json"
#define SUB_JSON_PATH "system-state/tel/ISubscriptionManagerState.json"
#define MANAGER    "IApSimProfileManager"
#define SLOT_1 1
#define SLOT_2 2

#define PROFILE_LIST_REQUEST         "profileListRequest"
#define PROFILE_OPERATION_REQUEST    "profileOperationRequest"
#define DEFAULT_SUB_ICCID            "89010020000011293999"

ApSimProfileManagerServerImpl::ApSimProfileManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
    profileIccid_ = DEFAULT_SUB_ICCID;
}

ApSimProfileManagerServerImpl::~ApSimProfileManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status ApSimProfileManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status ApSimProfileManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = JSON_PATH1;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus = rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay:: ", cbDelay, " cbStatus:: ", cbStatus);
    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_AP_SIM_PROFILE_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status ApSimProfileManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    std::string srvStatus = rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status ApSimProfileManagerServerImpl::SendRetrieveProfileListResponse(ServerContext* context,
    const telStub::ProfileListResponseRequest* request,
    telStub::ProfileListResponseReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "sendRetrieveProfileListResponse";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        int referenceId = static_cast<int>(request->reference_id());
        if (referenceId <= 0) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,"Invalid Arguements");
        }
        data.stateRootObj[MANAGER]\
            ["ProfileListResponse"]["reference_id"] = referenceId;
        data.stateRootObj[MANAGER]["apduExchangeResult"] = static_cast<int>(request->result());
        for (auto &iccid : request->profile_iccid()) {
            data.stateRootObj[MANAGER]["ProfileListResponse"]["profileIccids"] = iccid;
        }
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }
    // Create response
    if (data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ApSimProfileManagerServerImpl::SendProfileOperationResponse(ServerContext* context,
    const telStub::ProfileOperationResponseRequest* request,
    telStub::ProfileOperationResponseReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->slot_id();
    std::string apiJsonPath = (phoneId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "sendProfileOperationResponse";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (data.status == telux::common::Status::SUCCESS) {
        int referenceId = request->reference_id();
        if (referenceId <= 0) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,"Invalid Arguements");
        }
        data.stateRootObj[MANAGER]["ProfileOperationResponse"]["reference_id"] = referenceId;
        int result = static_cast<int>(request->result());
        data.stateRootObj[MANAGER]["ProfileOperationResponse"]["apduExchangeResult"] = result;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        if (result == static_cast<int>(telux::tel::ApduExchangeStatus::SUCCESS)) {
            // update iccid in subscription after profile operation
            Json::Value subRootObj;
            error = JsonParser::readFromJsonFile(subRootObj, SUB_JSON_PATH);
            if (error != ErrorCode::SUCCESS) {
                LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
                return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
            }
            if (isProfileEnable_) {
                subRootObj["ISubscriptionManager"]["Subscription"][phoneId-1]["iccId"]
                    = profileIccid_;
            } else {
                // update to default
                if (profileIccid_ != DEFAULT_SUB_ICCID) {
                    subRootObj["ISubscriptionManager"]["Subscription"][phoneId-1]["iccId"]
                        = DEFAULT_SUB_ICCID;
                } else {
                    subRootObj["ISubscriptionManager"]["Subscription"][phoneId-1]["iccId"] = "";
                }
            }
            JsonParser::writeToJsonFile(subRootObj, SUB_JSON_PATH);
        }
    }
    // Create response
    if (data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void ApSimProfileManagerServerImpl::handleProfileListRequest(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    ::telStub::ProfileListRequestEvent profileListEvent;
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }
        if (phoneId == SLOT_2) {
            if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
                LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
                return;
            }
        }

        // Read string to get reference id
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int referenceId = std::stoi(token);
        if (referenceId <= 0) {
            LOG(ERROR, __FUNCTION__, " Invalid input for reference id");
            return;
        }

        profileListEvent.set_slot_id(phoneId);
        profileListEvent.set_reference_id(referenceId);
        LOG(DEBUG, __FUNCTION__, " referenceId: ", referenceId);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter(telux::tel::TEL_AP_SIM_PROFILE_FILTER);
    anyResponse.mutable_any()->PackFrom(profileListEvent);
    auto f = std::async(std::launch::async, [this, anyResponse]() {
        this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void ApSimProfileManagerServerImpl::handleProfileOperationRequest(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId;
    ::telStub::ProfileOperationRequestEvent profileOperationEvent;
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }
        if (phoneId == SLOT_2) {
            if(!(telux::common::DeviceConfig::isMultiSimSupported())) {
                LOG(ERROR, __FUNCTION__, " Multi SIM is not enabled ");
                return;
            }
        }

        // Read string to get reference id
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int referenceId = std::stoi(token);
        if (referenceId <= 0) {
            LOG(ERROR, __FUNCTION__, " Invalid input for reference id");
            return;
        }

        // Read string to get iccid
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        std::string iccid = token;
        if (iccid.length() < 20) {
            LOG(ERROR, __FUNCTION__, " Invalid ICCID");
            return;
        }

         // Read string to get profile operation
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int isEnable = std::stoi(token);
        if (isEnable != 0 && isEnable != 1) {
            LOG(ERROR, __FUNCTION__, " Invalid input for profile operation");
            return;
        }

        profileOperationEvent.set_slot_id(phoneId);
        profileOperationEvent.set_reference_id(referenceId);
        profileOperationEvent.set_iccid(iccid);
        profileOperationEvent.set_is_enable(isEnable);
        LOG(DEBUG, __FUNCTION__, " referenceId: ", referenceId, " iccid: ", iccid,
            " isEnable: ", isEnable);
        isProfileEnable_ = isEnable;
        profileIccid_ = iccid;
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter(telux::tel::TEL_AP_SIM_PROFILE_FILTER);
    anyResponse.mutable_any()->PackFrom(profileOperationEvent);
    auto f = std::async(std::launch::async, [this, anyResponse]() {
        this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void ApSimProfileManagerServerImpl::triggerChangeEvent(
    ::eventService::EventResponse anyResponse) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void ApSimProfileManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == telux::tel::TEL_AP_SIM_PROFILE_FILTER) {
        std::string event = message.event();
        onEventUpdate(event);
    }
}

void ApSimProfileManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__," Event: ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__," Token: ", token );
    if (PROFILE_LIST_REQUEST == token) {
        handleProfileListRequest(event);
    } else if (PROFILE_OPERATION_REQUEST == token) {
        handleProfileOperationRequest(event);
    } else {
        LOG(ERROR, __FUNCTION__, " Event not supported");
    }
}
