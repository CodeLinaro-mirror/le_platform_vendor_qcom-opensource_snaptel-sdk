/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>

#include "Utils.hpp"
#include "AecsCall.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
// Specific to DSDA, in case of two simultaneous incoming calls in accept,reject scenario
#define NO_OF_SIMULTANEOUS_INCOMING_CALL 2

AecsCall::AecsCall(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    menuOptionsAdded_ = false;
}

AecsCall::~AecsCall() {
    aecsHangupCb_ = nullptr;
    aecsAnswerCb_ = nullptr;
    aecsRejectCb_ = nullptr;
    if (callMgr_ && callListener_) {
        callMgr_->removeListener(callListener_);
    }
    if (callListener_) {
        callListener_ = nullptr;
    }
    for (unsigned int index = 0; index < smsMgrs_.size(); index++) {
        smsMgrs_[index]->removeListener(smsListener_);
    }
    aecsSmsCmdCb_      = nullptr;
    smsListener_       = nullptr;
    aecsSmsDeliveryCb_ = nullptr;
}

bool AecsCall::init() {

    int noOfSlots = MIN_SIM_SLOT_COUNT;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        noOfSlots = MAX_SIM_SLOT_COUNT;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    auto &phoneFactory           = telux::tel::PhoneFactory::getInstance();
    std::promise<telux::common::ServiceStatus> callMgrprom;

    // Get the PhoneFactory and CallManager instances.
    callMgr_ = phoneFactory.getCallManager(
        [&](telux::common::ServiceStatus status) { callMgrprom.set_value(status); });
    if (!callMgr_) {
        std::cout << "ERROR - Failed to get CallManager instance \n";
        return false;
    }
    std::cout << "CallManager subsystem is not ready "
              << ", Please wait " << std::endl;
    telux::common::ServiceStatus callMgrStatus = callMgrprom.get_future().get();

    if (callMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "CallManager subsystem is ready \n";
        aecsHangupCb_ = std::make_shared<AecsCallCommandCallback>("Hang");
        aecsAnswerCb_ = std::make_shared<AecsCallCommandCallback>("Answer");
        aecsRejectCb_ = std::make_shared<AecsCallCommandCallback>("Reject");
        callListener_ = std::make_shared<AecsCallListener>();
        // registering listener
        status = callMgr_->registerListener(callListener_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Unable to register Call Manager listener" << std::endl;
            return false;
        }
    } else {
        std::cout << "Unable to initialise CallManager subsystem " << std::endl;
        return false;
    }

    // initialize SMS Manager
    aecsSmsCmdCb_      = std::make_shared<AecsSmsCommandCallback>();
    aecsSmsDeliveryCb_ = std::make_shared<AecsSmsDeliveryCallback>();
    smsListener_       = std::make_shared<AecsSmsListener>();

    for (auto index = 1; index <= noOfSlots; index++) {
        std::promise<telux::common::ServiceStatus> prom;
        auto smsMgr = phoneFactory.getSmsManager(
            index, [&](telux::common::ServiceStatus status) { prom.set_value(status); });

        if (!smsMgr) {
            std::cout << "ERROR - Failed to get SMS Manager instance \n";
            return false;
        }

        std::cout << " Waiting for SMS Manager to be ready \n";
        telux::common::ServiceStatus smsMgrStatus = prom.get_future().get();
        if (smsMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "SMS Manager is ready \n";
            status = smsMgr->registerListener(smsListener_);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "ERROR - Failed to register listener \n";
                return false;
            }
            smsMgrs_.emplace_back(smsMgr);
        } else {
            std::cout << "ERROR - Unable to initialize SMS Manager \n";
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> dialAecsCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Dial_AECS",
                {"number/URN"}, std::bind(&AecsCall::dialAecsCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> acceptCallCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Accept_call", {},
                std::bind(&AecsCall::acceptCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> rejectCallCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Reject_call", {},
                std::bind(&AecsCall::rejectCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> sendAecsMessageCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Send_AECS_Message", {},
                std::bind(&AecsCall::sendAecsMessage, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> hangupCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
                "5", "Hangup", {}, std::bind(&AecsCall::hangup, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getCallsCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Get_InProgress_Calls", {},
                std::bind(&AecsCall::getAllCalls, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> retryDroppedAecsCallCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Retry_Dropped_AECS_call",
                {}, std::bind(&AecsCall::retryDroppedAecsCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> retryFailedAecsCallCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Retry_Failed_AECS_call",
                {}, std::bind(&AecsCall::retryFailedAecsCall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> retryMsdSmsCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Retry_MSD_over_SMS", {},
                std::bind(&AecsCall::retryMsdOverSms, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setEmergencyModeCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Set_Emergency_Mode", {},
                std::bind(&AecsCall::setEmergencyMode, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
            = {dialAecsCommand, acceptCallCommand, rejectCallCommand, sendAecsMessageCommand,
                hangupCommand, getCallsCommand, retryDroppedAecsCallCommand,
                retryFailedAecsCallCommand, retryMsdSmsCommand, setEmergencyModeCommand};
        addCommands(commandsList);
    }

    if (!AecsCallManager::getInstance().init()) {
        std::cout << "Failed to initialize AecsCallManager" << std::endl;
        return false;
    }
    ConsoleApp::displayMenu();
    return true;
}

void AecsCall::makeCallResponse(
    telux::common::ErrorCode error, std::shared_ptr<telux::tel::ICall> call) {
    std::cout << std::endl << std::endl;
    std::cout << "makeAecsCall response ErrorCode: " << int(error)
              << ", description: " << Utils::getErrorCodeAsString(error)
              << ", slot id: " << call->getPhoneId() << std::endl;
}

/**
 * Dial AECS call
 */
void AecsCall::dialAecsCall(std::vector<std::string> userInput) {
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    int phoneId                               = getInputPhoneId();
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. dial AECS call failed." << std::endl;
        return;
    }

    telux::common::Status status  = telux::common::Status::FAILED;
    const std::string phoneNumber = userInput[1];

    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << " AECS information: AECS function being triggered" << std::endl;

    // Step1: Enter emergency mode
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        // 1.1 check if any ongoing calls on other phone before entering into
        // emergency mode and disconnect if any calls found
        std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
            = callMgr_->getInProgressCalls();
        for (auto &call : inProgressCalls) {
            if (call->getCallState() != telux::tel::CallState::CALL_ENDED
                && call->getPhoneId() != phoneId) {
                spCall = call;
                if (spCall) {
                    status = spCall->hangup(aecsHangupCb_);
                    if (status != telux::common::Status::SUCCESS) {
                        std::cout << " Failed to hangup ongoing call" << std::endl;
                    } else {
                        std::cout << " Ongoing call hanged up successfully" << std::endl;
                    }
                }
            }
        }
    }

    // 1.2 Enter emergency mode
    auto &aecsMgr = AecsCallManager::getInstance();
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callMgr_->getInProgressCalls();
    if (!aecsMgr.isEmergencyMode(phoneId)) {
        // 1.2.1 Check if any ongoing non-AECS call, if found disconnect.
        for (auto &call : inProgressCalls) {
            if (call->getCallState() != telux::tel::CallState::CALL_ENDED
                && call->getPhoneId() == phoneId) {
                spCall = call;
                if (spCall) {
                    status = spCall->hangup(aecsHangupCb_);
                    if (status != telux::common::Status::SUCCESS) {
                        std::cout << " Failed to hangup ongoing non-AECS call" << std::endl;
                    } else {
                        std::cout << " Ongoing non-AECS call hanged up successfully" << std::endl;
                    }
                }
            }
        }
        std::cout << std::endl << std::endl;
        std::cout << "Enter 10 to enable emergency mode" << std::endl;
        int opt          = -1;
        char delimiter   = '\n';
        std::string temp = "";
        std::getline(std::cin, temp, delimiter);
        if (!temp.empty()) {
            try {
                opt = std::stoi(temp);
            } catch (const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << opt
                          << std::endl;
            }
        }
        if (opt != 10) {
            return;
        } else {
            std::vector<std::string> args;  // menu 10 takes no explicit args
            setEmergencyMode(args);
        }
    } else {
        for (auto &call : inProgressCalls) {
            if (call->getCallState() != telux::tel::CallState::CALL_ENDED
                && call->getPhoneId() == phoneId) {
                std::cout << " AECS call already ongoing... can't make another AECS call"
                          << std::endl;
                return;
            }
        }
    }
    // start audio
    aecsMgr.startAudio(phoneId);

    // Step 2: Make AECS call
    telux::common::Status makeCallStatus = callMgr_->makeAecsCall(phoneId, phoneNumber,
        std::bind(&AecsCall::makeCallResponse, this, std::placeholders::_1, std::placeholders::_2));
    if (makeCallStatus == telux::common::Status::SUCCESS) {
        std::cout << "makeAecsCall is successful.\n";
        std::cout << std::endl << std::endl;
        PRINT_NOTIFICATION << " AECS information: -voice connection establishment in progress"
                           << std::endl;
    } else {
        std::cout << "makeAecsCall failed.\n";
        aecsMgr.stopAudioAll();
        if (aecsMgr.isEmergencyMode(phoneId)) {
            std::cout << std::endl << std::endl;
            std::cout << "Enter 10 to exit emergency mode" << std::endl;
        }
    }
}

/**
 * Accept (answer) incoming call with AECS rules
 *
 * Use case:
 *  - If incoming call is NON-AECS and any ongoing AECS call exists -> reject incoming call.
 *  - If incoming is AECS -> enter emergency mode, clear conflicting calls, then answer.
 */
void AecsCall::acceptCall(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    // Snapshot of current calls
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callMgr_->getInProgressCalls();

    // ------------------------------------------------------------------
    // If any ongoing AECS call, reject this incoming call.
    // ------------------------------------------------------------------
    bool aecsOngoing = false;
    auto &aecsMgr    = AecsCallManager::getInstance();

    for (auto &call : inProgressCalls) {
        if (call->getCallState() != telux::tel::CallState::CALL_ENDED
            && aecsMgr.isEmergencyMode(call->getPhoneId())) {
            aecsOngoing = true;
            break;
        }
    }

    std::shared_ptr<telux::tel::ICall> incomingCall = nullptr;
    int incomingCallPhoneId                         = DEFAULT_PHONE_ID;
    bool isIncomingAecs                             = false;

    // Step 1: Find the incoming/waiting call and determine if it is AECS
    for (auto &call : inProgressCalls) {
        auto state = call->getCallState();
        if (state == telux::tel::CallState::CALL_INCOMING
            || state == telux::tel::CallState::CALL_WAITING) {
            incomingCall        = call;
            incomingCallPhoneId = call->getPhoneId();
            if (AECS_CALL(call->getRemotePartyNumber())) {
                isIncomingAecs = true;
            } else {
                isIncomingAecs = false;
            }
            break;  // handle first incoming/waiting call
        }
    }
    // TODO: This will be updated once we have data for AECS number or URN
    std::string incomingAecsCall;
    char delimiter = '\n';
    std::cout << std::endl << std::endl;
    std::cout << "Could you confirm is this AECS call (y/n): ";
    std::getline(std::cin, incomingAecsCall, delimiter);
    if (incomingAecsCall == "y" && aecsOngoing) {
        std::cout << "This use case is not feasible. If an AECS call is already in progress,"
                  << " it will not be possible to receive another incoming AECS call." << std::endl;
        status = incomingCall->reject(aecsRejectCb_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << " Failed to reject incoming call" << std::endl;
        } else {
            std::cout << " Incoming call rejected as AECS call is ongoing" << std::endl;
        }
        return;
    }
    if (incomingAecsCall == "y") {
        isIncomingAecs = true;
    } else {
        std::cout << " Proceed with non-AECS call" << std::endl;
    }

    if (!incomingCall) {
        std::cout << "No incoming/waiting call found" << std::endl;
        return;
    }

    // ------------------------------------------------------------------
    // CASE A: Incoming call is AECS
    // ------------------------------------------------------------------
    if (isIncomingAecs) {
        // 1.1 If AECS call, enter into emergency mode (multi-sim handling)
        if (telux::common::DeviceConfig::isMultiSimSupported()) {
            // Disconnect any ongoing calls on other subs before emergency mode
            for (auto &call : inProgressCalls) {
                if (call->getCallState() != telux::tel::CallState::CALL_ENDED
                    && call->getPhoneId() != incomingCallPhoneId) {
                    auto spCall = call;
                    status      = spCall->hangup(aecsHangupCb_);
                    if (status != telux::common::Status::SUCCESS) {
                        std::cout << " Failed to hangup ongoing call" << std::endl;
                    } else {
                        std::cout << " Ongoing call hanged up successfully" << std::endl;
                    }
                }
            }
        }

        // Step 2: Answer AECS call
        // 2.1 Check if any ongoing non-AECS calls on same phoneId, if found disconnect.
        inProgressCalls = callMgr_->getInProgressCalls();
        for (auto &call : inProgressCalls) {
            auto state = call->getCallState();
            if ((state != telux::tel::CallState::CALL_ENDED
                    && state != telux::tel::CallState::CALL_INCOMING
                    && state != telux::tel::CallState::CALL_WAITING)
                && call->getPhoneId() == incomingCallPhoneId) {
                if (!aecsMgr.isEmergencyMode(call->getPhoneId())) {
                    status = call->hangup(aecsHangupCb_);
                    if (status != telux::common::Status::SUCCESS) {
                        std::cout << " Failed to hangup ongoing non-AECS call" << std::endl;
                    } else {
                        std::cout << " Ongoing non-AECS call hanged up successfully" << std::endl;
                    }
                }
            }
        }

        // 1.1.2 Enter emergency mode
        if (!aecsMgr.isEmergencyMode(incomingCallPhoneId)) {
            std::cout << std::endl << std::endl;
            std::cout << "Enter 10 to enable emergency mode" << std::endl;
            int opt          = -1;
            char delimiter   = '\n';
            std::string temp = "";
            std::getline(std::cin, temp, delimiter);
            if (!temp.empty()) {
                try {
                    opt = std::stoi(temp);
                } catch (const std::exception &e) {
                    std::cout << "ERROR: invalid input, please enter numerical values " << opt
                              << std::endl;
                }
            }
            if (opt != 10) {
                return;
            } else {
                std::vector<std::string> args;
                setEmergencyMode(args);
            }
        }
        // Step 2: Answer AECS call
        // start audio
        aecsMgr.startAudio(incomingCallPhoneId);
        status = incomingCall->answer(aecsAnswerCb_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << " Failed to answer AECS call" << std::endl;
            aecsMgr.stopAudioAll();
            if (aecsMgr.isEmergencyMode(incomingCallPhoneId)) {
                std::cout << std::endl << std::endl;
                std::cout << "Enter 10 to exit emergency mode" << std::endl;
            }
        } else {
            std::cout << std::endl << std::endl;
            PRINT_NOTIFICATION << " AECS information: -voice connection established,"
                               << " voice communication in progress" << std::endl;
        }
        return;
    }

    // ------------------------------------------------------------------
    // CASE B: Incoming call is NON-AECS
    // If any ongoing AECS call, reject this incoming call.
    // ------------------------------------------------------------------

    if (aecsOngoing) {
        // B.1 AECS call ongoing -> reject incoming non-AECS call
        status = incomingCall->reject(aecsRejectCb_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << " Failed to reject incoming non-AECS call" << std::endl;
        } else {
            std::cout << " Incoming non-AECS call rejected as AECS call is ongoing" << std::endl;
        }
    } else {
        // B.2 No AECS call in progress -> just answer normal call
        status = incomingCall->answer(aecsAnswerCb_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << " Failed to answer incoming call" << std::endl;
        } else {
            std::cout << "Voice call is in progress..." << std::endl;
        }
    }
}

/**
 * Reject incoming/waiting call (unchanged logic, just formatting)
 */
void AecsCall::rejectCall(std::vector<std::string> userInput) {
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callMgr_->getInProgressCalls();

    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        // Count incoming/waiting calls
        int incomingCalls = 0;
        for (auto &call : inProgressCalls) {
            if (call->getCallState() == telux::tel::CallState::CALL_INCOMING
                || call->getCallState() == telux::tel::CallState::CALL_WAITING) {
                ++incomingCalls;
            }
        }

        // If two simultaneous incoming calls, user selects slotId to reject
        int phoneId = getInputPhoneId();
        if (phoneId == INVALID_PHONE_ID) {
            std::cout << "Invalid Phone ID. Reject aborted." << std::endl;
            return;
        }

        if (incomingCalls >= NO_OF_SIMULTANEOUS_INCOMING_CALL) {
            for (auto &call : inProgressCalls) {
                if (call->getPhoneId() == phoneId
                    && (call->getCallState() == telux::tel::CallState::CALL_INCOMING
                        || call->getCallState() == telux::tel::CallState::CALL_WAITING)) {
                    spCall = call;
                    break;
                }
            }
        }
    }

    if (!spCall) {
        // Fallback: reject first incoming/waiting call
        for (auto &call : inProgressCalls) {
            if (call->getCallState() == telux::tel::CallState::CALL_INCOMING
                || call->getCallState() == telux::tel::CallState::CALL_WAITING) {
                spCall = call;
                break;
            }
        }
    }

    if (spCall) {
        spCall->reject(aecsRejectCb_);
    } else {
        std::cout << "No incoming/waiting call" << std::endl;
    }
}

/**
 * Get PhoneId from user (multi-SIM helper)
 */
int AecsCall::getInputPhoneId() {
    int phoneId = DEFAULT_PHONE_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        std::string slotSelection;
        char delimiter = '\n';

        std::cout << "Enter the desired Phone ID / SIM slot: ";
        std::getline(std::cin, slotSelection, delimiter);

        if (!slotSelection.empty()) {
            try {
                phoneId = std::stoi(slotSelection);
                if (phoneId < MIN_SIM_SLOT_COUNT || phoneId > MAX_SIM_SLOT_COUNT) {
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

/**
 * Send AECS-encoded MSD over SMS (one-shot)
 */
void AecsCall::sendAecsMessage(std::vector<std::string> userInput) {
    int phoneId = getInputPhoneId();
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. sendAecsMessage aborted." << std::endl;
        return;
    }
    auto &aecsMgr = AecsCallManager::getInstance();
    if (!aecsMgr.isEmergencyMode(phoneId)) {
        std::cout << std::endl << std::endl;
        std::cout << "Enter 10 to enable emergency mode" << std::endl;
        int opt          = -1;
        char delimiter   = '\n';
        std::string temp = "";
        std::getline(std::cin, temp, delimiter);
        if (!temp.empty()) {
            try {
                opt = std::stoi(temp);
            } catch (const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << opt
                          << std::endl;
            }
        }
        if (opt != 10) {
            return;
        } else {
            std::vector<std::string> args;
            setEmergencyMode(args);
        }
    }
    auto smsMgr    = smsMgrs_[phoneId - 1];
    char delimiter = '\n';

    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << " AECS information: data transmission in progress" << std::endl;

    std::string needMorePdu;
    std::vector<telux::tel::PduBuffer> rawPdus;

    do {
        std::string message;
        std::cout << "Enter AECS encoded message: ";
        std::getline(std::cin, message, delimiter);
        if (message.empty()) {
            std::cout << " AECS encoded message(PDU) input is empty\n";
            return;
        }

        std::vector<uint8_t> buffer(message.begin(), message.end());
        rawPdus.emplace_back(buffer);

        std::cout << "Do you want to enter more messages (y/n): ";
        std::getline(std::cin, needMorePdu, delimiter);
        std::transform(needMorePdu.begin(), needMorePdu.end(), needMorePdu.begin(), ::tolower);
    } while (needMorePdu == "y");

    if (needMorePdu != "n") {
        std::cout << "Invalid input provided \n";
        return;
    }

    telux::common::Status status
        = smsMgr->sendRawSms(rawPdus, AecsSmsCommandCallback::sendSmsResponse);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "sendAecsMessage is successful.\n";
    } else if (status == telux::common::Status::INVALIDPARAM) {
        std::cout << "Send SMS request failed - Invalid input(s)\n";
    } else {
        std::cout << "sendAecsMessaage failed.\n";
    }
}

/**
 * Hang up active call(s) on chosen phoneId
 */
void AecsCall::hangup(std::vector<std::string> userInput) {
    std::shared_ptr<telux::tel::ICall> spCall = nullptr;
    int noOfExistingCalls                     = 0;

    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callMgr_->getInProgressCalls();
    int phoneId = getInputPhoneId();
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. hangup aborted." << std::endl;
        return;
    }

    for (auto &call : inProgressCalls) {
        if (call->getCallState() != telux::tel::CallState::CALL_ENDED
            && call->getPhoneId() == phoneId) {
            noOfExistingCalls++;
            spCall = call;
        }
    }

    if (noOfExistingCalls > 1) {
        std::cout << "More than one call: use Hangup cmd with Index " << std::endl;
        return;
    }

    if (spCall) {
        int phoneIdOfCall = spCall->getPhoneId();
        spCall->hangup(aecsHangupCb_);
        std::cout << std::endl << std::endl;
        PRINT_NOTIFICATION << " AECS information: voice communication completed" << std::endl;

        // Step 4: exit emergency mode if no more calls on this slot
        auto &mgr = AecsCallManager::getInstance();
        mgr.stopAudioIfNoCalls(phoneIdOfCall);
        if (!mgr.isEmergencyMode(phoneIdOfCall)) {
            // already off
        } else {
            // check if there are any active AECS calls – simplest: reuse stopAudioIfNoCalls logic:
            auto cm             = mgr.getCallManager();
            auto calls          = cm->getInProgressCalls();
            bool aecsOnThisSlot = false;
            for (auto &c : calls) {
                if (c->getPhoneId() == phoneIdOfCall
                    && c->getCallState() != telux::tel::CallState::CALL_ENDED) {
                    aecsOnThisSlot = true;
                    break;
                }
            }
            if (!aecsOnThisSlot) {
                std::cout << std::endl << std::endl;
                std::cout << "Enter 10 to exit emergency mode" << std::endl;
            }
        }
    } else {
        std::cout << "No dialing or alerting call found" << std::endl;
    }
}

/**
 * Print all in-progress calls
 */
void AecsCall::getAllCalls(std::vector<std::string> userInput) {
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callMgr_->getInProgressCalls();
    if (inProgressCalls.empty()) {
        std::cout << "No calls detected in the system" << std::endl;
        return;
    }

    for (auto &call : inProgressCalls) {
        std::cout << " Call State: "
                  << (std::dynamic_pointer_cast<AecsCallListener>(callListener_))
                         ->getCallStateString(call->getCallState())
                  << " Call Index: " << (int)call->getCallIndex()
                  << " Call Direction: " << (int)call->getCallDirection()
                  << " Phone Number: " << call->getRemotePartyNumber()
                  << " SlotId: " << call->getPhoneId() << " isMpty: " << call->isMultiPartyCall()
                  << std::endl;
    }
}

/**
 * Retry dropped AECS call (isAecsCallDrop == true)
 */
void AecsCall::retryDroppedAecsCall(std::vector<std::string> userInput) {
    int phoneId = getInputPhoneId();
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. Retry aborted." << std::endl;
        return;
    }

    // Check if AECS call was dropped
    bool droppedDetected = false;
    auto &mgr            = AecsCallManager::getInstance();
    droppedDetected      = mgr.getAecsCallDropStatus();

    if (!droppedDetected) {
        std::cout << "No dropped AECS call detected for retry." << std::endl;
        return;
    }
    // AECS identifier
    std::string phoneNumber;
    std::cout << "\nEnter AECS identifier (dial number or URN): ";
    std::getline(std::cin, phoneNumber);
    if (phoneNumber.empty()) {
        std::cerr << "AECS identifier required for retry.\n";
        return;
    }

    int interval     = DEFAULT_RETRY_INTERVAL_SEC;
    int duration     = DEFAULT_RETRY_DURATION_SEC;
    char delimiter   = '\n';
    std::string temp = "";
    std::cout << "Enter interval for call retry in seconds(optional): ";
    std::getline(std::cin, temp, delimiter);
    if (temp.empty()) {
        std::cout << "No input received for call interval, proceeding with default interval"
                  << std::endl;
    } else {
        try {
            interval = std::stoi(temp);
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << interval
                      << std::endl;
        }
    }
    std::cout << "Enter duration for call retry in seconds(optional): ";
    std::getline(std::cin, temp, delimiter);
    if (temp.empty()) {
        std::cout << "No input received for call interval, proceeding with default interval"
                  << std::endl;
    } else {
        try {
            duration = std::stoi(temp);
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << duration
                      << std::endl;
        }
    }

    auto startTime = std::chrono::steady_clock::now();
    std::cout << "Starting retry for dropped AECS call for up to 60 minutes..." << std::endl;

    while (true) {
        telux::common::Status status = callMgr_->makeAecsCall(phoneId, phoneNumber,
            std::bind(
                &AecsCall::makeCallResponse, this, std::placeholders::_1, std::placeholders::_2));
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Dropped AECS call retry initiated successfully." << std::endl;
            break;
        } else {
            std::cout << "Dropped AECS call retry failed. Will retry every 2 minutes..."
                      << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval));

        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >= duration) {
            std::cout << "Retry window expired (60 minutes). Stopping retries." << std::endl;
            break;
        }
    }
}

/**
 * Retry failed AECS call (RedialState::MODEM_RETRY_END)
 */
void AecsCall::retryFailedAecsCall(std::vector<std::string> userInput) {
    int phoneId = getInputPhoneId();
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. Retry aborted." << std::endl;
        return;
    }

    bool failureDetected = false;
    auto &mgr            = AecsCallManager::getInstance();
    failureDetected      = mgr.getAecsCallFailStatus();

    if (!failureDetected) {
        std::cout << "No AECS call failure detected for retry." << std::endl;
        return;
    }

    // AECS identifier
    std::string phoneNumber;
    std::cout << "\nEnter AECS identifier (dial number or URN): ";
    std::getline(std::cin, phoneNumber);
    if (phoneNumber.empty()) {
        std::cerr << "AECS identifier required for retry.\n";
        return;
    }

    auto startTime = std::chrono::steady_clock::now();
    std::cout << "Starting AECS call retry for up to 60 minutes..." << std::endl;

    while (true) {
        telux::common::Status status = callMgr_->makeAecsCall(phoneId, phoneNumber,
            std::bind(
                &AecsCall::makeCallResponse, this, std::placeholders::_1, std::placeholders::_2));
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "AECS call retry initiated successfully." << std::endl;
            break;
        } else {
            std::cout << "AECS call retry failed. Will retry every 2 minutes..." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(DEFAULT_RETRY_INTERVAL_SEC));

        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
            >= DEFAULT_RETRY_DURATION_SEC) {
            std::cout << "Retry window expired (60 minutes). Stopping retries." << std::endl;
            break;
        }
    }
}

/**
 * Retry MSD over SMS with retry window
 */
void AecsCall::retryMsdOverSms(std::vector<std::string> userInput) {
    int phoneId = getInputPhoneId();
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. Retry aborted." << std::endl;
        return;
    }
    auto &aecsMgr = AecsCallManager::getInstance();
    if (!aecsMgr.isEmergencyMode(phoneId)) {
        std::cout << std::endl << std::endl;
        std::cout << "Enter 10 to enable emergency mode" << std::endl;
        int opt          = -1;
        char delimiter   = '\n';
        std::string temp = "";
        std::getline(std::cin, temp, delimiter);
        if (!temp.empty()) {
            try {
                opt = std::stoi(temp);
            } catch (const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << opt
                          << std::endl;
            }
        }
        if (opt != 10) {
            return;
        } else {
            std::vector<std::string> args;
            setEmergencyMode(args);
        }
    }

    auto smsMgr    = smsMgrs_[phoneId - 1];
    char delimiter = '\n';

    PRINT_NOTIFICATION << " AECS information: data transmission in progress" << std::endl;

    std::string needMorePdu;
    std::vector<telux::tel::PduBuffer> rawPdus;

    do {
        std::string message;
        std::cout << "Enter AECS encoded message: ";
        std::getline(std::cin, message, delimiter);
        if (message.empty()) {
            std::cout << " AECS encoded message(PDU) input is empty\n";
            return;
        }

        std::vector<uint8_t> buffer(message.begin(), message.end());
        rawPdus.emplace_back(buffer);

        std::cout << "Do you want to enter more messages (y/n): ";
        std::getline(std::cin, needMorePdu, delimiter);
        std::transform(needMorePdu.begin(), needMorePdu.end(), needMorePdu.begin(), ::tolower);
    } while (needMorePdu == "y");

    if (needMorePdu != "n") {
        std::cout << "Invalid input provided \n";
        return;
    }

    auto startTime = std::chrono::steady_clock::now();
    std::cout << "Starting MSD over SMS retry for up to 60 minutes..." << std::endl;

    while (true) {
        telux::common::Status status
            = smsMgr->sendRawSms(rawPdus, AecsSmsCommandCallback::sendSmsResponse);
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "MSD over SMS retry initiated successfully." << std::endl;
            break;
        } else {
            std::cout << "MSD over SMS retry failed. Will retry every 2 minutes..." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(DEFAULT_RETRY_INTERVAL_SEC));

        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count()
            >= DEFAULT_RETRY_DURATION_SEC) {
            std::cout << "Retry window expired (60 minutes). Stopping retries." << std::endl;
            break;
        }
    }
}

