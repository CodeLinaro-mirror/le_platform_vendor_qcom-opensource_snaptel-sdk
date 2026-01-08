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
    // Exit emergency mode before closing the application
    auto &aecsMgr = AecsCallManager::getInstance();
    // Only disable emergency mode if phoneId_ was actually set
    if (phoneId_.load() != INVALID_PHONE_ID) {
        aecsMgr.setEmergencyMode(phoneId_, false, false);
    }
    aecsMgr.stopAudioAll();

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
    smsListener_ = nullptr;
}

bool AecsCall::init() {

    // First, init the central manager (creates callMgr_ + smsMgrs_ + audio)
    auto &aecsMgr = AecsCallManager::getInstance();
    if (!aecsMgr.init()) {
        std::cout << "Failed to initialize AecsCallManager" << std::endl;
        return false;
    }

    // ---- Use CallManager from AecsCallManager ----
    callMgr_ = aecsMgr.getCallManager();
    if (!callMgr_) {
        std::cout << "ERROR - CallManager instance not available from AecsCallManager\n";
        return false;
    }

    // CallManager is ready at this point (AecsCallManager already waited).

    aecsHangupCb_ = std::make_shared<AecsCallCommandCallback>("Hang");
    aecsAnswerCb_ = std::make_shared<AecsCallCommandCallback>("Answer");
    aecsRejectCb_ = std::make_shared<AecsCallCommandCallback>("Reject");
    callListener_ = std::make_shared<AecsCallListener>();

    telux::common::Status status = callMgr_->registerListener(callListener_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Unable to register Call Manager listener" << std::endl;
        return false;
    }

    // ---- Use SmsManagers from AecsCallManager ----
    smsListener_ = std::make_shared<AecsSmsListener>();

    // Fill local smsMgrs_ for menu functions, but from central manager:
    smsMgrs_.clear();
    for (const auto &entry : aecsMgr.getAllSmsManagers()) {
        int slotId              = entry.first;
        auto mgr                = entry.second;
        telux::common::Status s = mgr->registerListener(smsListener_);
        if (s != telux::common::Status::SUCCESS) {
            std::cout << "ERROR - Failed to register SMS listener on slot " << slotId << "\n";
            return false;
        }
        // we still keep a per-slot vector as your existing code assumes phoneId-1 index
        smsMgrs_.push_back(mgr);
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
        std::shared_ptr<ConsoleAppCommand> setEmergencyModeCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Set_Emergency_Mode", {},
                std::bind(&AecsCall::setEmergencyMode, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
            = {dialAecsCommand, acceptCallCommand, rejectCallCommand, sendAecsMessageCommand,
                hangupCommand, getCallsCommand, setEmergencyModeCommand};
        addCommands(commandsList);
    }

    ConsoleApp::displayMenu();
    return true;
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
    auto &aecsMgr = AecsCallManager::getInstance();
    std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
        = callMgr_->getInProgressCalls();
    if (!aecsMgr.isEmergencyMode(phoneId)) {
        // 1.1 Check if any ongoing non-AECS call, if found disconnect.
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
        // 1.2 enable emergency mode
        std::cout << "Enter 7 to enable emergency mode" << std::endl;
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
        if (opt != 7) {
            return;
        } else {
            std::vector<std::string> args;  // menu 7 takes no explicit args
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
    telux::common::Status makeCallStatus
        = callMgr_->makeAecsCall(phoneId, phoneNumber, AecsDialCallback::makeCallResponse);
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
            std::cout << "Enter 7 to exit emergency mode" << std::endl;
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
        if ((call->getCallState() != telux::tel::CallState::CALL_ENDED
                && call->getCallState() != telux::tel::CallState::CALL_INCOMING
                && call->getCallState() != telux::tel::CallState::CALL_WAITING)
            && aecsMgr.isEmergencyMode(call->getPhoneId())) {
            std::cout << "AECS call ongoing\n";
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
        isIncomingAecs = false;
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
        // Step 2: Answer AECS call
        // 2.1 Check if any ongoing non-AECS calls on same phoneId, if found disconnect.
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
                        return;
                    } else {
                        std::cout << " Ongoing non-AECS call hanged up successfully" << std::endl;
                    }
                }
            }
        }

        // 2.2 Enter emergency mode
        phoneId_ = incomingCallPhoneId;
        if (!aecsMgr.isEmergencyMode(incomingCallPhoneId)) {
            std::cout << std::endl << std::endl;
            std::cout << "Enter 7 to enable emergency mode" << std::endl;
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
            if (opt != 7) {
                return;
            } else {
                std::vector<std::string> args;
                setEmergencyMode(args);
            }
        }
        // Step 2.3: Answer AECS call
        // start audio
        aecsMgr.startAudio(incomingCallPhoneId);
        status = incomingCall->answer(aecsAnswerCb_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << " Failed to answer AECS call" << std::endl;
            aecsMgr.stopAudioAll();
            if (aecsMgr.isEmergencyMode(incomingCallPhoneId)) {
                std::cout << std::endl << std::endl;
                std::cout << "Enter 7 to exit emergency mode" << std::endl;
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
        if (aecsMgr.isEmergencyMode(incomingCallPhoneId)) {
            status = aecsMgr.setEmergencyMode(incomingCallPhoneId, false, false);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Failed to set emergency mode" << std::endl;
                return;
            }
        }
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
    phoneId_ = phoneId;
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
        std::cout << "Enter 7 to enable emergency mode" << std::endl;
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
        if (opt != 7) {
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
        std::cout << "Enter AECS encoded message(raw PDU): ";
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

    aecsMgr.setRetryRawPdu(rawPdus);
    std::weak_ptr<telux::tel::ISmsListener> weakBaseListener(smsListener_);
    auto cb
        = [weakBaseListener, phoneId](std::vector<int> msgRefs, telux::common::ErrorCode error) {
              if (error == telux::common::ErrorCode::SUCCESS) {
                  std::cout << "\n\nCallback: sendSmsResponse successfully\n";
                  std::cout << "Callback:  MsgRefs Size: " << msgRefs.size() << "\n";
                  for (int i : msgRefs) {
                      std::cout << "Callback:  MsgRef : " << i << "\n";
                  }
              } else {
                  std::cout << "\n\nCallback: sendSmsResponse failed, errorCode: " << (int)error
                            << ", description: " << Utils::getErrorCodeAsString(error) << "\n";
                  PRINT_NOTIFICATION << "AECS information: data transmission failed" << std::endl;
                  if (auto baseSp = weakBaseListener.lock()) {
                      if (auto sp = std::dynamic_pointer_cast<AecsSmsListener>(baseSp)) {
                          sp->onMsdAttemptFailed(phoneId);
                      } else {
                          std::cout << "Warning: SMS listener type mismatch, cannot trigger retry"
                                    << std::endl;
                      }
                  }
              }
          };
    telux::common::Status status = smsMgr->sendRawSms(rawPdus, cb);
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
                std::cout << "Enter 7 to exit emergency mode" << std::endl;
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
        return;
    }
}
