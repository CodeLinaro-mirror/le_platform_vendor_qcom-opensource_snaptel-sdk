/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 *  Sample application that demonstrates how to place an AECS call
 *  (Automotive Accident Emergency Call System) using the CallManager API.
 *
 *  Build steps (from the repository root):
 *
 *      mkdir -p build && cd build
 *      cmake .. -DBUILD_FOR_SIMULATION=OFF   # or ON if you only have the sim build
 *      make -C public/apps/samples/phone/make_aecs_call_app
 *
 *  Run:
 *
 *      ./make_aecs_call_app
 *
 *  Flow implemented as per requirements:
 *    1. Get CallManager via PhoneFactory with init callback.
 *    2. Wait for subsystem initialization.
 *    3. Register ICallListener for call status change notifications.
 *    4. Enter emergency mode (setEmergencyMode) before AECS call and MSD transmission.
 *       - Terminate ongoing calls on other phones (DSDS/DSDA) before EM mode.
 *    5. On selected phone:
 *       - Terminate any ongoing non-AECS calls before AECS.
 *       - If AECS already in progress, reuse existing call (no new AECS).
 *    6. Dial AECS via makeAecsCall with callback.
 *    7. Optionally handle asynchronous makeCallResponse.
 *    8. Both AECS call and MSD transmission can happen in parallel. After setting emergency mode,
 *       - Application can send MSD over IP/HTTP or SMS (sendRawSms).
 *       - Send MSD over SMS using ISmsManager::sendRawSms()
 *    9. On failure / drop:
 *       - Application responsible for redial/retry (per AECS spec 4.1.3).
 *   10. While AECS in progress:
 *       - Reject normal incoming calls.
 *   11. Hang up via hangup API (with optional callback).
 *   12. On call ended:
 *       - Exit emergency mode using setEmergencyMode.
 */

#include <iostream>
#include <memory>
#include <future>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <vector>
#include <limits>
#include <algorithm>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>

#include <telux/tel/CallManager.hpp>   // ICallManager / ICall / ICallListener
#include <telux/tel/SmsManager.hpp>    // ISmsManager / PduBuffer

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2

using namespace telux::tel;
using namespace telux::common;

// --------------------------------------------------------------
//  Global state
// --------------------------------------------------------------
// Retry configuration
static constexpr int DEFAULT_RETRY_INTERVAL_SEC = 120;   // 2 minutes
static constexpr int DEFAULT_RETRY_DURATION_SEC = 3600;  // 60 minutes

static std::atomic<bool> gAecsInProgress{false};
static std::atomic<bool> gCallEnded{false};
static std::mutex        gCallMutex;
static std::shared_ptr<ICall> gAecsCall = nullptr;
static std::atomic<bool> gMsdDone{false};
static std::atomic<bool> gCallFailRetryDone{false};
static std::atomic<bool> gCallDropRetryDone{false};
static std::thread gDropRetryThread;
static std::thread gFailRetryThread;
static std::thread gMsdRetryThread;
// Emergency-mode per phoneId
std::map<int,bool> emergencyMode_;   // true when enabled
// Add mutex for emergency mode map
static std::mutex gEmergencyModeMutex;

// CallManager and SmsManager for the AECS slot
std::shared_ptr<ICallManager> callMgr_;
std::vector<std::shared_ptr<ISmsManager>> smsMgrs_;

// --------------------------------------------------------------
//  MSD over SMS helpers
// --------------------------------------------------------------
class AecsSmsCommandCallback {
 public:
      void sendSmsResponse(std::vector<int> msgRefs, ErrorCode error) {
          std::cout << "\n[MSD SMS Response]\n"
              << "  ErrorCode : " << static_cast<int>(error) << "\n"
              << "  MsgRefs   : ";
          if (msgRefs.empty()) {
              std::cout << "(none)";
          } else {
              for (auto r : msgRefs) {
                  std::cout << r << " ";
              }
          }
          std::cout << "\n";
      }
};

