/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is Data Settings Manager Sample Application using Telematics SDK.
 * It is used to demonstrate API to exercise Data Settings Manager Features.
 */

#ifndef DATASETTINGSMMENU_HPP
#define DATASETTINGSMMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <iomanip>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include "console_app_framework/ConsoleApp.hpp"
#include "DataSettingsListener.hpp"

using namespace telux::data;
using namespace telux::common;

class DataSettingsMenu : public ConsoleApp,
                         public std::enable_shared_from_this<DataSettingsMenu> {
public:
    // initialize menu and sdk
    bool init();
    // Menu Functions

    DataSettingsMenu(std::string appName, std::string cursor);

    //API
    void switchBackHaul(std::vector<std::string> inputCommand);

    ~DataSettingsMenu();
private:
    bool addMenuCmds_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::map<telux::data::OperationType, std::shared_ptr<IDataSettingsManager>>
        dataSettingsManager_;
    std::map<telux::data::OperationType, std::shared_ptr<IDataSettingsListener>>
         dataSettingsListener_;
    bool initDataSettingsManagerAndListener(telux::data::OperationType oprType);
};

#endif // DATASETTINGSMMENU_HPP
