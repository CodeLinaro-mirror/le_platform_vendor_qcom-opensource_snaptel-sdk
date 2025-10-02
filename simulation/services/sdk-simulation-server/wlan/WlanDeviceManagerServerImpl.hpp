/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLAN_DEVICE_MANAGER_SERVER_HPP
#define WLAN_DEVICE_MANAGER_SERVER_HPP

#include <telux/wlan/WlanDeviceManager.hpp>

#include "event/ServerEventManager.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/wlan_simulation.grpc.pb.h"

#include "protos/proto-src/common_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class WlanDeviceManagerServerImpl final
    : public wlanStub::WlanDeviceService::Service,
      public IServerEventListener,
      public std::enable_shared_from_this<WlanDeviceManagerServerImpl> {
public:
  WlanDeviceManagerServerImpl();
  ~WlanDeviceManagerServerImpl();

  // gRPC method implementations for IWlanDeviceManager API
  grpc::Status InitService(ServerContext *context,
                           const ::google::protobuf::Empty *request,
                           wlanStub::GetServiceStatusReply *response) override;

  grpc::Status Enable(ServerContext *context,
                      const wlanStub::EnableRequest *request,
                      wlanStub::DefaultReply *response) override;

  grpc::Status SetMode(ServerContext *context,
                       const wlanStub::SetModeRequest *request,
                       wlanStub::DefaultReply *response) override;

  grpc::Status GetConfig(ServerContext *context,
                         const ::google::protobuf::Empty *request,
                         wlanStub::GetConfigResponse *response) override;

  grpc::Status GetStatus(ServerContext *context,
                         const ::google::protobuf::Empty *request,
                         wlanStub::GetStatusResponse *response) override;

  grpc::Status
  SetActiveCountry(ServerContext *context,
                   const wlanStub::SetActiveCountryRequest *request,
                   wlanStub::DefaultReply *response) override;

  grpc::Status
  GetRegulatoryParams(ServerContext *context,
                      const ::google::protobuf::Empty *request,
                      wlanStub::GetRegulatoryParamsResponse *response) override;

  grpc::Status SetTxPower(ServerContext *context,
                          const wlanStub::SetTxPowerRequest *request,
                          wlanStub::DefaultReply *response) override;

  grpc::Status GetTxPower(ServerContext *context,
                          const ::google::protobuf::Empty *request,
                          wlanStub::GetTxPowerResponse *response) override;

  // IServerEventListener callback for unsolicited events
  void onEventUpdate(::eventService::UnsolicitedEvent event) override;

private:
  std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

  // Helper methods for event handling
  void onEventUpdate(std::string event);
  void handleWlanServiceStatusChangeEvent(std::string event);
  void handleWlanTempCrossedEvent(std::string event);
  void handleWlanEnableChangedEvent(std::string event);
};

#endif // WLAN_DEVICE_MANAGER_SERVER_HPP