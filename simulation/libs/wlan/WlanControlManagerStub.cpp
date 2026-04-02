/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanControlManagerStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include <thread>

namespace telux {
namespace wlan {

WlanControlManagerStub::WlanControlManagerStub()
   : subSystemStatus_(telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
}

WlanControlManagerStub::~WlanControlManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status WlanControlManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    initCb_ = callback;

    auto f = std::async(std::launch::async, [this, callback]() {
        this->invokeInitCallback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
    }).share();
    taskQ_->add(f);
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    return telux::common::Status::SUCCESS;
}

void WlanControlManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__, "Invoking init callback with status: ", static_cast<int>(status));
    if (initCb_) {
        initCb_(status);
    }
}

telux::common::ServiceStatus WlanControlManagerStub::getServiceStatus() {
    return subSystemStatus_;
}

telux::common::ErrorCode WlanControlManagerStub::getInterfaceStatus(
    std::vector<InterfaceStatus> &status) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanControlManagerStub::setStaIpConfig(
    Id staId, StaIpConfig ipConfig, const StaStaticIpConfig &staticIpConfig) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanControlManagerStub::getStaIpConfig(
    Id staId, StaIpConfig &ipConfig, StaStaticIpConfig &staticIpConfig) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanControlManagerStub::setApInterworking(
    Id id, ApInterworking interworking) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanControlManagerStub::getApInterworking(
    Id id, ApInterworking &interworking) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanControlManagerStub::registerListener(
    std::weak_ptr<IWlanControlListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

telux::common::ErrorCode WlanControlManagerStub::deregisterListener(
    std::weak_ptr<IWlanControlListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
}

}  // namespace wlan
}  // namespace telux
