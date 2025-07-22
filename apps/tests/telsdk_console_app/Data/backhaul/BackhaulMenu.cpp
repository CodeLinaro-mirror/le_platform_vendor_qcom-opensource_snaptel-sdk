/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "BackhaulMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

BackhaulMenu::BackhaulMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    backhaulManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

BackhaulMenu::~BackhaulMenu() {
}

bool BackhaulMenu::init() {
    bool initStatus = initBackhaulManager();
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

    if (not initStatus) {
        return false;
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> requestBackhaulStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "request_Backhaul_Status", {},
                std::bind(&BackhaulMenu::requestBackhaulStatus, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setBHLoadBalance
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "set_BHLoad_Balance", {},
                std::bind(&BackhaulMenu::setBHLoadBalance, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> requestBHLoadBalanceStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "request_BHLoad_Balance_Status", {},
                std::bind(&BackhaulMenu::requestBHLoadBalanceStatus, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {requestBackhaulStatus,
        setBHLoadBalance, requestBHLoadBalanceStatus};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

bool BackhaulMenu::initBackhaulManager() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    bool retVal = false;

    auto initCb = std::bind(&BackhaulMenu::onInitComplete, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto backhaulMgr = dataFactory.getBackhaulManager(initCb);
    if (backhaulMgr) {
        std::unique_lock<std::mutex> lck(mtx_);
        telux::common::ServiceStatus subSystemStatus = backhaulMgr->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing BackHaul Manager subsystem, Please wait \n";
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = backhaulMgr->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nBackHaul Manager is ready" << std::endl;
            retVal = true;
            backhaulManager_ = backhaulMgr;
            backhaulListener_ = std::make_shared<BackhaulListener>();
            telux::common::Status status = backhaulMgr->registerListener(backhaulListener_);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Unable to register client listener" << std::endl;
            }
        }
        else {
            std::cout << "\nBackHaul Manager is not ready" << std::endl;
        }
    }
    return retVal;
}


void BackhaulMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void BackhaulMenu::requestBackhaulStatus(std::vector<std::string> inputCommand) {
    std::cout << "Request BackHaul Status\n";
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    // Callback
    auto respCb = [this](const telux::data::net::BackhaulStatusInfo &backhaulStatusInfo, 
                         telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestBackHaulStatus Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (error == telux::common::ErrorCode::SUCCESS) {
           // If no BackHaul is present, print No Backhaul
           if ((backhaulStatusInfo.isV4BackhaulAvailable  == false) &&
               (backhaulStatusInfo.isV6BackhaulAvailable == false) &&
               (backhaulStatusInfo.isEthPduAvailable == false)) {
             std::cout <<"No Backhaul \n";
           }
           else {
               std::cout<< DataUtils::backhaulToString(backhaulStatusInfo.backhaulType) <<" Backhual\n";
               std::cout<<"IPV4 " << (backhaulStatusInfo.isV4BackhaulAvailable ? \
                                  "Connected": "Disconnected") <<"\n";
               std::cout<<"IPV6 " << (backhaulStatusInfo.isV6BackhaulAvailable ? \
                                  "Connected": "Disconnected") <<"\n";
               std::cout<<"ETH PDU " << (backhaulStatusInfo.isEthPduAvailable ? \
                                     "Connected": "Disconnected") <<"\n";
           }
        }
    };
    retStat = backhaulManager_->requestBackhaulStatus(respCb);
    Utils::printStatus(retStat);
}

void BackhaulMenu::setBHLoadBalance(std::vector<std::string> inputCommand) {
    std::cout << "Set Load Balance \n";
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    telux::data::net::BHLoadBalanceInfo bhLoadBalanceInfoReq {};

    std::cout<<"Please input Load Balance State (1-Enable/0-Disable) :";
    int enableFlag;
    std::cin >> enableFlag;
    Utils::validateInput(enableFlag, {0, 1});
    if (enableFlag) {
        bhLoadBalanceInfoReq.enable = true;
        int wanWeight;
        std::cout<<"Enter the value of wan weight in range [1, 9] : ";
        std::cin >> wanWeight;
        Utils::validateInput(wanWeight);
        bhLoadBalanceInfoReq.wan = wanWeight;
        bhLoadBalanceInfoReq.waneth = 10 - bhLoadBalanceInfoReq.wan;
        std::cout<<"\nwan traffic: "<< bhLoadBalanceInfoReq.wan * 10
                 <<"\nwaneth traffic: "<< bhLoadBalanceInfoReq.waneth * 10 <<std::endl;
    }
    else {
        bhLoadBalanceInfoReq.enable = false;
    }

    // Callback
    auto respCb = [this](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setBHLoadBalance Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "\n Backhaul Load Balance set succeeds \n";
        }
        else {
            std::cout << "\n Backhaul Load Balance set fails \n";
        }
    };
    retStat = backhaulManager_->setBHLoadBalance(bhLoadBalanceInfoReq, respCb);
    Utils::printStatus(retStat);
}

void BackhaulMenu::requestBHLoadBalanceStatus(std::vector<std::string> inputCommand) {
    std::cout << "Request BH Load Balance Status\n";
    telux::common::Status retStat = telux::common::Status::SUCCESS;

    // Callback
    auto respCb = [this](const telux::data::net::BHLoadBalanceInfo& bhLoadBalanceInfoResp, 
                         telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestBHLoadBalanceStatus Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "\n Backhaul Load Balance : " 
                      << (bhLoadBalanceInfoResp.enable ? "Enabled \n":"Disabled\n");
            if (bhLoadBalanceInfoResp.enable){
              std::cout<<"\nwan traffic: "<< bhLoadBalanceInfoResp.wan * 10 
                       <<"\nwaneth traffic: "<< bhLoadBalanceInfoResp.waneth * 10 <<"\n";
            }
        }

    };
    retStat = backhaulManager_->requestBHLoadBalanceStatus(respCb);
    Utils::printStatus(retStat);
}

