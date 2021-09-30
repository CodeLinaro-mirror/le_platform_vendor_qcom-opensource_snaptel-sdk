/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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
 * @file       ECallApp.cpp
 *
 * @brief      ECallApp class provides user interface to trigger an eCall and answer an incoming
 *             call(typically a PSAP callback).
 */

#include <iostream>
#include <sstream>
#include <algorithm>
#include <csignal>

#include <telux/tel/PhoneDefines.hpp>

#include "ECallApp.hpp"
#include "../../common/utils/Utils.hpp"

#define ECALL_CATEGORY_AUTO 1
#define ECALL_CATEGORY_MANUAL 2
#define ECALL_VARIANT_EMERGENCY 1
#define ECALL_VARIANT_TEST 2
#define ECALL_TRANSMIT_MSD 1
#define ECALL_DO_NOT_TRANSMIT_MSD 2
#define ECALL_T10_TIMER_MIN 60
#define ECALL_T10_TIMER_MAX 720

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

    std::shared_ptr<ConsoleAppCommand> getCallsCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("5", "Get_InProgress_Calls", {}, std::bind(&ECallApp::getCalls, this)));

    std::shared_ptr<ConsoleAppCommand> hlapTimerStatusCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("6", "Get_ECall_HLAP_Timers_Status", {},
                          std::bind(&ECallApp::requestECallHlapTimerStatus, this)));

    std::shared_ptr<ConsoleAppCommand> stopT10TimerCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("7", "Stop_T10_ECall_HLAP_Timer", {},
                          std::bind(&ECallApp::stopT10Timer, this)));

    std::shared_ptr<ConsoleAppCommand> setHlapTimerCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("8", "Set_ECall_HLAP_Timer", {},
                          std::bind(&ECallApp::setHlapTimer, this)));

    std::shared_ptr<ConsoleAppCommand> getHlapTimerCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("9", "Get_ECall_HLAP_Timer", {},
                          std::bind(&ECallApp::getHlapTimer, this)));

    std::shared_ptr<ConsoleAppCommand> getEcallConfigCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("10", "Get_ECall_Config", {}, std::bind(&ECallApp::getECallConfig,
        this)));

    std::shared_ptr<ConsoleAppCommand> setEcallConfigCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("11", "Set_ECall_Config", {}, std::bind(&ECallApp::setECallConfig,
        this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {eCallCommand,
        customNumberECallCommand, answerCallCommand, hangupCallCommand, getCallsCommand,
        hlapTimerStatusCommand, stopT10TimerCommand, setHlapTimerCommand, getHlapTimerCommand,
        getEcallConfigCommand, setEcallConfigCommand};
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
    // Configure MSD transmission at call connect
    bool transmitMsd = true;
    if( telux::common::Status::SUCCESS != getMsdTransmissionConfig(transmitMsd)) {
        return;
    }

    // Get phoneId from user
    int phoneId = getPhoneId();

    std::cout << "eCall Triggered" << std::endl;
    auto ret = eCallMgr_->triggerECall(phoneId, emergencyCategory, eCallVariant, transmitMsd);
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
    // Configure MSD transmission at call connect
    bool transmitMsd = true;
    if( telux::common::Status::SUCCESS != getMsdTransmissionConfig(transmitMsd)) {
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
    auto ret = eCallMgr_->triggerECall(phoneId, emergencyCategory, dialNumber, transmitMsd);
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
    // Get phoneId from user
    int phoneId = getPhoneId();
    int callIndex = -1;
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Enter call index (if more than one call exists): ";
    std::getline(std::cin, temp, delimiter);
    if(!temp.empty()) {
        try {
            callIndex = std::stoi(temp);
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values, " << callIndex
                    << std::endl;
        }
    } else {
        std::cout << "Trying to hangup the existing call" << std::endl;
    }
    auto ret = eCallMgr_->hangupCall(phoneId, callIndex);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to hangup the call" << std::endl;
    }
}

/**
 * Dump the list of calls in progress
 */
void ECallApp::getCalls() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    auto ret = eCallMgr_->getCalls();
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get current calls" << std::endl;
    }
}

