/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef SERVING_SYSTEM_MANAGER_STUB_HPP
 #define SERVING_SYSTEM_MANAGER_STUB_HPP

#include <telux/data/ServingSystemManager.hpp>
#include "common/AsyncTaskQueue.hpp"

namespace telux {
namespace data {

class ServingSystemManagerStub : public IServingSystemManager,
                                 public IServingSystemListener {
public:
    ServingSystemManagerStub (SlotId slotId);
    ~ServingSystemManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    DrbStatus getDrbStatus() override;

    telux::common::Status requestServiceStatus(RequestServiceStatusResponseCb callback) override;

    telux::common::Status requestRoamingStatus(RequestRoamingStatusResponseCb callback) override;

    telux::common::Status makeDormant(
       telux::common::ResponseCallback callback) override;

    telux::common::Status requestNrIconType(RequestNrIconTypeResponseCb callback) override;
    SlotId getSlotId() override;

    telux::common::Status registerListener(std::weak_ptr<IServingSystemListener> listener) override;
    telux::common::Status deregisterListener(std::weak_ptr<IServingSystemListener> listener) override;

    telux::common::Status cleanup();

private:
    SlotId slotId_ = DEFAULT_SLOT_ID;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;

    void initSync();
};

} // end of namespace data
} // end of namespace telux

 #endif //SERVING_SYSTEM_MANAGER_STUB_HPP