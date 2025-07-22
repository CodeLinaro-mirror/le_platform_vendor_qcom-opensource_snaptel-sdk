/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a Backhaul Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to requestBackHaulStatus.
 */

#ifndef BACKHAULMENU_HPP
#define BACKHAULMENU_HPP

#include <iostream>
#include <string>

#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/BackhaulManager.hpp>
#include "BackhaulListener.hpp"

using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class BackhaulMenu : public ConsoleApp ,
                 public IBackhaulManagerListener ,
                 public std::enable_shared_from_this<BackhaulMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // Backhaul Manager APIs
    void requestBackhaulStatus(std::vector<std::string> inputCommand);
    void setBHLoadBalance(std::vector<std::string> inputCommand);
    void requestBHLoadBalanceStatus(std::vector<std::string> inputCommand);

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    BackhaulMenu(std::string appName, std::string cursor);
    ~BackhaulMenu();
 private:
    bool initBackhaulManager();
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::shared_ptr<telux::data::net::IBackhaulManager> backhaulManager_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<BackhaulListener> backhaulListener_;
};
#endif
