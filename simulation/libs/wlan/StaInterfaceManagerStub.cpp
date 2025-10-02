/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "StaInterfaceManagerStub.hpp"
#include "WlanCommonUtilsStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include <chrono>
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100     // Default callback delay in ms
#define SKIP_CALLBACK -1      // Sentinel for skipping callback
#define STA_FILTER "wlan_sta" // Event filter name

namespace telux {
namespace wlan {

StaInterfaceManagerStub::StaInterfaceManagerStub()
    : subSystemStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
  LOG(DEBUG, __FUNCTION__);
  taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
  listenerMgr_ =
      std::make_shared<telux::common::ListenerManager<IStaListener>>();
}

StaInterfaceManagerStub::~StaInterfaceManagerStub() {
  LOG(DEBUG, __FUNCTION__);
}

telux::common::Status StaInterfaceManagerStub::init() {
  LOG(DEBUG, __FUNCTION__);
  auto f =
      std::async(std::launch::async, [this]() { this->initSync(); }).share();
  taskQ_->add(f);
  return telux::common::Status::SUCCESS;
}

void StaInterfaceManagerStub::initSync() {
  LOG(DEBUG, __FUNCTION__);
  std::lock_guard<std::mutex> lck(initMtx_);
  stub_ = telux::common::CommonUtils::getGrpcStub<
      ::wlanStub::StaInterfaceService>();

  ::wlanStub::GetServiceStatusReply response;
  ClientContext context;
  ::google::protobuf::Empty request{};

  grpc::Status reqStatus = stub_->InitService(&context, request, &response);
  telux::common::ServiceStatus cbStatus =
      telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
  int cbDelay = DEFAULT_DELAY;

  if (reqStatus.ok()) {
    cbStatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
    cbDelay = static_cast<int>(response.delay());
    LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
  } else {
    LOG(ERROR, __FUNCTION__,
        " InitService request failed: ", reqStatus.error_message());
  }
  setSubSystemStatus(cbStatus);

  if (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    std::vector<std::string> filters = {STA_FILTER};
    telux::common::ClientEventManager::getInstance().registerListener(
        shared_from_this(), filters);
  }

  if (cbDelay != SKIP_CALLBACK) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    LOG(DEBUG, __FUNCTION__, " Callback Delay: ", cbDelay,
        " Callback Status: ", static_cast<int>(cbStatus));
  }
}

void StaInterfaceManagerStub::setSubSystemStatus(
    telux::common::ServiceStatus status) {
  LOG(DEBUG, __FUNCTION__,
      "Setting subsystem status to: ", static_cast<int>(status));
  std::lock_guard<std::mutex> lk(mtx_);
  subSystemStatus_ = status;
}

telux::common::ServiceStatus StaInterfaceManagerStub::getServiceStatus() {
  LOG(DEBUG, __FUNCTION__);
  return subSystemStatus_;
}

