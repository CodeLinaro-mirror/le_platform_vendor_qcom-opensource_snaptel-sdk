/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ETHERNET_MANAGER_STUB_HPP
#define ETHERNET_MANAGER_STUB_HPP

#include <telux/data/EthernetManager.hpp>
#include <telux/common/CommonDefines.hpp>

#include "common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

#define ETHERNET_MANAGER_FILTER "eth_manager"

using ::dataStub::EthernetManager;

namespace telux {
namespace data {

class DataEventListener;

class EthernetManagerStub : public IEthernetManager,
                            public std::enable_shared_from_this<EthernetManagerStub> {
public:
    EthernetManagerStub();
    ~EthernetManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status setEthernetNicConfig(const EthConfig&  ethConfig,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status getEthernetNicConfig(EthConfigCb callback) override;
    telux::common::Status activateLAN(telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status registerListener(std::weak_ptr<IEthernetListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IEthernetListener> listener) override;
    telux::data::OperationType getOperationType() override;

    void handleSetEthernetNicConfigEvent(const ::dataStub::SetEthernetNicConfigRequest& event);
    void handleGetEthernetNicConfigEvent(const ::dataStub::GetEthernetNicConfigReply& event);
    void handleActivateLANEvent(const ::dataStub::DefaultReply& event);

private:
    std::mutex initMtx_;
    std::mutex mtx_;
    std::condition_variable cv_;

    bool ready_ = false;
    telux::common::ServiceStatus subSystemStatus_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::unique_ptr<::dataStub::EthernetManager::Stub> stub_;
    std::shared_ptr<DataEventListener> eventListener_;
    std::vector<std::weak_ptr<IEthernetListener>> listeners_;

    void initSync(telux::common::InitResponseCb callback);
    //bool waitForInitialization();
    void setSubsystemReady(bool status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay);
    void getAvailableListeners(
        std::vector<std::shared_ptr<IEthernetListener>> &listeners);
    void invokeEthernetListener(std::shared_ptr<IEthernetManager> manager);
    void onServiceStatusChange(telux::common::ServiceStatus status);
    bool isSubsystemReady();
};

} // end of namespace data

} // end of namespace telux

#endif // ETHERNET_MANAGER_STUB_HPP