// --------------------------------------------------------------
//  AECS MakeCall callback
// --------------------------------------------------------------
class AecsMakeCallCallback {
public:
    void makeCallResponse(ErrorCode error,
                          std::shared_ptr<ICall> call = nullptr) {
        std::cout << "\n[makeAecsCall Response]\n";
        std::cout << "  ErrorCode: " << static_cast<int>(error) << "\n";
        if (call) {
            std::cout << "  Call Index    : " << call->getCallIndex() << "\n";
            std::cout << "  PhoneId       : " << call->getPhoneId() << "\n";
            std::cout << "  Remote Party  : " << call->getRemotePartyNumber() << "\n";
            {
                std::lock_guard<std::mutex> lock(gCallMutex);
                gAecsCall = call;
            }
        } else {
            std::cout << "  AECS call origination failed (no call object)\n";
        }
    }
};

static bool isEmergencyMode(int phoneId) {
    std::lock_guard<std::mutex> lock(gEmergencyModeMutex);
    auto it = emergencyMode_.find(phoneId);
    if (it == emergencyMode_.end()) {
        return false;
    }
    return it->second;
}

/**
 * Retry dropped AECS call (isAecsCallDrop == true)
 */
void retryDroppedAecsCall(int phoneId) {
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. Retry aborted." << std::endl;
        return;
    }

    // AECS identifier
    std::string aecsId;
    std::cout << "\nEnter AECS identifier (dial number or URN): ";
    std::getline(std::cin, aecsId);
    if (aecsId.empty()) {
        std::cerr << "No AECS identifier entered – aborting.\n";
        gCallDropRetryDone.store(true);
        return;
    }

    // Check if AECS call was dropped
    if (!callMgr_) {
        std::cout << " CallManager is null" << std::endl;
        return;
    }
    std::vector<std::shared_ptr<ICall>> inProgressCalls = callMgr_->getInProgressCalls();
    bool droppedDetected = false;

    for (auto &call : inProgressCalls) {
        if (isEmergencyMode(call->getPhoneId()) &&
            call->getCallState() == CallState::CALL_ENDED &&
            call->isAecsCallDrop()) {
            droppedDetected = true;
            break;
        }
    }

    if (!droppedDetected) {
        std::cout << "No dropped AECS call detected for retry." << std::endl;
        return;
    }

    auto startTime = std::chrono::steady_clock::now();
    std::cout << "Starting retry for dropped AECS call for up to 60 minutes..."
              << std::endl;

    auto cbObj = std::make_shared<AecsMakeCallCallback>();
    while (true) {
        telux::common::Status status =
            callMgr_->makeAecsCall(phoneId, aecsId,
                std::bind(&AecsMakeCallCallback::makeCallResponse, cbObj, std::placeholders::_1,
                    std::placeholders::_2));
        if (status == Status::SUCCESS) {
            std::cout << "Dropped AECS call retry initiated successfully." << std::endl;
            break;
        } else {
            std::cout << "Dropped AECS call retry failed. Will retry every 2 minutes..."
                << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(DEFAULT_RETRY_INTERVAL_SEC));

        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >=
            DEFAULT_RETRY_DURATION_SEC) {
            std::cout << "Retry window expired (60 minutes). Stopping retries." << std::endl;
            break;
        }
    }
    gCallDropRetryDone.store(true);
}

/**
 * Retry failed AECS call (RedialState::MODEM_RETRY_END)
 */
void retryFailedAecsCall(int phoneId) {
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. Retry aborted." << std::endl;
        return;
    }

    // AECS identifier
    std::string aecsId;
    std::cout << "\nEnter AECS identifier (dial number or URN): ";
    std::getline(std::cin, aecsId);
    if (aecsId.empty()) {
        std::cerr << "No AECS identifier entered – aborting.\n";
        gCallFailRetryDone.store(true);
        return;
    }

    if (!callMgr_) {
        std::cout << " CallManager is null" << std::endl;
        return;
    }

    std::vector<std::shared_ptr<ICall>> inProgressCalls = callMgr_->getInProgressCalls();
    bool failureDetected = false;

    for (auto &call : inProgressCalls) {
        if (isEmergencyMode(call->getPhoneId()) &&
            call->getCallState() == CallState::CALL_ENDED &&
            call->getRedialState() == RedialState::MODEM_RETRY_END) {
            failureDetected = true;
            break;
        }
    }

    if (!failureDetected) {
        std::cout << "No AECS call failure detected for retry." << std::endl;
        return;
    }

    auto startTime = std::chrono::steady_clock::now();
    std::cout << "Starting AECS call retry for up to 60 minutes..." << std::endl;

    auto cbObj = std::make_shared<AecsMakeCallCallback>();
    while (true) {
        telux::common::Status status =
            callMgr_->makeAecsCall(phoneId, aecsId,
                std::bind(&AecsMakeCallCallback::makeCallResponse, cbObj, std::placeholders::_1,
                    std::placeholders::_2));
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "AECS call retry initiated successfully." << std::endl;
            break;
        } else {
            std::cout << "AECS call retry failed. Will retry every 2 minutes..." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(DEFAULT_RETRY_INTERVAL_SEC));

        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >=
            DEFAULT_RETRY_DURATION_SEC) {
            std::cout << "Retry window expired (60 minutes). Stopping retries." << std::endl;
            break;
        }
    }
    gCallFailRetryDone.store(true);
}

