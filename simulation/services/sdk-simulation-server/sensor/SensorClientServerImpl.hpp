/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorClientServerImpl.hpp
 *
 *
 */

#ifndef SENSOR_CLIENT_SERVER_HPP
#define SENSOR_CLIENT_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include <telux/sensor/SensorDefines.hpp>

#include "event/ServerEventManager.hpp"
#include "libs/sensor/SensorDefinesStub.hpp"
#include "protos/proto-src/sensor_simulation.grpc.pb.h"
#include "common/FileBuffer.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using sensorStub::SensorClientService;

class SensorClientServerImpl final :
    public sensorStub::SensorClientService::Service,
    public IServerEventListener,
    public std::enable_shared_from_this<SensorClientServerImpl> {
 public:
    SensorClientServerImpl();
    ~SensorClientServerImpl();
    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::GetServiceStatusReply* response);

    grpc::Status GetSensorList(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorInfoResponse* response);

    grpc::Status Configure(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status GetConfiguration(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status GetSensorInfo(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status Activate(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status Deactivate(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status SelfTest(ServerContext* context, const google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

    grpc::Status SensorUpdateRotationMatrix(ServerContext* context,
        const ::google::protobuf::Empty* request,
        sensorStub::SensorClientCommandReply* response);

 private:
    void apiJsonReader(std::string apiName, sensorStub::SensorClientCommandReply* response);
    void init();
    void updateSensorInfo();
    telux::sensor::SensorType getSensorType(std::string sensorType);
    void startStreaming();
    void updateStreamRequest();
    void triggerStreamingStoppedEvent();
    std::vector<telux::sensor::SensorInfo> sensorInfo_;
    std::shared_ptr<FileBuffer> fileBuffer_ = nullptr;
    std::vector<std::string> requestBuffer_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    bool bufferingInitialized_ = false;
    bool stopStreamingData_ = false;
    bool replayCsv_ = false;
    uint64_t previousTimestamp_ = 0;
    bool lastBatchStreamed_ = false;
};
#endif  // SENSOR_FEATURE_MANAGER_SERVER_HPP