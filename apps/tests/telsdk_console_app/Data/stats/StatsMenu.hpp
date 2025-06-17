/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STATSMENU_HPP
#define STATSMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>


#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/StatsManager.hpp>


using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class StatsMenu : public ConsoleApp ,
                 public IStatsListener ,
                 public std::enable_shared_from_this<StatsMenu> {
 public:
    // initialize menu and sdk
    bool init();
    bool initStatsManager();

    // Stats Manager APIs
    void setClientDataUsageStatsConfig(std::vector<std::string> inputCommand);
    void getClientDataUsageStats(std::vector<std::string> inputCommand);
    void resetClientDataUsageStats(std::vector<std::string> inputCommand);
    void getClientDataUsageStatsConfig(std::vector<std::string> inputCommand);

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    StatsMenu(std::string appName, std::string cursor);
    ~StatsMenu();
 private:
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::shared_ptr<telux::data::net::IStatsManager > statsManager_;
    std::shared_ptr<telux::data::net::IStatsListener> statsListener_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
#endif