/**
 * Retry MSD over SMS with retry window
 */
void retryMsdOverSms(int phoneId) {
    if (phoneId == INVALID_PHONE_ID) {
        std::cout << "Invalid Phone ID. Retry aborted." << std::endl;
        return;
    }

    if (smsMgrs_.size() == 0) {
        std::cout << "SmsManager is null" << std::endl;
        return;
    }
    auto smsMgr = smsMgrs_[phoneId - 1];

    std::cout << " AECS information: data transmission in progress" << std::endl;

    std::string needMorePdu;
    std::vector<telux::tel::PduBuffer> rawPdus;

    std::string message;
    std::cout << "Enter MSD encoded PDU payload (will be used as raw bytes): ";
    std::getline(std::cin, message);
    if (message.empty()) {
        std::cout << "[MSD SMS] Empty MSD input – skipping send.\n";
        return;
    }

    std::vector<uint8_t> buffer(message.begin(), message.end());
    rawPdus.emplace_back(buffer);

    auto startTime = std::chrono::steady_clock::now();
    std::cout << "Starting MSD over SMS retry for up to 60 minutes..." << std::endl;
    auto cbObj = std::make_shared<AecsSmsCommandCallback>();
    while (true) {
        Status status = smsMgr->sendRawSms(rawPdus,
            std::bind(&AecsSmsCommandCallback::sendSmsResponse, cbObj, std::placeholders::_1,
                 std::placeholders::_2));
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "MSD over SMS retry initiated successfully." << std::endl;
            break;
        } else {
            std::cout << "MSD over SMS retry failed. Will retry every 2 minutes..." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(DEFAULT_RETRY_INTERVAL_SEC));

        auto elapsed = std::chrono::steady_clock::now() - startTime;
        if (std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() >=
            DEFAULT_RETRY_DURATION_SEC) {
            std::cout << "Retry window expired (60 minutes). Stopping retries." << std::endl;
            break;
        }
    }
}

// Non-blocking wrappers
void startDroppedAecsRetryAsync(int phoneId) {
    gCallDropRetryDone.store(false);
    if (gDropRetryThread.joinable()) {
        gDropRetryThread.join();   // clean up previous thread if any
    }
    gDropRetryThread = std::thread([phoneId]() {
        retryDroppedAecsCall(phoneId);
        gCallDropRetryDone.store(true);
    });
}

void startFailedAecsRetryAsync(int phoneId) {
    gCallFailRetryDone.store(false);
    if (gFailRetryThread.joinable()) {
        gFailRetryThread.join();
    }
    gFailRetryThread = std::thread([phoneId]() {
        retryFailedAecsCall(phoneId);
        gCallFailRetryDone.store(true);
    });
}

void startMsdRetryAsync(int phoneId) {
    gMsdDone.store(false);
    if (gMsdRetryThread.joinable()) {
        gMsdRetryThread.join();
    }
    gMsdRetryThread = std::thread([phoneId]() {
        retryMsdOverSms(phoneId);
        gMsdDone.store(true);
    });
}

// --------------------------------------------------------------
//  SMS delivery callback
// --------------------------------------------------------------
class AecsSmsDeliveryCallback : public ICommandResponseCallback {
public:
   void commandResponse(ErrorCode error) {
      if (error == ErrorCode::SUCCESS) {
          std::cout << "MSD Delivered successfully" << std::endl;
      } else {
          std::cout << "MSD Delivery failed, errorCode: " << (int)error
              << ", description: " << static_cast<int>(error) << std::endl;
      }
   }
};

