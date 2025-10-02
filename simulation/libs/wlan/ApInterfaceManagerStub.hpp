/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef APINTERFACEMANAGERSTUB_HPP
#define APINTERFACEMANAGERSTUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/ApInterfaceManager.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "protos/proto-src/wlan_simulation.grpc.pb.h"

namespace telux {
namespace wlan {

/**
 * @brief ApInterfaceManagerStub is an implementation of IApInterfaceManager for
 * simulation. It handles gRPC communication with the simulation server for AP
 * management and dispatches events to registered listeners.
 */
class ApInterfaceManagerStub
    : public IApInterfaceManager,
      public IApListener,
      public telux::common::IEventListener,
      public std::enable_shared_from_this<ApInterfaceManagerStub> {
public:
  ApInterfaceManagerStub();
  ~ApInterfaceManagerStub();

  telux::common::Status init();
  void initSync();
  void setSubSystemStatus(telux::common::ServiceStatus status);
  telux::common::ServiceStatus getServiceStatus();

  telux::common::ErrorCode
  registerListener(std::weak_ptr<IApListener> listener) override;
  telux::common::ErrorCode
  deregisterListener(std::weak_ptr<IApListener> listener) override;

  telux::common::ErrorCode setConfig(ApConfig config) override;
  telux::common::ErrorCode setSecurityConfig(Id apId,
                                             ApSecurity apSecurity) override;
  telux::common::ErrorCode setSsid(Id apId, std::string ssid) override;
  telux::common::ErrorCode setVisibility(Id apId, bool isVisible) override;
  telux::common::ErrorCode
  setElementInfoConfig(Id apId, ApElementInfoConfig config) override;
  telux::common::ErrorCode setPassPhrase(Id apId,
                                         std::string passPhrase) override;
  telux::common::ErrorCode getConfig(std::vector<ApConfig> &config) override;
  telux::common::ErrorCode getStatus(std::vector<ApStatus> &status) override;
  telux::common::ErrorCode
  getConnectedDevices(std::vector<DeviceInfo> &clientsInfo) override;
  telux::common::ErrorCode manageApService(Id apId,
                                           ServiceOperation opr) override;

  void onApDeviceStatusChanged(ApDeviceConnectionEvent event,
                               std::vector<DeviceIndInfo> info) override;
  void onApBandChanged(BandType radio) override;
  void onApConfigChanged(Id apId) override;

  void onEventUpdate(google::protobuf::Any event) override;

private:
  std::mutex mtx_;
  std::mutex initMtx_;
  telux::common::ServiceStatus subSystemStatus_;
  std::unique_ptr<::wlanStub::ApInterfaceService::Stub> stub_;
  std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
  std::shared_ptr<telux::common::ListenerManager<IApListener>> listenerMgr_;

  void handleOnApDeviceStatusChangedEvent(
      ::wlanStub::OnApDeviceStatusChanged apDeviceStatusChangedEvent);
  void
  handleOnApBandChangedEvent(::wlanStub::OnApBandChanged apBandChangedEvent);
  void handleOnApConfigChangedEvent(
      ::wlanStub::OnApConfigChanged apConfigChangedEvent);
};

} // namespace wlan
} // namespace telux

#endif // APINTERFACEMANAGERSTUB_HPP