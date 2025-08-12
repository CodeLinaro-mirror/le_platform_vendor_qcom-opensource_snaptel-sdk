/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanDeviceManagerStub.hpp"
#include "WlanCommonUtilsStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include <chrono>
#include <thread>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

const int WLAN_DEFAULT_DELAY = 100;        // Default callback delay in ms
const int WLAN_SKIP_CALLBACK = -1;         // Sentinel for skipping callback
const char *WLAN_FILTER_NAME = "wlan_dev"; // Event filter name

namespace telux {
namespace wlan {

WlanDeviceManagerStub::WlanDeviceManagerStub()
    : subSystemStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
  LOG(DEBUG, __FUNCTION__);
  taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
  listenerMgr_ =
      std::make_shared<telux::common::ListenerManager<IWlanListener>>();
}

WlanDeviceManagerStub::~WlanDeviceManagerStub() {
  LOG(DEBUG, __FUNCTION__);
  if (taskQ_) {
    taskQ_ = nullptr;
  }
}

telux::common::Status
WlanDeviceManagerStub::init(telux::common::InitResponseCb callback) {
  LOG(DEBUG, __FUNCTION__);
  initCb_ = callback;
  auto f = std::async(std::launch::async, [this, callback]() {
             this->initSync(callback);
           }).share();
  taskQ_->add(f);
  return telux::common::Status::SUCCESS;
}

void WlanDeviceManagerStub::initSync(telux::common::InitResponseCb callback) {
  LOG(DEBUG, __FUNCTION__);
  std::lock_guard<std::mutex> lck(initMtx_);
  stub_ = CommonUtils::getGrpcStub<::wlanStub::WlanDeviceService>();

  ::wlanStub::GetServiceStatusReply response;
  ClientContext context;
  ::google::protobuf::Empty request;

  grpc::Status reqStatus = stub_->InitService(&context, request, &response);
  telux::common::ServiceStatus cbStatus =
      telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
  int cbDelay = WLAN_DEFAULT_DELAY;

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
    std::vector<std::string> filters = {WLAN_FILTER_NAME};
    telux::common::ClientEventManager::getInstance().registerListener(
        shared_from_this(), filters);
  }

  if (callback && (cbDelay != WLAN_SKIP_CALLBACK)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    LOG(DEBUG, __FUNCTION__, " Callback Delay: ", cbDelay,
        " Callback Status: ", static_cast<int>(cbStatus));
    invokeInitCallback(cbStatus);
  }
}

void WlanDeviceManagerStub::invokeInitCallback(
    telux::common::ServiceStatus status) {
  LOG(INFO, __FUNCTION__,
      "Invoking init callback with status: ", static_cast<int>(status));
  if (initCb_) {
    initCb_(status);
  }
}

void WlanDeviceManagerStub::setSubSystemStatus(
    telux::common::ServiceStatus status) {
  LOG(DEBUG, __FUNCTION__,
      "Setting subsystem status to: ", static_cast<int>(status));
  std::lock_guard<std::mutex> lk(mtx_);
  subSystemStatus_ = status;
}

telux::common::ServiceStatus WlanDeviceManagerStub::getServiceStatus() {
  return subSystemStatus_;
}

telux::common::ErrorCode WlanDeviceManagerStub::enable(bool enable) {
  LOG(DEBUG, __FUNCTION__, "Requesting WLAN enable: ", enable);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " WLAN manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  if (!stub_) {
    LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
    return telux::common::ErrorCode::INTERNAL_ERROR;
  }

  ::wlanStub::EnableRequest request;
  ::wlanStub::DefaultReply response;
  ClientContext context;

  request.set_enable(enable);
  grpc::Status reqStatus = stub_->Enable(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " enable request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode WlanDeviceManagerStub::setMode(int numOfAp,
                                                        int numOfSta) {
  LOG(DEBUG, __FUNCTION__, "Setting WLAN mode: APs=", numOfAp,
      ", STAs=", numOfSta);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " WLAN manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  if (!stub_) {
    LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
    return telux::common::ErrorCode::INTERNAL_ERROR;
  }

  ::wlanStub::SetModeRequest request;
  ::wlanStub::DefaultReply response;
  ClientContext context;

  request.set_num_of_ap(numOfAp);
  request.set_num_of_sta(numOfSta);
  grpc::Status reqStatus = stub_->SetMode(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setMode request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode WlanDeviceManagerStub::getConfig(int &numAp,
                                                          int &numSta) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " WLAN manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  if (!stub_) {
    LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
    return telux::common::ErrorCode::INTERNAL_ERROR;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::GetConfigResponse response;
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
    numAp = response.num_ap();
    numSta = response.num_sta();
    LOG(DEBUG, __FUNCTION__, " Retrieved config: APs=", numAp,
        ", STAs=", numSta);
  }
  return error;
}

