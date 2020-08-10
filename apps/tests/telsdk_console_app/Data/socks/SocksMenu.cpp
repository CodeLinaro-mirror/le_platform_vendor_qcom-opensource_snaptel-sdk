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

#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "SocksMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

SocksMenu::SocksMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    socksManager_ = nullptr;
    initComplete_ = false;
}

SocksMenu::~SocksMenu() {
}

bool SocksMenu::init() {
    bool subSystemStatus = false;
    if (initComplete_ == false) {
        initComplete_ = true;
        auto &dataFactory = telux::data::DataFactory::getInstance();
        auto localSocksMgr = dataFactory.getSocksManager(telux::data::OperationType::DATA_LOCAL);
        if(localSocksMgr) {
            socksManager_ = localSocksMgr;
        }
        auto remoteSocksMgr = dataFactory.getSocksManager(telux::data::OperationType::DATA_REMOTE);
        if(remoteSocksMgr) {
            socksManager_ = remoteSocksMgr;
        }
        if(socksManager_ == nullptr ) {
            std::cout << "\nUnable to create Socks Manager ... " << std::endl;
            return false;
        }
        subSystemStatus = socksManager_->isSubsystemReady();
        if (not subSystemStatus) {
            std::cout << "\nInitializing Socks Manager subsystem, Please wait" << std::endl;
            std::future<bool> f = socksManager_->onSubsystemReady();
            // Wait unconditionally for data subsystem to be ready
            subSystemStatus = f.get();
        }
        std::shared_ptr<ConsoleAppCommand> enableSocks
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "socks_enablement",
                {}, std::bind(&SocksMenu::enableSocks, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {enableSocks};

        addCommands(commandsList);
    }
    subSystemStatus = socksManager_->isSubsystemReady();
    if (subSystemStatus) {
        std::cout << "\nSocks Manager is ready" << std::endl;
    }
    else {
        std::cout << "\nSocks Manager is not ready" << std::endl;
        return false;
    }
    ConsoleApp::displayMenu();
    return true;
}

void SocksMenu::enableSocks(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int enableEntry;
    bool subSystemStatus = false;

    std::cout << "Enable/Disable Socks Proxy\n";

    std::cout << "Enter Enablement Type (0-Disable, 1-Enable): ";
    std::cin >> enableEntry;
    Utils::validateInput(enableEntry);
    if (enableEntry < 0 || enableEntry >1) {
        std::cout << "Invalid Entry. Please try again ...\n";
        return;
    }
    bool enablement = (enableEntry == 0 ? false : true);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "enableSocks Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = socksManager_->enableSocks(enablement,respCb);
    Utils::printStatus(retStat);
}
