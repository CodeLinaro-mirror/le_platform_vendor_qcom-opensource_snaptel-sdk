/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFeatureManagerStub.cpp
 *
 *
 */

#include "SensorFeatureManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"
#include "telux/sensor/SensorDefines.hpp"
#include "SensorDefinesStub.hpp"

#include <telux/common/CommonDefines.hpp>
#include <chrono>
#include <time.h>
#include <linux/iio/events.h>
#include <linux/iio/types.h>

//Default cb delay.
#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1

#define RPC_FAIL_SUFFIX " RPC Request failed - "

namespace telux {
namespace sensor {

SensorFeatureManagerStub::SensorFeatureManagerStub(){
    LOG(DEBUG, __FUNCTION__, " Creating");
    serviceStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
    stub_ =  CommonUtils::getGrpcStub<SensorFeatureManagerService>();
}

SensorFeatureManagerStub::~SensorFeatureManagerStub(){
    LOG(DEBUG, __FUNCTION__);
    cleanup();
}

telux::common::ServiceStatus SensorFeatureManagerStub::getServiceStatus(){
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void SensorFeatureManagerStub::cleanup(){
    LOG(DEBUG, __FUNCTION__);
    taskQ_.shutdown();
    tcuActivityMgr_ = nullptr;
}

telux::common::Status SensorFeatureManagerStub::init(telux::common::InitResponseCb initCb){
    LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async, [this, initCb]() { this->initSync(initCb); }).share();
    taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

bool SensorFeatureManagerStub::getSystemState() {
    std::lock_guard<std::mutex> lock(mutex_);
    return isSystemSuspended_;
}

void SensorFeatureManagerStub::onTcuActivityStateUpdate(TcuActivityState state,
    std::string machineName) {
    LOG(DEBUG, __FUNCTION__);

    if (state == TcuActivityState::SUSPEND) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            isSystemSuspended_ = true;
            LOG(DEBUG, "isSystemSuspended_: ", isSystemSuspended_);
        }
        telux::common::Status ackStatus = tcuActivityMgr_->sendActivityStateAck(StateChangeResponse::ACK,
            state);
        if (ackStatus == telux::common::Status::SUCCESS) {
            std::cout << " Sent SUSPEND acknowledgement" << std::endl;
        } else {
            std::cout << " Failed to send SUSPEND acknowledgement !" << std::endl;
        }
    } else if (state == TcuActivityState::RESUME) {
        std::lock_guard<std::mutex> lock(mutex_);
        isSystemSuspended_ = false;
    }
}

void SensorFeatureManagerStub::initTcuPowerManager() {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status status;
    telux::common::ServiceStatus serviceStatus;
    std::promise<telux::common::ServiceStatus> p{};
    telux::power::ClientInstanceConfig config{};

    std::cout << " Initializing the client as a SLAVE " << std::endl;

    config.clientType = telux::power::ClientType::SLAVE;
    config.clientName = "slaveClientSensorFeatureMgrStub";
    config.machineName = telux::power::LOCAL_MACHINE;

    // Get power factory instance
    auto &powerFactory = PowerFactory::getInstance();

    // Get TCU-activity manager object
    tcuActivityMgr_ = powerFactory.getTcuActivityManager(
        config, [&p](telux::common::ServiceStatus srvStatus) {
        p.set_value(srvStatus);
    });

    if (!tcuActivityMgr_) {
        std::cout << "Can't get ITcuActivityManager" << std::endl;
        return;
    }

    // Wait for TCU-activity manager to be ready
    std::cout << " Waiting for TCU Activity Manager to be ready " << std::endl;
    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Power service unavailable, status " <<
            static_cast<int>(serviceStatus) << std::endl;
        return;
    }

    // Registering a listener for TCU-activity state updates
    status = tcuActivityMgr_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Can't register listener, err " <<
            static_cast<int>(status) << std::endl;
        return;
    }

    std::cout << " Registered Listener for TCU-activity state updates" << std::endl;
}

