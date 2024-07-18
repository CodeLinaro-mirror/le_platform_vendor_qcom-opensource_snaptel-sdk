/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DiagTestApp.cpp
 *
 * @brief      This is entry class for test application for Diagnostics,
 *             It allows one to interactively invoke most of the public APIs in Diagnostics.
 */

#include "DiagTestApp.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

DiagTestApp::DiagTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DiagTestApp::~DiagTestApp() {
    // diagLoggingMethodMenu_ = nullptr;
    diagFileMethodMenu_ = nullptr;
    diagCallbackMethodMenu_ = nullptr;
}

bool DiagTestApp::init() {
    std::shared_ptr<ConsoleAppCommand> diagFileMethodMenu =
        std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "file_method_menu",
        {}, std::bind(&DiagTestApp::diagFileMethodMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> diagCallbackMethodMenu =
        std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "callback_method_menu",
        {}, std::bind(&DiagTestApp::diagCallbackMethodMenu, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
        diagFileMethodMenu, diagCallbackMethodMenu};

    addCommands(commandsList);
    ConsoleApp::displayMenu();
    return true;
}

void DiagTestApp::diagFileMethodMenu(std::vector<std::string> inputCommand) {
    if (diagFileMethodMenu_ == nullptr) {
        diagFileMethodMenu_ =
            std::make_shared<DiagFileMethodMenu>("File Method Menu", "file> ");
    }
    if (diagFileMethodMenu_->init()) {
        diagFileMethodMenu_->mainLoop();
        diagFileMethodMenu_ = nullptr;
    }
    ConsoleApp::displayMenu();
}

void DiagTestApp::diagCallbackMethodMenu(std::vector<std::string> inputCommand) {
    if (diagCallbackMethodMenu_ == nullptr) {
        diagCallbackMethodMenu_ =
            std::make_shared<DiagCallbackMethodMenu>("Callback Method Menu", "callback> ");
    }
    if (diagCallbackMethodMenu_->init()) {
        diagCallbackMethodMenu_->mainLoop();
        diagCallbackMethodMenu_ = nullptr;
    }
    ConsoleApp::displayMenu();
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "logd"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Diag Test App: Adding supplementary groups failed!" << std::endl;
    }

    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string appName = "Diag Test App v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch);
    DiagTestApp diagTestApp(appName, "Diag> ");
    if (diagTestApp.init()) {       // initialize commands and display
        diagTestApp.mainLoop();     // Main loop to continuously read and execute commands
    }
    return 0;
}
