/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "DataSettingsMenu.hpp"

using namespace std;

DataSettingsMenu::DataSettingsMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   menuOptionsAdded_ = false;
   subSystemStatusUpdated_ = false;
}

DataSettingsMenu::~DataSettingsMenu() {
}

bool DataSettingsMenu::init() {
    bool initStatus = initDataSettingsManager(telux::data::OperationType::DATA_LOCAL);
    initStatus |= initDataSettingsManager(telux::data::OperationType::DATA_REMOTE);
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

    //If both local and remote vlan managers fail, exit
    if (not initStatus) {
        return false;
    }
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> setBackhaulPreference
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "1", "Set_Backhaul_Preference", {},
            std::bind(&DataSettingsMenu::setBackhaulPref, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestBackhaulPreference
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "2", "Request_Backhaul_Preference", {},
            std::bind(&DataSettingsMenu::requestBackhaulPref, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            setBackhaulPreference, requestBackhaulPreference};

        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

bool DataSettingsMenu::initDataSettingsManager(telux::data::OperationType opType) {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    bool retVal = false;

    auto initCb = std::bind(&DataSettingsMenu::onInitComplete, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto settingsMgr = dataFactory.getDataSettingsManager(opType, initCb);
    std:: string opTypeStr = (opType == telux::data::OperationType::DATA_LOCAL)? "Local" : "Remote";
    if (settingsMgr) {
        settingsMgr->registerListener(shared_from_this());
        std::unique_lock<std::mutex> lck(mtx_);
        std::cout << "\nInitializing " << opTypeStr
        << " Data Settings Manager subsystem, Please wait \n";
        cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
        subSystemStatus = settingsMgr->getServiceStatus();
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\n" << opTypeStr << " Data Settings Manager is ready" << std::endl;
            retVal = true;
            dataSettingsManagerMap_[opType] = settingsMgr;
        }
        else {
            std::cout << "\n" << opTypeStr << " Data Settings Manager is not ready" << std::endl;
        }
    }
    return retVal;
}

void DataSettingsMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void DataSettingsMenu::setBackhaulPref(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Set Backhaul Preference \n";
#ifdef TELUX_FOR_EXTERNAL_AP
    telux::data::OperationType opType = telux::data::OperationType::DATA_REMOTE;
#else
    telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL;
#endif
    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    std::vector<BackhaulType> backhaulPref;
    int backhaul;
    bool inputIsValid = true;
    for(int i=0; i<static_cast<int>(BackhaulType::MAX_SUPPORTED); ++i) {
        do {
            std::cout << "Enter Backhaul " << i+1
            << " (0-ETH, 1-USB, 2-WLAN, 3-WWAN, 4-BLE): ";
            std::cin >> backhaul;
            std::cout << endl;
            Utils::validateInput(backhaul);
            if((backhaul < 0) || (backhaul > static_cast<int>(BackhaulType::MAX_SUPPORTED))) {
                std::cout << "Invalid input... Please try again" << std::endl;
                inputIsValid = false;
            }
            backhaulPref.push_back(static_cast<BackhaulType>(backhaul));
        } while(!inputIsValid);
    }
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setBackhaulPreference Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = dataSettingsManagerMap_[opType]->setBackhaulPreference(backhaulPref, respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::requestBackhaulPref(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    bool subSystemStatus = false;

    std::cout << "Request Backhaul Preference \n";
#ifdef TELUX_FOR_EXTERNAL_AP
    telux::data::OperationType opType = telux::data::OperationType::DATA_REMOTE;
#else
    telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL;
#endif
    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    auto respCb = [](std::vector<BackhaulType> backhaulPref, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestBackhaulPreference Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Current Backhaul Preference is: " << std::endl;
            for(size_t i=0; i<backhaulPref.size(); ++i) {
                switch(backhaulPref[i]) {
                    case BackhaulType::ETH:
                        std::cout << "Ethernet" << std::endl;
                        break;
                    case BackhaulType::USB:
                        std::cout << "USB" << std::endl;
                        break;
                    case BackhaulType::WLAN:
                        std::cout << "WLAN" << std::endl;
                        break;
                    case BackhaulType::WWAN:
                        std::cout << "WWAN" << std::endl;
                        break;
                    case BackhaulType::BLE:
                        std::cout << "BLE" << std::endl;
                        break;
                    default:
                        std::cout << "Unsupported Backhaul" << std::endl;
                }
            }
        }
    };
    retStat = dataSettingsManagerMap_[opType]->requestBackhaulPreference(respCb);
    Utils::printStatus(retStat);
}
