/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "WlanApInterfaceManagerMenu.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanApInterfaceManagerMenu::WlanApInterfaceManagerMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
       menuOptionsAdded_ = false;
}

WlanApInterfaceManagerMenu::~WlanApInterfaceManagerMenu() {
    if (wlanApInterfaceManager_) {
        wlanApInterfaceManager_ = nullptr;
    }
}

bool WlanApInterfaceManagerMenu::init() {
    if (wlanApInterfaceManager_ == nullptr) {
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
        wlanApInterfaceManager_ = wlanFactory.getApInterfaceManager();
        if (wlanApInterfaceManager_ == nullptr) {
            //Return immediately
            std::cout <<
                "\nError encountered in initializing Wlan Ap Interface Manager" << std::endl;
            return false;
        }
    }
    wlanApInterfaceManager_->registerListener(shared_from_this());
    return true;
}

void WlanApInterfaceManagerMenu::showMenu() {
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        int stepID = 1;
        std::shared_ptr<ConsoleAppCommand> setConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "set_config", {}, std::bind(&WlanApInterfaceManagerMenu::setConfig, this,
            std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_config", {},
            std::bind(&WlanApInterfaceManagerMenu::getConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_status", {},
            std::bind(&WlanApInterfaceManagerMenu::getStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConnectedDevices
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_connected_devices", {},
            std::bind(&WlanApInterfaceManagerMenu::getConnectedDevices,
            this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getConnectedDevicesStats
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "get_connected_devices_stats", {},
            std::bind(&WlanApInterfaceManagerMenu::getConnectedDevicesStats,
            this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> manageApService
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(stepID++),
            "manage_service", {},
            std::bind(&WlanApInterfaceManagerMenu::manageApService,
            this, std::placeholders::_1)));
        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            setConfig, getConfig, getStatus, getConnectedDevices,
            getConnectedDevicesStats, manageApService};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
}

void WlanApInterfaceManagerMenu::setConfig(std::vector<std::string> userInput) {

    std::cout << "Set AP Configuration \n";
    telux::wlan::ApConfig config;
    int apId = 1, apInterworking = 0, apType = 0;

    std::cout << "Enter Wlan Ap Id \
            (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> apId;
    WlanUtils::validateInput(apId, {1, 2, 3});
    std::cout << std::endl;
    config.id = static_cast<telux::wlan::Id>(apId);
    //Primary is defaulted to private and full access
    telux::wlan::ApNetConfig apNetConfig = {};

    std::cout << "Enter AP Type\
            (1-PRIVATE, 2-GUEST): ";
    std::cin >> apType;
    WlanUtils::validateInput(apType, {1, 2});
    std::cout << std::endl;
    apNetConfig.info.apType = static_cast<telux::wlan::ApType>(apType);

    std::cout << "Enter AP network access\
            (0-INTERNET_ACCESS, 1-FULL_ACCESS): ";
    std::cin >> apInterworking;
    WlanUtils::validateInput(apInterworking, {0, 1});
    std::cout << std::endl;

    apNetConfig.interworking =
        static_cast<telux::wlan::ApInterworking>(apInterworking);
    config.network.push_back(apNetConfig);

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->setConfig(config);
    std::cout << "\nSetting Wlan Mode Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::getConfig(std::vector<std::string> userInput) {
    std::vector<telux::wlan::ApConfig> config;
    telux::common::ErrorCode retCode = wlanApInterfaceManager_->getConfig(config);

    std::cout << "\nrequest AP Configuration Response"
                << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(retCode)
                << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        for (auto &cfg : config) {
            std::cout << "------------------------------------------" << std::endl;
            std::cout << "AP Id: " << WlanUtils::getWlanId(cfg.id) << std::endl;
            for (auto &netCfg : cfg.network) {
                std::cout << "AP Type: "
                    << WlanUtils::getWlanApType(netCfg.info.apType) << std::endl;
                std::cout << "AP Access: "
                    << ((netCfg.interworking == telux::wlan::ApInterworking::FULL_ACCESS)?
                        "FULL ACCESS":"INTERNET ACCESS") << std::endl;
            }
        }
    }
}

void WlanApInterfaceManagerMenu::getStatus(std::vector<std::string> userInput) {
    std::vector<telux::wlan::ApStatus> status;
    std::cout << "Request AP Status" << std::endl;

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->getStatus(status);
    std::cout << "\nRequest AP Status Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        WlanUtils::printAPStatus(status);
    }
}

