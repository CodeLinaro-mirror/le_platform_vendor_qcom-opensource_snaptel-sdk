/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * @file       ECallApp.cpp
 *
 * @brief      ECallApp class provides user interface to trigger an eCall and answer an incoming
 *             call(typically a PSAP callback).
 */

#include <iostream>
#include <algorithm>
#include <csignal>

#include <telux/tel/PhoneDefines.hpp>

#include "ECallApp.hpp"
#include "../../common/utils/Utils.hpp"

#define ECALL_CATEGORY_AUTO 1
#define ECALL_CATEGORY_MANUAL 2
#define ECALL_VARIANT_EMERGENCY 1
#define ECALL_VARIANT_VOICE 2
#define ECALL_VARIANT_TEST 3

ECallApp::ECallApp(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
    eCallMgr_ = std::make_shared<ECallManager>();
}

ECallApp::~ECallApp() {
   eCallMgr_ = nullptr;
}

ECallApp &ECallApp::getInstance() {
   static ECallApp instance("eCall App Menu", "eCall> ");
   return instance;
}

/**
 * Initialize console commands and Display
 */
void ECallApp::init() {

    std::shared_ptr<ConsoleAppCommand> eCallCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("1", "ECall", {},std::bind(&ECallApp::makeECall, this)));

    std::shared_ptr<ConsoleAppCommand> customNumberECallCommand =
        std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Custom_Number_ECall",
        {}, std::bind(&ECallApp::makeCustomNumberECall, this)));

    std::shared_ptr<ConsoleAppCommand> answerCallCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("3", "Answer_Incoming_Call", {}, std::bind(
        &ECallApp::answerIncomingCall, this)));

    std::shared_ptr<ConsoleAppCommand> hangupCallCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("4", "Hangup_Call", {}, std::bind(&ECallApp::hangupCall, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {eCallCommand,
            customNumberECallCommand, answerCallCommand, hangupCallCommand};
    addCommands(commandsList);

    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    if(telux::common::Status::SUCCESS == eCallMgr_->init()) {
        ConsoleApp::displayMenu();
    } else {
        std::cout << "Failed to initialize eCall Manager" << std::endl;
    }
}

/**
 * Trigger a standard eCall using the emergency number configured in FDN (eg.112)
 */
void ECallApp::makeECall() {

    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager, cannot trigger eCall" << std::endl;
        return;
    }
    // Get the emergency category from user
    telux::tel::ECallCategory emergencyCategory;
    if( -1 == getEcallCategory(emergencyCategory)) {
        return;
    }
    // Get eCall variant from user
    int opt = -1;
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Select variant:\n" << "1) Emergency : Initiates an emergency call \n"
                                     << "2) Test : Initiates an eCall for testing " << std::endl;
    std::getline(std::cin, temp, delimiter);
    if(!temp.empty()) {
        try {
            opt = std::stoi(temp);
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << opt << std::endl;
        }
    } else {
        std::cout << "No input, proceeding with default variant: Emergency" << std::endl;
        opt = ECALL_VARIANT_EMERGENCY;
    }
    telux::tel::ECallVariant eCallVariant;
    if(opt == ECALL_VARIANT_TEST) {  // Uses the PSAP number configured in NV settings
        eCallVariant = telux::tel::ECallVariant::ECALL_TEST;
    } else if(opt == ECALL_VARIANT_EMERGENCY) {  // Uses the emergency number configured in FDN
                                                     // i.e. 112.
        eCallVariant = telux::tel::ECallVariant::ECALL_EMERGENCY;
    } else {
        std::cout << "Invalid Emergency Call Variant" << std::endl;
        return;
    }
    // Get phoneId from user
    int phoneId = getPhoneId();

    std::cout << "eCall Triggered" << std::endl;
    auto ret = eCallMgr_->triggerECall(phoneId, emergencyCategory, eCallVariant);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "ECall request failed" << std::endl;
    } else {
        std::cout << "ECall request is successful" << std::endl;
    }
}

/**
 * Trigger a voice eCall to the specified phone number
 */
