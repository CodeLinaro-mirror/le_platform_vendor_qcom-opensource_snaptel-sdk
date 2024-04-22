/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "BridgeServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#define BRIDGE_MANAGER_API_LOCAL_JSON "api/data/IBridgeManager.json"

BridgeServerImpl::BridgeServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

BridgeServerImpl::~BridgeServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status BridgeServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request,
    dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = BRIDGE_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IBridgeManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IBridgeManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}