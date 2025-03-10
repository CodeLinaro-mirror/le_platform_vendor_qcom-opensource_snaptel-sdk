/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanDeviceManagerMenu.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanDeviceManagerMenu::WlanDeviceManagerMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
       menuOptionsAdded_ = false;
}

WlanDeviceManagerMenu::~WlanDeviceManagerMenu() {
    if (wlanDeviceManager_) {
        wlanDeviceManager_ = nullptr;
    }
}

bool WlanDeviceManagerMenu::isSubSystemReady()  {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (wlanDeviceManager_ == nullptr) {
        auto initCb =
            std::bind(&WlanDeviceManagerMenu::onInitComplete, this, std::placeholders::_1);
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
        wlanDeviceManager_ = wlanFactory.getWlanDeviceManager(initCb);
        if (wlanDeviceManager_ == nullptr) {
            //Return immediately
            std::cout << "\nError encountered in initializing Wlan Device Manager" << std::endl;
            return false;
        }
        wlanDeviceManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        //WlanDeviceManager is guaranteed to be valid pointer at this point. If manager init
        //fails and factory invalidated it's own pointer to WlanDevicemanager before reaching this
        //point, reference count of WlanDevicemanager should still be 1
        std::cout << "\nInitializing WlanDeviceManager, Please wait ..." << std::endl;
        cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
        subSystemStatus = wlanDeviceManager_->getServiceStatus();
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nWlan Device Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nWlan Device Manager initialization failed" << std::endl;
            wlanDeviceManager_ = nullptr;
            return false;
        }
    }
    return true;
}

bool WlanDeviceManagerMenu::init() {
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> enableWlan
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "enable_wlan", {},
                std::bind(&WlanDeviceManagerMenu::enableWlan, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setMode
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "set_mode", {},
                std::bind(&WlanDeviceManagerMenu::setMode, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "get_config", {},
                std::bind(&WlanDeviceManagerMenu::getConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "get_status", {},
                std::bind(&WlanDeviceManagerMenu::getStatus, this, std::placeholders::_1)));
         std::shared_ptr<ConsoleAppCommand> setDynamicMode
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "set_dynamic_mode", {},
                std::bind(&WlanDeviceManagerMenu::setDynamicMode, this, std::placeholders::_1)));
         std::shared_ptr<ConsoleAppCommand> getCurrentConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "get_current_config", {},
                std::bind(&WlanDeviceManagerMenu::getCurrentConfig, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            enableWlan, setMode, getConfig, getStatus, setDynamicMode, getCurrentConfig};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void WlanDeviceManagerMenu::enableWlan(std::vector<std::string> userInput) {
    int wlanEnable;

    std::cout << "Wlan Enablement \n";
    std::cout << "Enable/Disable Wlan (1-enable, 0-disable): ";
    std::cin >> wlanEnable;
    std::cout << std::endl;
    WlanUtils::validateInput(wlanEnable, {0, 1});

    telux::common::ErrorCode retCode =  wlanDeviceManager_->enable(static_cast<bool>(wlanEnable));

    if(wlanEnable) {
        std::cout << "\nWlan Enable Response";
    } else {
        std::cout << "\nWlan Disable Response";
    }
    std::cout << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanDeviceManagerMenu::setMode(std::vector<std::string> userInput) {
    int numAps;
    int numSta;

    std::cout << "Setting Wlan Mode \n";
    std::cout << "Enter Number of APs to be enabled: ";
    std::cin >> numAps;
    WlanUtils::validateInput(numAps, {0, 1, 2, 3});
    std::cout << std::endl;

    std::cout << "Enter Number of Stations to be enabled: ";
    std::cin >> numSta;
    WlanUtils::validateInput(numSta, {0, 1});
    std::cout << std::endl;

    telux::common::ErrorCode retCode = wlanDeviceManager_->setMode(numAps, numSta);
    std::cout << "\nSetting Wlan Mode Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanDeviceManagerMenu::getConfig(std::vector<std::string> userInput) {

    int numAp, numSta;

    telux::common::ErrorCode retCode =  wlanDeviceManager_->getConfig(numAp, numSta);
    std::cout << "\nrequest Wlan Config Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        if(numAp) {
            std::cout << "Num of configured AP: " << numAp << std::endl;
        } else {
            std::cout << "No AP is configured" << std::endl;
        }
        if(numSta) {
            std::cout << "Num of configured Sta: " << numSta << std::endl;
        } else {
            std::cout << "No Station is configured" << std::endl;
        }
    }
}