// --------------------------------------------------------------
//  AECS SMS Listener
// --------------------------------------------------------------
class AecsSmsListener : public ISmsListener {
public:
   void onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> smsMsg) {
       std::cout << std::endl << std::endl;
       std::shared_ptr<MessagePartInfo> partInfo = smsMsg->getMessagePartInfo();
       if (partInfo) {
           std::cout << "Received SMS on phone ID " << phoneId << " from: "
               << smsMsg->getSender() <<  " to: " << smsMsg->getReceiver()
               << "\n Message: " << smsMsg->getText() << "\n PDU: " << smsMsg->getPdu()
               << " \n RefNumber:" << static_cast <int>(partInfo->refNumber)
               << " NumberOfSegments:" << static_cast <int>(partInfo->numberOfSegments)
               << " SegmentNumber: " << static_cast <int>(partInfo->segmentNumber)
               << std::endl;
       } else {
           std::cout << "Received SMS on phone ID " << phoneId << " from: "
               << smsMsg->getSender() <<  " to: " << smsMsg->getReceiver()
               << "\n Message: " << smsMsg->getText() << "\n PDU: " << smsMsg->getPdu()
               << std::endl;
       }
   }

   void onIncomingSms(int phoneId, std::shared_ptr<std::vector<SmsMessage>> msgs) {
       std::cout << std::endl;

       std::string text = "";
       std::vector<SmsMessage> messages = *(msgs.get());
       if (messages.size() > 1) {
           std::cout << " Consolidated Multipart Message: " << std::endl;
           std::cout << " Count :" << messages.size() << std::endl;
        } else {
           std::cout << " Message: " << std::endl;
           std::cout << " Count :" << messages.size() << std::endl;
       }
       for (SmsMessage smsMsg : messages) {
           text = text + smsMsg.getText();
           std::shared_ptr<MessagePartInfo> partInfo
               = smsMsg.getMessagePartInfo();
           if (partInfo) {
               std::cout << "\033[1;35mSegment: \033[0m"
                   << static_cast<int>(partInfo->segmentNumber)
                   << "\n SMS Part on phone ID " << phoneId << " from: "
                   << smsMsg.getSender() <<  " to: " << smsMsg.getReceiver()
                   << "\n Message Part: " << smsMsg.getText() << "\n PDU: " << smsMsg.getPdu()
                   << "\n RefNumber:" << static_cast <int>(partInfo->refNumber)
                   << " NumberOfSegments:"
                   << static_cast <int>(partInfo->numberOfSegments) << " SegmentNumber: "
                   << static_cast <int>(partInfo->segmentNumber) << std::endl;
          }
       }
       std::cout << "\033[1;35mComplete Message: \033[0m" <<  "\n" << text << std::endl;
    }

    void onDeliveryReport(int phoneId, int msgRef, std::string receiverAddress,
        ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "Received delivery report from phone ID " << phoneId << " with MsgRef: "
            << msgRef << " Receiver Address: "<< receiverAddress <<" Error: "
                      << static_cast<int>(error) << std::endl;
        if (error == ErrorCode::SUCCESS) {
            std::cout << "AECS information: data transmission completed" << std::endl;
        } else {
            std::cout << "AECS information: data transmission failed... retry" << std::endl;
            startMsdRetryAsync(phoneId);
         }
    }
};

/**
 * One-shot MSD over SMS send for this sample.
 *
 * @param phoneId   Slot to use for SMS
 * @param msdPdu    Encoded MSD as one PDU (<= 140 bytes for CS AECS).
 */
static Status sendMsdOverSms(int phoneId, const std::vector<uint8_t> &msdPdu) {
    auto smsMgr = smsMgrs_[phoneId - 1];
    if (!smsMgr) {
        std::cout << " SMS manager is not initialized" << std::endl;
        return Status::FAILED;
    }
    std::cout << " AECS information: data transmission in progress" << std::endl;
    std::vector<PduBuffer> rawPdus;
    rawPdus.push_back(msdPdu);

    std::cout << "[MSD SMS] Sending MSD over SMS ("
              << msdPdu.size() << " bytes, 1 PDU) ...\n";
    auto cbObj = std::make_shared<AecsSmsCommandCallback>();
    Status st = smsMgr->sendRawSms(rawPdus,
        std::bind(&AecsSmsCommandCallback::sendSmsResponse, cbObj, std::placeholders::_1,
                 std::placeholders::_2));
    std::cout << "[MSD SMS] sendRawSms() returned status="
              << static_cast<int>(st) << "\n";
    return st;
}

