/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef BRIDGE_MANAGER_SERVER_HPP
#define BRIDGE_MANAGER_SERVER_HPP

#include <telux/data/net/BridgeManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class BridgeServerImpl final:
    public dataStub::BridgeManager::Service {
public:
    BridgeServerImpl();
    ~BridgeServerImpl();

    grpc::Status InitService(ServerContext* context,
        const google::protobuf::Empty* request,
        dataStub::GetServiceStatusReply* response) override;

private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

#endif //BRIDGE_MANAGER_SERVER_HPP