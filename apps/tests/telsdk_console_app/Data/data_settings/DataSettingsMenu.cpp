/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <sstream>
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
            std::bind(&DataSettingsMenu::requestBandInterferenceConfig,
                this, std::placeholders::_1)),
            std::make_pair("Configure_Backhaul_Connectivity",
            std::bind(&DataSettingsMenu::setWwanConnectivityConfig, this, std::placeholders::_1)) ,
            std::make_pair("Request_Backhaul_Connectivity",
            std::bind(&DataSettingsMenu::requestWwanConnectivityConfig, this, std::placeholders::_1)),
            std::make_pair("Request_DDS_Switch",
            std::bind(&DataSettingsMenu::requestDdsSwitch, this, std::placeholders::_1)),
            std::make_pair("Request_Current_DDS",
            std::bind(&DataSettingsMenu::requestCurrentDds, this, std::placeholders::_1)),
            std::make_pair("Set_MACsec_State",
            std::bind(&DataSettingsMenu::setMacSecState, this, std::placeholders::_1)),
            std::make_pair("Request_MACsec_State",
            std::bind(&DataSettingsMenu::requestMacSecState, this, std::placeholders::_1)),
            std::make_pair("Switch_Backhaul",
            std::bind(&DataSettingsMenu::switchBackHaul, this, std::placeholders::_1)),
            std::make_pair("Set_URL_ID_To_Backhaul_Mapping",
            std::bind(&DataSettingsMenu::setUrlIdToBackhaulMapping, this, std::placeholders::_1)) ,
            std::make_pair("Get_URL_ID_To_Backhaul_Mapping",
            std::bind(&DataSettingsMenu::getUrlIdToBackhaulMapping, this, std::placeholders::_1)) ,
            std::make_pair("Restore_Factory_Settings",
            std::bind(&DataSettingsMenu::restoreFactorySettings, this, std::placeholders::_1)),
            std::make_pair("Is_Device_Data_Usage_Monitoring_Enabled",
            std::bind(&DataSettingsMenu::isDeviceDataUsageMonitoringEnabled,
                this, std::placeholders::_1))
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
#if defined(TELUX_FOR_EXTERNAL_AP) || defined(TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED)
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
#if defined(TELUX_FOR_EXTERNAL_AP) || defined(TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED)
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
#if defined(TELUX_FOR_EXTERNAL_AP) || defined(TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED)
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

#if defined(TELUX_FOR_EXTERNAL_AP) || defined(TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED)
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

