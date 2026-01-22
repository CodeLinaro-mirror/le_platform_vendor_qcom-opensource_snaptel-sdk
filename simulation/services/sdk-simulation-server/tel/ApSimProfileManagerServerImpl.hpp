/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ApSimProfileManagerServerImpl.hpp
 *
 */

#ifndef AP_SIM_PROFILE_MANAGER_SERVER_HPP
#define AP_SIM_PROFILE_MANAGER_SERVER_HPP

#include <telux/common/CommonDefines.hpp>

#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#include "protos/proto-src/tel_simulation.grpc.pb.h"

#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"
#include <thread>

class ApSimProfileManagerServerImpl final
   : public telStub::ApSimProfileService::Service,
     public IServerEventListener,
     public std::enable_shared_from_this<ApSimProfileManagerServerImpl> {

 public:
    ApSimProfileManagerServerImpl();
    ~ApSimProfileManagerServerImpl();
    grpc::Status InitService(ServerContext *context,
        const ::commonStub::GetServiceStatusRequest *request,
        commonStub::GetServiceStatusReply *response) override;
    grpc::Status GetServiceStatus(ServerContext *context,
        const ::commonStub::GetServiceStatusRequest *request,
        commonStub::GetServiceStatusReply *response) override;
    grpc::Status SendRetrieveProfileListResponse(ServerContext *context,
        const telStub::ProfileListResponseRequest *request,
        telStub::ProfileListResponseReply *response) override;
    grpc::Status SendProfileOperationResponse(ServerContext *context,
        const telStub::ProfileOperationResponseRequest *request,
        telStub::ProfileOperationResponseReply *response) override;
    grpc::Status CleanUpService(ServerContext *context, const ::google::protobuf::Empty *request,
        ::google::protobuf::Empty *response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message) override;

 private:
    bool isProfileEnable_     = false;
    std::string profileIccid_ = "";
    void handleProfileListRequest(std::string eventParams);
    void handleProfileOperationRequest(std::string eventParams);
    void triggerChangeEvent(::eventService::EventResponse anyResponse);
    void onEventUpdate(std::string event);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

#endif  // AP_SIM_PROFILE_MANAGER_SERVER_HPP
