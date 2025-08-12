/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApInterfaceManagerStub.hpp"
#include "WlanCommonUtilsStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include <chrono>
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

const int AP_DEFAULT_DELAY = 100;       // Default callback delay in ms
const int AP_SKIP_CALLBACK = -1;        // Sentinel for skipping callback
const char *AP_FILTER_NAME = "wlan_ap"; // Event filter name

namespace telux {
namespace wlan {

ApInterfaceManagerStub::ApInterfaceManagerStub()
    : subSystemStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
  LOG(DEBUG, __FUNCTION__);
  taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
  listenerMgr_ =
      std::make_shared<telux::common::ListenerManager<IApListener>>();
}

ApInterfaceManagerStub::~ApInterfaceManagerStub() {
  LOG(DEBUG, __FUNCTION__);
  if (taskQ_) {
    taskQ_ = nullptr;
  }
}

telux::common::Status ApInterfaceManagerStub::init() {
  LOG(DEBUG, __FUNCTION__);
  auto f =
      std::async(std::launch::async, [this]() { this->initSync(); }).share();
  taskQ_->add(f);
  return telux::common::Status::SUCCESS;
}

void ApInterfaceManagerStub::initSync() {
  LOG(DEBUG, __FUNCTION__);
  std::lock_guard<std::mutex> lck(initMtx_);
  stub_ = CommonUtils::getGrpcStub<::wlanStub::ApInterfaceService>();

  ::wlanStub::GetServiceStatusReply response;
  ClientContext context;
  ::google::protobuf::Empty request{};

  grpc::Status reqStatus = stub_->InitService(&context, request, &response);
  telux::common::ServiceStatus cbStatus =
      telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
  int cbDelay = AP_DEFAULT_DELAY;

  if (reqStatus.ok()) {
    cbStatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
    cbDelay = static_cast<int>(response.delay());
    LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
  } else {
    LOG(ERROR, __FUNCTION__,
        " GetServiceStatus request failed: ", reqStatus.error_message());
  }
  setSubSystemStatus(cbStatus);

  if (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    std::vector<std::string> filters = {AP_FILTER_NAME};
    telux::common::ClientEventManager::getInstance().registerListener(
        shared_from_this(), filters);
  }

  if (cbDelay != AP_SKIP_CALLBACK) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    LOG(DEBUG, __FUNCTION__, " Callback Delay: ", cbDelay,
        " Callback Status: ", static_cast<int>(cbStatus));
  }
}

void ApInterfaceManagerStub::setSubSystemStatus(
    telux::common::ServiceStatus status) {
  LOG(DEBUG, __FUNCTION__,
      "Setting subsystem status to: ", static_cast<int>(status));
  std::lock_guard<std::mutex> lk(mtx_);
  subSystemStatus_ = status;
}

telux::common::ServiceStatus ApInterfaceManagerStub::getServiceStatus() {
  return subSystemStatus_;
}

telux::common::ErrorCode ApInterfaceManagerStub::setConfig(ApConfig config) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::SetConfigRequest request;
  *request.mutable_config() =
      WlanCommonUtilsStub::convertApConfigToGrpc(config);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->SetConfig(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setConfig request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
ApInterfaceManagerStub::setSecurityConfig(Id apId, ApSecurity apSecurity) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::SetSecurityConfigRequest request;
  request.set_ap_id(WlanCommonUtilsStub::convertIdToGrpc(apId));
  *request.mutable_ap_security() =
      WlanCommonUtilsStub::convertApSecurityToGrpc(apSecurity);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus =
      stub_->SetSecurityConfig(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setSecurityConfig request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode ApInterfaceManagerStub::setSsid(Id apId,
                                                         std::string ssid) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::SetSsidRequest request;
  request.set_ap_id(WlanCommonUtilsStub::convertIdToGrpc(apId));
  request.set_ssid(ssid);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->SetSsid(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setSsid request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode ApInterfaceManagerStub::setVisibility(Id apId,
                                                               bool isVisible) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::SetVisibilityRequest request;
  request.set_ap_id(WlanCommonUtilsStub::convertIdToGrpc(apId));
  request.set_is_visible(isVisible);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->SetVisibility(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setVisibility request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
ApInterfaceManagerStub::setElementInfoConfig(Id apId,
                                             ApElementInfoConfig config) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::SetElementInfoConfigRequest request;
  request.set_ap_id(WlanCommonUtilsStub::convertIdToGrpc(apId));
  *request.mutable_config() =
      WlanCommonUtilsStub::convertApElementInfoConfigToGrpc(config);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus =
      stub_->SetElementInfoConfig(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setElementInfoConfig request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
ApInterfaceManagerStub::setPassPhrase(Id apId, std::string passPhrase) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::SetPassPhraseRequest request;
  request.set_ap_id(WlanCommonUtilsStub::convertIdToGrpc(apId));
  request.set_pass_phrase(passPhrase);
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->SetPassPhrase(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setPassPhrase request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
ApInterfaceManagerStub::getConfig(std::vector<ApConfig> &config) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::ApGetConfigResponse response;
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
    config.clear();
    for (const auto &grpcApConfig : response.config()) {
      config.push_back(
          WlanCommonUtilsStub::convertApConfigFromGrpc(grpcApConfig));
    }
  }
  return error;
}

telux::common::ErrorCode
ApInterfaceManagerStub::getStatus(std::vector<ApStatus> &status) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::ApGetStatusResponse response;
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
    status.clear();
    for (const auto &grpcApStatus : response.status()) {
      status.push_back(
          WlanCommonUtilsStub::convertApStatusFromGrpc(grpcApStatus));
    }
  }
  return error;
}

telux::common::ErrorCode ApInterfaceManagerStub::getConnectedDevices(
    std::vector<DeviceInfo> &clientsInfo) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::GetConnectedDevicesResponse response;
  ClientContext context;

  grpc::Status reqStatus =
      stub_->GetConnectedDevices(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.default_reply().error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " getConnectedDevices request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }

  if (error == telux::common::ErrorCode::SUCCESS) {
    clientsInfo.clear();
    for (const auto &grpcDeviceInfo : response.clients_info()) {
      clientsInfo.push_back(
          WlanCommonUtilsStub::convertDeviceInfoFromGrpc(grpcDeviceInfo));
    }
  }
  return error;
}

telux::common::ErrorCode
ApInterfaceManagerStub::manageApService(Id apId, ServiceOperation opr) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " AP manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  ::wlanStub::ManageApServiceRequest request;
  request.set_ap_id(WlanCommonUtilsStub::convertIdToGrpc(apId));
  request.set_opr(WlanCommonUtilsStub::convertServiceOperationToGrpc(opr));
  ::wlanStub::DefaultReply response;
  ClientContext context;

  grpc::Status reqStatus = stub_->ManageApService(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " manageApService request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

void ApInterfaceManagerStub::onApDeviceStatusChanged(
    ApDeviceConnectionEvent event, std::vector<DeviceIndInfo> info) {
  LOG(DEBUG, __FUNCTION__);
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IApListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onApDeviceStatusChanged.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onApDeviceStatusChanged(event, info);
      }
    }
  }
}

