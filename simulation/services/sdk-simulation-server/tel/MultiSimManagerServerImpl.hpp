/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       MultiSimManagerServerImpl.hpp
 *
 *
 */

#ifndef MULTI_SIM_MANAGER_SERVER_HPP
#define MULTI_SIM_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <type_traits>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "libs/common/CommonUtils.hpp"
#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using commonStub::GetServiceStatusReply;
using commonStub::ServiceStatus;
using telStub::MultiSimService;

class MultiSimManagerServerImpl final
   : public telStub::MultiSimService::Service,
     public IServerEventListener,
     public std::enable_shared_from_this<MultiSimManagerServerImpl> {
 public:
    MultiSimManagerServerImpl();
    ~MultiSimManagerServerImpl();

    grpc::Status ConfigureLogicalSlotMapping(ServerContext *context,
        const ::telStub::ConfigureLogicalSlotMappingRequest *request,
        ::telStub::ConfigureLogicalSlotMappingResponse *response) override;

    grpc::Status GetLogicalSlotMapping(ServerContext *context,
        const google::protobuf::Empty *request,
        ::telStub::GetLogicalSlotMappingResponse *response) override;

    grpc::Status GetSimSlotStatus(ServerContext *context, const google::protobuf::Empty *request,
        ::telStub::GetSimSlotStatusResponse *response) override;

    grpc::Status GetServiceStatus(ServerContext *context,
        const commonStub::GetServiceStatusRequest *request,
        commonStub::GetServiceStatusReply *response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    Json::Value rootObjSystemStateSlot1_;
    Json::Value rootObjSystemStateSlot2_;
    Json::Value rootObjApiResponseSlot1_;
    Json::Value rootObjApiResponseSlot2_;
    std::map<int, Json::Value> jsonObjSystemStateSlot_;
    std::map<int, std::string> jsonObjSystemStateFileName_;
    std::map<int, Json::Value> jsonObjApiResponseSlot_;
    std::map<int, std::string> jsonObjApiResponseFileName_;

    grpc::Status readJson();
    bool isCallbackNeeded(Json::Value rootObj, std::string apiname);
    void getJsonForSystemData(int phoneId, std::string &jsonfilename, Json::Value &rootObj);
    void getJsonForApiResponseSlot(int phoneId, std::string &jsonfilename, Json::Value &rootObj);
    void getApiConfigureFromJson(const int slotId, const std::string apiname,
        telux::common::Status &status, telux::common::ErrorCode &ec, int &delay);
    void handleEvent(std::string token, std::string event);
    void notifySlotStatusChanged(const Json::Value &slotStatusArray);
    bool handleSlotStatusChanged(
        std::string eventParams, ::eventService::EventResponse &notification);
    void onEventUpdate(std::string event);
    // Add helper functions for port validation
    bool isPortAllowedForMode(const std::string &modeStr, int portId);
    std::string readCardModeForLogicalSlot(int logicalSlotId);
    std::string getCardMgrJsonPathForLogicalSlot(int logicalSlotId);
};

#endif  // MULTI_SIM_MANAGER_SERVER_HPP