/**
 * Request eCall High Level Application Protocol(HLAP) timers status
 */
void ECallApp::requestECallHlapTimerStatus() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    // Get phoneId from user
    int phoneId = getPhoneId();
    auto ret = eCallMgr_->requestHlapTimerStatus(phoneId);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get eCall HLAP timers status" << std::endl;
    }
}

/**
 * Request to stop T10 eCall High Level Application Protocol(HLAP) timer, which causes the
 * ecall-only modem to de-register from the network.
 */
void ECallApp::stopT10Timer() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    // Get phoneId from user
    int phoneId = getPhoneId();
    auto ret = eCallMgr_->stopT10Timer(phoneId);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to stop T10 HLAP timer" << std::endl;
    }
}

/**
 * Request to set the value of eCall High Level Application Protocol(HLAP) timer.
 */
void ECallApp::setHlapTimer() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    // Get phoneId from user
    int phoneId = getPhoneId();
    // Get timer type from user
    HlapTimerType type = HlapTimerType::UNKNOWN_TIMER;
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Enter HLAP timer type (10-T10 timer): ";
    std::getline(std::cin, temp, delimiter);
    if(!temp.empty()) {
        try {
            type = static_cast<HlapTimerType>(std::stoi(temp));
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values." << std::endl;
        }
    } else {
        std::cout << "No input" << std::endl;
        return;
    }
    switch (type) {
       case HlapTimerType::T10_TIMER:
          std::cout << "Enter the time duration in minutes (ranges from " << ECALL_T10_TIMER_MIN
              << " to " << ECALL_T10_TIMER_MAX << "): ";
          break;
       case HlapTimerType::T2_TIMER:
       case HlapTimerType::T5_TIMER:
       case HlapTimerType::T6_TIMER:
       case HlapTimerType::T7_TIMER:
       case HlapTimerType::T9_TIMER:
          std::cout << "Currently not supported" << std::endl;
          return;
       default:
          std::cout << "ERROR: invalid input, please enter corrected timer type, "
                  << static_cast<int>(type) << std::endl;
          return;
    }

    // Get time duration from user
    int timeDuration = -1;
    std::getline(std::cin, temp, delimiter);
    if(!temp.empty()) {
        try {
            timeDuration = std::stoi(temp);
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values, "
               << timeDuration << std::endl;
        }
    } else {
        std::cout << "No input" << std::endl;
        return;
    }
    auto ret = eCallMgr_->setHlapTimer(phoneId, type, timeDuration);
    if(ret != telux::common::Status::SUCCESS) {
       std::cout << "Failed to set HLAP timer" << std::endl;
    }
}

/**
 * Request to get the value of eCall High Level Application Protocol(HLAP) timer.
 */
void ECallApp::getHlapTimer() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    // Get phoneId from user
    int phoneId = getPhoneId();
    // Get timer type from user
    HlapTimerType type = HlapTimerType::UNKNOWN_TIMER;
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Enter HLAP timer type (10-T10 timer): ";
    std::getline(std::cin, temp, delimiter);
    if(!temp.empty()) {
        try {
            type = static_cast<HlapTimerType>(std::stoi(temp));
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values." << std::endl;
        }
    } else {
        std::cout << "No input" << std::endl;
        return;
    }
    switch (type) {
       case HlapTimerType::T10_TIMER:
           break;
       case HlapTimerType::T2_TIMER:
       case HlapTimerType::T5_TIMER:
       case HlapTimerType::T6_TIMER:
       case HlapTimerType::T7_TIMER:
       case HlapTimerType::T9_TIMER:
          std::cout << "Currently not supported" << std::endl;
          return;
       default:
          std::cout << "ERROR: invalid input, please enter corrected timer type, "
                  << static_cast<int>(type) << std::endl;
          return;
    }
    auto ret = eCallMgr_->getHlapTimer(phoneId, type);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get HLAP timer" << std::endl;
    }
}