void ApInterfaceManagerStub::onApBandChanged(BandType radio) {
  LOG(DEBUG, __FUNCTION__);
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IApListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onApBandChanged.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onApBandChanged(radio);
      }
    }
  }
}

void ApInterfaceManagerStub::onApConfigChanged(Id apId) {
  LOG(DEBUG, __FUNCTION__);
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IApListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onApConfigChanged.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onApConfigChanged(apId);
      }
    }
  }
}

telux::common::ErrorCode
ApInterfaceManagerStub::registerListener(std::weak_ptr<IApListener> listener) {
  LOG(DEBUG, __FUNCTION__);
  telux::common::Status status = listenerMgr_->registerListener(listener);
  if (status != telux::common::Status::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Failed to register listener.");
    return telux::common::CommonUtils::toErrorCode(status);
  }
  return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode ApInterfaceManagerStub::deregisterListener(
    std::weak_ptr<IApListener> listener) {
  LOG(DEBUG, __FUNCTION__);
  telux::common::Status status = listenerMgr_->deRegisterListener(listener);
  if (status != telux::common::Status::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Failed to deregister listener.");
    return telux::common::CommonUtils::toErrorCode(status);
  }
  return telux::common::ErrorCode::SUCCESS;
}

void ApInterfaceManagerStub::onEventUpdate(google::protobuf::Any event) {
  LOG(DEBUG, __FUNCTION__, " Received event update from ClientEventManager.");
  if (event.Is<::wlanStub::OnApDeviceStatusChanged>()) {
    ::wlanStub::OnApDeviceStatusChanged apDeviceStatusChangedEvent;
    event.UnpackTo(&apDeviceStatusChangedEvent);
    this->handleOnApDeviceStatusChangedEvent(apDeviceStatusChangedEvent);
  } else if (event.Is<::wlanStub::OnApBandChanged>()) {
    ::wlanStub::OnApBandChanged apBandChangedEvent;
    event.UnpackTo(&apBandChangedEvent);
    this->handleOnApBandChangedEvent(apBandChangedEvent);
  } else if (event.Is<::wlanStub::OnApConfigChanged>()) {
    ::wlanStub::OnApConfigChanged apConfigChangedEvent;
    event.UnpackTo(&apConfigChangedEvent);
    this->handleOnApConfigChangedEvent(apConfigChangedEvent);
  } else {
    LOG(WARNING, __FUNCTION__, " Unhandled event type received.");
  }
}

void ApInterfaceManagerStub::handleOnApDeviceStatusChangedEvent(
    ::wlanStub::OnApDeviceStatusChanged apDeviceStatusChangedEvent) {
  LOG(DEBUG, __FUNCTION__);
  ApDeviceConnectionEvent event =
      static_cast<ApDeviceConnectionEvent>(apDeviceStatusChangedEvent.event());
  std::vector<DeviceIndInfo> info;
  for (const auto &grpcIndInfo : apDeviceStatusChangedEvent.info()) {
    info.push_back(
        WlanCommonUtilsStub::convertDeviceIndInfoFromGrpc(grpcIndInfo));
  }
  onApDeviceStatusChanged(event, info);
}

void ApInterfaceManagerStub::handleOnApBandChangedEvent(
    ::wlanStub::OnApBandChanged apBandChangedEvent) {
  LOG(DEBUG, __FUNCTION__);
  BandType radio =
      WlanCommonUtilsStub::convertBandTypeFromGrpc(apBandChangedEvent.radio());
  onApBandChanged(radio);
}

void ApInterfaceManagerStub::handleOnApConfigChangedEvent(
    ::wlanStub::OnApConfigChanged apConfigChangedEvent) {
  LOG(DEBUG, __FUNCTION__);
  Id apId =
      WlanCommonUtilsStub::convertIdFromGrpc(apConfigChangedEvent.ap_id());
  onApConfigChanged(apId);
}

} // namespace wlan
} // namespace telux