void WlanDeviceManagerMenu::getStatus(std::vector<std::string> userInput) {
    std::vector<telux::wlan::InterfaceStatus> status;
    bool isEnabled;

    std::cout << "Request Wlan Status \n";
    telux::common::ErrorCode retCode =  wlanDeviceManager_->getStatus(isEnabled, status);
    std::cout << "\nrequest Wlan Status Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Wlan is " <<((isEnabled)? "enabled":"disabled") << std::endl;
        if(status.size() > 0) {
            for (auto &ifStatus : status) {
                std::cout << "------------------------------------------" << std::endl;
                std::cout << "device: "
                        << WlanUtils::getWlanDeviceName(ifStatus.device) << std::endl;
                WlanUtils::printAPStatus(ifStatus.apStatus);
                WlanUtils::printStaStatus(ifStatus.staStatus);
            }
        } else {
            std::cout << "No AP or station is currently active" << std::endl;
        }
    }
}
void WlanDeviceManagerMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void WlanDeviceManagerMenu::onServiceStatusChange(telux::common::ServiceStatus status) {
   PRINT_NOTIFICATION << " ** Wlan onServiceStatusChange **\n";
   switch(status) {
      case telux::common::ServiceStatus::SERVICE_AVAILABLE:
         std::cout << " SERVICE_AVAILABLE\n";
         break;
      case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
         std::cout << " SERVICE_UNAVAILABLE\n";
         break;
      default:
         std::cout << " Unknown service status \n";
         break;
   }
}

void WlanDeviceManagerMenu::onEnableChanged(bool enable) {
   PRINT_NOTIFICATION << " ** Wlan onEnableChanged **\n";
   if(enable) {
       std::cout << "Wlan is enabled" << std::endl;
   } else {
       std::cout << "Wlan is disabled" << std::endl;
   }
}

void WlanDeviceManagerMenu::setDynamicMode(std::vector<std::string> userInput)
{
    int numAps;
    int numSta;
    int isPersistent;
    int updateDynamically;

    std::cout << "Setting Wlan Mode Dynamically\n";
    std::cout << "Enter Number of APs to be enabled: ";
    std::cin >> numAps;
    WlanUtils::validateInput(numAps, {0, 1, 2, 3});
    std::cout << std::endl;

    std::cout << "Enter Number of Stations to be enabled: ";
    std::cin >> numSta;
    WlanUtils::validateInput(numSta, {0, 1});
    std::cout << std::endl;

    std::cout << "Make configuration persistent across reboots? (1-yes, 0-no): ";
    std::cin >> isPersistent;
    WlanUtils::validateInput(isPersistent, {0, 1});
    std::cout << std::endl;

    std::cout << "Update mode dynamically without restarting WLAN? (1-yes, 0-no): ";
    std::cin >> updateDynamically;
    WlanUtils::validateInput(updateDynamically, {0, 1});
    std::cout << std::endl;

    telux::wlan::ModeConfig config;
    config.numOfAp = numAps;
    config.numOfSta = numSta;
    config.isPersistent = (isPersistent == 1);
    config.updateImmediately = (updateDynamically == 1);

    telux::common::ErrorCode retCode = wlanDeviceManager_->setMode(config);
    std::cout << "\nSetting Wlan Mode Dynamically Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanDeviceManagerMenu::getCurrentConfig(std::vector<std::string> userInput) {
    int numAp, numSta;

    telux::common::ErrorCode retCode = wlanDeviceManager_->getCurrentConfig(numAp, numSta);
    std::cout << "\nrequest Wlan Current Config Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;

    if(retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Current configured APs: " << numAp << std::endl;
        std::cout << "Current configured Stations: " << numSta << std::endl;
    }
}