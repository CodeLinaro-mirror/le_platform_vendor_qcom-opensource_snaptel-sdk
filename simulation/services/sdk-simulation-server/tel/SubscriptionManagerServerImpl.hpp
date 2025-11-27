/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SubscriptionManagerServerImpl.hpp
 *             It handles solicited requests and formulates responses to get the subscription
 *             information and updates new subscription information injected by event injector
 *             utility.
 *
 */

#ifndef SUBSCRIPTION_MANAGER_SERVER_HPP
#define SUBSCRIPTION_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "event/ServerEventManager.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/EventService.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using commonStub::ServiceStatus;
using telStub::SubscriptionService;

class SubscriptionManagerServerImpl final
   : public telStub::SubscriptionService::Service,
     public IServerEventListener,
     public std::enable_shared_from_this<SubscriptionManagerServerImpl> {

 public:
    SubscriptionManagerServerImpl();
    grpc::Status InitService(ServerContext *context, const google::protobuf::Empty *request,
        commonStub::GetServiceStatusReply *response) override;
    grpc::Status GetServiceStatus(ServerContext *context, const google::protobuf::Empty *request,
        commonStub::GetServiceStatusReply *response) override;
    grpc::Status IsSubsystemReady(ServerContext *context, const google::protobuf::Empty *request,
        commonStub::IsSubsystemReadyReply *response) override;
    grpc::Status GetSubscription(ServerContext *context,
        const ::telStub::GetSubscriptionRequest *request, telStub::Subscription *response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message);

 private:
    Json::Value rootObj;
    grpc::Status readJson();
    void handleEvent(std::string token, std::string event);
    void handlesubscriptionInfoChanged(std::string eventParams);
    void onEventUpdate(std::string event);
};

#endif  // SUBSCRIPTION_MANAGER_SERVER_HPP
