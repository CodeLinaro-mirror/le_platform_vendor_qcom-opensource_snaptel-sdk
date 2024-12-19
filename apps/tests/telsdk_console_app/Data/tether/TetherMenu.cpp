/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "TetherMenu.hpp"

using namespace std;

TetherMenu::TetherMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    tetherManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

TetherMenu::~TetherMenu() {
}

bool TetherMenu::init() {

    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;

    if (tetherManager_ == nullptr) {
        auto initCb = std::bind(&TetherMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();

        auto localTetherMgr = dataFactory.getTetherManager(
            telux::data::OperationType::DATA_LOCAL, initCb);
        if (localTetherMgr ) {
            tetherManager_ = localTetherMgr ;
        }

        auto remoteTetherMgr = dataFactory.getTetherManager(
            telux::data::OperationType::DATA_REMOTE, initCb);
        if (remoteTetherMgr) {
            tetherManager_ = remoteTetherMgr;
        }

        if(tetherManager_ == nullptr ) {
            //Return immediately
            std::cout << "\nError encountered in initializing Tether Manager" << std::endl;
            return false;
        }
         tetherManager_->registerListener(shared_from_this());
    }

    {
        std::unique_lock<std::mutex> lck(mtx_);

        telux::common::ServiceStatus subSystemStatus = tetherManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing Tether Manager, Please wait ..." << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = tetherManager_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nTether Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nTether Manager initialization failed" << std::endl;
            tetherManager_ = nullptr;
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> startBTTether
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "start_BT_Tether", {},
                std::bind(&TetherMenu::startBTTether, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> stopBTTether
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "stop_BT_Tether", {},
                std::bind(&TetherMenu::stopBTTether, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestBTTetherStatus =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "request_BT_Tether_Status",
            {}, std::bind(&TetherMenu::requestBTTetherStatus, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {startBTTether,
            stopBTTether, requestBTTetherStatus };
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void TetherMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void TetherMenu::startBTTether(std::vector<std::string> inputCommand){
    telux::common::Status retStat;
    telux::data::net::BTTetherMode  btMode {};

    std::cout << "Bring UP BT Tethering \n";

    int btModeVal;
    std::cout << "Enter BT Mode (0-LAN/1-WAN): " << std::endl;
    std::cin >> btModeVal;
    Utils::validateInput(btModeVal,
                        {static_cast<int>(telux::data::net::BTTetherMode::LAN),
                         static_cast<int>(telux::data::net::BTTetherMode::WAN)}
                        );
    btMode = static_cast<telux::data::net::BTTetherMode>(btModeVal);

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "startBTTether Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = tetherManager_->startBTTether(btMode, respCb);
    Utils::printStatus(retStat);
}

void TetherMenu::stopBTTether(std::vector<std::string> inputCommand){
    telux::common::Status retStat;

    std::cout << "Bring down BT Tethering \n";

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "stopBTTether Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = tetherManager_->stopBTTether(respCb);
    Utils::printStatus(retStat);
}

void TetherMenu::requestBTTetherStatus(std::vector<std::string> inputCommand){

    telux::common::Status retStat;
    std::cout << "Request BT Tethering Status \n";

    // Callback
    auto respCb = [](const telux::data::net::BTTetherMode  btMode,
                     const telux::data::net::BTTetherStatus  btStatus,
                     telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestBTTetherStatus Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
            if (error == telux::common::ErrorCode::SUCCESS) {
                if (btStatus == telux::data::net::BTTetherStatus::UP){
                    std::cout<<"\n BT Tethering is UP in";
                    std::cout<<" Mode : "
                             << ((btMode == telux::data::net::BTTetherMode::WAN) ? "WAN" : "LAN")
                             <<std::endl;
                }
                else
                    std::cout<<"BT Tethering is DOWN\n";
            } else {
                  std::cout<< "Failed to Get BT Tethering Status"<<std::endl;
            }
    };

    retStat = tetherManager_->requestBTTetherStatus(respCb);
    Utils::printStatus(retStat);
}