void DataSettingsMenu::requestDdsSwitch(std::vector<std::string> inputCommand)
{
    telux::common::Status retStat;
    int operationType;

    std::cout << "Trigger DDS Switch \n";

#if defined(TELUX_FOR_EXTERNAL_AP) || defined(TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED)
    telux::data::OperationType opType = telux::data::OperationType::DATA_REMOTE;
#else
    telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL;
#endif

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported())
    {
        slotId = Utils::getValidSlotId();
    }

    int switchType = 0;
    std::cout << "Enter switch Type (0-Perm_Switch, 1-Temp_Switch): ";
    std::cin >> switchType;
    DataUtils::validateInput(switchType, {0, 1});

    DdsInfo requestInfo;

    requestInfo.slotId = static_cast<SlotId>(slotId);
    requestInfo.type = static_cast<DdsType>(switchType);

    auto respCb = [](telux::common::ErrorCode error)
    {
        std::cout << std::endl
                  << std::endl;

        std::cout << "CALLBACK: "
                  << "requestDdsSwitch Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = dataSettingsManagerMap_[opType]->requestDdsSwitch(requestInfo, respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::requestCurrentDds(std::vector<std::string> inputCommand)
{
    telux::common::Status retStat;
    int operationType;

    std::cout << "Request current DDS info \n";

#if defined(TELUX_FOR_EXTERNAL_AP) || defined(TELSDK_FEATURE_FOR_SECONDARY_VM_ENABLED)
    telux::data::OperationType opType = telux::data::OperationType::DATA_REMOTE;
#else
    telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL;
#endif

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    auto respCb = [](DdsInfo currentState, telux::common::ErrorCode error)
    {
        std::cout << std::endl
                  << std::endl;

        std::cout << "CALLBACK: "
                  << "requestCurrentDds Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Slot_Id: " << currentState.slotId << std::endl;
            std::string type = (currentState.type == DdsType::PERMANENT) ?
                "Permanent" : "Temporary";
            std::cout << "Switch Type: " << type << std::endl;
        }
    };

    retStat = dataSettingsManagerMap_[opType]->requestCurrentDds(respCb);
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

void DataSettingsMenu::setMacSecState(std::vector<std::string> inputCommand)
{
    telux::common::Status retStat;
    int operationType;

    std::cout << "Trigger MACsec state change \n";

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    DataUtils::validateInput(operationType, {0, 1});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    bool enable = false;
    int userInput = 0;
    std::cout << "Enter MACsec state (0-Disable, 1-Enable): ";
    std::cin >> userInput;
    DataUtils::validateInput(userInput, {0, 1});

    enable = (userInput) ? true : false;

    auto respCb = [](telux::common::ErrorCode error)
    {
        std::cout << std::endl
                  << std::endl;
        std::cout << "CALLBACK: "
                  << "setMacSecState Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = dataSettingsManagerMap_[opType]->setMacSecState(enable, respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::requestMacSecState(std::vector<std::string> inputCommand)
{
    telux::common::Status retStat;
    int operationType;

    std::cout << "Request MACsec state \n";

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    DataUtils::validateInput(operationType, {0, 1});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    auto respCb = [](bool enable, telux::common::ErrorCode error)
    {
        std::cout << std::endl
                  << std::endl;
        std::cout << "CALLBACK: "
                  << "requestMacSecState Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << std::endl;
            std::cout << "Current MACsec state is " << ((enable)? "Enabled ":"Disabled")
                          << std::endl;
        }
    };

    retStat = dataSettingsManagerMap_[opType]->requestMacSecState(respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::restoreFactorySettings(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    int operationType;
    std::cout << "Restore Network Settings To Factory\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    DataUtils::validateInput(operationType, {0, 1});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    int rebootNeeded;
    std::cout << "Trigger reboot after factory reset? (0-NO, 1-YES): ";
    std::cin >> rebootNeeded;
    DataUtils::validateInput(rebootNeeded, {0, 1});

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "\nData Settings Manager is not ready" << std::endl;
        return;
    }

    std::cout << std::endl;
    auto respCb = [](telux::common::ErrorCode error)
    {
        std::cout << std::endl;
        std::cout << "CALLBACK: "
                  << "restoreFactorySettings Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = dataSettingsManagerMap_[opType]->restoreFactorySettings(
        opType, respCb, static_cast<bool>(rebootNeeded));
    Utils::printStatus(retStat);
}

void DataSettingsMenu::switchBackHaul(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;

    std::cout << "Switch BackHaul / Route Backhaul Traffic\n";

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    DataUtils::validateInput(operationType, {0, 1});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    BackhaulInfo source{}, dest{};
    int backhaul, switchAll, slotId = DEFAULT_SLOT_ID;

    std::cout << "Do you want to switch All WWAN Backhauls (0-No, 1-Yes): ";
    std::cin >> switchAll;
    Utils::validateInput(switchAll, {0, 1});
    bool applyToAll = (switchAll == 0)? false:true;

    std::cout << "Enter Backhaul Type to switch from (0-Wlan, 1-WWAN): ";
    std::cin >> backhaul;
    Utils::validateInput(backhaul, {0, 1});
    std::cout << std::endl;
    if(backhaul) {
        source.backhaul = telux::data::BackhaulType::WWAN;
        if (!applyToAll) {
            if (telux::common::DeviceConfig::isMultiSimSupported()) {
                slotId = Utils::getValidSlotId();
            }
            int profileId;
            std::cout << "Enter Profile Id: ";
            std::cin >> profileId;
            Utils::validateInput(profileId);
            source.profileId = profileId;
        }
        source.slotId = static_cast<SlotId>(slotId);
    } else {
        source.backhaul = telux::data::BackhaulType::WLAN;
    }

    std::cout << "Enter Backhaul Type to switch to (0-Wlan, 1-WWAN): ";
    std::cin >> backhaul;
    Utils::validateInput(backhaul, {0, 1});
    std::cout << std::endl;
    if (backhaul) {
        dest.backhaul = telux::data::BackhaulType::WWAN;
        if(!applyToAll) {
            if (telux::common::DeviceConfig::isMultiSimSupported()) {
                slotId = Utils::getValidSlotId();
            }
            int profileId;
            std::cout << "Enter Profile Id: ";
            std::cin >> profileId;
            Utils::validateInput(profileId);
            dest.profileId = profileId;
        }
        dest.slotId = static_cast<SlotId>(slotId);
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

    retStat = dataSettingsManagerMap_[opType]->switchBackHaul(source, dest, applyToAll, respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::setUrlIdToBackhaulMapping(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;

    std::cout << "Set URL ID to Backhaul Mapping \n";

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    DataUtils::validateInput(operationType, {0, 1});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    // Read multiple URL IDs from the user
    std::cout << "Enter URL IDs (comma-separated): ";
    std::string input;
    std::getline(std::cin, input);

    // Trim leading/trailing whitespace from the entire input
    input.erase(0, input.find_first_not_of(" \t\n\r\f\v"));
    input.erase(input.find_last_not_of(" \t\n\r\f\v") + 1);

    // Split the input string into individual URL IDs
    std::stringstream ss(input);
    std::string token;
    std::vector<uint32_t> urlIds;
    while (std::getline(ss, token, ',')) {
        // Trim each token
        token.erase(0, token.find_first_not_of(" \t\n\r\f\v"));
        token.erase(token.find_last_not_of(" \t\n\r\f\v") + 1);

        if (!token.empty()) {
            uint32_t urlId = std::stoul(token);
            Utils::validateInput(urlId);
            urlIds.push_back(urlId);
        }
    }

    // Create the UrlIdToBackhaulMapping object
    telux::data::UrlIdToBackhaulMapping urlToBackhaulMapping;
    urlToBackhaulMapping.urlIds = urlIds;

    int backhaul;
    std::cout << "Enter Backhaul Type (0-Wlan, 1-WWAN): ";
    std::cin >> backhaul;
    DataUtils::validateInput(backhaul, {0, 1});
    if (backhaul) {
        urlToBackhaulMapping.backhaul.backhaul = telux::data::BackhaulType::WWAN;
        int slotId = DEFAULT_SLOT_ID;
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
            slotId = Utils::getValidSlotId();
        }
        urlToBackhaulMapping.backhaul.slotId = static_cast<SlotId>(slotId);
        int profileId;
        std::cout << "Enter Profile Id: ";
        std::cin >> profileId;
        Utils::validateInput(profileId);
        urlToBackhaulMapping.backhaul.profileId = profileId;
    } else {
        urlToBackhaulMapping.backhaul.backhaul = telux::data::BackhaulType::WLAN;
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setUrlIdToBackhaulMapping Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = dataSettingsManagerMap_[opType]->setUrlIdToBackhaulMapping(urlToBackhaulMapping, respCb);
    Utils::printStatus(retStat);
}

void DataSettingsMenu::getUrlIdToBackhaulMapping(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    int operationType;

    std::cout << "Get URL ID to Backhaul Mapping \n";

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    DataUtils::validateInput(operationType, {0, 1});
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    if (dataSettingsManagerMap_.find(opType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    telux::data::BackhaulType backhaulType = {};
    int backhaul;
    std::cout << "Enter Backhaul Type (0-Wlan, 1-WWAN): ";
    std::cin >> backhaul;
    Utils::validateInput(backhaul, {0, 1});
    std::cout << std::endl;
    if (backhaul == 1) {
        backhaulType = telux::data::BackhaulType::WWAN;
    } else {
        backhaulType = telux::data::BackhaulType::WLAN;
    }

    std::vector<telux::data::UrlIdToBackhaulMapping> urlToBackhaulMappingList;
    error = dataSettingsManagerMap_[opType]->getUrlIdToBackhaulMapping(backhaulType, urlToBackhaulMappingList);

    if (error == telux::common::ErrorCode::SUCCESS) {
        std::cout << "RESPONSE: getUrlIdToBackhaulMapping is successful" << std::endl;
        std::cout << "URL ID to Backhaul Mapping:" << std::endl;
        for (const auto& mapping : urlToBackhaulMappingList) {
            std::cout << "URL IDs : ";
            bool isFirst = true;
            for (const auto& urlId : mapping.urlIds) {
                if (!isFirst) {
                    std::cout << " ";
                }
                std::cout << urlId;
                isFirst = false;
            }
            std::cout << std::endl;
            std::cout << "Backhaul: " << DataUtils::backhaulToString(mapping.backhaul.backhaul) << std::endl;
            if (mapping.backhaul.backhaul == telux::data::BackhaulType::WWAN) {
                std::cout << "Slot ID: " << mapping.backhaul.slotId << std::endl;
                std::cout << "Profile ID: " << mapping.backhaul.profileId << std::endl;
            }
        }
    } else {
        std::cout << "RESPONSE: getUrlIdToBackhaulMapping failed"
            << ", ErrorCode: " << static_cast<int>(error)
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
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

void DataSettingsMenu::onDdsChange(DdsInfo currentState) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** DDS sub has changed ** \n";

    std::cout <<  "DDS Info : " << "Slot_Id: " << currentState.slotId << std::endl;
        std::string type = (currentState.type == DdsType::PERMANENT) ?
            "Permanent" : "Temporary";
        std::cout << "Switch Type: " << type << std::endl;

    std::cout << std::endl << std::endl;
}

void DataSettingsMenu::isDeviceDataUsageMonitoringEnabled(std::vector<std::string> inputCommand) {
    std::cout << "\nIs device data usage monitoring enabled" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    SlotId slotId = DEFAULT_SLOT_ID;

    #ifdef FEATURE_EXTERNAL_AP
        OperationType oprType = telux::data::OperationType::DATA_REMOTE;
    #else
        OperationType oprType = telux::data::OperationType::DATA_LOCAL;
    #endif

    if (dataSettingsManagerMap_.find(oprType) == dataSettingsManagerMap_.end()) {
        std::cout << "Data Settings Manager is not ready" << std::endl;
        return;
    }

    bool enable = dataSettingsManagerMap_[oprType]->isDeviceDataUsageMonitoringEnabled();
    std::cout << "RESPONSE: isDeviceDataUsageMonitoringEnabled "
        << ", Device data usage monitoring is " << (enable ? "enabled" : "disabled") << std::endl;
}
