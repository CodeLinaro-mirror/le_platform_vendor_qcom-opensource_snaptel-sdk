/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
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

#include "ServingSystemMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

#define PRINT_NOTIFICATION std::cout << "\n\033[1;35mNOTIFICATION: \033[0m"

DataServingSystemMenu::DataServingSystemMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    dataServingSystemManager_ = nullptr;
    addMenuCmds_ = false;
    subSystemStatusUpdated_ = false;
    dataServingSystemListener_ = std::make_shared<ServingSystemListener>();
}

DataServingSystemMenu::~DataServingSystemMenu() {
}

bool DataServingSystemMenu::init() {
    bool initStat = initServingSystemManagerAndListener();

    if (addMenuCmds_ == false) {
        addMenuCmds_ = true;
        std::shared_ptr<ConsoleAppCommand> requestServiceStatus =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "request_service_status", {},
            std::bind(&DataServingSystemMenu::requestServiceStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestRoamingStatus =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "request_roaming_status", {},
            std::bind(&DataServingSystemMenu::requestRoamingStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> makeDormant =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "make_dormant", {},
            std::bind(&DataServingSystemMenu::makeDormant, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
            requestServiceStatus, requestRoamingStatus, makeDormant};
        addCommands(commandsList);
    }

    ConsoleApp::displayMenu();
    return initStat;
}

bool DataServingSystemMenu::initServingSystemManagerAndListener() {

    bool subSystemStatus = false;
    auto &dataFactory = telux::data::DataFactory::getInstance();
    dataServingSystemManager_ = dataFactory.getServingSystemManager();
    subSystemStatus = dataServingSystemManager_->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nServing System Manager is not ready, Please wait" << std::endl;
        std::future<bool> f = dataServingSystemManager_->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
        if(subSystemStatus) {
            std::cout << "\nServing System Manager is ready" << std::endl;
            dataServingSystemManager_->registerListener(dataServingSystemListener_);
        }
    }
    return subSystemStatus;
}

void DataServingSystemMenu::requestServiceStatus(std::vector<std::string> inputCommand) {
    std::cout << "Request Service Status\n";

    if (dataServingSystemManager_ == nullptr) {
        std::cout << "Serving System Manager is not ready" << std::endl;
        return;
    }

    // Callback
    auto respCb = [](telux::data::ServiceStatus serviceStatus,
                           telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "requestServiceStatus Response ";
        if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            if(serviceStatus.serviceState == telux::data::DataServiceState::OUT_OF_SERVICE) {
                std::cout << "Current Status is Out Of Service" << std::endl;
            } else {
                std::cout << "Current Status is In Service" << std::endl;
                std::cout << "Preferred Rat is "
                            << DataUtils::serviceRatToString(serviceStatus.networkRat) << std::endl;
            }
        }
        else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error)
                      << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        }
    };

    telux::common::Status retStat = dataServingSystemManager_->requestServiceStatus(respCb);
    Utils::printStatus(retStat);
}

void DataServingSystemMenu::requestRoamingStatus(std::vector<std::string> inputCommand) {
    std::cout << "Request Roaming Status\n";
    telux::common::Status retStat;

    if (dataServingSystemManager_ == nullptr) {
        std::cout << "Serving System Manager is not ready" << std::endl;
        return;
    }

    // Callback
    auto respCb = [](
            telux::data::RoamingStatus roamingStatus, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "requestRoamingStatus Response ";
        if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            bool isRoaming = roamingStatus.isRoaming;
            if(isRoaming) {
                std::cout << "System is in Roaming State" << std::endl;
                std::cout << "Roaming Type: ";
                switch(roamingStatus.type)  {
                    case telux::data::RoamingType::INTERNATIONAL:
                        std::cout << "International" << std::endl;
                    break;
                    case telux::data::RoamingType::DOMESTIC:
                        std::cout << "Domestic" << std::endl;
                    break;
                    default:
                        std::cout << "Unknown" << std::endl;
                }
            } else {
                std::cout << "System is not in Roaming State" << std::endl;
            }
        }
        else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error)
                      << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        }
    };

    retStat =
        dataServingSystemManager_->requestRoamingStatus(respCb);
    Utils::printStatus(retStat);
}

void DataServingSystemMenu::makeDormant(std::vector<std::string> inputCommand) {
    std::cout << "Make Doramnt\n";
    auto respCb = [](telux::common::ErrorCode errCode) {
        std::cout << std::endl << std::endl;
        std::cout << "Callback: "
                  << "makeDormant Response "
                  <<((errCode == telux::common::ErrorCode::SUCCESS)? "is Successful":"failed")
                  << ". ErrorCode = " << static_cast<int>(errCode)
                  << ", Descrition: " << Utils::getErrorCodeAsString(errCode) << std::endl;
    };
    telux::common::Status status = dataServingSystemManager_->makeDormant(respCb);
    Utils::printStatus(status);
}

