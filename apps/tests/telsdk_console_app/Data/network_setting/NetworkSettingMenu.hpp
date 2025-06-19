/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a NetworkSetting Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to Port Trigger, Alg Update, DataPathOpt Status.
 */

#ifndef NETWORKSETTINGMENU_HPP
#define NETWORKSETTINGMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>


#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/NetworkSettingManager.hpp>


using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class NetworkSettingMenu : public ConsoleApp ,
                 public INetworkSettingListener,
                 public std::enable_shared_from_this<NetworkSettingMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // NetworkSettingManager APIs
    void addPortTriggerEntry(std::vector<std::string> inputCommand);
    void requestPortTriggerEntry(std::vector<std::string> inputCommand);
    void deletePortTriggerEntry(std::vector<std::string> inputCommand);
    void updateAlg(std::vector<std::string> inputCommand);
    void setDataPathOptStatus(std::vector<std::string> inputCommand);
    void requestDataPathOptStatus(std::vector<std::string> inputCommand);

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    NetworkSettingMenu(std::string appName, std::string cursor);
    ~NetworkSettingMenu();
 private:
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::shared_ptr<telux::data::net::INetworkSettingManager> networkSettingManager_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
#endif
