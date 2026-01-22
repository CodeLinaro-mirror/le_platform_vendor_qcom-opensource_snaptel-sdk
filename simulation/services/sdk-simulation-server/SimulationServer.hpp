/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SimulationServer.hpp
 *
 * @brief      Declares the SimulationServer class
 *
 */

#ifndef SIMULATION_SERVER_HPP
#define SIMULATION_SERVER_HPP

#include <string>
#include <memory>
#include "libs/common/AsyncTaskQueue.hpp"

#define APP_NAME "SimulationServer"

class SimulationServer {
 public:
    static SimulationServer &getInstance();
    telux::common::Status start();

 private:
    SimulationServer();
    ~SimulationServer();

    std::string createServerAddress(std::string ipAddress, std::string portNo);
    void startGrpcServer();
    void updateJsonValue(std::string message);

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

#endif  // SIMULATION_SERVER_HPP
