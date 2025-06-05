/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "StatsMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

StatsMenu::StatsMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    statsManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

StatsMenu::~StatsMenu() {
}

bool StatsMenu::init() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool initStatus = initStatsManager();

    if (not initStatus) {
        return false;
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> setClientDataUsageStatsConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "set_Client_Data_Usage_Stats_Config", {},
                std::bind(&StatsMenu::setClientDataUsageStatsConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getClientDataUsageStats
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "get_Client_Data_Usage_Stats", {},
                std::bind(&StatsMenu::getClientDataUsageStats, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> resetClientDataUsageStats
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "reset_Client_Data_Usage_Stats",
                {}, std::bind(&StatsMenu::resetClientDataUsageStats, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getClientDataUsageStatsConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "get_Client_Data_Usage_Stats_Config",
                {}, std::bind(&StatsMenu::getClientDataUsageStatsConfig, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {setClientDataUsageStatsConfig,
            getClientDataUsageStats, resetClientDataUsageStats, getClientDataUsageStatsConfig};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void StatsMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

bool StatsMenu::initStatsManager() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    bool retVal = false;

    auto initCb = std::bind(&StatsMenu::onInitComplete, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto statsMgr = dataFactory.getStatsManager(initCb);
    if (statsMgr) {
        std::unique_lock<std::mutex> lck(mtx_);
        telux::common::ServiceStatus subSystemStatus = statsMgr->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing Stats Manager subsystem, Please wait \n";
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = statsMgr->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\n Stats Manager is ready" << std::endl;
            retVal = true;
            statsManager_ = statsMgr;
            statsListener_ = std::make_shared<telux::data::net::IStatsListener>();
            telux::common::Status status = statsMgr->registerListener(statsListener_);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Unable to register stats listener" << std::endl;
            }
        }
        else {
            std::cout << "\n Stats Manager is not ready" << std::endl;
        }
    }
    return retVal;
}

void StatsMenu::setClientDataUsageStatsConfig(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode retStat;
    telux::data::net::ClientStatsConfig clientStatsConfig {};
    std::cout << "Set Client Data Usage Stats Config "<< std::endl;

    int enableFlag;
    clientStatsConfig.enable = false;
    std::cout << "Please input enable/disable (1-Enable/0-Disable): ";
    std::cin >> enableFlag;
    Utils::validateInput(enableFlag, {0, 1});
    if (enableFlag) {
        clientStatsConfig.enable = true;
    }

    if(clientStatsConfig.enable) {
        int statsType;
        std::cout <<"Enter statsType ("
                  << static_cast<int>(telux::data::net::StatsType::IP_BASED)
                  <<"-IP_BASED"
                  << static_cast<int>(telux::data::net::StatsType::MAC_BASED)
                  <<"-MAC_BASED) :";
        std::cin >> statsType;
        Utils::validateInput(statsType,{
            static_cast<int>(telux::data::net::StatsType ::IP_BASED),
            static_cast<int>(telux::data::net::StatsType ::MAC_BASED)});
        clientStatsConfig.type = static_cast<telux::data::net::StatsType>(statsType);
    }

    retStat = statsManager_->setClientDataUsageStatsConfig(clientStatsConfig);

    std::cout << std::endl << std::endl;
    std::cout << "setClientDataUsageStatsConfig Response"
              << (telux::common::ErrorCode::SUCCESS == retStat ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retStat)
              << ", description: " << Utils::getErrorCodeAsString(retStat) << std::endl;
    if (retStat == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Set Client Data Usage StatsConfig succeeds" << std::endl;
    } else {
        std::cout << " Set Client Data Usage Stats Config failed" << std::endl;
    }
}

void StatsMenu::getClientDataUsageStats(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode retStat;
    telux::data::net::StatsType type;
    vector<telux::data::ClientDataUsage> clientDataUsageStats;
    std::cout << "Get Client Data Usage Stats\n";

    int statsType;
    std::cout <<"Enter statsType ("
              << static_cast<int>(telux::data::net::StatsType::IP_BASED)
              <<"-IP_BASED,"
              << static_cast<int>(telux::data::net::StatsType::MAC_BASED)
              <<"-MAC_BASED) :";
    std::cin >> statsType;
    Utils::validateInput(statsType,{
        static_cast<int>(telux::data::net::StatsType ::IP_BASED),
        static_cast<int>(telux::data::net::StatsType ::MAC_BASED)});
    type = static_cast<telux::data::net::StatsType>(statsType);

    retStat = statsManager_->getClientDataUsageStats(type, clientDataUsageStats);

    std::cout << std::endl << std::endl;
    std::cout << "getClientDataUsageStats Response"
              << (telux::common::ErrorCode::SUCCESS == retStat ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retStat)
              << ", description: " << Utils::getErrorCodeAsString(retStat) << std::endl;

    if (retStat == telux::common::ErrorCode::SUCCESS) {
        std::cout << "getClientDataUsageStats succeeds" << std::endl;
        for (const telux::data::ClientDataUsage x : clientDataUsageStats) {
            std::cout<<"IPV4: "<< x.v4Addr << std::endl;
            std::cout<<"IPV6: ";
            for(const auto& addr: x.v6Addr){
                std::cout<< addr << " ";
            }
            std::cout<<"\nMac Address "<< x.macAddress << std::endl;
            std::cout<<"Bytes Rx: "<< x.usage.bytesRx << std::endl;
            std::cout<<"Bytes Tx: "<< x.usage.bytesTx << std::endl;
        }
    } else {
        std::cout << "getClientDataUsageStats failed" << std::endl;
    }
}

void StatsMenu::resetClientDataUsageStats(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode retStat;
    std::cout << "Reset Client Data Usage Stats\n";

    retStat = statsManager_->resetClientDataUsageStats();

    std::cout << std::endl << std::endl;
    std::cout << "resetClientDataUsageStats Response"
              << (telux::common::ErrorCode::SUCCESS == retStat ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retStat)
              << ", description: " << Utils::getErrorCodeAsString(retStat) << std::endl;
    if (retStat == telux::common::ErrorCode::SUCCESS) {
        std::cout << " resetClientDataUsageStats succeeds" << std::endl;
    } else {
        std::cout << " resetClientDataUsageStats failed" << std::endl;
    }
}

void StatsMenu::getClientDataUsageStatsConfig(std::vector<std::string> inputCommand) {
    telux::common::ErrorCode retStat;
    telux::data::net::ClientStatsConfig clientStatsConfig {};
    std::cout << "Get Client Data Usage Stats Config\n";

    retStat = statsManager_->getClientDataUsageStatsConfig(clientStatsConfig);

    std::cout << std::endl << std::endl;
    std::cout << "getClientDataUsageStatsConfig Response"
              << (telux::common::ErrorCode::SUCCESS == retStat ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retStat)
              << ", description: " << Utils::getErrorCodeAsString(retStat) << std::endl;

    if (retStat == telux::common::ErrorCode::SUCCESS) {
        std::cout<<"getClientDataUsageStatsConfig succeeds" << std::endl;
        std::cout<< "Config Status : "<< (clientStatsConfig.enable ? "Enabled" : "Disabled") 
                 << std::endl;
        std::cout<< "StatsType : " << (clientStatsConfig.type == telux::data::net::StatsType::IP_BASED \ 
                   ? "IP_BASED" : "MAC_BASED") << std::endl;
    } else {
        std::cout << " getClientDataUsageStatsConfig failed" << std::endl;
    }
}