telux::common::ErrorCode
StaInterfaceManagerStub::setIpConfig(Id staId, StaIpConfig ipConfig,
                                     StaStaticIpConfig staticIpConfig) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaSetIpConfigRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  request.set_ip_config(
      telux::wlan::WlanCommonUtilsStub::convertStaIpConfigToGrpc(ipConfig));
  *request.mutable_static_ip_config() =
      telux::wlan::WlanCommonUtilsStub::convertStaStaticIpConfigToGrpc(
          staticIpConfig);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->SetIpConfig(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setIpConfig request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
StaInterfaceManagerStub::setBridgeMode(Id staId, StaBridgeMode bridgeMode) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaSetBridgeModeRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  request.set_bridge_mode(
      telux::wlan::WlanCommonUtilsStub::convertStaBridgeModeToGrpc(bridgeMode));
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->SetBridgeMode(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setBridgeMode request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode StaInterfaceManagerStub::enableHotspot2(Id staId,
                                                                 bool enable) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaEnableHotspot2Request request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  request.set_enable(enable);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->EnableHotspot2(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " enableHotspot2 request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
StaInterfaceManagerStub::getConfig(std::vector<StaConfig> &staConfigs) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::StaGetConfigResponse response;
  ClientContext context;

  grpc::Status reqStatus = stub_->GetConfig(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.default_reply().error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " getConfig request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }

  if (error == telux::common::ErrorCode::SUCCESS) {
    staConfigs.clear();
    for (const auto &grpcConfig : response.config()) {
      staConfigs.push_back(
          telux::wlan::WlanCommonUtilsStub::convertStaConfigFromGrpc(
              grpcConfig));
    }
  }
  return error;
}

telux::common::ErrorCode
StaInterfaceManagerStub::getStatus(std::vector<StaStatus> &staStatuses) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::StaGetStatusResponse response;
  ClientContext context;

  grpc::Status reqStatus = stub_->GetStatus(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.default_reply().error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " getStatus request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }

  if (error == telux::common::ErrorCode::SUCCESS) {
    staStatuses.clear();
    for (const auto &grpcStatus : response.status()) {
      staStatuses.push_back(
          telux::wlan::WlanCommonUtilsStub::convertStaStatusFromGrpc(
              grpcStatus));
    }
  }
  return error;
}

telux::common::ErrorCode StaInterfaceManagerStub::startScan(Id staId) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaStartScanRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->StartScan(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " startScan request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode StaInterfaceManagerStub::addNetworkConfig(
    Id staId, const StaNetworkConfigEntry &network) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaAddNetworkConfigRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  *request.mutable_network() =
      telux::wlan::WlanCommonUtilsStub::convertStaNetworkConfigEntryToGrpc(
          network);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus =
      stub_->AddNetworkConfig(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " addNetworkConfig request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
StaInterfaceManagerStub::removeNetworkConfig(Id staId, NetworkId networkId) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaRemoveNetworkConfigRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  request.set_network_id(networkId);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus =
      stub_->RemoveNetworkConfig(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " removeNetworkConfig request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode StaInterfaceManagerStub::getNetworkConfigs(
    Id staId, std::vector<StaNetworkConfigInfo> &networkConfigs) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaGetNetworkConfigsRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));

  ::wlanStub::StaGetNetworkConfigsResponse response;
  ClientContext context;

  grpc::Status reqStatus =
      stub_->GetNetworkConfigs(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.default_reply().error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " getNetworkConfigs request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }

  if (error == telux::common::ErrorCode::SUCCESS) {
    networkConfigs.clear();
    for (const auto &grpcNetworkConfig : response.network()) {
      networkConfigs.push_back(
          telux::wlan::WlanCommonUtilsStub::convertStaNetworkConfigInfoFromGrpc(
              grpcNetworkConfig));
    }
  }
  return error;
}

telux::common::ErrorCode StaInterfaceManagerStub::connect(Id staId,
                                                          NetworkId networkId) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaConnectRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  request.set_network_id(networkId);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->Connect(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " connect request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode StaInterfaceManagerStub::disconnect(Id staId) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaDisconnectRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->Disconnect(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " disconnect request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
StaInterfaceManagerStub::manageStaService(Id staId, ServiceOperation opr) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " STA manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::StaManageStaServiceRequest request;
  request.set_sta_id(telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(staId));
  request.set_opr(
      telux::wlan::WlanCommonUtilsStub::convertServiceOperationToGrpc(opr));
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus =
      stub_->ManageStaService(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " manageStaService request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

void StaInterfaceManagerStub::onStationStatusChanged(
    std::vector<StaStatus> staStatuses) {
  LOG(DEBUG, __FUNCTION__);
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IStaListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onStationStatusChanged.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onStationStatusChanged(staStatuses);
      }
    }
  }
}

void StaInterfaceManagerStub::onScanResultUpdated(
    const StaScanResult &staScanResult) {
  LOG(DEBUG, __FUNCTION__);
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IStaListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onScanResultUpdated.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onScanResultUpdated(staScanResult);
      }
    }
  }
}