/**
 * Send MSD over SMS.
 * For this sample we:
 *   - obtain MSD PDU from user (simple console input)
 *   - call sendMsdOverSms()
 *
 * In a real product:
 *   - build MSD from vehicle data and encode it per EN 15722
 *   - no console I/O
 */
static void sendMsdOverSms(int phoneId) {

    std::cout << "\n[MSD SMS] starting MSD-over-SMS on phoneId=" << phoneId << std::endl;

    // SIMPLE DEMO INPUT:
    // You can swap this with real encoded MSD bytes produced by your encoder.
    std::string msdInput;
    std::cout << "Enter MSD encoded PDU payload (will be used as raw bytes): ";
    std::getline(std::cin, msdInput);
    if (msdInput.empty()) {
        std::cout << "[MSD SMS] Empty MSD input – skipping send.\n";
        gMsdDone.store(true);
        return;
    }

    // For demo: treat ASCII bytes as the PDU payload.
    std::vector<uint8_t> msdPdu(msdInput.begin(), msdInput.end());

    Status st = sendMsdOverSms(phoneId, msdPdu);
    if (st != Status::SUCCESS) {
        std::cout << "[MSD SMS] Initial MSD send failed. Per AECS spec, "
                  << "application must retry (interval <= 2min, duration <= 60min).\n";
        startMsdRetryAsync(phoneId);
    } else {
       gMsdDone.store(true);
    }
}


// --------------------------------------------------------------
//  AECS Call Listener
// --------------------------------------------------------------
class AecsCallListener : public ICallListener {
public:
    void onIncomingCall(std::shared_ptr<ICall> call) override {
        bool aecsInProgress = gAecsInProgress.load();
        bool isEmergencyModeEnabled = isEmergencyMode(call->getPhoneId());

        std::cout << "\n[Listener] Incoming call: number="
                  << call->getRemotePartyNumber()
                  << " isEmergencyModeEnabled =" << isEmergencyModeEnabled << "\n";

        if (aecsInProgress && !isEmergencyModeEnabled) {
            std::cout << "[Listener] AECS in progress → rejecting non-AECS incoming call\n";
            auto status = call->reject();
            std::cout << "  reject() status: " << static_cast<int>(status) << "\n";
        } else {
            std::cout << "[Listener] (sample) Not auto-handling this incoming call\n";
        }
    }

    void onCallInfoChange(std::shared_ptr<ICall> call) override {
        auto state = call->getCallState();
        std::cout << "\n[Listener] Call info change: index=" << call->getCallIndex()
                  << " phoneId=" << call->getPhoneId()
                  << " state=" << static_cast<int>(state) << "\n";

        switch (state) {
        case CallState::CALL_DIALING:
            std::cout << "  State: DIALING\n";
            break;
        case CallState::CALL_ALERTING:
            std::cout << "  State: ALERTING\n";
            break;
        case CallState::CALL_ACTIVE:
            std::cout << "  State: ACTIVE (voice path up)\n";
            gAecsInProgress.store(true);
            {
                std::lock_guard<std::mutex> lock(gCallMutex);
                gAecsCall = call;
            }
            break;
        case CallState::CALL_ENDED:
            std::cout << "  State: ENDED\n";
            std::cout << " Cause of call termination: "
                << static_cast<int>(call->getCallEndCause()) << std::endl;
            if (call->isAecsCallDrop()) {
                std::cout << " AECS information: -voice connection is dropped,"
                    << " retry the voice connection" << std::endl;
                startDroppedAecsRetryAsync(call->getPhoneId());
            } else if (call->getRedialState() == RedialState::MODEM_RETRY_END) {
                 std::cout << " AECS information: - voice connection failure,"
                     << " retrying the voice connection" << std::endl;
                 startFailedAecsRetryAsync(call->getPhoneId());
            }
            if (isEmergencyMode(call->getPhoneId())) {
                gAecsInProgress.store(false);
                gCallEnded.store(true);
            }
            break;
        default:
            std::cout << "  State: " << static_cast<int>(state) << "\n";
            break;
        }
    }
};

