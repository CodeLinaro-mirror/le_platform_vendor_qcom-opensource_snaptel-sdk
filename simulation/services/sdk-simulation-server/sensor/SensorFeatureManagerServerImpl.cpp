/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFeatureManagerServerImpl.hpp
 *
 *
 */


#include "SensorFeatureManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include <telux/common/CommonDefines.hpp>
#include "event/EventService.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "FileInfo.hpp"
#include <fstream>
#include <sstream>

#define SENSOR_FEATURE_MGR_API_JSON "api/sensor/ISensorFeatureManager.json"
#define SENSOR_FEATURE_INFO_JSON  "system-info/sensor/ISensorFeatureManager.json"
#define SUPPORTED_SENSOR_JSON "api/sensor/SupportedSensors.json"
#define DEFAULT_DELIMITER " "

SensorFeatureManagerServerImpl::SensorFeatureManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

SensorFeatureManagerServerImpl::~SensorFeatureManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__ , " Destructing");
}

grpc::Status SensorFeatureManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, sensorStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int cbDelay = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_MGR_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ISensorFeatureManager"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["ISensorFeatureManager"]["IsSubsystemReady"].asString();
        serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
        try{
            errorCode = JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_INFO_JSON);
            if (errorCode == ErrorCode::SUCCESS) {
                unsigned int numOfSensors = rootNode["features"].size();
                /* As Json::Value::ArrayIndex is a typedef of unsigned int. */
                for (Json::Value::ArrayIndex i = 0; i < numOfSensors; i++) {
                    std::string feature =  rootNode["features"][i].asString();
                    featureStatusMap_[feature] = false;
                }
            }
        } catch(std::exception const & ex) {
            LOG(DEBUG, "Exception Occur ", ex.what());
        }
    } else {
        LOG(ERROR, " Unable to read SensorFeatureManager JSON");
    }
    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    if(serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {"sensor_feature"};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_delay(cbDelay);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
    return grpc::Status::OK;
}

void SensorFeatureManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event){
    LOG(DEBUG, __FUNCTION__);
    if (event.filter() == "sensor_feature") {
        onEventUpdate(event.event());
    }
}

void SensorFeatureManagerServerImpl::onEventUpdate(std::string event){
    LOG(DEBUG, __FUNCTION__,event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    handleEvent(token,event);
}

void SensorFeatureManagerServerImpl::handleEvent(std::string token , std::string event){
    LOG(DEBUG, __FUNCTION__, " The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, " The leftover string is: ", event);
    if (token == "sensor_event") {
        handleFeatureEvent(event);
    } else if (token == "motion_detection_event") {
        handleMotionDetectionEvent(event);
    }
}

void SensorFeatureManagerServerImpl::handleFeatureEvent(std::string eventParams){
   LOG(DEBUG, __FUNCTION__);
    std::string featureName = "";
    int eventId = -1;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " The featureName is not passed");
    } else {
        featureName = token;
        if(featureStatusMap_.find(featureName) == featureStatusMap_.end()){
           LOG(INFO, __FUNCTION__, " The featureName not exists");
           return;
        }
        if(!featureStatusMap_[featureName]){
            LOG(INFO, __FUNCTION__, " Feature not enabled");
            return;
        }
    }
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
     if(token == "") {
        LOG(INFO, __FUNCTION__, " The eventId is not passed");
    } else {
        eventId =std::stoi(token);
    }
    std::string eventString =
        readBufferedEventStringFromFile("sim.sensor.sensor_buffered_events_file_name",eventId);
    if(eventString != "") {
        auto f = std::async(std::launch::async, [this,featureName,eventId,eventString](){
            this->triggerFeatureEvent(featureName,eventId,eventString);
        }).share();
        taskQ_->add(f);
    }
}

