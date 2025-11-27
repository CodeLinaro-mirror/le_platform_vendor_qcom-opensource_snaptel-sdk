/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DUAL_DATA_MANAGER_SERVER_HPP
#define DUAL_DATA_MANAGER_SERVER_HPP

#include <telux/data/DualDataManager.hpp>

#include "libs/common/AsyncTaskQueue.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"
#include "event/ServerEventManager.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class DualDataServerImpl final : public dataStub::DualDataManager::Service,
                                 public IServerEventListener,
                                 public std::enable_shared_from_this<DualDataServerImpl> {
 public:
    DualDataServerImpl();
    ~DualDataServerImpl();

    grpc::Status InitService(ServerContext *context, const dataStub::InitRequest *request,
        dataStub::GetServiceStatusReply *response) override;

    grpc::Status GetDualDataCapability(ServerContext *context,
        const ::google::protobuf::Empty *request,
        dataStub::GetDualDataCapabilityReply *response) override;

    grpc::Status GetDualDataUsageRecommendation(ServerContext *context,
        const ::google::protobuf::Empty *request,
        dataStub::GetDualDataUsageRecommendationReply *response) override;

    grpc::Status ConfigureDdsSwitchRecommendation(ServerContext *context,
        const dataStub::ConfigureDdsSwitchRecommendationRequest *request,
        dataStub::ConfigureDdsSwitchRecommendationReply *response) override;

    grpc::Status GetDdsSwitchRecommendation(ServerContext *context,
        const ::google::protobuf::Empty *request,
        dataStub::GetDdsSwitchRecommendationReply *response) override;

    grpc::Status SetDdsSwitch(ServerContext *context, const dataStub::SetDdsSwitchRequest *request,
        dataStub::DefaultReply *response) override;

    grpc::Status RequestCurrentDdsSwitch(ServerContext *context,
        const dataStub::CurrentDdsSwitchRequest *request,
        dataStub::CurrentDdsSwitchResponse *response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    int slot_id_            = 1;
    std::string ddsTypeStr_ = "TEMPORARY";
    std::string tempType_   = "HIGH";
    std::string tempCause_  = "TEMP_CAUSE_CODE_UNKNOWN";
    std::string permCause_  = "PERM_CAUSE_CODE_UNKNOWN";
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::data::DdsInfo ddsInfo_;

    ::dataStub::UsageRecommendation::Recommendation convertUsageRecommendationStringToEnum(
        std::string recommendation);
    ::dataStub::DdsInfo::DdsType convertDdsTypeStringToEnum(const std::string &ddsTypeStr);
    ::dataStub::RecommendationDetails::TemporaryRecommendationType convertTempTypeStringToEnum(
        const std::string &tempTypeStr);
    ::dataStub::TemporaryRecommendationCauseCodes convertTemporaryCauseStringToEnum(
        const std::string &causeStr);
    ::dataStub::PermanentRecommendationCauseCodes convertPermanentCauseStringToEnum(
        const std::string &causeStr);
    void onEventUpdate(std::string event);
    void handleCapabilityChangeRequest(std::string event);
    void handleRecommendationChnageRequest(std::string event);
    void handleDdsSwitchRecommendationChangeRequest(std::string event);
};

#endif  // DUAL_DATA_MANAGER_SERVER_HPP