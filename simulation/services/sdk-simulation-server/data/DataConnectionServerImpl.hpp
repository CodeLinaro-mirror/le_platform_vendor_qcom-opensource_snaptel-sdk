/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATA_CONNECTION_SERVER_HPP
#define DATA_CONNECTION_SERVER_HPP

#include <iostream>
#include <memory>
#include <list>
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

using ::dataStub::DataConnectionManager;

struct DataCallParams {
    int slotId;
    std::string ifaceName;
    std::string ipFamilyType;
    std::string v4IpAddress;
    std::string v4GwAddress;
    std::string v4dnsPrimaryAddress;
    std::string v4dnsSecondaryAddress;
    std::string v6IpAddress;
    std::string v6GwAddress;
    std::string v6dnsPrimaryAddress;
    std::string v6dnsSecondaryAddress;
    std::set<int> ownersId;
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
        const ::dataStub::ClientInfo* request,
        ::google::protobuf::Empty* response) override;

    grpc::Status requestConnectedDataCallLists(ServerContext* context,
        const dataStub::CachedDataCallsRequest* request,
        dataStub::CachedDataCalls* response) override;

    /* Could be used if all the datacalls need to be teared down.
     * For ex: if WWAN connectivity is disabled via DataSettingsManager, then
     * all the datacalls need to be teared down.
     */
    void stopActiveDataCalls(SlotId slotId);

    /* Could be used to check if any datacall exist.
     * For ex: DataRestrictMode is enabled only if atleast one datacall exist.
     */
    bool isAnyDataCallActive(SlotId slotId);

private:
    bool getIpv4Address(const std::string &ifaceName,
        std::string &ipAddress, std::string &gatewayAddress,
        std::string &dnsPrimaryAddress, std::string &dnsSecondaryAddress);
    bool getIpv6Address(const std::string &ifaceName,
        std::string &ipAddress, std::string &gatewayAddress,
        std::string &dnsPrimaryAddress, std::string &dnsSecondaryAddress);

    void triggerStartDataCallEvent(int profileId, int slotId, std::string ipFamilyType,
        unsigned int client_id, std::string ifaceName = "");
    void triggerStopDataCallEvent(int profileId, int slotId, std::string ipFamilyType,
        std::string ifaceName);

    void getInactiveInterfaces();
    bool isWwanConnectivityAllowed(int slotId);

    void clearCachedDataCall(std::map<int, std::shared_ptr<DataCallParams>>& dataCallsMap,
        bool stopAllCalls = false, const unsigned int& client_id = 0);

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::map<int, std::shared_ptr<DataCallParams>> dataCallsSlot1_;
    std::map<int, std::shared_ptr<DataCallParams>> dataCallsSlot2_;
    /* Everytime datacall is triggered, we are reading list of interfaces from conf file.
     * In activeNwIfaces_ we are maintaining interfaces that are associated with a datacall
     * & in inactiveNwIfaces_ we are maintaining interfaces that are not yet associated with
     * datacall.
     */
    std::list<std::string> activeNwIfaces_;
    std::list<std::string> inactiveNwIfaces_;
    std::mutex mtx_;
};

#endif //DATA_CONNECTION_SERVER_HPP