void ECallApp::makeCustomNumberECall() {

    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager, cannot trigger eCall" << std::endl;
        return;
    }
    // Get the emergency category from user
    telux::tel::ECallCategory emergencyCategory;
    if( -1 == getEcallCategory(emergencyCategory)) {
        return;
    }
    // Get phone number from user
    char delimiter = '\n';
    std::string dialNumber = "";
    std::cout << "Enter phone number: ";
    std::getline(std::cin, dialNumber, delimiter);
    if(dialNumber.empty()) {
        std::cout << "No input, please provide a valid phone number" << std::endl;
        return;
    }
    // Get phoneId from user
    int phoneId = getPhoneId();

    std::cout << "Custom number eCall Triggered" << std::endl;
    auto ret = eCallMgr_->triggerECall(phoneId, emergencyCategory, dialNumber);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "ECall request failed" << std::endl;
    } else {
        std::cout << "ECall request is successful" << std::endl;
    }
}

/**
 * Answer an incoming Call
 */
void ECallApp::answerIncomingCall() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    // Get phoneId from user
    int phoneId = getPhoneId();
    auto ret = eCallMgr_->answerCall(phoneId);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to answer call" << std::endl;
    }
}

/**
 * Hang-up an ongoing Call
 */
void ECallApp::hangupCall() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    auto ret = eCallMgr_->hangupCall();
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to hangup the call" << std::endl;
    }
}

/**
 * Hangs up a triggered eCall and gracefully clears down the subsystems.
 */
void ECallApp::cleanup() {
    hangupCall();
}

/**
 * Function to get phoneId from the user-interface
 */
int ECallApp::getPhoneId() {
    int phoneId = DEFAULT_PHONE_ID;
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Enter phone ID (uses default phoneID for no input): ";
    std::getline(std::cin, temp, delimiter);
    if(!temp.empty()) {
        try {
            phoneId = std::stoi(temp);
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values, " << phoneId
                    << std::endl;
        }
    } else {
        std::cout << "No input, proceeding with default phoneID: " << phoneId << std::endl;
    }
    return phoneId;
}

/**
 * Function to get eCall category from the user-interface
 */
int ECallApp::getEcallCategory(telux::tel::ECallCategory &emergencyCategory) {
    char delimiter = '\n';
    std::string temp;
    int opt = -1;
    // Get eCall category
    std::cout << "Select category:\n" << "1) Automatic : Vehicle initiated eCall \n"
                                      << "2) Manual : User initiated eCall " << std::endl;
    std::getline(std::cin, temp, delimiter);
    if(!temp.empty()) {
        try {
            opt = std::stoi(temp);
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << opt << std::endl;
        }
    } else {
        std::cout << "No input, proceeding with default category: automatic" << std::endl;;
        opt = ECALL_CATEGORY_AUTO;
    }
    if(opt == ECALL_CATEGORY_AUTO) {  // Automatically triggered eCall.
        emergencyCategory = telux::tel::ECallCategory::VOICE_EMER_CAT_AUTO_ECALL;
    } else if(opt == ECALL_CATEGORY_MANUAL) {  // Manually triggered eCall.
        emergencyCategory = telux::tel::ECallCategory::VOICE_EMER_CAT_MANUAL;
    } else {
        std::cout << "Invalid Emergency Call Category" << std::endl;
        return -1;
    }
    return 0;
}

void signalHandler(int sig) {
    ECallApp::getInstance().cleanup();
    exit(1);
}

void setupSignalHandler() {
    signal(SIGINT, signalHandler);
    signal(SIGSEGV, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGBUS, signalHandler);
    signal(SIGILL, signalHandler);
    signal(SIGFPE, signalHandler);
    signal(SIGPIPE, signalHandler);
}

// Main function that displays the interactive console for eCall related operations
int main(int argc, char **argv) {
    setupSignalHandler();
    std::vector<std::string> supplementaryGrps{"system"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    auto &eCallApp = ECallApp::getInstance();
    eCallApp.init();  // initialize commands and display
    return eCallApp.mainLoop();  // Main loop to continuously read and execute commands
}