static bool prepareSelectedPhoneForAecs(std::shared_ptr<ICallManager> cm, int phoneId) {
    auto calls = cm->getInProgressCalls();
    for (auto &c : calls) {
        if (!c) continue;
        if (c->getPhoneId() != phoneId) continue;
        if (c->getCallState() == CallState::CALL_ENDED) continue;

        if (isEmergencyMode(phoneId)) {
            std::cout << "[Helper] AECS call already ongoing on phoneId=" << phoneId
                      << " – reusing existing call.\n";
            {
                std::lock_guard<std::mutex> lock(gCallMutex);
                gAecsCall = c;
            }
            gAecsInProgress.store(true);
            return false;
        } else {
            std::cout << "[Helper] Terminating non-AECS call on phoneId=" << phoneId << "\n";
            auto st = c->hangup();
            std::cout << "  hangup() status: " << static_cast<int>(st) << "\n";
        }
    }
    return true;
}

static bool enterEmergencyMode(std::shared_ptr<ICallManager> cm, int phoneId) {
    std::cout << "[EM] Enabling emergency mode on phoneId=" << phoneId << "\n";
    // If already in emergency mode, just return success
    {
        std::lock_guard<std::mutex> lock(gEmergencyModeMutex);
        auto it = emergencyMode_.find(phoneId);
        if (it != emergencyMode_.end() && it->second) {
            std::cout << "[EM] Emergency mode already enabled on phoneId "
                  << phoneId << std::endl;
            return true;
        }
    }
    Status st = cm->setEmergencyMode(phoneId, true, false);
    if (st == Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(gEmergencyModeMutex);
        emergencyMode_[phoneId] = true;
        std::cout << "[EM] Emergency mode enabled on phoneId "
                  << phoneId << std::endl;
    } else {
        std::cout << "[EM] Failed to enable emergency mode on phoneId "
                  << phoneId << ", status=" << (int)st << std::endl;
    }
    return (st == Status::SUCCESS);
}

static void exitEmergencyMode(std::shared_ptr<ICallManager> cm, int phoneId) {
    std::cout << "[EM] Disabling emergency mode on phoneId=" << phoneId << "\n";
    auto it = emergencyMode_.find(phoneId);
    if (it == emergencyMode_.end() || !it->second) {
        std::cout << "[EM] Emergency mode already disabled on phoneId "
                  << phoneId << std::endl;
        return;
    }
    Status st = cm->setEmergencyMode(phoneId, false, false);
    if (st == Status::SUCCESS) {
        emergencyMode_[phoneId] = false;
        std::cout << "[EM] Emergency mode disabled on phoneId "
                  << phoneId << std::endl;
    } else {
        std::cout << "[EM] Failed to disable emergency mode on phoneId "
                  << phoneId << ", status=" << (int)st << std::endl;
    }
}