void StaInterfaceManagerStub::onStationBandChanged(BandType band) {
  LOG(DEBUG, __FUNCTION__);
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IStaListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onStationBandChanged.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onStationBandChanged(band);
      }
    }
  }
}

telux::common::ErrorCode StaInterfaceManagerStub::registerListener(
    std::weak_ptr<IStaListener> listener) {
  LOG(DEBUG, __FUNCTION__);
  telux::common::Status status = listenerMgr_->registerListener(listener);
  if (status != telux::common::Status::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Failed to register listener.");
    return telux::common::CommonUtils::toErrorCode(status);
  }
  return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode StaInterfaceManagerStub::deregisterListener(
    std::weak_ptr<IStaListener> listener) {
  LOG(DEBUG, __FUNCTION__);
  telux::common::Status status = listenerMgr_->deRegisterListener(listener);
  if (status != telux::common::Status::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Failed to deregister listener.");
    return telux::common::CommonUtils::toErrorCode(status);
  }
  return telux::common::ErrorCode::SUCCESS;
}

void StaInterfaceManagerStub::onEventUpdate(google::protobuf::Any event) {
  LOG(DEBUG, __FUNCTION__, " Received event update from ClientEventManager.");
  if (event.Is<::wlanStub::OnStaStatusChanged>()) {
    ::wlanStub::OnStaStatusChanged staStatusChangedEvent;
    event.UnpackTo(&staStatusChangedEvent);
    this->handleStaStatusChangedEvent(staStatusChangedEvent);
  } else if (event.Is<::wlanStub::OnStaScanResultUpdated>()) {
    ::wlanStub::OnStaScanResultUpdated staScanResultUpdatedEvent;
    event.UnpackTo(&staScanResultUpdatedEvent);
    this->handleStaScanResultUpdatedEvent(staScanResultUpdatedEvent);
  } else if (event.Is<::wlanStub::OnStaBandChanged>()) {
    ::wlanStub::OnStaBandChanged staBandChangedEvent;
    event.UnpackTo(&staBandChangedEvent);
    this->handleStaBandChangedEvent(staBandChangedEvent);
  } else {
    LOG(WARNING, __FUNCTION__, " Unknown event type received for STA.");
  }
}

void StaInterfaceManagerStub::handleStaStatusChangedEvent(
    ::wlanStub::OnStaStatusChanged staStatusChangedEvent) {
  LOG(DEBUG, __FUNCTION__);
  std::vector<StaStatus> teluxStaStatuses;
  for (const auto &grpcStaStatus : staStatusChangedEvent.sta_status()) {
    teluxStaStatuses.push_back(
        telux::wlan::WlanCommonUtilsStub::convertStaStatusFromGrpc(
            grpcStaStatus));
  }
  onStationStatusChanged(teluxStaStatuses);
}

void StaInterfaceManagerStub::handleStaScanResultUpdatedEvent(
    ::wlanStub::OnStaScanResultUpdated staScanResultUpdatedEvent) {
  LOG(DEBUG, __FUNCTION__);
  StaScanResult teluxScanResult =
      telux::wlan::WlanCommonUtilsStub::convertStaScanResultFromGrpc(
          staScanResultUpdatedEvent.sta_scan_result());
  onScanResultUpdated(teluxScanResult);
}

void StaInterfaceManagerStub::handleStaBandChangedEvent(
    ::wlanStub::OnStaBandChanged staBandChangedEvent) {
  LOG(DEBUG, __FUNCTION__);
  BandType teluxBand =
      telux::wlan::WlanCommonUtilsStub::convertBandTypeFromGrpc(
          staBandChangedEvent.band());
  onStationBandChanged(teluxBand);
}

} // namespace wlan
} // namespace telux