telux::common::ErrorCode
WlanDeviceManagerStub::getStatus(bool &isEnabled,
                                 std::vector<InterfaceStatus> &statusList) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " WLAN manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  if (!stub_) {
    LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
    return telux::common::ErrorCode::INTERNAL_ERROR;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::GetStatusResponse response;
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
    isEnabled = response.is_enabled();
    statusList.clear();
    for (const auto &grpcInterfaceStatus : response.status()) {
      telux::wlan::InterfaceStatus interfaceStatus;
      interfaceStatus.device =
          static_cast<telux::wlan::HwDeviceType>(grpcInterfaceStatus.device());
      for (const auto &apStatus : grpcInterfaceStatus.ap_status()) {
        interfaceStatus.apStatus.push_back(
            WlanCommonUtilsStub::convertApStatusFromGrpc(apStatus));
      }
      for (const auto &staStatus : grpcInterfaceStatus.sta_status()) {
        interfaceStatus.staStatus.push_back(
            WlanCommonUtilsStub::convertStaStatusFromGrpc(staStatus));
      }
      statusList.push_back(interfaceStatus);
    }
    LOG(DEBUG, __FUNCTION__, " Retrieved status: Enabled=", isEnabled,
        ", Interfaces=", statusList.size());
  }
  return error;
}

telux::common::ErrorCode
WlanDeviceManagerStub::setActiveCountry(std::string country) {
  LOG(DEBUG, __FUNCTION__, "Setting active country to: ", country);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " WLAN manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  if (!stub_) {
    LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
    return telux::common::ErrorCode::INTERNAL_ERROR;
  }

  ::wlanStub::SetActiveCountryRequest request;
  ::wlanStub::DefaultReply response;
  ClientContext context;

  request.set_country(country);
  grpc::Status reqStatus =
      stub_->SetActiveCountry(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setActiveCountry request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
WlanDeviceManagerStub::getRegulatoryParams(RegulatoryParams &regulatoryParams) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " WLAN manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  if (!stub_) {
    LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
    return telux::common::ErrorCode::INTERNAL_ERROR;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::GetRegulatoryParamsResponse response;
  ClientContext context;

  grpc::Status reqStatus =
      stub_->GetRegulatoryParams(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.default_reply().error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " getRegulatoryParams request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }

  if (error == telux::common::ErrorCode::SUCCESS) {
    regulatoryParams.country = response.regulatory_params().country();
    regulatoryParams.opChannel = response.regulatory_params().op_channel();
    regulatoryParams.txPowerMw = response.regulatory_params().tx_power_mw();
    regulatoryParams.opClass.clear();
    for (float op_class_val : response.regulatory_params().op_class()) {
      regulatoryParams.opClass.push_back(op_class_val);
    }
    LOG(DEBUG, __FUNCTION__,
        " Retrieved regulatory params for country: ", regulatoryParams.country);
  }
  return error;
}

telux::common::ErrorCode WlanDeviceManagerStub::setTxPower(uint32_t txPowerMw) {
  LOG(DEBUG, __FUNCTION__, "Setting Tx Power to: ", txPowerMw, " mW");
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " WLAN manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  if (!stub_) {
    LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
    return telux::common::ErrorCode::INTERNAL_ERROR;
  }

  ::wlanStub::SetTxPowerRequest request;
  ::wlanStub::DefaultReply response;
  ClientContext context;

  request.set_tx_power_mw(txPowerMw);
  grpc::Status reqStatus = stub_->SetTxPower(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " setTxPower request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }
  return error;
}

telux::common::ErrorCode
WlanDeviceManagerStub::getTxPower(uint32_t &txPowerMw) {
  LOG(DEBUG, __FUNCTION__);
  if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    LOG(ERROR, __FUNCTION__, " WLAN manager not ready. Service Unavailable.");
    return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
  }

  if (!stub_) {
    LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
    return telux::common::ErrorCode::INTERNAL_ERROR;
  }

  ::google::protobuf::Empty request;
  ::wlanStub::GetTxPowerResponse response;
  ClientContext context;

  grpc::Status reqStatus = stub_->GetTxPower(&context, request, &response);
  telux::common::ErrorCode error =
      static_cast<telux::common::ErrorCode>(response.default_reply().error());
  if (!reqStatus.ok()) {
    LOG(ERROR, __FUNCTION__,
        " getTxPower request failed: ", reqStatus.error_message());
    error = telux::common::ErrorCode::INTERNAL_ERROR;
  }

  if (error == telux::common::ErrorCode::SUCCESS) {
    txPowerMw = response.tx_power_mw();
    LOG(DEBUG, __FUNCTION__, " Retrieved Tx Power: ", txPowerMw, " mW");
  }
  return error;
}