// --------------------------------------------------------------
//  main()
// --------------------------------------------------------------
int main() {
    auto &phoneFactory = PhoneFactory::getInstance();
    int noOfSlots = MIN_SIM_SLOT_COUNT;
    std::promise<ServiceStatus> readyPromise;
    auto callMgr = phoneFactory.getCallManager(
        [&readyPromise](ServiceStatus s) { readyPromise.set_value(s); });

    if (!callMgr) {
        std::cerr << "ERROR: Failed to get ICallManager instance\n";
        return 1;
    }

    std::cout << "Waiting for CallManager subsystem to be ready...\n";
    ServiceStatus svcStatus = readyPromise.get_future().get();
    if (svcStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cerr << "ERROR: CallManager not ready, status="
                  << static_cast<int>(svcStatus) << "\n";
        return 1;
    }
    std::cout << "CallManager subsystem is ready\n";

    auto callListener = std::make_shared<AecsCallListener>();
    Status regSt = callMgr->registerListener(callListener);
    std::cout << "registerListener() status=" << static_cast<int>(regSt) << "\n";
    if (regSt != Status::SUCCESS) {
        std::cerr << "ERROR: Unable to register call listener\n";
        return 1;
    }
     if (telux::common::DeviceConfig::isMultiSimSupported()) {
        noOfSlots = MAX_SIM_SLOT_COUNT;
    }

    auto smsListener = std::make_shared<AecsSmsListener>();
    auto deliveryCb = std::make_shared<AecsSmsDeliveryCallback>();
    auto smsCmdCb_ = std::make_shared<AecsSmsCommandCallback>();
    for (auto index = 1; index <= noOfSlots; index++) {
        std::promise<telux::common::ServiceStatus> prom;
        auto smsMgr = phoneFactory.getSmsManager(
            index, [&](telux::common::ServiceStatus status) { prom.set_value(status); });

        if (!smsMgr) {
            std::cout << "ERROR - Failed to get SMS Manager instance \n";
            return 0;
        }

        std::cout << " Waiting for SMS Manager to be ready \n";
        telux::common::ServiceStatus smsMgrStatus = prom.get_future().get();
        if (smsMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "SMS Manager is ready \n";
            auto status = smsMgr->registerListener(smsListener);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "ERROR - Failed to register listener \n";
                return 0;
            }
            smsMgrs_.emplace_back(smsMgr);
        } else {
            std::cout << "ERROR - Unable to initialize SMS Manager \n";
            return 0;
        }
    }

    std::cout << " AECS information: AECS function being triggered" << std::endl;
    // AECS identifier
    std::string aecsId;
    std::cout << "\nEnter AECS identifier (dial number or URN): ";
    std::getline(std::cin, aecsId);
    if (aecsId.empty()) {
        std::cerr << "No AECS identifier entered – aborting.\n";
        return 1;
    }

    const int phoneId = DEFAULT_PHONE_ID;

    if (!enterEmergencyMode(callMgr, phoneId)) {
        std::cerr << "ERROR: Failed to enter emergency mode – aborting.\n";
        return 1;
    }

    bool shouldOriginate = prepareSelectedPhoneForAecs(callMgr, phoneId);

    auto cbObj = std::make_shared<AecsMakeCallCallback>();

    if (shouldOriginate) {
        std::cout << "\nPlacing AECS call on phoneId=" << phoneId
                  << " to '" << aecsId << "'\n";
        Status makeStatus = callMgr->makeAecsCall(phoneId, aecsId,
             std::bind(&AecsMakeCallCallback::makeCallResponse, cbObj, std::placeholders::_1,
                 std::placeholders::_2));
        std::cout << "makeAecsCall() returned status="
                  << static_cast<int>(makeStatus) << "\n";
        if (makeStatus != Status::SUCCESS) {
            std::cerr << "WARNING: makeAecsCall not accepted. "
                         "Implement AECS spec redial here (<=2 min interval, <=60 min).\n";
        } else {
            std::cout << " AECS information: -voice connection establishment in progress"
                << std::endl;
        }
    } else {
        std::cout << "\nUsing existing AECS call – no new makeAecsCall.\n";
    }

    std::cout << "\nWaiting for AECS call events (DIALING/ALERTING/ACTIVE/ENDED)...\n"
              << "Press q at any time to hang up and exit.\n";

    // start MSD over SMS
    sendMsdOverSms(phoneId);

    std::string exitInput;
    while (true) {
        if (gCallEnded.load()) {
            std::cout << "\nAECS call ended.\n";
            break;
        }
        if (!gCallFailRetryDone.load() || !gCallDropRetryDone.load() || !gMsdDone.load()) {
            // call retry or MSD still in progress – don't touch stdin
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        std::getline(std::cin, exitInput);
        if (exitInput == "q" || exitInput == "Q") {
            std::cout << "\nUser requested hangup.\n";
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    {
        std::lock_guard<std::mutex> lock(gCallMutex);
        if (gAecsCall &&
            gAecsCall->getCallState() != CallState::CALL_ENDED) {
            std::cout << "Issuing hangup() on AECS call...\n";
            Status hSt = gAecsCall->hangup();
            std::cout << "hangup() status=" << static_cast<int>(hSt) << "\n";
        } else {
            std::cout << "No active AECS call to hang up (already ended).\n";
        }
    }

    exitEmergencyMode(callMgr, phoneId);

    Status unreg = callMgr->removeListener(callListener);
    std::cout << "Call removeListener() status=" << static_cast<int>(unreg) << "\n";
    unreg = smsMgrs_[ phoneId - 1]->removeListener(smsListener);
    std::cout << "SMS removeListener() status=" << static_cast<int>(unreg) << "\n";
    std::cout << "\nAECS sample finished. Press q to exit.\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (gDropRetryThread.joinable()) gDropRetryThread.join();
    if (gFailRetryThread.joinable()) gFailRetryThread.join();
    if (gMsdRetryThread.joinable())  gMsdRetryThread.join();
    return 0;
}
