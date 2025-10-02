/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STA_INTERFACE_MANAGER_SERVER_HPP
#define STA_INTERFACE_MANAGER_SERVER_HPP

#include <telux/wlan/StaInterfaceManager.hpp>

#include "event/ServerEventManager.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/common_simulation.grpc.pb.h" // For commonStub::ErrorCode etc.
#include "protos/proto-src/wlan_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class StaInterfaceManagerServerImpl final
    : public wlanStub::StaInterfaceService::Service, // Corrected service name
      public IServerEventListener,                   // Fully qualified
      public std::enable_shared_from_this<StaInterfaceManagerServerImpl> {
public:
  StaInterfaceManagerServerImpl();
  ~StaInterfaceManagerServerImpl();

  grpc::Status InitService(ServerContext *context,
                           const ::google::protobuf::Empty *request,
                           wlanStub::GetServiceStatusReply *response) override;

  grpc::Status SetIpConfig(ServerContext *context,
                           const wlanStub::StaSetIpConfigRequest *request,
                           wlanStub::DefaultReply *response) override;

  grpc::Status SetBridgeMode(ServerContext *context,
                             const wlanStub::StaSetBridgeModeRequest *request,
                             wlanStub::DefaultReply *response) override;

  grpc::Status EnableHotspot2(ServerContext *context,
                              const wlanStub::StaEnableHotspot2Request *request,
                              wlanStub::DefaultReply *response) override;

  grpc::Status GetConfig(ServerContext *context,
                         const ::google::protobuf::Empty *request,
                         wlanStub::StaGetConfigResponse *response) override;

  grpc::Status GetStatus(ServerContext *context,
                         const ::google::protobuf::Empty *request,
                         wlanStub::StaGetStatusResponse *response) override;

  grpc::Status StartScan(ServerContext *context,
                         const wlanStub::StaStartScanRequest *request,
                         wlanStub::DefaultReply *response) override;

  grpc::Status
  AddNetworkConfig(ServerContext *context,
                   const wlanStub::StaAddNetworkConfigRequest *request,
                   wlanStub::DefaultReply *response) override;

  grpc::Status
  RemoveNetworkConfig(ServerContext *context,
                      const wlanStub::StaRemoveNetworkConfigRequest *request,
                      wlanStub::DefaultReply *response) override;

  grpc::Status
  GetNetworkConfigs(ServerContext *context,
                    const wlanStub::StaGetNetworkConfigsRequest *request,
                    wlanStub::StaGetNetworkConfigsResponse *response) override;

  grpc::Status Connect(ServerContext *context,
                       const wlanStub::StaConnectRequest *request,
                       wlanStub::DefaultReply *response) override;

  grpc::Status Disconnect(ServerContext *context,
                          const wlanStub::StaDisconnectRequest *request,
                          wlanStub::DefaultReply *response) override;

  grpc::Status
  ManageStaService(ServerContext *context,
                   const wlanStub::StaManageStaServiceRequest *request,
                   wlanStub::DefaultReply *response) override;

  void onEventUpdate(::eventService::UnsolicitedEvent event) override;

private:
  std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
  void onEventUpdate(std::string event);
  void handleStaStatusChangedEvent(std::string event);
  void handleStaScanResultUpdatedEvent(std::string event);
  void handleStaBandChangedEvent(std::string event);
};

#endif // STA_INTERFACE_MANAGER_SERVER_HPP