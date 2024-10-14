/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "common/Logger.hpp"
#include "DeviceInfoManagerStub.hpp"
#include "libs/common/CommonUtils.hpp"

#define RPC_FAIL_SUFFIX " RPC Request failed - "
#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace platform {

using namespace telux::common;

DeviceInfoManagerStub::DeviceInfoManagerStub()
   : serviceStatus_(ServiceStatus::SERVICE_UNAVAILABLE)
   , initCb_(nullptr) {
    LOG(DEBUG, __FUNCTION__);

    stub_ = CommonUtils::getGrpcStub<DeviceInfoManagerService>();
}

DeviceInfoManagerStub::~DeviceInfoManagerStub() {
    LOG(DEBUG, __FUNCTION__);

    cleanup();
}

ServiceStatus DeviceInfoManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lock(mutex_);
    return serviceStatus_;
}

void DeviceInfoManagerStub::setServiceStatus(ServiceStatus cbStatus, int cbDelay) {
    LOG(DEBUG, __FUNCTION__);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        serviceStatus_ = cbStatus;
        if (cbStatus != ServiceStatus::SERVICE_AVAILABLE) {
            isInitsyncTriggered_ = false;
        }
    }

    if (initCb_ && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        initCb_(cbStatus);
    }

    std::vector<std::weak_ptr<IDeviceInfoListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    LOG(DEBUG, __FUNCTION__, ": Notifying service status: ", static_cast<int>(cbStatus),
        " to listeners: ", applisteners.size());
    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onServiceStatusChange(cbStatus);
        }
    }
}

Status DeviceInfoManagerStub::init(InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    Status status = telux::common::Status::SUCCESS;
    std::lock_guard<std::mutex> lock(mutex_);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IDeviceInfoListener>>();
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " FAILED to create ListenerManager instance");
        status = Status::FAILED;
    }

    initCb_ = callback;
    auto f  = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    taskQ_.add(f);

    return status;
}

void DeviceInfoManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    initCb_       = nullptr;
}

void DeviceInfoManagerStub::initSync() {
    LOG(DEBUG, __FUNCTION__);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isInitsyncTriggered_) {
            LOG(DEBUG, __FUNCTION__, " Initialization is already triggered");
            return;
        } else {
            isInitsyncTriggered_ = true;
        }
    }

    telux::common::ServiceStatus status = telux::common::ServiceStatus::SERVICE_FAILED;
    ::platformStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    int cbDelay = DEFAULT_DELAY;

    ::grpc::Status reqstatus = stub_->InitService(&context, request, &response);

    if(!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        setServiceStatus(telux::common::ServiceStatus::SERVICE_FAILED, cbDelay);
    }

    status = static_cast<telux::common::ServiceStatus>(response.service_status());
    cbDelay = static_cast<int>(response.delay());
    setServiceStatus(status, cbDelay);
}

Status DeviceInfoManagerStub::registerListener(std::weak_ptr<IDeviceInfoListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return listenerMgr_->registerListener(listener);
}

Status DeviceInfoManagerStub::deregisterListener(std::weak_ptr<IDeviceInfoListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return listenerMgr_->deRegisterListener(listener);
}

Status DeviceInfoManagerStub::getPlatformVersion(PlatformVersion &pv) {
    LOG(DEBUG, __FUNCTION__);

    if (serviceStatus_ != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        return Status::NOTREADY;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    ::platformStub::PlatformVersionInfo response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->GetPlatformVersion(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        LOG(ERROR, __FUNCTION__, "Get platform version failed");
    }

    status = static_cast<telux::common::Status>(response.reply().status());
    if (status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, "Get platform version successful");
        pv.modem = response.modem_details();
        pv.integratedApp = response.integrated_app();
        pv.externalApp = response.external_app();
        pv.meta = response.meta_details();
    }

    return status;
}


Status DeviceInfoManagerStub::getIMEI(std::string &imei) {
    LOG(DEBUG, __FUNCTION__);

    if (serviceStatus_ != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        return Status::NOTREADY;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    ::platformStub::PlatformImeiInfo response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    ::grpc::Status reqstatus = stub_->GetIMEI(&context, request, &response);

    status = static_cast<telux::common::Status>(response.reply().status());

    if (!reqstatus.ok()) {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        LOG(ERROR, __FUNCTION__, "Unable to get IMEI");
    }

    status = static_cast<telux::common::Status>(response.reply().status());
    if (status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " IMEI is ", imei);
        imei = response.imei_info();
    }

    return status;
}

}  // end of namespace platform
}  // end of namespace telux
