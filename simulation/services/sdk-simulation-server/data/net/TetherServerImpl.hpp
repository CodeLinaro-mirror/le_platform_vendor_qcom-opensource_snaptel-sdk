/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TETHER_SERVER_IMPL_HPP
#define TETHER_SERVER_IMPL_HPP

#include "telux/data/net/TetherManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

#include <string>

class TetherServerImpl final:
    public dataStub::TetherManager::Service,
    public std::enable_shared_from_this<TetherServerImpl> {

public:
    TetherServerImpl();
    ~TetherServerImpl();

    grpc::Status InitService(grpc::ServerContext* context, const google::protobuf::Empty* request, dataStub::GetServiceStatusReply* response) override;
    grpc::Status GetServiceStatus(grpc::ServerContext* context, const google::protobuf::Empty* request, dataStub::GetServiceStatusReply* response) override;
    grpc::Status StartBTTether(grpc::ServerContext* context, const dataStub::BTTetherMode* request, dataStub::DefaultReply* response) override;
    grpc::Status StopBTTether(grpc::ServerContext* context, const google::protobuf::Empty* request, dataStub::DefaultReply* response) override;
    grpc::Status RequestBTTetherStatus(grpc::ServerContext* context, const google::protobuf::Empty* request, dataStub::BTTetherStatusReply* response) override;

private:

    std::string convertBTTetherStatusEnumtoString(dataStub::BTTetherStatus::Status status);
    dataStub::BTTetherStatus::Status convertBTTetherStatusStringtoEnum(std::string status);

    std::string convertBTTetherModeEnumtoString(dataStub::BTTetherMode::Mode status);
    dataStub::BTTetherMode::Mode convertBTTetherModeStringtoEnum(std::string status);

};

#endif