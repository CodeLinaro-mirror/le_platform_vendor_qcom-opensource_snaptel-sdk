/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DeviceInfoManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#define DEVICE_INFO_MANAGER_API_JSON "api/platform/IDeviceInfoManager.json"
#define DEVICE_INFO_MANAGER_SYSTEM_INFO_JSON "system-info/platform/IDeviceInfoManager.json"
#define META_BUILD_VER_INFO_FILE "system-info/platform/version_info.json"


DeviceInfoManagerServerImpl::DeviceInfoManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

DeviceInfoManagerServerImpl::~DeviceInfoManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
}

grpc::Status DeviceInfoManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, platformStub::GetServiceStatusReply* response) {
    LOG(DEBUG,__FUNCTION__);

    int cbDelay;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;

    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, DEVICE_INFO_MANAGER_API_JSON);

    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["IDeviceInfoManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["IDeviceInfoManager"]["IsSubsystemReady"].asString();
        serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
    } else {
        LOG(ERROR, "Unable to read DeviceInfoManager JSON");
    }

    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status DeviceInfoManagerServerImpl::GetPlatformVersion(ServerContext* context,
const google::protobuf::Empty* request, platformStub::PlatformVersionInfo* response){
    LOG(DEBUG,__FUNCTION__);

    std::string apiJsonPath = DEVICE_INFO_MANAGER_API_JSON;
    std::string systemInfoJsonPath = META_BUILD_VER_INFO_FILE;
    std::string subsystem = "IDeviceInfoManager";
    std::string method = "GetPlatformVersion";
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, systemInfoJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    std::string modem;
    std::string meta_build_id;
    std::string apps_fsl;
    std::string apps;

    if (data.status == telux::common::Status::SUCCESS) {
        modem = data.stateRootObj["Image_Build_IDs"]["modem"].asString();
        meta_build_id = data.stateRootObj["Metabuild_Info"]["Meta_Build_ID"].asString();
        apps_fsl = data.stateRootObj["Image_Build_IDs"]["apps_fsl"].asString();
        apps = data.stateRootObj["Image_Build_IDs"]["apps"].asString();
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);
    response->set_modem_details(modem);
    response->set_meta_details(meta_build_id);
    response->set_external_app(apps_fsl);
    response->set_integrated_app(apps);

    return grpc::Status::OK;
}

grpc::Status DeviceInfoManagerServerImpl::GetIMEI(ServerContext* context,
const google::protobuf::Empty* request, platformStub::PlatformImeiInfo* response){
    LOG(DEBUG,__FUNCTION__);

    std::string apiJsonPath = DEVICE_INFO_MANAGER_API_JSON;
    std::string systemInfoJsonPath = DEVICE_INFO_MANAGER_SYSTEM_INFO_JSON;
    std::string subsystem = "IDeviceInfoManager";
    std::string method = "GetIMEI";
    std::string imei;
    JsonData data;

    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, systemInfoJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        imei = data.stateRootObj["IDeviceInfoManager"]["GetIMEI"]["imei"].asString();
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);
    response->set_imei_info(imei);

    return grpc::Status::OK;
}