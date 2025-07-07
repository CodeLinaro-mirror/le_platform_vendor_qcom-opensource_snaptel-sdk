/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ApSimProfileManagerStub.hpp
 *
 * @brief      Implementation of ApSimProfileManager
 *
 */

#ifndef APSIMPROFILE_MANAGER_STUB_HPP
#define APSIMPROFILE_MANAGER_STUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/ApSimProfileManager.hpp>

#include "common/event-manager/ClientEventManager.hpp"
#include "common/ListenerManager.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"

namespace telux {
namespace tel {

class ApSimProfileManagerStub : public IApSimProfileManager,
                                public IEventListener,
                                public std::enable_shared_from_this<ApSimProfileManagerStub> {
public:
    ApSimProfileManagerStub();
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~ApSimProfileManagerStub();

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener(std::weak_ptr<IApSimProfileListener> listener) override;
    telux::common::Status
        deregisterListener(std::weak_ptr<telux::tel::IApSimProfileListener> listener) override;

    telux::common::Status sendRetrieveProfileListResponse(SlotId slotId,
        ApduExchangeStatus result, uint32_t referenceId, std::vector<std::string> profileIccIds,
        common::ResponseCallback callback) override;
    telux::common::Status sendProfileOperationResponse(SlotId slotId,  ApduExchangeStatus result,
        uint32_t referenceId, common::ResponseCallback callback) override;

    void onServiceStatusChange(telux::common::ServiceStatus status);

    void cleanup();
    void onEventUpdate(google::protobuf::Any event)  override;

private:
    int noOfSlots_ = 0;
    std::mutex mtx_;
    telux::common::InitResponseCb initCb_;
    int cbDelay_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<telux::common::ListenerManager<IApSimProfileListener>> listenerMgr_;
    std::unique_ptr<::telStub::ApSimProfileService::Stub> stub_;
    telux::common::ServiceStatus subSystemStatus_;
    void setServiceStatus(telux::common::ServiceStatus status);
    void initSync();
    void handleRetrieveProfileListRequest(::telStub::ProfileListRequestEvent event);
    void handleProfileOperationRequest(::telStub::ProfileOperationRequestEvent event);
};

} // end of namespace tel
} // end of namespace telux

#endif // APSIMPROFILE_MANAGER_STUB_HPP
