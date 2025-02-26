/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ETHERNET_MANAGER_SERVER_HPP
#define ETHERNET_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/AsyncTaskQueue.hpp"

#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using ::dataStub::EthernetManager;

class EthernetManagerServerImpl final:
    public dataStub::EthernetManager::Service {
public:
    EthernetManagerServerImpl();
    ~EthernetManagerServerImpl();

    grpc::Status InitService(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status GetServiceStatus(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetEthernetNicConfig(ServerContext* context,
        const dataStub::SetEthernetNicConfigRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status GetEthernetNicConfig(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::GetEthernetNicConfigReply* response) override;

    grpc::Status ActivateLAN(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RegisterListener(ServerContext* context,
        const google::protobuf::Empty* request,
        grpc::ServerWriter<dataStub::RegisterListenerReply>* writer) override;

    grpc::Status DeregisterListener(ServerContext* context,
        const dataStub::DeregisterListenerRequest* request,
        google::protobuf::Empty* response) override;

    grpc::Status GetOperationType(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::GetOperationTypeReply* response) override;

private:
    bool readConfig(const std::string& filePath, Json::Value& rootObj);
    void triggerRegisterListenerEvent(int clientId, telux::common::ServiceStatus status);
    void clearCachedListeners();

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::map<int, telux::common::ServiceStatus> listeners_;
    std::mutex mtx_;
};

#endif //ETHERNET_MANAGER_SERVER_HPP
