/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ApInterfaceManagerStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include <thread>


namespace telux {
namespace wlan {

ApInterfaceManagerStub::ApInterfaceManagerStub() {
  LOG(DEBUG, __FUNCTION__);
}

ApInterfaceManagerStub::~ApInterfaceManagerStub() {
  LOG(DEBUG, __FUNCTION__);
}

telux::common::Status ApInterfaceManagerStub::init() {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::Status::SUCCESS;
}

telux::common::ErrorCode ApInterfaceManagerStub::setConfig(ApConfig config) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode
ApInterfaceManagerStub::setSecurityConfig(Id apId, ApSecurity apSecurity) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode ApInterfaceManagerStub::setSsid(Id apId,
                                                         std::string ssid) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode ApInterfaceManagerStub::setVisibility(Id apId,
                                                               bool isVisible) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode
ApInterfaceManagerStub::setElementInfoConfig(Id apId,
                                             ApElementInfoConfig config) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode
ApInterfaceManagerStub::setPassPhrase(Id apId, std::string passPhrase) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode
ApInterfaceManagerStub::getConfig(std::vector<ApConfig> &config) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode
ApInterfaceManagerStub::getStatus(std::vector<ApStatus> &status) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode ApInterfaceManagerStub::getConnectedDevices(
    std::vector<DeviceInfo> &clientsInfo) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode
ApInterfaceManagerStub::manageApService(Id apId, ServiceOperation opr) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

void ApInterfaceManagerStub::onApDeviceStatusChanged(
    ApDeviceConnectionEvent event, std::vector<DeviceIndInfo> info) {
  LOG(DEBUG, __FUNCTION__);
  (void)event;
  (void)info;
}

void ApInterfaceManagerStub::onApBandChanged(BandType radio) {
  LOG(DEBUG, __FUNCTION__);
  (void)radio;
}

void ApInterfaceManagerStub::onApConfigChanged(Id apId) {
  LOG(DEBUG, __FUNCTION__);
  (void) apId;
}

telux::common::ErrorCode
ApInterfaceManagerStub::registerListener(std::weak_ptr<IApListener> listener) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode ApInterfaceManagerStub::deregisterListener(
    std::weak_ptr<IApListener> listener) {
  LOG(DEBUG, __FUNCTION__);
  return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

} // namespace wlan
} // namespace telux