/**
 * Get various configuration parameters related to eCall
 */
void ECallApp::getECallConfig() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    auto ret = eCallMgr_->getECallConfig();
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get eCall configuration" << std::endl;
    }
}

/**
 * Set various configuration parameters related to eCall
 */
void ECallApp::setECallConfig() {
    if(!eCallMgr_) {
        std::cout << "Invalid eCall Manager" << std::endl;
        return;
    }
    telux::tel::EcallConfig config = {};
    uint32_t temp = 0;
    std::string tempStr = "";
    char delimiter = '\n';

    std::cout << "Available configurations for eCall: \n    \
        \r\t0 - Mute/Unmute audio during MSD transmission \n    \
        \r\t1 - Use default or overridden dial number for eCall\n   \
        \r\t2 - Overridden number to be dialed\n    \
        \r\t3 - Use canned MSD\n    \
        \r\t4 - GNSS update interval(ms)\n  \
        \r\t5 - T2 Timer value\n    \
        \r\t6 - T7 Timer value\n    \
        \r\t7 - T9 Timer value\n    \
        \r\t8 - MSD Version \n\n";
    std::cout << " Choose the parameters to be configured\n \
        \r(For example, enter 5,8 to configure T2 Timer and Msd version): ";
    std::getline(std::cin, tempStr, delimiter);
    std::stringstream ss(tempStr);
    int i = -1;
    std::vector<int> options;
    while(ss >> i) {
        options.push_back(i);
        if(ss.peek() == ',' || ss.peek() == ' ')
        ss.ignore();
    }
    std::string promptStr = "";
    for(auto iter : options) {
        switch(iter) {
            case ECALL_CONFIG_MUTE_RX_AUDIO:
                promptStr = " Mute audio during MSD transmission? (1-True/0-False): ";
                if(telux::common::Status::SUCCESS == getIntegerInput(temp, promptStr,
                    std::vector<uint32_t>{0,1})) {
                    config.configValidityMask.set(ECALL_CONFIG_MUTE_RX_AUDIO);
                    config.muteRxAudio = temp;
                }
                break;
            case ECALL_CONFIG_NUM_TYPE:
                promptStr = " Use default or overridden dial number for eCall? "
                            "(0-Default/1-Overridden): ";
                if(telux::common::Status::SUCCESS == getIntegerInput(temp, promptStr,
                    std::vector<uint32_t>{0,1})) {
                    config.configValidityMask.set(ECALL_CONFIG_NUM_TYPE);
                    if(temp == 0) {
                        config.numType = ECallNumType::DEFAULT;
                    } else {
                        config.numType = ECallNumType::OVERRIDDEN;
                    }
                }
                break;
            case ECALL_CONFIG_OVERRIDDEN_NUM:
                std::cout << " Enter the dial number to be overridden: ";
                std::getline(std::cin, tempStr, delimiter);
                config.configValidityMask.set(ECALL_CONFIG_OVERRIDDEN_NUM);
                config.overriddenNum = tempStr;
                break;
            case ECALL_CONFIG_USE_CANNED_MSD:
                promptStr = " Use canned MSD? (1-True/0-False): ";
                if(telux::common::Status::SUCCESS == getIntegerInput(temp, promptStr,
                    std::vector<uint32_t>{0,1})) {
                    config.configValidityMask.set(ECALL_CONFIG_USE_CANNED_MSD);
                    config.useCannedMsd = temp;
                }
                break;
            case ECALL_CONFIG_GNSS_UPDATE_INTERVAL:
                promptStr = " Enter GNSS update interval(ms): ";
                if(telux::common::Status::SUCCESS == getIntegerInput(temp, promptStr,
                    std::vector<uint32_t>{})) {
                    config.configValidityMask.set(ECALL_CONFIG_GNSS_UPDATE_INTERVAL);
                    config.gnssUpdateInterval = temp;
                }
                break;
            case ECALL_CONFIG_T2_TIMER:
                promptStr = " Set T2 Timer value(ms): ";
                if(telux::common::Status::SUCCESS == getIntegerInput(temp, promptStr,
                    std::vector<uint32_t>{})) {
                    config.configValidityMask.set(ECALL_CONFIG_T2_TIMER);
                    config.t2Timer = temp;
                }
                break;
            case ECALL_CONFIG_T7_TIMER:
                promptStr = " Set T7 Timer value(ms): ";
                if(telux::common::Status::SUCCESS == getIntegerInput(temp, promptStr,
                    std::vector<uint32_t>{})) {
                    config.configValidityMask.set(ECALL_CONFIG_T7_TIMER);
                    config.t7Timer = temp;
                }
                break;
            case ECALL_CONFIG_T9_TIMER:
                promptStr = " Set T9 Timer value(ms): ";
                if(telux::common::Status::SUCCESS == getIntegerInput(temp, promptStr,
                    std::vector<uint32_t>{})) {
                    config.configValidityMask.set(ECALL_CONFIG_T9_TIMER);
                    config.t9Timer = temp;
                }
                break;
            case ECALL_CONFIG_MSD_VERSION:
                promptStr = " Set MSD version: ";
                if(telux::common::Status::SUCCESS == getIntegerInput(temp, promptStr,
                    std::vector<uint32_t>{})) {
                    config.configValidityMask.set(ECALL_CONFIG_MSD_VERSION);
                    config.msdVersion = temp;
                }
                break;
            default:
                std::cout << " Ignoring invalid input "<< iter << std::endl;
                break;
        }
    }

    auto ret = eCallMgr_->setECallConfig(config);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Failed to set eCall configuration" << std::endl;
        return;
    }
}

