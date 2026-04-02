/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanControlManagerMenu.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanControlManagerMenu::WlanControlManagerMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

WlanControlManagerMenu::~WlanControlManagerMenu() {
    if (wlanControlManager_) {
        wlanControlManager_ = nullptr;
    }
}

bool WlanControlManagerMenu::isSubSystemReady() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    if (wlanControlManager_ == nullptr) {
        auto initCb =
            std::bind(&WlanControlManagerMenu::onInitComplete, this, std::placeholders::_1);
        auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
        wlanControlManager_ = wlanFactory.getWlanControlManager(initCb);
        if (wlanControlManager_ == nullptr) {
            std::cout << "\nError encountered in initializing Wlan Control Manager" << std::endl;
            return false;
        }
        wlanControlManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        std::cout << "\nInitializing WlanControlManager, Please wait ..." << std::endl;
        bool notified = cv_.wait_for(lck, std::chrono::seconds(5),
            [this]{ return this->subSystemStatusUpdated_; });
        if (!notified) {
            std::cout << "\nWlan Control Manager initialization timed out" << std::endl;
            wlanControlManager_ = nullptr;
            return false;
        }
        subSystemStatus = wlanControlManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nWlan Control Manager is Ready" << std::endl;
        } else {
            wlanControlManager_ = nullptr;
            return false;
        }
    }
    return true;
}

bool WlanControlManagerMenu::init() {
    ConsoleApp::displayMenu();
    return true;
}

void WlanControlManagerMenu::showMenu() {
    ConsoleApp::displayMenu();
}

// ---------------------------------------------------------------------------
// Initialization callback
// ---------------------------------------------------------------------------

void WlanControlManagerMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

// ---------------------------------------------------------------------------
// IWlanControlListener callbacks
// ---------------------------------------------------------------------------

void WlanControlManagerMenu::onServiceStatusChange(telux::common::ServiceStatus status) {
    PRINT_NOTIFICATION << " ** WlanControl onServiceStatusChange **\n";
    switch (status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            std::cout << " SERVICE_AVAILABLE\n";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            std::cout << " SERVICE_UNAVAILABLE\n";
            break;
        default:
            std::cout << " Unknown service status\n";
            break;
    }
}
