/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       NetworkSelectionManagerStub.hpp
 *
 * @brief      Implementation of NetworkSelectionManager
 *
 */

#ifndef NETWORK_SELECTION_MANAGER_STUB_HPP
#define NETWORK_SELECTION_MANAGER_STUB_HPP

#include <telux/tel/NetworkSelectionManager.hpp>

#include "common/event-manager/ClientEventManager.hpp"
#include "common/ListenerManager.hpp"

#include "protos/proto-src/tel_simulation.grpc.pb.h"

namespace telux {
namespace tel {

class NetworkSelectionManagerStub : public INetworkSelectionManager,
                                    public IEventListener,
                                    public std::enable_shared_from_this<INetworkSelectionManager> {
public:
    NetworkSelectionManagerStub(int phoneId, telux::common::InitResponseCb callback);
    ~NetworkSelectionManagerStub();

    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener
        (std::weak_ptr<INetworkSelectionListener> listener) override;
    telux::common::Status deregisterListener
        (std::weak_ptr<INetworkSelectionListener> listener) override;

    telux::common::Status requestNetworkSelectionMode(SelectionModeInfoCb callback) override;
    telux::common::Status setNetworkSelectionMode(NetworkSelectionMode selectMode,
        std::string mcc, std::string mnc,common::ResponseCallback callback ) override;

    telux::common::Status requestPreferredNetworks(PreferredNetworksCallback callback) override;
    telux::common::Status setPreferredNetworks(
        std::vector<PreferredNetworkInfo> preferredNetworksInfo, bool clearPrevious,
        common::ResponseCallback callback) override;

    telux::common::Status performNetworkScan(NetworkScanCallback callback) override;
    telux::common::Status performNetworkScan(NetworkScanInfo info,
        common::ResponseCallback callback) override;

    telux::common::Status
        requestNetworkSelectionMode(SelectionModeResponseCallback callback) override;

    void cleanup();

    void onEventUpdate(google::protobuf::Any event)  override;

private:
    int phoneId_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<telux::common::ListenerManager<INetworkSelectionListener>> listenerMgr_;
    std::unique_ptr<::telStub::NetworkSelectionService::Stub> stub_;
    void initSync(telux::common::InitResponseCb callback);
};

} // end of namespace tel

} // end of namespace telux

#endif // NETWORK_SELECTION_MANAGER_STUB_HPP