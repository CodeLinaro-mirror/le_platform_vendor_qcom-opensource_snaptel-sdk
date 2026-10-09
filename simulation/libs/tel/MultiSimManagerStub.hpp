/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       MultiSimManagerStub.hpp
 *
 * @brief      Implementation of IMultiSimManager
 *
 */

#ifndef MULTISIM_MANAGER_STUB_HPP
#define MULTISIM_MANAGER_STUB_HPP

#include <telux/tel/MultiSimManager.hpp>
#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::MultiSimService;

namespace telux {
namespace tel {

class MultiSimManagerStub : public IMultiSimManager,
                            public IEventListener,
                            public std::enable_shared_from_this<MultiSimManagerStub> {
 public:
    MultiSimManagerStub();
    telux::common::Status init(telux::common::InitResponseCb callback);
    ~MultiSimManagerStub();
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<IMultiSimListener>) override;
    telux::common::Status deregisterListener(std::weak_ptr<IMultiSimListener>) override;
    telux::common::Status getSlotCount(int &count) override;
    telux::common::Status requestHighCapability(HighCapabilityCallback callback) override;
    telux::common::Status setHighCapability(int slotId, common::ResponseCallback callback) override;
    telux::common::Status switchActiveSlot(
        SlotId slotId, common::ResponseCallback callback) override;
    telux::common::Status requestSlotStatus(SlotStatusCallback callback) override;
    telux::common::Status configureLogicalSlotMapping(
        std::map<LogicalSlotId, LogicalSlotMapInfo> mapInfo,
        common::ResponseCallback callback) override;
    telux::common::ErrorCode getLogicalSlotMapping(
        std::map<LogicalSlotId, LogicalSlotMapInfo> &mapInfo) override;
    telux::common::ErrorCode getPhysicalSlotStatus(
        std::map<PhysicalSlotId, SimSlotStatus> &slotStatus) override;
    void onEventUpdate(google::protobuf::Any event);
    void cleanup();

 private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::unique_ptr<::telStub::MultiSimService::Stub> stub_;
    void initSync(telux::common::InitResponseCb callback);
    void invokeCallback(
        telux::common::ResponseCallback callback, telux::common::ErrorCode error, int cbDelayMs);
    std::vector<std::weak_ptr<IMultiSimListener>> listeners_;
    std::mutex listenersMutex_;
    int slotCount_                              = 1;
    telux::common::ServiceStatus serviceStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    PortState toClientPortState(::telStub::PortState protoState);
    SlotState toClientSlotState(::telStub::SlotState protoState);
    int cbDelay_;
};

}  // end of namespace tel

}  // end of namespace telux

#endif  // MULTISIM_MANAGER_STUB_HPP
