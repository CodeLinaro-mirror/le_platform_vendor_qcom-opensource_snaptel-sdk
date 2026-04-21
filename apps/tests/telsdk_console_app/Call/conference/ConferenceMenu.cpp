/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <iostream>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include "ConferenceMenu.hpp"

using namespace std;

ConferenceMenu::ConferenceMenu(std::string id, std::string name)
    : CallMenu("Conference Call Menu", "conference> ") {
    menuOptionsAdded_ = false;
}

ConferenceMenu::~ConferenceMenu() {
}

bool ConferenceMenu::init() {

auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    std::promise<ServiceStatus> callMgrprom;
    //  Get the PhoneFactory and CallManager instances.
    callManager_ = phoneFactory.getCallManager([&](ServiceStatus status) {
       callMgrprom.set_value(status);
    });
    if(!callManager_) {
       std::cout << "ERROR - Failed to get CallManager instance \n";
       return false;
    }
    std::cout << "CallManager subsystem is not ready " << ", Please wait " << std::endl;
    ServiceStatus callMgrsubSystemStatus = callMgrprom.get_future().get();

    if(callMgrsubSystemStatus == ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "CallManager subsystem is ready \n";
       myDialCallCmdCb_  = std::make_shared<MyDialCallback>();
       myHoldCb_ = std::make_shared<MyCallCommandCallback>("Hold");
       myResumeCb_ = std::make_shared<MyCallCommandCallback>("Resume");
       myHangupCb_ = std::make_shared<MyCallCommandCallback>("Hang");
       callListener_ = std::make_shared<MyCallListener>();
    } else {
       std::cout << "Unable to initialise CallManager subsystem " << std::endl;
       return false;
    }
    if (menuOptionsAdded_ == false) {

        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> add
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
                "1", "Add_Participant", {"number"},
                std::bind(&ConferenceMenu::dial, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> remove
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
                "2", "Remove_Participant", {"index"},
                std::bind(&ConferenceMenu::hangupWithCallIndex, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> list
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "List_Participant", {},
                std::bind(&ConferenceMenu::listCalls, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> merge
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Merge", {},
                std::bind(&ConferenceMenu::conference, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> hold
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Hold", {},
                std::bind(&ConferenceMenu::holdCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> resume
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Resume", {},
                std::bind(&ConferenceMenu::resumeCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> hangup
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Hangup", {},
                std::bind(&ConferenceMenu::hangup, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {add, remove, list, merge,
                hold, resume, hangup};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

int ConferenceMenu::getInputPhoneId() {
    int phoneId = DEFAULT_PHONE_ID;

    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        std::string slotSelection;
        char delimiter = '\n';

        std::cout << "Enter the desired Phone ID / SIM slot: ";
        std::getline(std::cin, slotSelection, delimiter);

        if (!slotSelection.empty()) {
            try {
                phoneId = std::stoi(slotSelection);
                if (phoneId < MIN_SIM_SLOT_COUNT || phoneId > MAX_SIM_SLOT_COUNT ) {
                    std::cout << "ERROR: Invalid slot entered" << std::endl;
                    return INVALID_PHONE_ID;
                }
            } catch (const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
                    << slotSelection << std::endl;
                return INVALID_PHONE_ID;
            }
        } else {
            std::cout << "Empty input, enter the correct slot" << std::endl;
            return INVALID_PHONE_ID;
        }
    }
    return phoneId;
}

void ConferenceMenu::listCalls(std::vector<std::string> userInput) {
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callManager_->getInProgressCalls();
    int phoneId = getInputPhoneId();

    if(phoneId == INVALID_PHONE_ID) {
        return;
    }
    for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
        ++callIterator) {
        if((*callIterator)->getPhoneId() == phoneId && (*callIterator)->isMultiPartyCall()) {
            spCall = *callIterator;
            std::cout << "The conference call ID is " << spCall->getCallIndex()
                    << ", state is " << (std::dynamic_pointer_cast<MyCallListener>(callListener_))
                      ->getCallStateString(spCall->getCallState()) << std::endl;
        }
    }
    if(spCall == nullptr) {
        std::cout << "No conference call found" << std::endl;
    }
}

void ConferenceMenu::holdCall(std::vector<std::string> userInput) {
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callManager_->getInProgressCalls();
    int phoneId = getInputPhoneId();

    if(phoneId == INVALID_PHONE_ID) {
        return;
    }
    for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
        ++callIterator) {
        if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ACTIVE
            && (*callIterator)->getPhoneId() == phoneId && (*callIterator)->isMultiPartyCall()) {
                spCall = *callIterator;
                break;
        }
    }
    if(spCall) {
        static std::shared_ptr<AudioClient> audioClient = AudioClient::getInstance();
        if (audioClient->isReady()) {
            // Ask the user for the mute functionality.
            if (queryMuteState(MUTE)) {
                audioClient->setMuteStatus(static_cast<SlotId>(phoneId), MUTE);
            }
        }
        spCall->hold(myHoldCb_);
    } else {
        std::cout << "No active call found" << std::endl;
    }
}

void ConferenceMenu::resumeCall(std::vector<std::string> userInput) {
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callManager_->getInProgressCalls();
    int phoneId = getInputPhoneId();

    if(phoneId == INVALID_PHONE_ID) {
        return;
    }
    // Iterate through the call list in the application and resume the
    // call which is on hold
    for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
        ++callIterator) {
        if ((*callIterator)->getPhoneId() == phoneId && (*callIterator)->isMultiPartyCall()
            && (*callIterator)->getCallState() == telux::tel::CallState::CALL_ON_HOLD) {
            spCall = *callIterator;
            break;
        }
    }
    if(spCall) {
        static std::shared_ptr<AudioClient> audioClient = AudioClient::getInstance();
        if (audioClient->isReady()) {
            // Ask the user for the mute functionality.
            if (queryMuteState(UNMUTE)) {
                audioClient->setMuteStatus(static_cast<SlotId>(phoneId), UNMUTE);
            }
        }
        spCall->resume(myResumeCb_);
    } else {
        std::cout << "No call to resume which is on hold " << std::endl;
    }
}

void ConferenceMenu::hangup(std::vector<std::string> userInput) {
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;

    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callManager_->getInProgressCalls();
    int phoneId = getInputPhoneId();

    if(phoneId == INVALID_PHONE_ID) {
        return;
    }
    for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
        ++callIterator) {
        if ((*callIterator)->getPhoneId() == phoneId && (*callIterator)->isMultiPartyCall()) {
            spCall = *callIterator;
            break;
        }
    }
    if(spCall) {
        if(spCall->getCallState() == telux::tel::CallState::CALL_ACTIVE) {
            callManager_->hangupForegroundResumeBackground(phoneId,
                MyHangupCallback::hangupFgResumeBgResponse);
        } else if(spCall->getCallState() == telux::tel::CallState::CALL_ON_HOLD){
            callManager_->hangupWaitingOrBackground(phoneId,
                MyHangupCallback::hangupWaitingOrBgResponse);
        }
    } else {
        std::cout << "No call found\n";
    }
}