void SensorFeatureManagerServerImpl::triggerFeatureEvent(std::string featureName,
    int id, std::string eventString){
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(mtx_);
    ::sensorStub::FeatureEvent featureEvent;
    ::eventService::EventResponse anyResponse;
    featureEvent.set_id(id);
    featureEvent.set_featurename(featureName);
    featureEvent.set_events(eventString);
    anyResponse.set_filter("sensor_feature");
    anyResponse.mutable_any()->PackFrom(featureEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

inline bool fileExists(const std::string &csvFile) {
    std::ifstream f(csvFile.c_str());
    return f.good();
}

std::string SensorFeatureManagerServerImpl::readBufferedEventStringFromFile(std::string fileName,
    int eventId){
    LOG(DEBUG, __FUNCTION__);
    bool eventFound=false;
    std::shared_ptr<SimulationConfigParser> configParser =
        std::make_shared<SimulationConfigParser>();
    std::string file = configParser->getValue(fileName);
    std::string csvFilePath = std::string(DEFAULT_SIM_CSV_FILE_PATH) + file;
    if (!fileExists(csvFilePath)) {
        csvFilePath = std::string(DEFAULT_SIM_FILE_PREFIX)
            + std::string(DEFAULT_SIM_CSV_FILE_PATH) + file;
        if (!fileExists(csvFilePath)) {
            LOG(ERROR, __FUNCTION__, "file not exists: ", csvFilePath);
            return "";
        }
    }
    std::ifstream ifs(csvFilePath);
    if(!ifs.is_open()) {
        LOG(ERROR, __FUNCTION__, "Could not open the file: ", csvFilePath);
        return "";
    }

    if(ifs.good()) {
        LOG(DEBUG,__FUNCTION__, " Begin Reading ", csvFilePath);
    }
    std::string line;
    // skip the copyright
    while (ifs.peek() != EOF) {
        std::getline(ifs, line);
        // each line of copyright starts with "##"
        if (line.size() != 0 && line.find("##") != 0) {
            break;
        }
    }

    while (false == eventFound) {
        std::size_t pos = line.find(',');
        if (pos != std::string::npos) {
            std::string id = line.substr(0, pos);
            if(std::stoi(id) == eventId){
                eventFound = true;
                break;
            }
        }
        if (ifs.peek() == EOF) {
            break;
        }
        std::getline(ifs, line);
    }

    if(eventFound == false){
       LOG(ERROR, __FUNCTION__, "EventId not Found in file: ", csvFilePath);
       return "";
    }
    return line;
}

grpc::Status SensorFeatureManagerServerImpl::EnableFeature(ServerContext* context,
    const sensorStub::SensorEnableFeature* request,
    sensorStub::SensorFeatureManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("enableFeature", response);
    if (response->status() == ::commonStub::Status::SUCCESS) {
        if(featureStatusMap_.find(request->feature()) == featureStatusMap_.end()){
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,"feature not exists");
        }
        featureStatusMap_[request->feature()] = true;
    }
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::DisableFeature(ServerContext* context,
    const sensorStub::SensorEnableFeature* request,
    sensorStub::SensorFeatureManagerCommandReply* response){
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("disableFeature", response);
    if (response->status() == ::commonStub::Status::SUCCESS) {
        if(featureStatusMap_.find(request->feature()) == featureStatusMap_.end()){
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,"feature not exists");
        }
        featureStatusMap_[request->feature()] = false;
    }
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::GetFeatureList(ServerContext* context,
    const google::protobuf::Empty* request, sensorStub::GetFeatureListReply* response){
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    telux::common::Status status;
    int cbDelay = 100;
    std::string featureList = "";
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_MGR_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay = rootNode["ISensorFeatureManager"]["DefaultCallbackDelay"].asInt();
        std::string cbStatus = rootNode["ISensorFeatureManager"]["getAvailableFeatures"]
            ["status"].asString();
        status = CommonUtils::mapStatus(cbStatus);
        for (auto const& x : featureStatusMap_) {
            featureList = featureList + x.first + ",";
        }
        featureList.pop_back();
    } else {
        LOG(ERROR, " Unable to read SensorFeatureManager JSON");
    }
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_delay(cbDelay);
    response->set_list(featureList);
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::GetSensorList(ServerContext* context,
    const google::protobuf::Empty* request, sensorStub::SensorInfoResponse* response) {
    LOG(DEBUG, __FUNCTION__);
    updateSensorInfo();
    for (const auto& dataStruct : sensorInfo_) {
        sensorStub::SensorInfo* data = response->add_sensor_info();
        data->set_id(dataStruct.id);
        data->set_sensor_type(static_cast<uint32_t>(dataStruct.type));
        data->set_name(dataStruct.name);
        data->set_vendor(dataStruct.vendor);
        for (const auto& samplingRate : dataStruct.samplingRates) {
            data->add_sampling_rates(samplingRate);
        }
        data->set_max_sampling_rate(dataStruct.maxSamplingRate);
        data->set_max_batch_count_supported(dataStruct.maxBatchCountSupported);
        data->set_min_batch_count_supported(dataStruct.minBatchCountSupported);
        data->set_range(dataStruct.range);
        data->set_version(dataStruct.version);
        data->set_resolution(dataStruct.resolution);
        data->set_max_range(dataStruct.maxRange);
    }
    sensorInfo_.clear();
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::GetMotionDetectionLimits(ServerContext* context,
    const sensorStub::MotionDetectionRequest* request,
    sensorStub::MotionDetectionLimitsReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status;
    uint32_t sensorId = request->sensor_id();
    if(sensorId != 1) {
        LOG(ERROR, __FUNCTION__, " Unsupported SensorId");
        status = telux::common::Status::NOTSUPPORTED;
    } else {
        Json::Value rootNode;
        JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_MGR_API_JSON);
        status = telux::common::Status::SUCCESS;
        float minThreshold = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["minThreshold"].asFloat();
        float maxThreshold = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["maxThreshold"].asFloat();
        int minDuration = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["minDuration"].asInt();
        int maxDuration = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["maxDuration"].asInt();
        float minSamplingRate = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["minSamplingRate"].asFloat();
        float maxSamplingRate = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["maxSamplingRate"].asFloat();
        response->set_min_threshold(minThreshold);
        response->set_max_threshold(maxThreshold);
        response->set_min_duration(minDuration);
        response->set_max_duration(maxDuration);
        response->set_min_samplingrate(minSamplingRate);
        response->set_max_samplingrate(maxSamplingRate);
    }

    response->set_status(static_cast<::commonStub::Status>(status));
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::EnableMotionDetection(ServerContext* context,
    const sensorStub::EnableMotionDetectionRequest* request,
    sensorStub::SensorFeatureManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status;
    uint32_t sensorId = request->sensor_id();
    if(sensorId != 1) {
        LOG(ERROR, __FUNCTION__, " Unsupported SensorId");
        status = telux::common::Status::NOTSUPPORTED;
    } else {
        if(motionDetectionEnabled_) {
            status = telux::common::Status::ALREADY;
            triggerMotionDetectionEnabledEvent();
        } else {
            if(checkMotionConfigLimits(request)) {
                float threshold = request->threshold();
                uint32_t duration = request->duration();
                float samplingRate = request->sampling_rate();
                status = telux::common::Status::SUCCESS;
                motionDetectionEnabled_ = true;
                CommonUtils::writeSystemDataValue<string>("sensor/ISensorFeatureManager",
                    std::to_string(sensorId),
                        {"ISensorFeatureManager", "MotionDetection", "sensorId"});
                CommonUtils::writeSystemDataValue<string>("sensor/ISensorFeatureManager",
                    std::to_string(threshold),
                        {"ISensorFeatureManager", "MotionDetection", "threshold"});
                CommonUtils::writeSystemDataValue<string>("sensor/ISensorFeatureManager",
                    std::to_string(duration),
                        {"ISensorFeatureManager", "MotionDetection", "duration"});
                CommonUtils::writeSystemDataValue<string>("sensor/ISensorFeatureManager",
                    std::to_string(samplingRate),
                        {"ISensorFeatureManager", "MotionDetection", "samplingRate"});
                triggerMotionDetectionEnabledEvent();
            } else {
                status = telux::common::Status::INVALIDPARAM;
            }
        }
    }
    response->set_status(static_cast<::commonStub::Status>(status));
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::DisableMotionDetection(ServerContext* context,
    const sensorStub::MotionDetectionRequest* request,
    sensorStub::SensorFeatureManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status;
    uint32_t sensorId = request->sensor_id();
    if(sensorId != 1) {
        LOG(ERROR, __FUNCTION__, " Unsupported SensorId");
        status = telux::common::Status::NOTSUPPORTED;
    } else {
        if(!motionDetectionEnabled_) {
            status = telux::common::Status::ALREADY;
        } else {
            motionDetectionEnabled_ = false;
            status = telux::common::Status::SUCCESS;
            triggerMotionDetectionDisabledEvent();
        }
    }
    response->set_status(static_cast<::commonStub::Status>(status));
    return grpc::Status::OK;
}

grpc::Status SensorFeatureManagerServerImpl::GetMotionDetectionConfig(ServerContext* context,
    const google::protobuf::Empty* request,
    sensorStub::MotionDetectionConfigReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status;
    if(!motionDetectionEnabled_) {
        status = telux::common::Status::NOSUCH;
    } else {
        status = telux::common::Status::SUCCESS;
        int sensorId = std::stoi(CommonUtils::readSystemDataValue(
            "sensor/ISensorFeatureManager", "0",
                {"ISensorFeatureManager", "MotionDetection", "sensorId"}));
        float threshold = std::stof(CommonUtils::readSystemDataValue(
            "sensor/ISensorFeatureManager", "0",
                {"ISensorFeatureManager", "MotionDetection", "threshold"}));
        int duration = std::stoi(CommonUtils::readSystemDataValue(
            "sensor/ISensorFeatureManager", "0",
                {"ISensorFeatureManager", "MotionDetection", "duration"}));
        float samplingRate = std::stof(CommonUtils::readSystemDataValue(
            "sensor/ISensorFeatureManager", "0",
                {"ISensorFeatureManager", "MotionDetection", "samplingRate"}));
        response->set_sensor_id(sensorId);
        response->set_threshold(threshold);
        response->set_duration(duration);
        response->set_sampling_rate(samplingRate);
    }
    response->set_status(static_cast<::commonStub::Status>(status));
    return grpc::Status::OK;
}

void SensorFeatureManagerServerImpl::handleMotionDetectionEvent(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    if(!motionDetectionEnabled_) {
        return;
    }
    uint64_t eventId;
    uint64_t timestamp;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, " The eventId is not passed");
        return;
    } else {
        eventId = std::stoull(token);
    }
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
     if(token == "") {
        LOG(INFO, __FUNCTION__, " The timestamp is not passed");
    } else {
        timestamp = std::stoull(token);
    }
    auto f = std::async(std::launch::async, [this, eventId, timestamp](){
        this->triggerMotionDetectionEvent(eventId, timestamp);
    }).share();
    taskQ_->add(f);
}

void SensorFeatureManagerServerImpl::triggerMotionDetectionEvent(
    uint64_t eventId, uint64_t timestamp) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(mtx_);
    ::sensorStub::MotionDetectionEvent motionDetectionEvent;
    ::eventService::EventResponse anyResponse;
    int sensorId = std::stoi(CommonUtils::readSystemDataValue(
        "sensor/ISensorFeatureManager", "0",
            {"ISensorFeatureManager", "MotionDetection", "sensorId"}));
    motionDetectionEvent.set_sensor_id(sensorId);
    motionDetectionEvent.set_event_id(eventId);
    motionDetectionEvent.set_timestamp(timestamp);
    anyResponse.set_filter("sensor_feature");
    anyResponse.mutable_any()->PackFrom(motionDetectionEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void SensorFeatureManagerServerImpl::triggerMotionDetectionEnabledEvent() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(mtx_);
    ::sensorStub::MotionDetectionEnabledEvent motionDetectionEnabledEvent;
    ::eventService::EventResponse anyResponse;
    int sensorId = std::stoi(CommonUtils::readSystemDataValue(
        "sensor/ISensorFeatureManager", "0",
            {"ISensorFeatureManager", "MotionDetection", "sensorId"}));
    float threshold = std::stof(CommonUtils::readSystemDataValue(
        "sensor/ISensorFeatureManager", "0",
            {"ISensorFeatureManager", "MotionDetection", "threshold"}));
    int duration = std::stoi(CommonUtils::readSystemDataValue(
        "sensor/ISensorFeatureManager", "0",
            {"ISensorFeatureManager", "MotionDetection", "duration"}));
    float samplingRate = std::stof(CommonUtils::readSystemDataValue(
        "sensor/ISensorFeatureManager", "0",
            {"ISensorFeatureManager", "MotionDetection", "samplingRate"}));
    motionDetectionEnabledEvent.set_sensor_id(sensorId);
    motionDetectionEnabledEvent.set_threshold(threshold);
    motionDetectionEnabledEvent.set_duration(duration);
    motionDetectionEnabledEvent.set_sampling_rate(samplingRate);
    anyResponse.set_filter("sensor_feature");
    anyResponse.mutable_any()->PackFrom(motionDetectionEnabledEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void SensorFeatureManagerServerImpl::triggerMotionDetectionDisabledEvent() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(mtx_);
    ::sensorStub::MotionDetectionDisabledEvent motionDetectionDisabledEvent;
    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter("sensor_feature");
    anyResponse.mutable_any()->PackFrom(motionDetectionDisabledEvent);
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

telux::sensor::SensorType SensorFeatureManagerServerImpl::getSensorType(std::string sensorType) {
    LOG(DEBUG,__FUNCTION__);
    telux::sensor::SensorType type = telux::sensor::SensorType::INVALID;
    if (sensorType == "Accelerometer") {
        type = telux::sensor::SensorType::ACCELEROMETER;
    } else if (sensorType == "Gyroscope") {
        type = telux::sensor::SensorType::GYROSCOPE;
    } else if (sensorType == "Accelerometer_Uncalibrated") {
        type = telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED;
    } else if (sensorType == "Gyroscope_Uncalibrated") {
        type = telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED;
    }
    return type;
}

void SensorFeatureManagerServerImpl::updateSensorInfo() {
    LOG(DEBUG,__FUNCTION__);
    try{
        Json::Value rootNode;
        telux::common::ErrorCode errorCode
            = JsonParser::readFromJsonFile(rootNode, SUPPORTED_SENSOR_JSON);
        if (errorCode == ErrorCode::SUCCESS) {
            unsigned int numOfSensors = rootNode["sensors"].size();
        /* As Json::Value::ArrayIndex is a typedef of unsigned int. */
            for (Json::Value::ArrayIndex i = 0; i < numOfSensors; i++) {
                telux::sensor::SensorInfo info;
                info.id = std::stoi(rootNode["sensors"][i]["id"].asString());
                info.type = getSensorType(
                rootNode["sensors"][i]["sensor_type"].asString());
                info.name = rootNode["sensors"][i]["sensor_name"].asString();
                info.vendor = rootNode["sensors"][i]["vendor"].asString();
                for (Json::Value::ArrayIndex j = 0;
                    j < rootNode["sensors"][i]["sampling_rate"].size(); j++) {
                    info.samplingRates.push_back(
                    rootNode["sensors"][i]["sampling_rate"][j].asFloat());
                }
                info.maxSamplingRate =
                    std::stof(rootNode["sensors"][i]["max_sampling_rate"].asString());
                info.maxBatchCountSupported =
                    std::stoi(rootNode["sensors"][i]["max_batch_count"].asString());
                info.minBatchCountSupported =
                    std::stoi(rootNode["sensors"][i]["min_batch_count"].asString());
                info.range = std::stoi(rootNode["sensors"][i]["range"].asString());
                info.version = std::stoi(rootNode["sensors"][i]["version"].asString());
                info.resolution = std::stof(rootNode["sensors"][i]["resolution"].asString());
                info.maxRange = std::stof(rootNode["sensors"][i]["max_range"].asString());
                sensorInfo_.emplace_back(info);
            }
        }
    } catch(std::exception const & ex){
        LOG(DEBUG, "Exception Occur ", ex.what());
    }
}


bool SensorFeatureManagerServerImpl::checkMotionConfigLimits(
    const sensorStub::EnableMotionDetectionRequest* request) {
    float threshold = request->threshold();
    int duration = request->duration();
    float samplingRate = request->sampling_rate();
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_MGR_API_JSON);
    float minThreshold = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["minThreshold"].asFloat();
    float maxThreshold = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["maxThreshold"].asFloat();
    if(threshold < minThreshold || threshold > maxThreshold) {
        LOG(ERROR, __FUNCTION__, " Threshold not in limits");
        return false;
    }
    int minDuration = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["minDuration"].asInt();
    int maxDuration = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["maxDuration"].asInt();
    if(duration < minDuration || duration > maxDuration) {
        LOG(ERROR, __FUNCTION__, " Duration not in limits");
        return false;
    }
    float minSamplingRate = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["minSamplingRate"].asFloat();
    float maxSamplingRate = rootNode["ISensorFeatureManager"]["getMotionDetectionLimits"]["maxSamplingRate"].asFloat();
    if(samplingRate < minSamplingRate || samplingRate > maxSamplingRate) {
        LOG(ERROR, __FUNCTION__, " Samplingrate not in limits");
        return false;
    }
    return true;
}

void SensorFeatureManagerServerImpl::apiJsonReader(
    std::string apiName, sensorStub::SensorFeatureManagerCommandReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, SENSOR_FEATURE_MGR_API_JSON);
    telux::common::Status status;
    int cbDelay = 100;
    cbDelay = rootNode["ISensorFeatureManager"]["DefaultCallbackDelay"].asInt();
    std::string cbStatus = rootNode["ISensorFeatureManager"][apiName]["status"].asString();
    status = CommonUtils::mapStatus(cbStatus);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_delay(cbDelay);
}
