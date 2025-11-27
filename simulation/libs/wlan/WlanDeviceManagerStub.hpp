/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLANDEVICEMANAGERSTUB_HPP
#define WLANDEVICEMANAGERSTUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"

namespace telux {
namespace wlan {

/**
 * @brief WlanDeviceManagerStub is an implementation of IWlanDeviceManager for
 * simulation. It handles gRPC communication with the simulation server for WLAN
 * device management and dispatches events to registered listeners.
 */
class WlanDeviceManagerStub : public IWlanDeviceManager,
                              public IWlanListener,
                              public telux::common::IEventListener,
                              public std::enable_shared_from_this<WlanDeviceManagerStub> {
 public:
    WlanDeviceManagerStub();
    ~WlanDeviceManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);
    telux::common::ServiceStatus getServiceStatus();

    telux::common::ErrorCode registerListener(std::weak_ptr<IWlanListener> listener) override;
    telux::common::ErrorCode deregisterListener(std::weak_ptr<IWlanListener> listener) override;

    telux::common::ErrorCode enable(bool enable) override;
    telux::common::ErrorCode setMode(int numOfAp, int numOfSta) override;
    telux::common::ErrorCode getConfig(int &numAp, int &numSta) override;
    telux::common::ErrorCode getStatus(
        bool &isEnabled, std::vector<InterfaceStatus> &status) override;
    telux::common::ErrorCode setActiveCountry(std::string country) override;
    telux::common::ErrorCode getRegulatoryParams(RegulatoryParams &regulatoryParams) override;
    telux::common::ErrorCode setTxPower(uint32_t txPowerMw) override;
    telux::common::ErrorCode getTxPower(uint32_t &txPowerMw) override;

    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onEnableChanged(bool enable) override;
    void onTempCrossed(float temperature, DevicePerfState perfState) override;

 private:
    telux::common::ServiceStatus subSystemStatus_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;

    void invokeInitCallback(telux::common::ServiceStatus status);
};

}  // namespace wlan
}  // namespace telux

#endif  // WLANDEVICEMANAGERSTUB_HPP