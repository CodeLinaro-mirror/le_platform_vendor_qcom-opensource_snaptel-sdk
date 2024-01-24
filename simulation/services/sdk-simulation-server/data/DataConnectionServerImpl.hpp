/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_CONNECTION_SERVER_HPP
#define DATA_CONNECTION_SERVER_HPP

#include <iostream>
#include <memory>
#include <set>
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

#include "protos/proto-src/data.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using ::dataStub::DataConnectionManager;

struct DataCallParams {
    int slotId;
    std::string ifaceName;
    std::string ipFamilyType;
    std::string v4IpAddress;
    std::string v4GwAddress;
    std::string dnsPrimaryAddress;
    std::string dnsSecondaryAddress;
    std::string v6IpAddress;
    std::string v6GwAddress;
};

class DataConnectionServerImpl final:
    public dataStub::DataConnectionManager::Service {
public:
    DataConnectionServerImpl();
    ~DataConnectionServerImpl();

    grpc::Status InitService(ServerContext* context,
        const dataStub::SlotInfo* request,
        dataStub::GetServiceStatusReply* response) override;

    grpc::Status SetDefaultProfile(ServerContext* context,
        const dataStub::SetDefaultProfileRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status GetDefaultProfile(ServerContext* context,
        const dataStub::GetDefaultProfileRequest* request,
        dataStub::GetDefaultProfileReply* response) override;

    grpc::Status SetRoamingMode(ServerContext* context,
        const dataStub::SetRoamingModeRequest* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestRoamingMode(ServerContext* context,
        const dataStub::RequestRoamingModeRequest* request,
        dataStub::RequestRoamingModeReply* response) override;

    grpc::Status StartDatacall(ServerContext* context,
        const dataStub::DataCallInputParams* request,
        dataStub::DefaultReply* response) override;

    grpc::Status StopDatacall(ServerContext* context,
        const dataStub::DataCallInputParams* request,
        dataStub::DefaultReply* response) override;

    grpc::Status RequestDatacallList(ServerContext* context,
        const dataStub::DataCallInputParams* request,
        dataStub::RequestDataCallListReply* response) override;

    grpc::Status CleanUpService(ServerContext* context,
        const ::google::protobuf::Empty* request,
        ::google::protobuf::Empty* response) override;

    void stopActiveDataCalls(SlotId slotId);

private:
    bool getIpv4Address(const std::string &ifaceName,
        std::string &ipAddress, std::string &gatewayAddress,
        std::string &dnsPrimaryAddress, std::string &dnsSecondaryAddress);
    bool getIpv6Address(const std::string &ifaceName,
        std::string &ipAddress, std::string &gatewayAddress);

    void triggerStartDataCallEvent(int profileId, int slotId, std::string ipFamilyType);
    void triggerStopDataCallEvent(int profileId, int slotId, std::string ipFamilyType,
        std::string ifaceName);

    void getInactiveInterfaces();
    void clearCachedDataCall(std::map<int, std::shared_ptr<DataCallParams>>& dataCallsMap);
    bool isWwanConnectivityAllowed(int slotId);

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::map<int, std::shared_ptr<DataCallParams>> dataCallsSlot1_;
    std::map<int, std::shared_ptr<DataCallParams>> dataCallsSlot2_;
    /* Everytime datacall is triggered, we are reading list of interfaces from conf file. In activeNwIfaces_
     * we are maintaining interfaces that are associated with a datacall & in inactiveNwIfaces_ we are maintaining
     * interfaces that are not yet associated with datacall.
     */
    std::set<std::string> activeNwIfaces_;
    std::set<std::string> inactiveNwIfaces_;
    std::mutex mtx_;
};

#endif //DATA_CONNECTION_SERVER_HPP