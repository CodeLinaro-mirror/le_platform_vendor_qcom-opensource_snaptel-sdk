/*
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include "../DataUtils.hpp"

#include "DataSettingsMenu.hpp"

using namespace std;
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

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
        std::vector<std::pair<std::string, std::function<void(std::vector<std::string> &)>>>
            settingsCommandPairList = {
            std::make_pair("Set_Backhaul_Preference",
            std::bind(&DataSettingsMenu::setBackhaulPref, this, std::placeholders::_1)) ,
            std::make_pair("Request_Backhaul_Preference",
            std::bind(&DataSettingsMenu::requestBackhaulPref, this, std::placeholders::_1)) ,
            std::make_pair("Set_Band_Interference_Configuration",
            std::bind(&DataSettingsMenu::setBandInterferenceConfig, this, std::placeholders::_1)) ,
            std::make_pair("Request_Band_Interference_Configuration",
            std::bind(&DataSettingsMenu::requestBandInterferenceConfig, this, std::placeholders::_1)),
            std::make_pair("Configure_Backhaul_Connectivity",
            std::bind(&DataSettingsMenu::setWwanConnectivityConfig, this, std::placeholders::_1)) ,
            std::make_pair("Request_Backhaul_Connectivity",
            std::bind(&DataSettingsMenu::requestWwanConnectivityConfig, this, std::placeholders::_1)),
        };
        std::vector<std::shared_ptr<ConsoleAppCommand>> settingsMenuCommandList;
        int commandId = 1;
        for(auto& menuItem: settingsCommandPairList) {
            settingsMenuCommandList.push_back(
                std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandId),
                menuItem.first, {}, menuItem.second)));
            commandId++;
        }
        addCommands(settingsMenuCommandList);
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
            inputIsValid = true;
            std::cout << "Enter Backhaul " << i+1
            << " (0-ETH, 1-USB, 2-WLAN, 3-WWAN, 4-BLE): ";
            std::cin >> backhaul;
            std::cout << endl;
            Utils::validateInput(backhaul, {static_cast<int>(BackhaulType::ETH),
                static_cast<int>(BackhaulType::USB),
                static_cast<int>(BackhaulType::WLAN),
                static_cast<int>(BackhaulType::WWAN),
                static_cast<int>(BackhaulType::BLE)});
            if((backhaul < 0) || (backhaul >= static_cast<int>(BackhaulType::MAX_SUPPORTED))) {
                std::cout << "Invalid backhaul... Please try again" << std::endl;
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

void DataSettingsMenu::setBandInterferenceConfig(std::vector<std::string> inputCommand) {
    std::cout << "Set Band Interference Configuration" << std::endl;
    telux::common::Status retStat;
    int operationType;
    bool enable = true;
    int userInput = 0;

    std::shared_ptr<BandInterferenceConfig> config = nullptr;
#ifdef TELUX_FOR_EXTERNAL_AP
    telux::data::OperationType opType = telux::data::OperationType::DATA_REMOTE;
#else
    telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL;
#endif
    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }
    while(1) {
        std::cout << "Enable Band Interference Configuration (1:Enable, 0:Disable): " ;
        std::cin >> userInput;
        std::cout << endl;
        Utils::validateInput(userInput);
        if(userInput == 0) {
            enable = false;
            break;
        } else if(userInput == 1) {
            enable = true;
            break;
        } else {
            std::cout << "Invalid input... Please try again" << std::endl;
        }
    }
    if(enable) {
        config = std::make_shared<telux::data::BandInterferenceConfig>();
        while(1) {
            std::cout << "Enter high priority (1:N79 5G, 0:WLAN 5GHz): " ;
            std::cin >> userInput;
            std::cout << endl;
            Utils::validateInput(userInput);
            if(userInput == 0) {
                config->priority = telux::data::BandPriority::WLAN;
                break;
            } else if(userInput == 1) {
                config->priority = telux::data::BandPriority::N79;
                break;
            } else {
                std::cout << "Invalid input... Please try again" << std::endl;
            }
        }

        std::cout << "Enter Wait For Wlan 5GHz Timer (1:Yes, 0:No-use default): " ;
        std::cin >> userInput;
        std::cout << endl;
        Utils::validateInput(userInput);
        if(userInput) {
            std::cout << "Enter Wait For Wlan 5GHz Timer in Seconds: " ;
            std::cin >> userInput;
            std::cout << endl;
            Utils::validateInput(userInput);
            config->wlanWaitTimeInSec = userInput;
        }

        std::cout << "Enter Wait For N79 5G Timer (1:Yes, 0:No-use default): " ;
        std::cin >> userInput;
        std::cout << endl;
        Utils::validateInput(userInput);
        if(userInput) {
            std::cout << "Enter Wait For  N79 5G Timer in Seconds: " ;
            std::cin >> userInput;
            std::cout << endl;
            Utils::validateInput(userInput);
            config->n79WaitTimeInSec = userInput;
        }
    }
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                << "setBandInterferenceConfig Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat = dataSettingsManagerMap_[opType]->setBandInterferenceConfig(enable, config, respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::requestBandInterferenceConfig(std::vector<std::string> inputCommand) {
    std::cout << "Request Band Interference Configuration" << std::endl;
    telux::common::Status retStat;
    int operationType;
    bool enable = true;
    int userInput = 0;

#ifdef TELUX_FOR_EXTERNAL_AP
    telux::data::OperationType opType = telux::data::OperationType::DATA_REMOTE;
#else
    telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL;
#endif
    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }
    auto respCb = [](bool isEnabled, std::shared_ptr<telux::data::BandInterferenceConfig> config,
    telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                << "requestBandInterferenceConfig Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error)
                << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if(error == telux::common::ErrorCode::SUCCESS) {
            if(isEnabled) {
                std::cout << "Band Interference is enabled" << std::endl;
                std::cout << "Band Interference Config: " << std::endl;
                std::cout << "  High Prioroty: " << ((config->priority ==
                    telux::data::BandPriority::WLAN)? "Wlan 5GHz":"N79 5G") << std::endl;
                std::cout << "  Wait for Wlan 5GHz timer in seconds: " << config->wlanWaitTimeInSec;
                std::cout << std::endl;
                std::cout << "  Wait for N79 5G timer in seconds: " << config->n79WaitTimeInSec;
                std::cout << std::endl;
            } else {
                std::cout << "Band Interference is disabled" << std::endl;
            }
        }
    };
    retStat = dataSettingsManagerMap_[opType]->requestBandInterferenceConfig(respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::setWwanConnectivityConfig(std::vector<std::string> inputCommand) {
    int connectivity;
    bool inputIsValid = true;
    bool allowConnectivity = true;
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    std::cout << "Configure WWAN Connectivity \n";

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    DataUtils::validateInput(operationType, {0, 1});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    DataUtils::validateInput(slotId, {1, 2});

    std::cout << "Allow WWAN Connectivity? (0-No, 1-Yes): ";
    std::cin >> connectivity;
    DataUtils::validateInput(connectivity, {0, 1});
    allowConnectivity = static_cast<bool>(connectivity);
    std::cout << endl;

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setWwanConnectivityConfig Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat = dataSettingsManagerMap_[opType]->setWwanConnectivityConfig(
        static_cast<SlotId>(slotId), allowConnectivity, respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::requestWwanConnectivityConfig(std::vector<std::string> inputCommand) {
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    std::cout << "Request WWAN Connectivity \n";

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    DataUtils::validateInput(slotId, {1, 2});

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    DataUtils::validateInput(operationType, {0, 1});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }
    std::cout << endl;

    auto respCb = [](SlotId slotId, bool isAllowed, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestWwanConnectivityConfig Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
            if (error == telux::common::ErrorCode::SUCCESS) {
                std::cout << std::endl;
                std::cout << "WWAN Connectivity is " << ((isAllowed)? "allowed ":"not allowed ")
                          << "for SlotId : " << slotId << std::endl;
            }
    };

    retStat = dataSettingsManagerMap_[opType]->requestWwanConnectivityConfig(
        static_cast<SlotId>(slotId), respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::onWwanConnectivityConfigChange(SlotId slotId, bool isConnectivityAllowed) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** WWAN Connectivity Config has changed ** \n";
    std::cout << "WWAN Connectivity Config on SlotId: " << static_cast<int>(slotId) << " is: ";
    if(isConnectivityAllowed) {
        std::cout << "Allowed";
    } else {
        std::cout << "Disallowed";
    }
    std::cout << std::endl << std::endl;
}
