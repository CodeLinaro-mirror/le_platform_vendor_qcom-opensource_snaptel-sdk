/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanDeviceManagerStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include <thread>

namespace telux {
namespace wlan {

WlanDeviceManagerStub::WlanDeviceManagerStub()
   : subSystemStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
}

WlanDeviceManagerStub::~WlanDeviceManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status WlanDeviceManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    initCb_ = callback;

    auto f = std::async(std::launch::async, [this, callback]() {
        this->invokeInitCallback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
    }).share();
    taskQ_->add(f);
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    return telux::common::Status::SUCCESS;
}

void WlanDeviceManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__, "Invoking init callback with status: ", static_cast<int>(status));
    if (initCb_) {
        initCb_(status);
    }
}

telux::common::ServiceStatus WlanDeviceManagerStub::getServiceStatus() {
    return subSystemStatus_;
}

telux::common::ErrorCode WlanDeviceManagerStub::enable(bool enable) {
    LOG(DEBUG, __FUNCTION__, "Requesting WLAN enable: ", enable);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanDeviceManagerStub::setMode(int numOfAp, int numOfSta) {
    LOG(DEBUG, __FUNCTION__, "Setting WLAN mode: APs=", numOfAp, ", STAs=", numOfSta);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanDeviceManagerStub::getConfig(int &numAp, int &numSta) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanDeviceManagerStub::getStatus(
    bool &isEnabled, std::vector<InterfaceStatus> &statusList) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanDeviceManagerStub::setActiveCountry(std::string country) {
    LOG(DEBUG, __FUNCTION__, "Setting active country to: ", country);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanDeviceManagerStub::getRegulatoryParams(
    RegulatoryParams &regulatoryParams) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanDeviceManagerStub::setTxPower(uint32_t txPowerMw) {
    LOG(DEBUG, __FUNCTION__, "Setting Tx Power to: ", txPowerMw, " mW");
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanDeviceManagerStub::getTxPower(uint32_t &txPowerMw) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

void WlanDeviceManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    (void)status;
}

void WlanDeviceManagerStub::onEnableChanged(bool enable) {
    LOG(DEBUG, __FUNCTION__);
    (void)enable;
}

void WlanDeviceManagerStub::onTempCrossed(float temperature, DevicePerfState perfState) {
    LOG(DEBUG, __FUNCTION__);
    (void)temperature;
    (void)perfState;
}

telux::common::ErrorCode WlanDeviceManagerStub::registerListener(
    std::weak_ptr<IWlanListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanDeviceManagerStub::deregisterListener(
    std::weak_ptr<IWlanListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

}  // namespace wlan
}  // namespace telux