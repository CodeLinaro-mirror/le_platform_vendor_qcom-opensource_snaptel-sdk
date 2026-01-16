/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STAINTERFACEMANAGERSTUB_HPP
#define STAINTERFACEMANAGERSTUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/StaInterfaceManager.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "common/ListenerManager.hpp"
#include "common/event-manager/ClientEventManager.hpp"

namespace telux {
namespace wlan {

/**
 * @brief StaInterfaceManagerStub is an implementation of IStaInterfaceManager
 * for simulation. It handles gRPC communication with the simulation server for
 * STA management and dispatches events to registered listeners.
 */
class StaInterfaceManagerStub : public IStaInterfaceManager,
                                public IStaListener,
                                public telux::common::IEventListener,
                                public std::enable_shared_from_this<StaInterfaceManagerStub> {
 public:
    StaInterfaceManagerStub();
    ~StaInterfaceManagerStub();

    telux::common::Status init();
    telux::common::ErrorCode registerListener(std::weak_ptr<IStaListener> listener) override;
    telux::common::ErrorCode deregisterListener(std::weak_ptr<IStaListener> listener) override;

    telux::common::ErrorCode setIpConfig(
        Id staId, StaIpConfig ipConfig, StaStaticIpConfig staticIpConfig) override;
    telux::common::ErrorCode setBridgeMode(Id staId, StaBridgeMode bridgeMode) override;
    telux::common::ErrorCode enableHotspot2(Id staId, bool enable) override;
    telux::common::ErrorCode getConfig(std::vector<StaConfig> &config) override;
    telux::common::ErrorCode getStatus(std::vector<StaStatus> &status) override;
    telux::common::ErrorCode startScan(Id staId) override;
    telux::common::ErrorCode addNetworkConfig(
        Id staId, const StaNetworkConfigEntry &network) override;
    telux::common::ErrorCode removeNetworkConfig(Id staId, NetworkId networkId) override;
    telux::common::ErrorCode getNetworkConfigs(
        Id staId, std::vector<StaNetworkConfigInfo> &network) override;
    telux::common::ErrorCode connect(Id staId, NetworkId networkId) override;
    telux::common::ErrorCode disconnect(Id staId) override;
    telux::common::ErrorCode manageStaService(Id staId, ServiceOperation opr) override;

    void onStationStatusChanged(std::vector<StaStatus> status) override;
    void onScanResultUpdated(const StaScanResult &staScanResult) override;
    void onStationBandChanged(BandType radio) override;
};

}  // namespace wlan
}  // namespace telux

#endif  // STAINTERFACEMANAGERSTUB_HPP