/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <fstream>
#include <sstream>
#include <vector>
#include <utility>

#define LOC_MGR_API_JSON "api/loc/ILocationManager.json"
#define CSV_BATCH_COUNT 1000

LocationManagerServerImpl::LocationManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    init();
}

inline bool fileExists(const std::string &csvFile) {
    std::ifstream f(csvFile.c_str());
    return f.good();
}

void LocationManagerServerImpl::init() {
    LOG(DEBUG, __FUNCTION__);
    SimulationConfigParser configParser;
    std::string fileName = configParser.getValue("sim.loc.location_report_file_name");
    std::string filePath = std::string(DEFAULT_SIM_CSV_FILE_PATH) + fileName;
    if (!fileExists(filePath)) {
        filePath = std::string(DEFAULT_SIM_FILE_PREFIX)
            + std::string(DEFAULT_SIM_CSV_FILE_PATH) + fileName;
        if (!fileExists(filePath)) {
            LOG(DEBUG, __FUNCTION__ , " Failed to open CSV");
            return;
        }
    }
    fileBuffer_ = std::make_shared<FileBuffer>(filePath, CSV_BATCH_COUNT);
    fileBuffer_->startBuffering();
    bufferingInitialized_ = true;
    streamRequestCount_.store(0);
}

void LocationManagerServerImpl::startStreaming() {
    LOG(DEBUG, __FUNCTION__);
    while(true) {
        if(fileBuffer_->getNextBuffer(requestBuffer_)) {
            while(!requestBuffer_.empty()) {
                //Send requestBuffer_[0] to clients via streams

                // Sleep to match the frequency by extracting the current timestamp
                // and subtracting from the previous.
                std::size_t pos = requestBuffer_[0].find(',');
                uint64_t currentTimestamp = std::stoull(requestBuffer_[0].substr(0, pos));
                if(previousTimestamp_ != 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(currentTimestamp - previousTimestamp_));
                }
                previousTimestamp_ = currentTimestamp;
                requestBuffer_.erase(requestBuffer_.begin());
            }
        } else {
            //EOF is reached and request buffer is empty.
            previousTimestamp_ = 0;
            LOG(DEBUG, " Last batch streamed. Streaming stopped.");
            break;
        }

        // Stop Stream on Request as per config. Will be checked for last client on stop reports.
        if(stopStreamingData_) {
            LOG(DEBUG, " Last client de-registered. Streaming stopped.");
            break;
        }
    }
}

LocationManagerServerImpl::~LocationManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
    if(fileBuffer_) {
        fileBuffer_->cleanup();
    }
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

void LocationManagerServerImpl::updateStreamRequest() {
    if(bufferingInitialized_) {
        if(streamRequestCount_ == 0) {
            //Initializing/Resetting the flag.
            stopStreamingData_ = false;
            //Starting the stream.
            auto f = std::async(std::launch::async,
                [=]() {
                    this->startStreaming();
                }).share();
            taskQ_.add(f);
        }
        streamRequestCount_++;
    }
}

grpc::Status LocationManagerServerImpl::StartBasicReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startBasicReports", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        updateStreamRequest();
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StartDetailedReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startDetailedReports", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        updateStreamRequest();
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StartDetailedEngineReports(ServerContext* context,
    const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("startDetailedEngineReports", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        updateStreamRequest();
    }
    return grpc::Status::OK;
}

grpc::Status LocationManagerServerImpl::StopReports(ServerContext* context,
    const google::protobuf::Empty* request, google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    if(bufferingInitialized_) {
        streamRequestCount_--;
        if(streamRequestCount_ == 0) {
            SimulationConfigParser configParser;
            std::string stopStreamStr = configParser.getValue("sim.loc.location_report_consumption");
            if(stopStreamStr == "TRUE") {
                stopStreamingData_ = true;
            }
        }
    }
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
