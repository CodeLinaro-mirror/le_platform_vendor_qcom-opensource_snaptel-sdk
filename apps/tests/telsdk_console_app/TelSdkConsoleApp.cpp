/*
 *  Copyright (c) 2017-2020 The Linux Foundation. All rights reserved.
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

/**
 * @file       TelSdkConsoleApp.cpp
 *
 * @brief      This is entry class for console application for Telematics SDK,
 *             It allows one to interactively invoke most of the public APIs in the Telematics SDK.
 */

#include <iostream>
#include <memory>

extern "C" {
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
}

#include <telux/common/Version.hpp>
#include <telux/common/DeviceConfig.hpp>

#include "Call/CallMenu.hpp"
#include "ECall/ECallMenu.hpp"
#include "Phone/PhoneMenu.hpp"
#include "Sms/SmsMenu.hpp"
#include "Data/DataMenu.hpp"
#include "SimCardServices/SimCardServicesMenu.hpp"
#include "MultiSim/MultiSimMenu.hpp"
#include "Cellbroadcast/CellbroadcastMenu.hpp"
#include "Rsp/RspMenu.hpp"
#include "../../common/utils/Utils.hpp"

#include "TelSdkConsoleApp.hpp"

TelSdkConsoleApp::TelSdkConsoleApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

TelSdkConsoleApp::~TelSdkConsoleApp() {
}

/**
 * Used for creating a menus of high level features
 */
void TelSdkConsoleApp::init() {
    std::shared_ptr<ConsoleAppCommand> phoneMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Phone_Status", {},
            std::bind(&TelSdkConsoleApp::phoneMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> callMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Dialer", {},
            std::bind(&TelSdkConsoleApp::callMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> eCallMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "eCall", {},
            std::bind(&TelSdkConsoleApp::eCallMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> smsMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "4", "SMS", {}, std::bind(&TelSdkConsoleApp::smsMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> simCardMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Card_Services", {},
            std::bind(&TelSdkConsoleApp::simCardMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> dataMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "Data", {}, std::bind(&TelSdkConsoleApp::dataMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> multiSimMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "MultiSim", {},
        std::bind(&TelSdkConsoleApp::multiSimMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> cbMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "CellBroadcast", {},
            std::bind(&TelSdkConsoleApp::cellbroadcastMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> rspMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "9", "Sim_Profile_Management", {}, std::bind(&TelSdkConsoleApp::rspMenu, this,
               std::placeholders::_1)));
    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
        = {phoneMenuCommand, callMenuCommand, eCallMenuCommand, smsMenuCommand, simCardMenuCommand,
             dataMenuCommand, multiSimMenuCommand, cbMenuCommand, rspMenuCommand};

    // This instance is needed to hold the audio for the voice call in case the user comes out of
    // dialer menu.
    AudioClient &audioClient_ = AudioClient::getInstance();
    addCommands(mainMenuCommands);
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::phoneMenu(std::vector<std::string> userInput) {
    TelSdkConsoleApp::onModemAvailable();
    PhoneMenu phoneMenu("Phone Menu", "phone> ");
    phoneMenu.init();
    phoneMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::callMenu(std::vector<std::string> userInput) {
    TelSdkConsoleApp::onModemAvailable();
    CallMenu callMenu("Dialer Menu", "dialer> ");
    callMenu.init();
    callMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::eCallMenu(std::vector<std::string> userInput) {
    TelSdkConsoleApp::onModemAvailable();
    ECallMenu eCallMenu("eCall Menu", "eCall> ");
    eCallMenu.init();
    eCallMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::simCardMenu(std::vector<std::string> userInput) {
    TelSdkConsoleApp::onModemAvailable();
    SimCardServicesMenu simCardServicesMenu("SIM Card Services Menu", "card_services> ");
    simCardServicesMenu.init();
    simCardServicesMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::smsMenu(std::vector<std::string> userInput) {
    TelSdkConsoleApp::onModemAvailable();
    SmsMenu smsMenu("SMS Menu", "sms> ");
    smsMenu.init();
    smsMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::dataMenu(std::vector<std::string> userInput) {
    DataMenu dataMenu("Data Menu", "data> ");
    dataMenu.init();
    dataMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::multiSimMenu(std::vector<std::string> userInput) {
    MultiSimMenu multiSimMenu("MultiSim Menu", "multisim> ");
    multiSimMenu.init();
    multiSimMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::cellbroadcastMenu(std::vector<std::string> userInput) {
    CellbroadcastMenu cbMenu("Cellbroadcast Menu", "cb> ");
    cbMenu.init();
    cbMenu.mainLoop();
}

void TelSdkConsoleApp::rspMenu(std::vector<std::string> userInput) {
    RemoteSimProfileMenu rspMenu("Sim Profile Management Menu", "sim_profile_management> ");
    rspMenu.init();
    rspMenu.mainLoop();
    TelSdkConsoleApp::displayMenu();
}

void TelSdkConsoleApp::displayMenu() {
    ConsoleApp::displayMenu();
}

void TelSdkConsoleApp::onModemAvailable() {
// Do not perform requestOperatingMode in CV2X machine
// since operating mode cannot be changed
#ifndef FEATURE_CV2X_ONLY
    std::cout << "\n\nChecking telephony subsystem, Please wait!!!..." << std::endl;
    std::shared_ptr<ModemStatus> modemStatus = std::make_shared<ModemStatus>();
    modemStatus->printOperatingMode();
#endif
}

void signalHandler(int sig) {
    exit(1);
}

void setupSignal() {
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGBUS, signalHandler);
    signal(SIGILL, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGPIPE, signalHandler);
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string appName = "Telematics SDK v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch);
    setupSignal();
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "radio"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    TelSdkConsoleApp telsdkConsoleApp(appName, "tel_sdk> ");

    telsdkConsoleApp.init();  // initialize commands and display

    return telsdkConsoleApp.mainLoop();  // Main loop to continuously read and execute commands
}
