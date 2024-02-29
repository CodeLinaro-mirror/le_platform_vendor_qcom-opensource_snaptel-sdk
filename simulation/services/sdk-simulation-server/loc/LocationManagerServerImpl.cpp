/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       LocationManagerServerImpl.hpp
 *
 *
 */

#include "LocationManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#define LOC_MGR_API_JSON "api/loc/ILocationManager.json"

LocationManagerServerImpl::LocationManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

LocationManagerServerImpl::~LocationManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
}

grpc::Status LocationManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, locStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int cbDelay = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ILocationManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["ILocationManager"]["IsSubsystemReady"].asString();
        serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
    } else {
        LOG(ERROR, "Unable to read LocationManager JSON");
    }
    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    response->set_delay(cbDelay);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StartBasicReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startBasicReports", response);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StartDetailedReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startDetailedReports", response);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StartDetailedEngineReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startDetailedEngineReports", response);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::RegisterLocationSystemInfo(ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("registerForSystemInfoUpdates", response);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::DeregisterLocationSystemInfo(ServerContext* context,
      const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("deRegisterForSystemInfoUpdates", response);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::GetTerrestrialPosition(ServerContext* context,
      const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("getTerrestrialPosition", response);
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::CancelTerrestrialPosition(ServerContext* context,
      const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("cancelTerrestrialPositionRequest", response);
    return grpc::Status::OK;
}

void LocationManagerServerImpl::apiJsonReader(
    std::string apiName, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationManager", apiName, status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
}

grpc::Status LocationManagerServerImpl::RequestEnergyConsumedInfo(ServerContext* context,
    const google::protobuf::Empty* request, locStub::RequestEnergyConsumedInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationManager", "requestEnergyConsumedInfo",
        status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    if (errorCode == ErrorCode::SUCCESS) {
        uint32_t validity =
            std::stoi(telux::common::CommonUtils::readSystemDataValue("loc/ILocationManager",
                "0", {"ILocationManager", "GnssEnergyConsumedInfo", "valid"}));

        uint32_t energyConsumed =
            std::stoi(telux::common::CommonUtils::readSystemDataValue("loc/ILocationManager",
                "0", {"ILocationManager", "GnssEnergyConsumedInfo", "energySinceFirstBoot"}));

        {
            CommonUtils::writeSystemDataValue("loc/ILocationManager", "1",
                {"ILocationManager", "GnssEnergyConsumedInfo", "valid"});
            CommonUtils::writeSystemDataValue("loc/ILocationManager",
                std::to_string(energyConsumed + 100),
                    {"ILocationManager", "GnssEnergyConsumedInfo", "energySinceFirstBoot"});
        }
        response->set_validity(validity);
        response->set_energy_consumed(energyConsumed);
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::GetYearOfHw(ServerContext* context,
    const google::protobuf::Empty* request, locStub::GetYearOfHwReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationManager", "getYearOfHw",
        status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    if (errorCode == ErrorCode::SUCCESS) {
        uint16_t yearOfHw = std::stoi(telux::common::CommonUtils::readSystemDataValue(
            "loc/ILocationManager", "0", {"ILocationManager", "yearOfHw"}));
        if (yearOfHw == 0) {
            yearOfHw = 2023;
            CommonUtils::writeSystemDataValue("loc/ILocationManager", std::to_string(yearOfHw),
                {"ILocationManager", "yearOfHw"});
        }
        response->set_year_of_hw(yearOfHw);
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::GetCapabilities(ServerContext* context,
    const google::protobuf::Empty* request, locStub::GetCapabilitiesReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_MGR_API_JSON);
    uint32_t capabilities = rootNode["ILocationManager"]["getCapabilities"]["capabilities"].asInt();
    response->set_loc_capability(capabilities);
    return grpc::Status::OK;
}