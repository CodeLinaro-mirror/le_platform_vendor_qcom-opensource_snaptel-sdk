/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLANCONTROLMANAGERSTUB_HPP
#define WLANCONTROLMANAGERSTUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanControlManager.hpp>

#include "common/AsyncTaskQueue.hpp"

namespace telux {
namespace wlan {

/**
 * @brief WlanControlManagerStub is an implementation of IWlanControlManager for
 * simulation.
 */
class WlanControlManagerStub : public IWlanControlManager,
                               public std::enable_shared_from_this<WlanControlManagerStub> {
 public:
    WlanControlManagerStub();
    ~WlanControlManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);
    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::ErrorCode getInterfaceStatus(std::vector<InterfaceStatus> &status) override;
    telux::common::ErrorCode setStaIpConfig(
        Id staId, StaIpConfig ipConfig, const StaStaticIpConfig &staticIpConfig) override;
    telux::common::ErrorCode getStaIpConfig(
        Id staId, StaIpConfig &ipConfig, StaStaticIpConfig &staticIpConfig) override;
    telux::common::ErrorCode setApInterworking(Id id, ApInterworking interworking) override;
    telux::common::ErrorCode getApInterworking(Id id, ApInterworking &interworking) override;
    telux::common::ErrorCode registerListener(
        std::weak_ptr<IWlanControlListener> listener) override;
    telux::common::ErrorCode deregisterListener(
        std::weak_ptr<IWlanControlListener> listener) override;

 private:
    telux::common::ServiceStatus subSystemStatus_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;

    void invokeInitCallback(telux::common::ServiceStatus status);
};

}  // namespace wlan
}  // namespace telux

#endif  // WLANCONTROLMANAGERSTUB_HPP
