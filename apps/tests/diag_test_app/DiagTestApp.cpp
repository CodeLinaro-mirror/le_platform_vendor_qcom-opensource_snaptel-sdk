/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    std::vector<std::string> supplementaryGrps{"system", "diag"};
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
