/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TETHER_MANAGER_STUB_HPP
#define TETHER_MANAGER_STUB_HPP

#include "telux/data/net/TetherManager.hpp"
#include "common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

#include <vector>
#include <mutex>

namespace telux {
namespace data {
namespace net {

class TetherManagerStub : public ITetherManager,
                      public std::enable_shared_from_this<TetherManagerStub> {

public:
    TetherManagerStub();
    ~TetherManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status startBTTether(const telux::data::net::BTTetherMode  btMode,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status stopBTTether(telux::common::ResponseCallback callback = nullptr) override;

    telux::common::Status requestBTTetherStatus(telux::data::net::BTTetherCb callback) override;

    telux::common::Status registerListener(std::weak_ptr<ITetherListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<ITetherListener> listener) override;

    telux::data::OperationType getOperationType() override;

private:
    std::mutex initMtx_;
    telux::common::ServiceStatus subSystemStatus_;
    std::mutex listenersMutex_;
    std::vector<std::weak_ptr<ITetherListener>> listeners_;
    std::unique_ptr<::dataStub::TetherManager::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;

    void initSync(telux::common::InitResponseCb callback);
    void onServiceStatusChange(telux::common::ServiceStatus status);
    void setSubSystemStatus(telux::common::ServiceStatus status);
    void getAvailableListeners(std::vector<std::shared_ptr<telux::data::net::ITetherListener>> &listeners);
};

}

}

}

#endif