void AecsCall::setEmergencyMode(std::vector<std::string> userInput) {
    int phoneId = getInputPhoneId();
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. Emergency mode configuration aborted." << std::endl;
        return;
    }

    // Get configuration from user
    char delimiter   = '\n';
    std::string temp = "";
    std::cout << "Enter emergency mode configuration (0-disable, 1- enable): ";
    std::getline(std::cin, temp, delimiter);
    uint32_t emergencyModeEnabled = 0;
    uint32_t antennaSwitchEnabled = 0;
    if (!temp.empty()) {
        try {
            emergencyModeEnabled = std::stoi(temp);
            if (emergencyModeEnabled != 0 && emergencyModeEnabled != 1) {
                std::cout << "ERROR: Invalid value. Please enter 0 (disable) or 1 (enable)"
                          << std::endl;
                return;
            }
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values, "
                      << emergencyModeEnabled << std::endl;
            return;
        }
    } else {
        std::cout << "No input" << std::endl;
        return;
    }
    std::cout << "Enter antenna switching configuration (0-disable, 1- enable): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        try {
            antennaSwitchEnabled = std::stoi(temp);
            if (antennaSwitchEnabled != 0 && antennaSwitchEnabled != 1) {
                std::cout << "ERROR: Invalid value. Please enter 0 (disable) or 1 (enable)"
                          << std::endl;
                return;
            }
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values, "
                      << antennaSwitchEnabled << std::endl;
            return;
        }
    } else {
        std::cout << "No input" << std::endl;
        return;
    }
    auto &aecsMgr                  = AecsCallManager::getInstance();
    telux::common::Status emStatus = aecsMgr.setEmergencyMode(
        phoneId, static_cast<bool>(emergencyModeEnabled), static_cast<bool>(antennaSwitchEnabled));
    if (emStatus != telux::common::Status::SUCCESS) {
        std::cout << "Failed to set emergency mode" << std::endl;
    }
}