void WlanDeviceManagerStub::onServiceStatusChange(
    telux::common::ServiceStatus status) {
  LOG(DEBUG, __FUNCTION__,
      "Service status changed to: ", static_cast<int>(status));
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IWlanListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onServiceStatusChange.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onServiceStatusChange(status);
      }
    }
  }
}

void WlanDeviceManagerStub::onEnableChanged(bool enable) {
  LOG(DEBUG, __FUNCTION__, "WLAN enable state changed to: ", enable);
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IWlanListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onEnableChanged.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onEnableChanged(enable);
      }
    }
  }
}

void WlanDeviceManagerStub::onTempCrossed(float temperature,
                                          DevicePerfState perfState) {
  LOG(DEBUG, __FUNCTION__, "Temperature crossed threshold: ", temperature,
      ", Perf State: ", static_cast<int>(perfState));
  if (listenerMgr_) {
    std::vector<std::weak_ptr<IWlanListener>> listeners;
    listenerMgr_->getAvailableListeners(listeners);
    LOG(DEBUG, __FUNCTION__, " Notifying ", listeners.size(),
        " listeners for onTempCrossed.");
    for (auto &wp : listeners) {
      if (auto sp = wp.lock()) {
        sp->onTempCrossed(temperature, perfState);
      }
    }
  }
}

telux::common::ErrorCode
WlanDeviceManagerStub::registerListener(std::weak_ptr<IWlanListener> listener) {
  LOG(DEBUG, __FUNCTION__);
  telux::common::Status status = listenerMgr_->registerListener(listener);
  if (status != telux::common::Status::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Failed to register listener.");
    return telux::common::CommonUtils::toErrorCode(status);
  }
  return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode WlanDeviceManagerStub::deregisterListener(
    std::weak_ptr<IWlanListener> listener) {
  LOG(DEBUG, __FUNCTION__);
  telux::common::Status status = listenerMgr_->deRegisterListener(listener);
  if (status != telux::common::Status::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Failed to deregister listener.");
    return telux::common::CommonUtils::toErrorCode(status);
  }
  return telux::common::ErrorCode::SUCCESS;
}

void WlanDeviceManagerStub::onEventUpdate(google::protobuf::Any event) {
  LOG(DEBUG, __FUNCTION__, " Received event update from ClientEventManager.");
  if (event.Is<::wlanStub::OnWlanEnableChanged>()) {
    ::wlanStub::OnWlanEnableChanged enableChangedEvent;
    event.UnpackTo(&enableChangedEvent);
    this->handleEnableChangedEvent(enableChangedEvent);
  } else if (event.Is<::wlanStub::OnWlanTempCrossed>()) {
    ::wlanStub::OnWlanTempCrossed tempCrossedEvent;
    event.UnpackTo(&tempCrossedEvent);
    this->handleTempCrossedEvent(tempCrossedEvent);
  } else {
    LOG(WARNING, __FUNCTION__, " Unhandled event type received.");
  }
}

void WlanDeviceManagerStub::handleEnableChangedEvent(
    ::wlanStub::OnWlanEnableChanged enableChangedEvent) {
  LOG(DEBUG, __FUNCTION__);
  bool enable = enableChangedEvent.enable();
  onEnableChanged(enable);
}

void WlanDeviceManagerStub::handleTempCrossedEvent(
    ::wlanStub::OnWlanTempCrossed tempCrossedEvent) {
  LOG(DEBUG, __FUNCTION__);
  float temperature = tempCrossedEvent.temperature();
  DevicePerfState perfState =
      static_cast<DevicePerfState>(tempCrossedEvent.perf_state());
  onTempCrossed(temperature, perfState);
}

} // namespace wlan
} // namespace telux