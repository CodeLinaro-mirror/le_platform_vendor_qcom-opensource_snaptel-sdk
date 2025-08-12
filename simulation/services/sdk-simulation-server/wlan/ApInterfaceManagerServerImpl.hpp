/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AP_INTERFACE_MANAGER_SERVER_HPP
#define AP_INTERFACE_MANAGER_SERVER_HPP

#include <telux/wlan/ApInterfaceManager.hpp>

#include "event/ServerEventManager.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/wlan_simulation.grpc.pb.h"

#include "google/protobuf/util/json_util.h"

#include "protos/proto-src/common_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class ApInterfaceManagerServerImpl final
    : public wlanStub::ApInterfaceService::Service,
      public IServerEventListener,
      public std::enable_shared_from_this<ApInterfaceManagerServerImpl> {
public:
  ApInterfaceManagerServerImpl();
  ~ApInterfaceManagerServerImpl();

  // gRPC method implementations for IApInterfaceManager API
  grpc::Status InitService(ServerContext *context,
                           const ::google::protobuf::Empty *request,
                           wlanStub::GetServiceStatusReply *response) override;

  grpc::Status SetConfig(ServerContext *context,
                         const wlanStub::SetConfigRequest *request,
                         wlanStub::DefaultReply *response) override;

  grpc::Status
  SetSecurityConfig(ServerContext *context,
                    const wlanStub::SetSecurityConfigRequest *request,
                    wlanStub::DefaultReply *response) override;

  grpc::Status SetSsid(ServerContext *context,
                       const wlanStub::SetSsidRequest *request,
                       wlanStub::DefaultReply *response) override;

  grpc::Status SetVisibility(ServerContext *context,
                             const wlanStub::SetVisibilityRequest *request,
                             wlanStub::DefaultReply *response) override;

  grpc::Status
  SetElementInfoConfig(ServerContext *context,
                       const wlanStub::SetElementInfoConfigRequest *request,
                       wlanStub::DefaultReply *response) override;

  grpc::Status SetPassPhrase(ServerContext *context,
                             const wlanStub::SetPassPhraseRequest *request,
                             wlanStub::DefaultReply *response) override;

  grpc::Status GetConfig(ServerContext *context,
                         const ::google::protobuf::Empty *request,
                         wlanStub::ApGetConfigResponse *response) override;

  grpc::Status GetStatus(ServerContext *context,
                         const ::google::protobuf::Empty *request,
                         wlanStub::ApGetStatusResponse *response) override;

  grpc::Status
  GetConnectedDevices(ServerContext *context,
                      const ::google::protobuf::Empty *request,
                      wlanStub::GetConnectedDevicesResponse *response) override;

  grpc::Status ManageApService(ServerContext *context,
                               const wlanStub::ManageApServiceRequest *request,
                               wlanStub::DefaultReply *response) override;

  // IServerEventListener callback for unsolicited events
  void onEventUpdate(::eventService::UnsolicitedEvent event) override;

private:
  std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

  // Helper methods for event handling
  void onEventUpdate(std::string event);
  void handleDeviceStatusChangedEvent(std::string event);
  void handleApBandChangedEvent(std::string event);
  void handleApConfigChangedEvent(std::string event);
};

#endif // AP_INTERFACE_MANAGER_SERVER_HPP