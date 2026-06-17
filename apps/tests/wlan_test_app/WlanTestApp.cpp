/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanTestApp.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanTestApp::WlanTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

WlanTestApp::~WlanTestApp() {
    wlanControlManagerMenu_ = nullptr;
}

bool WlanTestApp::initWlan() {
    if (wlanControlManagerMenu_ == nullptr) {
        wlanControlManagerMenu_ =
            std::make_shared<WlanControlManagerMenu>("Control Manager Menu", "ctrl> ");
    }
    if (wlanControlManagerMenu_->isSubSystemReady()) {
        return true;
    } else {
        std::cout << "Wlan Control Manager initialization failed, "
                  << "control_manager_menu will be unavailable." << std::endl;
        wlanControlManagerMenu_ = nullptr;
        return false;
    }
}

bool WlanTestApp::init() {
    initWlan();

    std::shared_ptr<ConsoleAppCommand> wlanControlManagerMenu =
        std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "control_manager_menu",
        {}, std::bind(&WlanTestApp::wlanControlManagerMenu, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {wlanControlManagerMenu};
    addCommands(commandsList);
    ConsoleApp::displayMenu();
    return true;
}

void WlanTestApp::wlanControlManagerMenu(std::vector<std::string> inputCommand) {
    if (wlanControlManagerMenu_) {
        wlanControlManagerMenu_->init();
        wlanControlManagerMenu_->mainLoop();
    } else {
        std::cout << "Wlan Control Manager is not available. Type 'q' to exit." << std::endl;
    }
    ConsoleApp::displayMenu();
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << "Wlan Test App: Adding supplementary groups failed!" << std::endl;
    }
    auto sdkVersion     = telux::common::Version::getSdkVersion();
    std::string appName = "Wlan Test App v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch);
    WlanTestApp wlanTestApp(appName, "Wlan> ");
    if (wlanTestApp.init()) {  // initialize commands and display
        wlanTestApp.mainLoop();  // Main loop to continuously read and execute commands
    }
    return 0;
}