void SensorFeatureManagerStub::initSync(telux::common::InitResponseCb callback){
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    if(reqstatus.ok()) {
        std::lock_guard<std::mutex> lock(mutex_);
        serviceStatus_ = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        std::lock_guard<std::mutex> lock(mutex_);
        serviceStatus_ = telux::common::ServiceStatus::SERVICE_FAILED;
    }
    if(serviceStatus_ == ServiceStatus::SERVICE_AVAILABLE ){
        auto myself = shared_from_this();
        myself_ = myself;
        initTcuPowerManager();
        LOG(DEBUG, "Sensor sub-system is now available, retrieving sensor list");
        const ::google::protobuf::Empty request;
        ::sensorStub::SensorInfoResponse response;
        ClientContext context;
        ::grpc::Status reqstatus = stub_->GetSensorList(&context, request, &response);
        if(reqstatus.ok()) {
            for (const auto& Sensorinfo : response.sensor_info()) {
                SensorInfo info;
                info.id = Sensorinfo.id();
                info.type = static_cast<SensorType>(Sensorinfo.sensor_type());
                info.name = Sensorinfo.name();
                info.vendor = Sensorinfo.vendor();
                for (const auto& samplingRate : Sensorinfo.sampling_rates()) {
                    info.samplingRates.push_back(samplingRate);
                }
                info.maxSamplingRate = Sensorinfo.max_sampling_rate();
                info.maxBatchCountSupported = Sensorinfo.max_batch_count_supported();
                info.minBatchCountSupported = Sensorinfo.min_batch_count_supported();
                info.range = Sensorinfo.range();
                info.version = Sensorinfo.version();
                info.resolution = Sensorinfo.resolution();
                info.maxRange = Sensorinfo.max_range();

                sensorInfo_.push_back(info);
            }
            if (sensorInfo_.empty()) {
                LOG(ERROR, "Received sensor list with ", sensorInfo_.size(), " sensors");
                serviceStatus_ = telux::common::ServiceStatus::SERVICE_FAILED;
            } else {
                LOG(DEBUG, "Received sensor list with ", sensorInfo_.size(), " sensors");
            }
        } else {
            LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
            serviceStatus_ = telux::common::ServiceStatus::SERVICE_FAILED;
        }
    }
    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        callback(serviceStatus_);
    }
    return;
}

void SensorFeatureManagerStub::onEventUpdate(google::protobuf::Any event){
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::sensorStub::FeatureEvent>()) {
        ::sensorStub::FeatureEvent featureEvent;
        event.UnpackTo(&featureEvent);
        handleFeatureEvent(featureEvent);
    } else if (event.Is<::sensorStub::MotionDetectionEvent>()) {
        LOG(DEBUG, __FUNCTION__, " MotionDetectionEvent update");
        ::sensorStub::MotionDetectionEvent motionDetectionEvent;
        event.UnpackTo(&motionDetectionEvent);
        handleMotionDetectionEvent(motionDetectionEvent);
    } else if (event.Is<::sensorStub::MotionDetectionEnabledEvent>()) {
        LOG(DEBUG, __FUNCTION__, " MotionDetectionEnabledEvent update");
        ::sensorStub::MotionDetectionEnabledEvent motionDetectionEnabledEvent;
        event.UnpackTo(&motionDetectionEnabledEvent);
        handleMotionDetectionEnabledEvent(motionDetectionEnabledEvent);
    } else if (event.Is<::sensorStub::MotionDetectionDisabledEvent>()) {
        LOG(DEBUG, __FUNCTION__, " MotionDetectionDisabledEvent update");
        ::sensorStub::MotionDetectionDisabledEvent motionDetectionDisabledEvent;
        event.UnpackTo(&motionDetectionDisabledEvent);
        handleMotionDetectionDisabledEvent();
    }
}

void SensorFeatureManagerStub::handleFeatureEvent(::sensorStub::FeatureEvent event){
    LOG(DEBUG, __FUNCTION__);
    std::string sensorName = " ";
    SensorFeatureEvent featureEvent;
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    featureEvent.timestamp =  (uint64_t)ts.tv_sec * SEC_TO_NANOS + (uint64_t)ts.tv_nsec;
    featureEvent.name = event.featurename();
    featureEvent.id = event.id();
    auto bufferedEvents = std::make_shared<std::vector<SensorEvent>>();
    parseBufferedEvent(event.events(),bufferedEvents,sensorName);
    invokeEventListener(featureEvent);
    if(getSystemState()) {
        invokeBufferedEventListener(sensorName, bufferedEvents, true);
    }
}

void SensorFeatureManagerStub::parseBufferedEvent(std::string eventString,
    std::shared_ptr<std::vector<SensorEvent>> &events, std::string &sensorName) {
    LOG(DEBUG, __FUNCTION__,eventString.length());
    std::stringstream ss(eventString);
    std::vector<std::string> eventValues;
    while( ss.good() ) {
        std::string substr;
        std::getline( ss, substr, ',' );
        eventValues.push_back(substr);
    }
    int dataLength = eventValues.size();
    sensorName = eventValues[dataLength-1];
    for(int i=1;i<dataLength-1;i+=6){
        SensorEvent event;
        timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        event.timestamp =  (uint64_t)ts.tv_sec * SEC_TO_NANOS + (uint64_t)ts.tv_nsec;
        event.uncalibrated.data.x = std::stof(eventValues[i]);
        event.uncalibrated.data.y = std::stof(eventValues[i+1]);
        event.uncalibrated.data.z = std::stof(eventValues[i+2]);
        event.uncalibrated.bias.x = std::stof(eventValues[i+3]);
        event.uncalibrated.bias.y = std::stof(eventValues[i+4]);
        event.uncalibrated.bias.z = std::stof(eventValues[i+5]);
        events->push_back(event);
    }
    return;
}

