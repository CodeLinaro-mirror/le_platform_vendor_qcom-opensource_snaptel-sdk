/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DEVICE_INFO_MANAGER_STUB_HPP
#define DEVICE_INFO_MANAGER_STUB_HPP

#include <grpcpp/grpcpp.h>

#include "telux/platform/DeviceInfoManager.hpp"
#include "telux/platform/DeviceInfoListener.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/platform_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;

using platformStub::DeviceInfoManagerService;

namespace telux {
namespace platform {

using namespace telux::common;

class DeviceInfoManagerStub : public IDeviceInfoManager,
                              public IDeviceInfoListener,
                              public std::enable_shared_from_this<DeviceInfoManagerStub> {
 public:
    DeviceInfoManagerStub();

    /**
     * Overridden from IDeviceInfoManager
     */
    ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<IDeviceInfoListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IDeviceInfoListener> listener) override;
    ~DeviceInfoManagerStub();

    /**
     * Overridden from IDeviceInfoManager
     */
    telux::common::Status getPlatformVersion(PlatformVersion &pv) override;
    telux::common::Status getIMEI(std::string &imei) override;

    /**
     * Internal function to initialize connection to Device Info management.
     */
    telux::common::Status init(InitResponseCb initCb);

 private:
    /**
     * Internal method to set the status of Device Info management services
     */
    void setServiceStatus(ServiceStatus cbStatus, int cbDelay);
    void cleanup();
    void initSync();

    std::shared_ptr<telux::common::ListenerManager<IDeviceInfoListener>> listenerMgr_;
    std::mutex mutex_;
    ServiceStatus serviceStatus_;
    bool isInitsyncTriggered_ = false;
    telux::common::InitResponseCb initCb_;
    telux::common::AsyncTaskQueue<void> taskQ_;
    std::unique_ptr<::platformStub::DeviceInfoManagerService::Stub> stub_;
};

}  // end of namespace platform
}  // end of namespace telux

#endif  // DEVICE_INFO_MANAGER_STUB_HPP