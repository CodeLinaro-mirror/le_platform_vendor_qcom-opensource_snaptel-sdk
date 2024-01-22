/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/**
 * @file       SimulationServer.cpp
 *
 * @brief      Implements the @ref SimulationServer class.
 *
 */

#include <iostream>
#include <thread>
#include <telux/common/CommonDefines.hpp>

#include <grpcpp/grpcpp.h>

#include "libs/common/SimulationConfigParser.hpp"
#include "libs/common/Logger.hpp"

#include "SimulationServer.hpp"
#include "tel/CardManagerServerImpl.hpp"
#include "tel/SubscriptionManagerServerImpl.hpp"
#include "tel/SmsManagerServerImpl.hpp"
#include "data/DataConnectionServerImpl.hpp"
#include "data/DataProfileServerImpl.hpp"
#include "data/DataSettingsServerImpl.hpp"
#include "loc/LocationManagerServerImpl.hpp"
#include "loc/LocationConfiguratorServerImpl.hpp"
#include "event/EventService.hpp"
#include "sensor/SensorFeatureManagerServerImpl.hpp"

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

SimulationServer::~SimulationServer(){
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

    std::thread grpc_sim_server([this] {
            startGrpcServer();
        }
    );

    grpc_sim_server.join();
    return telux::common::Status::SUCCESS;
}

std::string SimulationServer::createServerAddress(std::string ipAddress, std::string portNo) {
    return ipAddress+ ":" + portNo;
}

void SimulationServer::startGrpcServer() {
    LOG(DEBUG, __FUNCTION__);
    std::string serverIpAddress = LOCAL_HOST;
    auto config = std::make_shared<SimulationConfigParser>();
    std::string serverAddress = createServerAddress(serverIpAddress, config->getValue("RPC_PORT"));
    std::string server_address(serverAddress);

    grpc::EnableDefaultHealthCheckService(true);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    std::shared_ptr<CardManagerServerImpl> cardService = std::make_shared<CardManagerServerImpl>();
    builder.RegisterService(cardService.get());

    std::shared_ptr<SubscriptionManagerServerImpl> subscriptionService =
        std::make_shared<SubscriptionManagerServerImpl>();
    builder.RegisterService(subscriptionService.get());

    std::shared_ptr<SmsManagerServerImpl> smsService = std::make_shared<SmsManagerServerImpl>();
    builder.RegisterService(smsService.get());

    std::shared_ptr<DataConnectionServerImpl> dcmService =
        std::make_shared<DataConnectionServerImpl>();
    builder.RegisterService(dcmService.get());

    std::shared_ptr<DataProfileServerImpl> dataprofileService =
        std::make_shared<DataProfileServerImpl>();
    builder.RegisterService(dataprofileService.get());

    std::shared_ptr<DataSettingsServerImpl> dataSettingsService =
        std::make_shared<DataSettingsServerImpl>(dcmService);
    builder.RegisterService(dataSettingsService.get());

    std::shared_ptr<LocationManagerServerImpl> locManagerService =
        std::make_shared<LocationManagerServerImpl>();
    builder.RegisterService(locManagerService.get());

    std::shared_ptr<LocationConfiguratorServerImpl> locConfigService =
        std::make_shared<LocationConfiguratorServerImpl>();
    builder.RegisterService(locConfigService.get());

    auto& eventService = EventService::getInstance();
    builder.RegisterService(&eventService);

    std::shared_ptr<SensorFeatureManagerServerImpl> sensorService =
        std::make_shared<SensorFeatureManagerServerImpl>();
    builder.RegisterService(sensorService.get());

    std::unique_ptr<Server> server(builder.BuildAndStart());
    LOG(DEBUG, __FUNCTION__, " Server listening on ", server_address);
    server->Wait();
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {
    auto &simulationServer = SimulationServer::getInstance();

    if (simulationServer.start() != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed to start ", APP_NAME);
        return -1;
    }

    std::cout << "\nInfo: Exiting application..." << std::endl;
    return 0;
}