telux::common::Status SensorFeatureManagerStub::getAvailableFeatures(
    std::vector<SensorFeature> &features){
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::GetFeatureListReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    telux::common::Status status = telux::common::Status::FAILED;
    ::grpc::Status reqstatus = stub_->GetFeatureList(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        cbDelay = static_cast<int>(response.delay());
        std::string sensorFeatureString = response.list();
        std::stringstream ss(sensorFeatureString);
        while( ss.good() ) {
            SensorFeature sensorFeature;
            std::string substr;
            std::getline( ss, substr, ',' );
            sensorFeature.name = substr;
            features.emplace_back(sensorFeature);
        }
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    auto f = std::async(std::launch::async, [=]() {
        if (cbDelay != SKIP_CALLBACK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status SensorFeatureManagerStub::enableFeature(std::string name){
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::SensorFeatureManagerCommandReply response;
    ::sensorStub::SensorEnableFeature request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    telux::common::Status status = telux::common::Status::FAILED;
    request.set_feature(name);
    ::grpc::Status reqstatus = stub_->EnableFeature(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        cbDelay = static_cast<int>(response.delay());
        LOG(DEBUG , __FUNCTION__ , " Request Sent Successfully ");
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    auto f = std::async(std::launch::async, [=]() {
        if (cbDelay != SKIP_CALLBACK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status SensorFeatureManagerStub::disableFeature(std::string name) {
    LOG(DEBUG, __FUNCTION__);
    ::sensorStub::SensorFeatureManagerCommandReply response;
    ::sensorStub::SensorEnableFeature request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    telux::common::Status status = telux::common::Status::FAILED;
    request.set_feature(name);
    ::grpc::Status reqstatus = stub_->DisableFeature(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        cbDelay = static_cast<int>(response.delay());
        LOG(DEBUG , __FUNCTION__ , " Request Sent Successfully ");
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    auto f = std::async(std::launch::async, [=]() {
        if (cbDelay != SKIP_CALLBACK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status SensorFeatureManagerStub::registerListener(
    std::weak_ptr<ISensorFeatureEventListener> listener){
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(mutex_);
    telux::common::Status status = telux::common::Status::SUCCESS;
    auto spt = listener.lock();
    if (spt) {
        if (listeners_.size() == 0) {
            std::vector<std::string> filters = {"sensor_feature"};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(myself_, filters);
        }
        bool existing = 0;
        for (auto iter = listeners_.begin(); iter < listeners_.end(); ++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, " Register Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
        }
    } else {
        status = telux::common::Status::INVALIDPARAM;
    }
    return status;
}

telux::common::Status SensorFeatureManagerStub::deregisterListener(
    std::weak_ptr<ISensorFeatureEventListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt) {
        for (auto iter=listeners_.begin(); iter<listeners_.end(); ++iter) {
            if (spt == (*iter).lock()) {
                LOG(DEBUG, __FUNCTION__, " In deRegister Listener : Removing");
                iter = listeners_.erase(iter);
                status = telux::common::Status::SUCCESS;
                break;
            }
        }
    } else {
        status = telux::common::Status::INVALIDPARAM;
    }
    return status;
}

telux::common::Status SensorFeatureManagerStub::getAvailableSensorInfo(
    std::vector<SensorInfo> &info) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::SUCCESS;
    if (sensorInfo_.empty()) {
        LOG(ERROR, "sensorInfo_ is empty");
        return telux::common::Status::FAILED;
    }
    info = sensorInfo_;
    return status;
}

telux::common::Status SensorFeatureManagerStub::getMotionDetectionConfigLimits(int sensorId,
    MotionDetectionConfigLimits &motionDetectionConfigLimits) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    ::sensorStub::MotionDetectionRequest request {};
    ::sensorStub::MotionDetectionLimitsReply response {};
    ClientContext context{};
    request.set_sensor_id(sensorId);
    grpc::Status reqStatus;
    reqStatus = stub_->GetMotionDetectionLimits(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
    } else {
        status = static_cast<telux::common::Status>(response.status());
        if(status == telux::common::Status::SUCCESS) {
            motionDetectionConfigLimits.minThreshold = static_cast<float>(response.min_threshold());
            motionDetectionConfigLimits.maxThreshold = static_cast<float>(response.max_threshold());
            motionDetectionConfigLimits.minDuration = static_cast<int>(response.min_duration());
            motionDetectionConfigLimits.maxDuration = static_cast<int>(response.max_duration());
            motionDetectionConfigLimits.minSamplingRate = static_cast<float>(response.min_samplingrate());
            motionDetectionConfigLimits.maxSamplingRate = static_cast<float>(response.max_samplingrate());
        } else {
            LOG(ERROR, __FUNCTION__, " Failed to get motion detection config limits");
        }
    }

    return status;
}

telux::common::Status SensorFeatureManagerStub::enableMotionDetection(
    MotionDetectionConfig motionDetectionConfig) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    ::sensorStub::EnableMotionDetectionRequest request {};
    ::sensorStub::SensorFeatureManagerCommandReply response {};
    ClientContext context{};
    request.set_sensor_id(motionDetectionConfig.sensorId);
    request.set_threshold(motionDetectionConfig.threshold);
    request.set_duration(motionDetectionConfig.duration);
    request.set_sampling_rate(motionDetectionConfig.samplingRate);
    grpc::Status reqStatus;
    reqStatus = stub_->EnableMotionDetection(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
    } else {
        status = static_cast<telux::common::Status>(response.status());
    }

    return status;
}

telux::common::Status SensorFeatureManagerStub::disableMotionDetection(int sensorId) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    ::sensorStub::MotionDetectionRequest request {};
    ::sensorStub::SensorFeatureManagerCommandReply response {};
    ClientContext context{};
    request.set_sensor_id(sensorId);
    grpc::Status reqStatus;
    reqStatus = stub_->DisableMotionDetection(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
    } else {
        status = static_cast<telux::common::Status>(response.status());
    }

    return status;
}

telux::common::Status SensorFeatureManagerStub::getMotionDetectionConfigs(
    std::vector<MotionDetectionConfig> &motionDetectionConfigs) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    const ::google::protobuf::Empty request {};
    ::sensorStub::MotionDetectionConfigReply response {};
    ClientContext context{};
    grpc::Status reqStatus;
    reqStatus = stub_->GetMotionDetectionConfig(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqStatus.error_code());
    } else {
        status = static_cast<telux::common::Status>(response.status());
        if(status == telux::common::Status::SUCCESS) {
            MotionDetectionConfig motionDetectionConfig;
            motionDetectionConfig.sensorId = static_cast<int>(response.sensor_id());
            motionDetectionConfig.threshold = static_cast<float>(response.threshold());
            motionDetectionConfig.duration = static_cast<int>(response.duration());
            motionDetectionConfig.samplingRate = static_cast<float>(response.sampling_rate());
            motionDetectionConfigs.push_back(motionDetectionConfig);
        } else {
            LOG(ERROR, __FUNCTION__, " Failed to get motion detection configs");
        }
    }

    return status;
}

void SensorFeatureManagerStub::invokeEventListener(SensorFeatureEvent event){
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onEvent(event);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

void SensorFeatureManagerStub::invokeBufferedEventListener(std::string sensorName,
    std::shared_ptr<std::vector<SensorEvent>> events, bool isLast){
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onBufferedEvent(sensorName,events,isLast);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

void SensorFeatureManagerStub::handleMotionDetectionEvent(
    ::sensorStub::MotionDetectionEvent motionDetectionEvent) {
    LOG(DEBUG, __FUNCTION__);
    int sensorId = motionDetectionEvent.sensor_id();
    struct iio_event_data event;
    event.id = motionDetectionEvent.event_id();
    event.timestamp = motionDetectionEvent.timestamp();
    for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onMotionDetected(sensorId, event);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

void SensorFeatureManagerStub::handleMotionDetectionEnabledEvent(
    ::sensorStub::MotionDetectionEnabledEvent motionDetectionEnabledEvent) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<MotionDetectionConfig> motionDetectionConfigs;
    telux::sensor::MotionDetectionConfig motionDetectionConfig;
    motionDetectionConfig.sensorId = motionDetectionEnabledEvent.sensor_id();
    motionDetectionConfig.threshold = motionDetectionEnabledEvent.threshold();
    motionDetectionConfig.duration = motionDetectionEnabledEvent.duration();
    motionDetectionConfig.samplingRate = motionDetectionEnabledEvent.sampling_rate();
    motionDetectionConfigs.push_back(motionDetectionConfig);
    for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onMotionDetectionEnabled(motionDetectionConfigs);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

void SensorFeatureManagerStub::handleMotionDetectionDisabledEvent() {
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            spt->onMotionDetectionDisabled();
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

}
}