/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * This application demonstrates how to perform OTA. The steps are as follows:
 *
 *  1. Get a SwUpdateFactory instance.
 *  2. the menu will be displayed.
 *  3. if press 1 then menu will ask to enter OTA zip file location and then it will call performOTA API.
 *  4. after step 3 device will go to reboot and perform OTA.
 *  5. again run the app and rpovide choice 2 to check OTA status
 *
 * Usage:
 * # ./swupdate_console_app
 */


#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <cstdlib>
#include <telux/swupdate/SwUpdateFactory.hpp>
#include <telux/swupdate/SwUpdateManager.hpp>

#include "SwUpdateConsoleApp.hpp"
#include "../../common/utils/Utils.hpp"
#include <telux/common/Version.hpp>

using namespace telux::swupdate;
using namespace telux::common;

SwUpdateConsoleApp::SwUpdateConsoleApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

SwUpdateConsoleApp::~SwUpdateConsoleApp() {
}

void SwUpdateConsoleApp::init() {

    auto &swUpdateFactory = SwUpdateFactory::getInstance();
    swUpdateManager_ = swUpdateFactory.getSwUpdateManager();
    if (!swUpdateManager_) {
        std::cout << "Failed to get SwUpdateManager object" << std::endl;
        return;
    }
    initConsole();
}

void SwUpdateConsoleApp::initConsole() {
    std::shared_ptr<ConsoleAppCommand> performSwUpdateCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Perform Update", {},
        std::bind(&SwUpdateConsoleApp::performSwUpdate, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> readSwUpdateStatusCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Read SwUpdateStatus", {},
        std::bind(&SwUpdateConsoleApp::readSwUpdateStatus, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
    = {performSwUpdateCommand, readSwUpdateStatusCommand};

    ConsoleApp::addCommands(mainMenuCommands);
    ConsoleApp::displayMenu();
}

void SwUpdateConsoleApp::performSwUpdate(std::vector<std::string> userInput) {
    std::cout<<"Kindly enter the zip path";
    std::string zipPath;
    std::cin>>zipPath;
    swUpdateManager_->performUpdate(zipPath);
    std::cout << "Please read /cache/recovery/ota_status file to check ota" << std::endl;
}


std::string toString(telux::swupdate::UpdateStatus status) {
    switch (status) {
        case UpdateStatus::SUCCESS: return "SUCCESS";
        case UpdateStatus::FAILED: return "FAILED";
        case UpdateStatus::INPROGRESS: return "INPROGRESS";
        case UpdateStatus::INVALIDSTATE: return "INVALIDSTATE";
        case UpdateStatus::NOSUCH: return "NOSUCH";
        default: return "Unknown";
 }
}

void SwUpdateConsoleApp::readSwUpdateStatus(std::vector<std::string> userInput) {
    telux::swupdate::UpdateStatus status = swUpdateManager_->readUpdateStatus();
    std::cout<<"Value of SwUpdateStatus is " << toString(status) << "Please chcek UpdateStatus enum class";
}


int main() {

    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "SwUpdate console app - SDK v" + std::to_string(sdkVersion.major) + "."
                             + std::to_string(sdkVersion.minor) + "."
                             + std::to_string(sdkVersion.patch) + "\n" +
                             "Release name: " + sdkReleaseName;
    auto swUpdateConsoleApp = std::make_shared<SwUpdateConsoleApp>(appName, "swupdate> ");

    swUpdateConsoleApp->init();  // initialize commands and display

    return swUpdateConsoleApp->mainLoop();  // Main loop to continuously read and execute commands
}
