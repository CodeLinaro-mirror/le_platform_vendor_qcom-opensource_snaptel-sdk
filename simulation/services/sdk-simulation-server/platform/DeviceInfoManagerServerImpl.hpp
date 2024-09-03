/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DEVICE_INFO_MANAGER_SERVER_HPP
#define DEVICE_INFO_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>

#include "protos/proto-src/platform_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using platformStub::DeviceInfoManagerService;

class DeviceInfoManagerServerImpl final :
    public platformStub::DeviceInfoManagerService::Service,
    public std::enable_shared_from_this<DeviceInfoManagerServerImpl> {
 public:
    DeviceInfoManagerServerImpl();
    ~DeviceInfoManagerServerImpl();

    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::GetServiceStatusReply* response);

    grpc::Status GetPlatformVersion(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::PlatformVersionInfo* response);

    grpc::Status GetIMEI(ServerContext* context, const google::protobuf::Empty* request,
        platformStub::PlatformImeiInfo* response);

};
#endif  // DEVICE_INFO_MANAGER_SERVER_HPP