/**
 * Executes any cleanup procedure if necessary
 */
void ECallApp::cleanup() {
    std::cout << "Exiting the application.." << std::endl;
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

/**
 * Function to configure MSD transmission at call connect
 */
telux::common::Status ECallApp::getMsdTransmissionConfig(bool &transmitMsd) {
    char delimiter = '\n';
    std::string temp;
    int opt = -1;
    // Get user input to transmit MSD or not
    std::cout << "Configure MSD transmission at MO call connect:\n"
              << "1) Transmit MSD on call connect \n"
              << "2) Do not transmit MSD on call connect " << std::endl;
    std::getline(std::cin, temp, delimiter);
    if(!temp.empty()) {
        try {
            opt = std::stoi(temp);
        } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << opt << std::endl;
        }
    } else {
        std::cout << "No input, proceeding with MSD transmission " << std::endl;
        opt = ECALL_TRANSMIT_MSD;
    }
    if(opt == ECALL_TRANSMIT_MSD) {  // Transmit MSD
        transmitMsd = true;
    } else if(opt == ECALL_DO_NOT_TRANSMIT_MSD) {  // Do not transmit MSD
        transmitMsd = false;
    } else {
        std::cout << "Invalid MSD transmission configuration" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Utility function to get user input for an unsigned integer value
 */
telux::common::Status ECallApp::getIntegerInput(uint32_t &value, std::string prompt,
    std::vector<uint32_t> validValues) {
    char delimiter = '\n';
    std::string temp;
    uint32_t opt = 0;
    do {
        std::cout << prompt ;
        std::getline(std::cin, temp, delimiter);
        if(!temp.empty()) {
            try {
                opt = std::stoul(temp);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << opt
                    << std::endl;
            }
        } else {
            std::cout << " Invalid input, try again" << std::endl;
            continue;
        }
        if(validValues.size() > 0) {
            if(std::find(validValues.begin(), validValues.end(), opt) != validValues.end()) {
                value = opt;
                break;
            } else {
                std::cout << " Invalid input, try again" << std::endl;
            }
        } else {
            value = opt;
            break;
        }
    } while(1);
    return telux::common::Status::SUCCESS;
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
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "locclient"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    auto &eCallApp = ECallApp::getInstance();
    eCallApp.init();  // initialize commands and display
    return eCallApp.mainLoop();  // Main loop to continuously read and execute commands
}
