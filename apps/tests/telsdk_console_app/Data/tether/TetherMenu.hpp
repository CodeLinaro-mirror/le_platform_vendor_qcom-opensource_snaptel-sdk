/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a Bluetooth Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to startBTTether , stopBTTether, requestBTTetherStatus.
 */

#ifndef TETHERMENU_HPP
#define TETHERMENU_HPP

#include <iostream>

#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataFactory.hpp>
#include <telux/data/net/TetherManager.hpp>

using namespace telux::data::net;

class TetherMenu : public ConsoleApp ,
                 public ITetherListener ,
                 public std::enable_shared_from_this<TetherMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // TetherMenu Manager APIs
    void startBTTether(std::vector<std::string> inputCommand);
    void stopBTTether(std::vector<std::string> inputCommand);
    void requestBTTetherStatus(std::vector<std::string> inputCommand);

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    TetherMenu(std::string appName, std::string cursor);
    ~TetherMenu();
 private:
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<telux::data::net::ITetherManager> tetherManager_;
};
#endif /*TETHERMENU_HPP*/
