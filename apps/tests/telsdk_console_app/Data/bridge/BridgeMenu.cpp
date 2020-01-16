/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include <Utils.hpp>

#include "BridgeMenu.hpp"

using namespace std;

BridgeMenu::BridgeMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    bridgeMgr_ = nullptr;
}

BridgeMenu::~BridgeMenu() {
}

int BridgeMenu::init() {
    bool subSystemStatus = false;
    auto &dataFactory = telux::data::DataFactory::getInstance();
    bridgeMgr_ = dataFactory.getBridgeManager();
    subSystemStatus = bridgeMgr_->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nBridge Manager is not ready, Please wait" << std::endl;
        std::future<bool> f = bridgeMgr_->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
        if(not subSystemStatus) {
            return -1;
        } else {
            std::cout << "\nBridge Manager is ready" << std::endl;
        }
    }

    std::shared_ptr<ConsoleAppCommand> enableBridge
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Set_Bridge_State", {},
            std::bind(&BridgeMenu::enableBridge, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> addBridge
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Add_Bridge", {},
            std::bind(&BridgeMenu::addBridge, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getBridgeInfo
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get_Bridge_Info", {},
            std::bind(&BridgeMenu::getBridgeInfo, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> removeBridge
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Remove_Bridge", {},
            std::bind(&BridgeMenu::removeBridge, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {enableBridge, addBridge,
        getBridgeInfo, removeBridge};

    addCommands(commandsList);
    ConsoleApp::displayMenu();
    return 0;
}


void BridgeMenu::enableBridge(std::vector<std::string> inputCommand) {
    bool enableBridge = false;
    int temp = 0;
    std::cout << "Enter the desired state \n (1-enable, 0-disable): ";
    std::cin >> temp;
    Utils::validateInput(temp);
    if(temp == 1) {
        enableBridge = true;
    } else if(temp == 0) {
        enableBridge = false;
    } else {
        std::cout << "Invalid bridge state input, try again" << std::endl;
        return;
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Set bridge state request is"
                  << (error == telux::common::ErrorCode::SUCCESS ? " successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    if(telux::common::Status::SUCCESS != bridgeMgr_->enableBridge(enableBridge, respCb)) {
        std::cout << "Request to set bridge state failed" << std::endl;
    }
}

void BridgeMenu::addBridge(std::vector<std::string> inputCommand) {
    int temp;
    char delimiter = '\n';
    BridgeInfo config;

    std::cout << "Enter Interface name(ex:wlan0, eth0, etc.) : ";
    std::getline(std::cin, config.ifaceName, delimiter);

    std::cout << "Enter Interface Type\n (1-WLAN_AP, 2-WLAN_STA, 3-ETH): ";
    std::cin >> temp;
    Utils::validateInput(temp);
    config.ifaceType = static_cast<telux::data::net::BridgeIFaceType>(temp);

    std::cout << "Enter required bandwidth(max 900Mbps) : ";
    std::cin >> temp;
    Utils::validateInput(temp);
    config.bandwidth = temp;

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Add software bridge request"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    if(telux::common::Status::SUCCESS != bridgeMgr_->addBridge(config, respCb)) {
        std::cout << "Request to add a software bridge failed" << std::endl;
    }
}

void BridgeMenu::getBridgeInfo(std::vector<std::string> inputCommand) {

    auto respCb = [](const std::vector<BridgeInfo> &configs, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Get software bridge info request"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        for (auto c : configs) {
            std::cout << "Iface name: " << c.ifaceName << ", ifaceType: " << (int)c.ifaceType
                      << ", bandwidth: " << c.bandwidth << std::endl;
        }
    };
    if(telux::common::Status::SUCCESS != bridgeMgr_->requestBridgeInfo(respCb)) {
        std::cout << "Request to get software bridge info failed" << std::endl;
    }
}

void BridgeMenu::removeBridge(std::vector<std::string> inputCommand) {
    std::string ifaceName;
    char delimiter = '\n';
    std::cout << "Enter Interface name(ex:wlan0, eth0, etc.) : ";
    std::getline(std::cin, ifaceName, delimiter);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Delete software bridge request"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    if(telux::common::Status::SUCCESS != bridgeMgr_->removeBridge(ifaceName, respCb)) {
        std::cout << "Request to delete a software bridge failed" << std::endl;
    }
}
