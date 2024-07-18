/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <sstream>

extern "C" {
#include "unistd.h"
}

#include "DualDataManagementMenu.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

DualDataManagementMenu::DualDataManagementMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

DualDataManagementMenu::~DualDataManagementMenu() {
}

bool DualDataManagementMenu::init() {
    bool initStatus = initDualDataManager();

    if (not initStatus) {
        return false;
    }
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> getDualDataCapibility
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1",
                "get_dual_data_capability", {},
                std::bind(&DualDataManagementMenu::getDualDataCapibility, this,
                std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getDualDataUsageRecommendation
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2",
                "get_dual_data_usage_recommendation", {},
                std::bind(&DualDataManagementMenu::getDualDataUsageRecommendation, this,
                std::placeholders::_1)));
        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
            = {getDualDataCapibility, getDualDataUsageRecommendation};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

bool DualDataManagementMenu::initDualDataManager() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;

    bool retVal = false;
    auto initCb = std::bind(&DualDataManagementMenu::onInitComplete, this, std::placeholders::_1);
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto dualDataMgr = dataFactory.getDualDataManager(initCb);

    if (dualDataMgr) {
        dualDataMgr->registerListener(shared_from_this());
        std::unique_lock<std::mutex> lck(mtx_);

        telux::common::ServiceStatus subSystemStatus = dualDataMgr->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing "
                      << " DualData Manager subsystem, Please wait \n";
            cv_.wait(lck, [this] { return this->subSystemStatusUpdated_; });
            subSystemStatus = dualDataMgr->getServiceStatus();
        }

        // At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\n"
                      << " DualData Manager is ready" << std::endl;
            retVal = true;
            dualDataManager_ = dualDataMgr;
        } else {
            std::cout << "\n"
                      << " DualData Manager is not ready" << std::endl;
        }
    }
    return retVal;
}

void DualDataManagementMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void DualDataManagementMenu::getDualDataCapibility(std::vector<std::string> &inputCommand) {
    std::cout << "get dual data capibility" << std::endl;

    bool capibility;
    telux::common::ErrorCode errorCode = dualDataManager_->getDualDataCapability(capibility);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        if (capibility) {
            std::cout << " Device does supports dual data feature." << std::endl;
        } else {
            std::cout << " Device does not support dual data feature." << std::endl;
        }
    } else {
        std::cout << " failed to get dual data capibility. ErrorCode: "
                  << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
    }
}

void DualDataManagementMenu::getDualDataUsageRecommendation(
    std::vector<std::string> &inputCommand) {
    std::cout << "get dual data usage recommendation" << std::endl;

    telux::data::DualDataUsageRecommendation recommendation;
    telux::common::ErrorCode errorCode =
        dualDataManager_->getDualDataUsageRecommendation(recommendation);
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
            std::cout << " dual data usage is: " << convertRecommendationToString(recommendation)
                      << "." << std::endl;
    } else {
        std::cout << " failed to get dual data usage recommendation. ErrorCode: "
                  << static_cast<int>(errorCode)
                  << ", description: " << Utils::getErrorCodeAsString(errorCode) << std::endl;
    }
}

std::string DualDataManagementMenu::convertRecommendationToString(
    telux::data::DualDataUsageRecommendation recommendation) {
    std::string recommendationStr;

    switch(recommendation) {
        case telux::data::DualDataUsageRecommendation::ALLOWED:
            recommendationStr = "ALLOWED";
        break;
        case telux::data::DualDataUsageRecommendation::NOT_ALLOWED:
            recommendationStr = "NOT_ALLOWED";
        break;
        case telux::data::DualDataUsageRecommendation::NOT_RECOMMENDED:
            recommendationStr = "NOT_RECOMMENDED";
        break;
        default:
        break;
    };

    return recommendationStr;
}

void DualDataManagementMenu::onDualDataCapabilityChange(bool isDualDataCapable) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** Dual data capability has changed ** \n";
    if(isDualDataCapable) {
        std::cout << "Device does supports dual data feature.";
    } else {
        std::cout << "Device does not support dual data feature.";
    }
    std::cout << std::endl << std::endl;
}

void DualDataManagementMenu::onDualDataUsageRecommendationChange(
        telux::data::DualDataUsageRecommendation recommendation) {
    std::cout << "\n\n";
    PRINT_NOTIFICATION << " ** Dual data usage recommendation has changed ** \n";
    std::cout << "Dual data usage is: " << convertRecommendationToString(recommendation);
    std::cout << std::endl << std::endl;
}