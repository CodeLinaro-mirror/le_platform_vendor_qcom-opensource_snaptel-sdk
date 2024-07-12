/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DUALDATAMANAGEMENTMENU_HPP
#define DUALDATAMANAGEMENTMENU_HPP

#include "console_app_framework/ConsoleApp.hpp"
#include <memory>
#include <string>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DualDataManager.hpp>

class DualDataManagementMenu : public ConsoleApp,
                               public telux::data::IDualDataListener,
                               public std::enable_shared_from_this<DualDataManagementMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // Initialization Callback
    void onInitComplete(telux::common::ServiceStatus status);

    // DualData Manager APIs
    void getDualDataCapibility(std::vector<std::string> &inputCommand);
    void getDualDataUsageRecommendation(std::vector<std::string> &inputCommand);

    void onDualDataCapabilityChange(bool isDualDataCapable);
    void onDualDataUsageRecommendationChange(
        telux::data::DualDataUsageRecommendation recommendation);

    DualDataManagementMenu(std::string appName, std::string cursor);
    ~DualDataManagementMenu();

 private:
    bool initDualDataManager();
    std::string convertRecommendationToString(
        telux::data::DualDataUsageRecommendation recommendation);

    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<telux::data::IDualDataManager> dualDataManager_;
};

#endif //DUALDATAMANAGEMENTMENU_HPP
