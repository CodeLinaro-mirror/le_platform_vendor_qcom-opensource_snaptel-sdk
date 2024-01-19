/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#include"ImsServingManagerServerImpl.hpp"

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#include <telux/tel/ImsServingSystemManager.hpp>
#include <telux/tel/PhoneDefines.hpp>

#define JSON_PATH1 "api/tel/IImsServingSystemManagerSlot1.json"
#define JSON_PATH2 "api/tel/IImsServingSystemManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/IImsServingSystemManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/IImsServingSystemManagerStateSlot2.json"
#define MANAGER "IImsServingSystemManager"
#define SLOT_1 1
#define SLOT_2 2

ImsServingManagerServerImpl::ImsServingManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status ImsServingManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_IMS_SERVING_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    std::string srvStatus = rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::RequestRegistrationInfo(ServerContext* context,
    const ::telStub::RequestRegistrationInfoRequest* request,
    telStub::RequestRegistrationInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestRegistrationInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        telStub::RegistrationStatus regStatus =
                static_cast<telStub::RegistrationStatus>(data.stateRootObj[MANAGER]\
                ["ImsRegistrationInfo"]["RegStatus"].asInt());
            telStub::RadioTechnology rat =
                static_cast<telStub::RadioTechnology>(data.stateRootObj[MANAGER]\
                ["ImsRegistrationInfo"]["rat"].asInt());
            int errorCode = data.stateRootObj[MANAGER]["ImsRegistrationInfo"]["errorCode"].asInt();
            std::string errorString =
                data.stateRootObj[MANAGER]["ImsRegistrationInfo"]["errorString"].asString();
            response->set_ims_reg_status(regStatus);
            response->set_rat(rat);
            response->set_error_code(errorCode);
            response->set_error_string(errorString);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::RequestServiceInfo(ServerContext* context,
    const ::telStub::RequestServiceInfoRequest* request,
    telStub::RequestServiceInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->slot_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestServiceInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        telStub::CellularService_Status sms =
            static_cast<telStub::CellularService_Status>(data.stateRootObj[MANAGER]\
                ["ImsServiceInfo"]["sms"].asInt());
        telStub::CellularService_Status voice =
            static_cast<telStub::CellularService_Status>(data.stateRootObj[MANAGER]\
                ["ImsServiceInfo"]["voice"].asInt());
        response->set_sms(sms);
        response->set_voice(voice);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ImsServingManagerServerImpl::RequestPdpStatus(ServerContext* context,
    const ::telStub::RequestPdpStatusRequest* request,
    telStub::RequestPdpStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestPdpStatus";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        bool value = data.stateRootObj[MANAGER]["ImsPdpStatusInfo"]["isPdpConnected"].asBool();
        telStub::PdpFailureCode pdpFailure =
            static_cast<telStub::PdpFailureCode>(data.stateRootObj[MANAGER]\
            ["ImsPdpStatusInfo"]["failureCode"].asInt());
        telStub::EndReasonType dataCallEndReason =
            static_cast<telStub::EndReasonType>(data.stateRootObj[MANAGER]
            ["ImsPdpStatusInfo"]["failureReason"].asInt());
        std::string apnName = data.stateRootObj[MANAGER]["ImsPdpStatusInfo"]["apnName"].asString();
        response->set_is_pdp_connected(value);
        response->set_failure_code(pdpFailure);
        response->set_failure_reason(dataCallEndReason);
        response->set_apn_name(apnName);

    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void ImsServingManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    LOG(DEBUG, __FUNCTION__, "Not Supported");
}
