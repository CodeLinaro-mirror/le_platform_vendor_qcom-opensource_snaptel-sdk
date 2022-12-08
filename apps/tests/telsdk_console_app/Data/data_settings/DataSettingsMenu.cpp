/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "DataSettingsMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

#define PRINT_NOTIFICATION std::cout << "\n\033[1;35mNOTIFICATION: \033[0m"

DataSettingsMenu::DataSettingsMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    dataSettingsManager_.clear();
    addMenuCmds_ = false;
    subSystemStatusUpdated_ = false;
    dataSettingsListener_[telux::data::OperationType::DATA_LOCAL]
        = std::make_shared<DataSettingsListener>();
    dataSettingsListener_[telux::data::OperationType::DATA_REMOTE]
        = std::make_shared<DataSettingsListener>();
}

DataSettingsMenu::~DataSettingsMenu() {
}

bool DataSettingsMenu::init() {
    bool initStat = initDataSettingsManagerAndListener(telux::data::OperationType::DATA_LOCAL);
    initStat |= initDataSettingsManagerAndListener(telux::data::OperationType::DATA_REMOTE);

    if (addMenuCmds_ == false) {
        addMenuCmds_ = true;
        std::shared_ptr<ConsoleAppCommand> switchBackHaul =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "switch_backhaul", {},
            std::bind(&DataSettingsMenu::switchBackHaul, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {switchBackHaul};
        addCommands(commandsList);
    }

    ConsoleApp::displayMenu();
    return initStat;
}

bool DataSettingsMenu::initDataSettingsManagerAndListener(telux::data::OperationType oprType) {

    bool subSystemStatus = false;
    auto &dataFactory = telux::data::DataFactory::getInstance();
    dataSettingsManager_[oprType] = dataFactory.getDataSettingsManager(oprType);
    subSystemStatus = dataSettingsManager_[oprType]->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nData Settings Manager is not ready, Please wait" << std::endl;
        std::future<bool> f = dataSettingsManager_[oprType]->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
        if(subSystemStatus) {
            std::cout << "\nData Settings Manager is ready" << std::endl;
            dataSettingsManager_[oprType]->registerListener(dataSettingsListener_[oprType]);
        }
    }
    return subSystemStatus;
}

void DataSettingsMenu::switchBackHaul(std::vector<std::string> inputCommand) {
    std::cout << "Route Backhaul Traffic\n";

    int backhaul, operationType, switchAll;
    BackhaulInfo source{}, dest{};

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType oprType = static_cast<telux::data::OperationType>(operationType);

    std::cout << "Do you want to switch All WWAN Backhauls (0-No, 1-Yes): ";
    std::cin >> switchAll;
    Utils::validateInput(switchAll);
    bool applyToAll = (switchAll == 0)? false:true;

    std::cout << "Enter Backhaul Type to switch from (0-Wlan, 1-WWAN): ";
    std::cin >> backhaul;
    Utils::validateInput(backhaul);
    std::cout << std::endl;
    if(backhaul) {
        source.backhaul = telux::data::BackhaulType::WWAN;
        if(!applyToAll) {
            int profileId;
            std::cout << "Enter Profile Id: ";
            std::cin >> profileId;
            Utils::validateInput(profileId);
            source.profileId = profileId;
        }
    } else {
        source.backhaul = telux::data::BackhaulType::WLAN;
    }

    std::cout << "Enter Backhaul Type to switch to (0-Wlan, 1-WWAN): ";
    std::cin >> backhaul;
    Utils::validateInput(backhaul);
    std::cout << std::endl;
    if(backhaul) {
        dest.backhaul = telux::data::BackhaulType::WWAN;
        if(!applyToAll) {
            int profileId;
            std::cout << "Enter Profile Id: ";
            std::cin >> profileId;
            Utils::validateInput(profileId);
            dest.profileId = profileId;
        }
    } else {
        dest.backhaul = telux::data::BackhaulType::WLAN;
    }
    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "switchBackHaul Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    telux::common::Status retStat =
        dataSettingsManager_[oprType]->switchBackHaul(source, dest, applyToAll, respCb);
    Utils::printStatus(retStat);
}

