/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a Client Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to set create, remove, bind, unbind, and query existing Clients
 */

#ifndef CLIENTMENU_HPP
#define CLIENTMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>

#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/ClientManager.hpp>

#include "ClientListener.hpp"

using namespace telux::data;
using namespace telux::common;

class ClientMenu : public ConsoleApp,
                   public IClientListener,
                   public std::enable_shared_from_this<ClientMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // Initialization Callback
    void onInitComplete(telux::common::ServiceStatus status);

    // Client Manager APIs
    void getDeviceDataUsageStats(std::vector<std::string> inputCommand);
    void resetDataUsageStats(std::vector<std::string> inputCommand);
    ClientMenu(std::string appName, std::string cursor);
    ~ClientMenu();

 private:
    bool initClientManager();

    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<telux::data::IClientManager> clientManager_;
    std::shared_ptr<ClientListener> clientListener_;
};
#endif
