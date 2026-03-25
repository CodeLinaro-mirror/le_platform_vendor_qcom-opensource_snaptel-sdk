/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_KEEPALIVE_SERVER_IMPL_HPP
#define TELUX_KEEPALIVE_SERVER_IMPL_HPP

#include <telux/data/KeepAliveManager.hpp>

#include "data/DataConnectionServerImpl.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "event/ServerEventManager.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class KeepAliveServerImpl : public dataStub::KeepAliveManager::Service,
                            public IServerEventListener,
                            public std::enable_shared_from_this<KeepAliveServerImpl> {
 public:
    KeepAliveServerImpl(std::shared_ptr<DataConnectionServerImpl> dcmServerImpl);

    ~KeepAliveServerImpl();

    grpc::Status InitService(grpc::ServerContext *context, const dataStub::InitRequest *request,
        dataStub::GetServiceStatusReply *response) override;

    grpc::Status EnableTCPMonitor(grpc::ServerContext *context,
        const dataStub::EnableTCPMonitorRequest *request,
        dataStub::EnableTCPMonitorReply *response) override;

    grpc::Status DisableTCPMonitor(grpc::ServerContext *context,
        const dataStub::DisableTCPMonitorRequest *request,
        dataStub::DefaultReply *response) override;

    grpc::Status StartTCPKeepAliveOffload(grpc::ServerContext *context,
        const dataStub::StartTCPKeepAliveOffloadRequest *request,
        dataStub::StartTCPKeepAliveOffloadReply *response) override;

    grpc::Status StopTCPKeepAliveOffload(grpc::ServerContext *context,
        const dataStub::StopTCPKeepAliveOffloadRequest *request,
        dataStub::DefaultReply *response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    void onEventUpdate(std::string event);
    void handleKeepAliveStateChangeRequest(std::string event);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<DataConnectionServerImpl> dcmServerImpl_;
    std::string subsystem_ = "IKeepAliveManager";
    std::atomic<uint32_t> nextMonitorHandle_{1};  // Start from 1
    std::atomic<uint32_t> nextOffloadHandle_{10};  // Start from 10
    std::unordered_map<uint32_t, bool> activeMonitorHandles_;
    std::unordered_map<uint32_t, bool> activeOffloadHandles_;
    std::mutex handleMapMutex_;
};

#endif  // TELUX_KEEPALIVE_SERVER_IMPL_HPP