void WlanApInterfaceManagerMenu::getConnectedDevices(std::vector<std::string> userInput) {
    std::vector<telux::wlan::DeviceInfo> clientsInfo;
    std::cout << "Request Connected Devices" << std::endl;

    telux::common::ErrorCode retCode =
        wlanApInterfaceManager_->getConnectedDevices(clientsInfo);
    std::cout << "\nRequest Connected Devices Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        WlanUtils::printDeviceInfo(clientsInfo);
    }
}

void WlanApInterfaceManagerMenu::getConnectedDevicesStats(
    std::vector<std::string> userInput) {
    std::vector<telux::wlan::DeviceStats> clientsStats;
    std::cout << "Request Connected Devices Statistics" << std::endl;

    telux::common::ErrorCode retCode =
        wlanApInterfaceManager_->getConnectedDevicesStats(clientsStats);

    std::cout << "\nRequest Connected Devices Statistics Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    if(retCode == telux::common::ErrorCode::SUCCESS) {
        if(clientsStats.size() > 0) {
            for (auto &client : clientsStats) {
                std::cout << "------------------------------------------" << std::endl;
                std::cout << "Mac Addr: " << client.macAddress << std::endl;
                std::cout << "Tx Bytes: " << client.bytesTx << std::endl;
                std::cout << "Rx Bytes: " << client.bytesRx << std::endl;
            }
        } else {
            std::cout << "No Active Devices" << std::endl;
        }
    }
}

void WlanApInterfaceManagerMenu::manageApService(std::vector<std::string> userInput) {
    std::cout << "Manage Ap Service" << std::endl;

    int apId = 1, opr = 0;
    std::cout << "Select AP Service Operation\
            (0-STOP, 1-START, 2-RESTART): ";
    std::cin >> opr;
    WlanUtils::validateInput(opr, {0, 1, 2});
    std::cout << std::endl;

    std::cout << "Select Ap Id \
            (1-PRIMARY, 2-SECONDARY, 3-TERTIARY): ";
    std::cin >> apId;
    WlanUtils::validateInput(apId, {1, 2, 3});
    std::cout << std::endl;

    telux::common::ErrorCode retCode = wlanApInterfaceManager_->manageApService(
        static_cast<telux::wlan::Id>(apId),
        static_cast<telux::wlan::ServiceOperation>(opr));

    std::cout << "\nManage Ap Service Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanApInterfaceManagerMenu::onApBandChanged(telux::wlan::BandType band) {
   PRINT_NOTIFICATION << " ** Wlan onApOperBandChanged **\n";

   if(band == telux::wlan::BandType::BAND_2GHZ) {
       std::cout << "AP has switched to 2G band" << std::endl;
   } else {
       std::cout << "AP has switched to 5G band" << std::endl;
   }
}

void WlanApInterfaceManagerMenu::onApDeviceStatusChanged(
    telux::wlan::ApDeviceConnectionEvent event, std::vector<telux::wlan::DeviceInfo> info) {
   PRINT_NOTIFICATION << " ** Wlan onApDeviceStatusChanged **\n";
   std::cout << "Event: ";
   switch(event) {
       case telux::wlan::ApDeviceConnectionEvent::CONNECTED:
           std::cout << "New Device is connected" << std::endl;
           break;
       case telux::wlan::ApDeviceConnectionEvent::DISCONNECTED:
           std::cout << "Existing Device is disconnected" << std::endl;
           break;
        case telux::wlan::ApDeviceConnectionEvent::IPV4_UPDATED:
           std::cout << "Existing Device IPv4 is Updated" << std::endl;
           break;
        case telux::wlan::ApDeviceConnectionEvent::IPV6_UPDATED:
           std::cout << "Existing Device IPv6 is Updated" << std::endl;
           break;
        default:
           break;
   }
   WlanUtils::printDeviceInfo(info);
}
