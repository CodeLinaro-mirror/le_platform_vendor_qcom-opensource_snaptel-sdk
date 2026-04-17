/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file   SimulationServer.cpp
 * @brief  Implements the @ref SimulationServer class.
 */

#include <iostream>
#include <thread>
#include <telux/common/CommonDefines.hpp>

#include <grpcpp/grpcpp.h>

#include "../../libs/common/SimulationConfigParser.hpp"
#include "../../libs/common/Logger.hpp"

#include "SimulationServer.hpp"
#include "satcom/NtnServerImpl.hpp"
#include "event/EventService.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

#define LOCAL_HOST "127.0.0.1"

/* Defining the SimulationServer app instance */
SimulationServer::SimulationServer() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

SimulationServer::~SimulationServer() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = nullptr;
}

SimulationServer &SimulationServer::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static SimulationServer instance;
    return instance;
}

telux::common::Status SimulationServer::start() {
    LOG(DEBUG, __FUNCTION__);

    std::thread grpc_sim_server([this] { startGrpcServer(); });

    grpc_sim_server.join();
    return telux::common::Status::SUCCESS;
}

std::string SimulationServer::createServerAddress(std::string ipAddress, std::string portNo) {
    return ipAddress + ":" + portNo;
}

void SimulationServer::startGrpcServer() {
    LOG(DEBUG, __FUNCTION__);
    std::string serverIpAddress = LOCAL_HOST;
    auto config                 = std::make_shared<SimulationConfigParser>();
    std::string serverAddress = createServerAddress(serverIpAddress, config->getValue("RPC_PORT"));
    std::string server_address(serverAddress);

    grpc::EnableDefaultHealthCheckService(true);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    auto &eventService = EventService::getInstance();
    builder.RegisterService(&eventService);

    std::shared_ptr<NtnServerImpl> NtnService = std::make_shared<NtnServerImpl>();
    builder.RegisterService(NtnService.get());

    std::unique_ptr<Server> server(builder.BuildAndStart());
    LOG(DEBUG, __FUNCTION__, " Server listening on ", server_address);
    server->Wait();
}

/**
 * Main routine
 */
int main(int argc, char **argv) {
    auto &simulationServer = SimulationServer::getInstance();

    if (simulationServer.start() != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed to start ", APP_NAME);
        return -1;
    }

    std::cout << "\nInfo: Exiting application..." << std::endl;
    return 0;
}
