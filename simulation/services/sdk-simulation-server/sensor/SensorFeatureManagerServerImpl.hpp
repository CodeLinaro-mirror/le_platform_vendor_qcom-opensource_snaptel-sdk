/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFeatureManagerServerImpl.hpp
 *
 *
 */

#ifndef SENSOR_FEATURE_MANAGER_SERVER_HPP
#define SENSOR_FEATURE_MANAGER_SERVER_HPP

#include <memory>
#include <string>
#include <map>

#include "libs/common/AsyncTaskQueue.hpp"
#include "event/ServerEventManager.hpp"

#include "protos/proto-src/sensor_simulation.grpc.pb.h"

#include <telux/sensor/SensorDefines.hpp>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using sensorStub::SensorFeatureManagerService;

class SensorFeatureManagerServerImpl final :
    public sensorStub::SensorFeatureManagerService::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<SensorFeatureManagerServerImpl> {
 public:
    SensorFeatureManagerServerImpl();
    ~SensorFeatureManagerServerImpl();
    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::GetServiceStatusReply* response);

    grpc::Status GetFeatureList(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::GetFeatureListReply* response);

    grpc::Status EnableFeature(ServerContext* context,
        const sensorStub::SensorEnableFeature* request,
        sensorStub::SensorFeatureManagerCommandReply* response);

    grpc::Status DisableFeature(ServerContext* context,
        const sensorStub::SensorEnableFeature* request,
        sensorStub::SensorFeatureManagerCommandReply* response);

    grpc::Status GetSensorList(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorInfoResponse* response);

    grpc::Status GetMotionDetectionLimits(ServerContext* context,
        const sensorStub::MotionDetectionRequest* request,
        sensorStub::MotionDetectionLimitsReply* response);

    grpc::Status EnableMotionDetection(ServerContext* context,
        const sensorStub::EnableMotionDetectionRequest* request,
        sensorStub::SensorFeatureManagerCommandReply* response);

    grpc::Status DisableMotionDetection(ServerContext* context,
        const sensorStub::MotionDetectionRequest* request,
        sensorStub::SensorFeatureManagerCommandReply* response);

    grpc::Status GetMotionDetectionConfig(ServerContext* context,
        const google::protobuf::Empty* request,
        sensorStub::MotionDetectionConfigReply* response);

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    void apiJsonReader(std::string apiName, sensorStub::SensorFeatureManagerCommandReply* response);
    void handleEvent(std::string token , std::string event);
    void handleFeatureEvent(std::string eventParams);
    void triggerFeatureEvent(std::string featureName, int id, std::string events);
    void handleMotionDetectionEvent(std::string eventParams);
    void triggerMotionDetectionEvent(uint64_t eventId, uint64_t timestamp);
    void triggerMotionDetectionEnabledEvent();
    void triggerMotionDetectionDisabledEvent();
    std::string readBufferedEventStringFromFile(std::string filename,int eventId);
    void onEventUpdate(std::string event);
    telux::sensor::SensorType getSensorType(std::string sensorType);
    void updateSensorInfo();
    bool checkMotionConfigLimits(const sensorStub::EnableMotionDetectionRequest* request);
    std::map<std::string, bool> featureStatusMap_;
    std::mutex mtx_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    bool motionDetectionEnabled_ = false;
    std::vector<telux::sensor::SensorInfo> sensorInfo_;
};
#endif  // SENSOR_FEATURE_MANAGER_SERVER_HPP