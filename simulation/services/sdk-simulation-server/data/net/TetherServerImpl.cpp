/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "TetherServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/CommonUtils.hpp"

#define BT_MANAGER_API_JSON "api/data/ITetherManager.json"
#define BT_MANAGER_STATE_JSON "api/system-state/ITetherManagerState.json"

TetherServerImpl::TetherServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

TetherServerImpl::~TetherServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status TetherServerImpl::InitService(grpc::ServerContext* context, const google::protobuf::Empty* request, dataStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;
    std::string filePath = BT_MANAGER_API_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["ITetherManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus = rootObj["ITetherManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status TetherServerImpl::GetServiceStatus(grpc::ServerContext* context, const google::protobuf::Empty *request, dataStub::GetServiceStatusReply *response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;
    std::string filePath = BT_MANAGER_API_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["ITetherManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus = rootObj["ITetherManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status TetherServerImpl::StartBTTether(grpc::ServerContext* context, const dataStub::BTTetherMode* request, dataStub::DefaultReply* response) {
    LOG(DEBUG, __FUNCTION__);
    
    Json::Value rootObj;
    std::string filePath = BT_MANAGER_API_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    Json::Value stateObj;
    std::string stateFilePath = BT_MANAGER_STATE_JSON;
    error = JsonParser::readFromJsonFile(stateObj, stateFilePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    if(rootObj["ITetherManager"]["StartBTTether"]["status"].asString() == "SUCCESS") {
        stateObj["ITetherManager"]["BTTether"]["Status"] = convertBTTetherStatusEnumtoString(dataStub::BTTetherStatus::UP);
        stateObj["ITetherManager"]["BTTether"]["Mode"] = convertBTTetherModeEnumtoString(request->mode());
    }

    JsonParser::writeToJsonFile(stateObj, stateFilePath);

    int cbDelay = rootObj["ITetherManager"]["DefaultCallbackDelay"].asInt();

    response->set_error(static_cast<commonStub::ErrorCode>(CommonUtils::mapErrorCode(rootObj["ITetherManager"]["StartBTTether"]["error"].asString())));
    response->set_status(static_cast<commonStub::Status>(CommonUtils::mapStatus(rootObj["ITetherManager"]["StartBTTether"]["status"].asString())));
    response->set_delay(cbDelay);
    
    return grpc::Status::OK;
}

grpc::Status TetherServerImpl::StopBTTether(grpc::ServerContext* context, const google::protobuf::Empty* request, dataStub::DefaultReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;
    std::string filePath = BT_MANAGER_API_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    Json::Value stateObj;
    std::string stateFilePath = BT_MANAGER_STATE_JSON;
    error = JsonParser::readFromJsonFile(stateObj, stateFilePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    if(rootObj["ITetherManager"]["StopBTTether"]["status"].asString() == "SUCCESS") {
        stateObj["ITetherManager"]["BTTether"]["Status"] = convertBTTetherStatusEnumtoString(dataStub::BTTetherStatus::DOWN);
    }

    JsonParser::writeToJsonFile(stateObj, stateFilePath);

    int cbDelay = rootObj["ITetherManager"]["DefaultCallbackDelay"].asInt();

    response->set_error(static_cast<commonStub::ErrorCode>(CommonUtils::mapErrorCode(rootObj["ITetherManager"]["StopBTTether"]["error"].asString())));
    response->set_status(static_cast<commonStub::Status>(CommonUtils::mapStatus(rootObj["ITetherManager"]["StopBTTether"]["status"].asString())));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status TetherServerImpl::RequestBTTetherStatus(grpc::ServerContext* context, const google::protobuf::Empty* request, dataStub::BTTetherStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);

    Json::Value rootObj;
    std::string filePath = BT_MANAGER_API_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["ITetherManager"]["IsSubsystemReadyDelay"].asInt();

    Json::Value stateObj;
    std::string stateFilePath = BT_MANAGER_STATE_JSON;
    error = JsonParser::readFromJsonFile(stateObj, stateFilePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    dataStub::BTTetherMode::Mode tetherMode = dataStub::BTTetherMode::LAN;
    dataStub::BTTetherStatus::Status tetherStatus = dataStub::BTTetherStatus::DOWN;

    if(rootObj["ITetherManager"]["RequestBTTetherStatus"]["Status"].asString() == "SUCCESS") {
        tetherStatus = convertBTTetherStatusStringtoEnum(stateObj["ITetherManager"]["BTTether"]["Status"].asString());
        tetherMode = convertBTTetherModeStringtoEnum(stateObj["ITetherManager"]["BTTether"]["Mode"].asString());
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(CommonUtils::mapStatus(rootObj["ITetherManager"]["RequestBTTetherStatus"]["status"].asString())));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(CommonUtils::mapErrorCode(rootObj["ITetherManager"]["RequestBTTetherStatus"]["error"].asString())));
    response->mutable_reply()->set_delay(cbDelay);
    response->mutable_bt_tether_status()->set_status(tetherStatus);
    response->mutable_bt_tether_mode()->set_mode(tetherMode);

    return grpc::Status::OK;
}

std::string TetherServerImpl::convertBTTetherStatusEnumtoString(dataStub::BTTetherStatus::Status status) {
    if(status == dataStub::BTTetherStatus::UP) {
        return "UP";
    }
    else if(status == dataStub::BTTetherStatus::DOWN) {
        return "DOWN";
    }
    return "DOWN";
}

dataStub::BTTetherStatus::Status TetherServerImpl::convertBTTetherStatusStringtoEnum(std::string status) {
    if(status == "UP") {
        return dataStub::BTTetherStatus::UP;
    }
    else if(status == "DOWN") {
        return dataStub::BTTetherStatus::DOWN;
    }
    return dataStub::BTTetherStatus::DOWN;
}

std::string TetherServerImpl::convertBTTetherModeEnumtoString(dataStub::BTTetherMode::Mode status) {
    if(status == dataStub::BTTetherMode::LAN) {
        return "LAN";
    }
    else if(status == dataStub::BTTetherMode::WAN) {
        return "WAN";
    }
    return "LAN";
}

dataStub::BTTetherMode::Mode TetherServerImpl::convertBTTetherModeStringtoEnum(std::string status) {
    if(status == "LAN") {
        return dataStub::BTTetherMode::LAN;
    }
    else if(status == "WAN") {
        return dataStub::BTTetherMode::WAN;
    }
    return dataStub::BTTetherMode::LAN;
}
