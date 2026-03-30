/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

extern "C" {
#include <sys/time.h>
}

#include "AecsSmsListener.hpp"
#include "Utils.hpp"
#include "AecsCallManager.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void AecsSmsListener::onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> smsMsg) {
    std::cout << std::endl << std::endl;
    std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg->getMessagePartInfo();
    if (partInfo) {
        PRINT_NOTIFICATION << "Received SMS on phone ID " << phoneId
                           << " from: " << smsMsg->getSender() << " to: " << smsMsg->getReceiver()
                           << "\n Message: " << smsMsg->getText() << "\n PDU: " << smsMsg->getPdu()
                           << " \n RefNumber:" << static_cast<int>(partInfo->refNumber)
                           << " NumberOfSegments:" << static_cast<int>(partInfo->numberOfSegments)
                           << " SegmentNumber: " << static_cast<int>(partInfo->segmentNumber)
                           << std::endl;
    } else {
        PRINT_NOTIFICATION << "Received SMS on phone ID " << phoneId
                           << " from: " << smsMsg->getSender() << " to: " << smsMsg->getReceiver()
                           << "\n Message: " << smsMsg->getText() << "\n PDU: " << smsMsg->getPdu()
                           << std::endl;
    }
}

void AecsSmsListener::onIncomingSms(
    int phoneId, std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs) {
    std::cout << std::endl;

    std::string text                             = "";
    std::vector<telux::tel::SmsMessage> messages = *(msgs.get());
    if (messages.size() > 1) {
        PRINT_NOTIFICATION << " Consolidated Multipart Message: " << std::endl;
        PRINT_NOTIFICATION << " Count :" << messages.size() << std::endl;
    } else {
        PRINT_NOTIFICATION << " Message: " << std::endl;
        PRINT_NOTIFICATION << " Count :" << messages.size() << std::endl;
    }
    for (telux::tel::SmsMessage smsMsg : messages) {
        text                                                  = text + smsMsg.getText();
        std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg.getMessagePartInfo();
        if (partInfo) {
            std::cout << "\033[1;35mSegment: \033[0m" << static_cast<int>(partInfo->segmentNumber)
                      << "\n SMS Part on phone ID " << phoneId << " from: " << smsMsg.getSender()
                      << " to: " << smsMsg.getReceiver() << "\n Message Part: " << smsMsg.getText()
                      << "\n PDU: " << smsMsg.getPdu()
                      << "\n RefNumber:" << static_cast<int>(partInfo->refNumber)
                      << " NumberOfSegments:" << static_cast<int>(partInfo->numberOfSegments)
                      << " SegmentNumber: " << static_cast<int>(partInfo->segmentNumber)
                      << std::endl;
        }
    }
    std::cout << "\033[1;35mComplete Message: \033[0m"
              << "\n"
              << text << std::endl;
}

void AecsSmsListener::onDeliveryReport(
    int phoneId, int msgRef, std::string receiverAddress, telux::common::ErrorCode error) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << "Received delivery report from phone ID " << phoneId
                       << " with MsgRef: " << msgRef << " Receiver Address: " << receiverAddress
                       << " Error Desc: " << Utils::getErrorCodeAsString(error) << std::endl;
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_NOTIFICATION << "AECS information: data transmission completed" << std::endl;
        // Success: stop any ongoing MSD retry
        stopMsdRetryLoop();

        // Clear manager's copy as well
        auto &aecsMgr = AecsCallManager::getInstance();
        std::vector<telux::tel::PduBuffer> emptyPdus;
        aecsMgr.setRetryRawPdu(emptyPdus);
        return;
    }

    PRINT_NOTIFICATION << "AECS information: data transmission failed" << std::endl;

    // Failure anchors the schedule: failure -> (interval) -> next send
    msdLastSentPhoneId_ = phoneId;
    onMsdAttemptFailed(msdLastSentPhoneId_);
}

AecsSmsListener::~AecsSmsListener() {
    stopMsdRetryLoop();
}

std::shared_ptr<telux::tel::ISmsManager> AecsSmsListener::getSmsManagerForPhoneId(int phoneId) {
    auto &mgr = AecsCallManager::getInstance();
    return mgr.getSmsManager(phoneId);
}

void AecsSmsListener::stopMsdRetryLoop() {
    msdRetryStop_.store(true);
    msdRetryCv_.notify_all();
    if (msdRetryThread_.joinable()) {
        msdRetryThread_.join();
    }
    msdRetryInProgress_.store(false);
    {
        std::lock_guard<std::mutex> lk(msdRetryMutex_);
        msdRetryScheduled_ = false;
    }
    // Clear PDU buffers to free memory
    msdRetryRawPdus_.clear();
}

void AecsSmsListener::markMsdRetryStopped() {
    msdRetryStop_.store(true);
    msdRetryInProgress_.store(false);
    {
        std::lock_guard<std::mutex> lk(msdRetryMutex_);
        msdRetryScheduled_ = false;
    }
}

void AecsSmsListener::scheduleNextMsdRetryFromNow() {
    std::lock_guard<std::mutex> lk(msdRetryMutex_);
    msdNextRetryAt_ = std::chrono::steady_clock::now() + std::chrono::seconds(msdRetryIntervalSec_);
    msdRetryScheduled_ = true;
    msdRetryCv_.notify_all();
}

void AecsSmsListener::onMsdAttemptFailed(int phoneId) {
    msdLastSentPhoneId_ = phoneId;

    if (!msdRetryInProgress_.load()) {
        // Start the retry window now; first attempt is scheduled, not immediate.
        retryMsdOverSms(msdLastSentPhoneId_ == -1 ? msdRetryPhoneId_ : msdLastSentPhoneId_);
        return;
    }
    // Already in a retry window -> reset the schedule from this failure time.
    scheduleNextMsdRetryFromNow();
}

void AecsSmsListener::sendMsdOnce(int phoneId, std::vector<telux::tel::PduBuffer> rawPdus) {
    auto smsMgr = getSmsManagerForPhoneId(phoneId);
    if (!smsMgr) {
        stopMsdRetryLoop();
        std::cout << "AecsSmsListener: No SmsManager for phoneId " << phoneId << std::endl;
        return;
    }
    msdLastSentPhoneId_ = phoneId;  // in case sendSmsResponse fails and we need to start loop
    auto cb             = [this](std::vector<int> msgRefs, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            PRINT_CB << "sendSmsResponse successfully" << std::endl;
            PRINT_CB << " MsgRefs Size: " << msgRefs.size() << std::endl;
            for (int i : msgRefs) {
                PRINT_CB << " MsgRef : " << i << std::endl;
            }
        } else {
            PRINT_CB << "sendSmsResponse failed, errorCode: " << static_cast<int>(error)
                     << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
            // Start or re-schedule MSD retry window just like delivery-failure path.
            onMsdAttemptFailed(msdLastSentPhoneId_);
        }
    };

    telux::common::Status status = smsMgr->sendRawSms(rawPdus, cb);
    std::cout << "MSD-over-SMS send requested, status = " << (int)status << std::endl;
}

/**
 * Retry MSD over SMS with AECS policy:
 * interval <= 2 min, duration <= 60 min.
 */
void AecsSmsListener::retryMsdOverSms(int phoneId) {
    if (phoneId == INVALID_SLOT_ID) {
        std::cout << "Invalid Phone ID. Retry aborted." << std::endl;
        return;
    }

    if (msdRetryThread_.joinable() && std::this_thread::get_id() != msdRetryThread_.get_id()) {
        msdRetryThread_.join();
    }

    auto &aecsMgr = AecsCallManager::getInstance();
    if (!aecsMgr.isEmergencyMode(phoneId)) {
        std::cout << std::endl << std::endl;
        std::cout << "Enter 7 to enable emergency mode" << std::endl;
    }

    // If a previous MSD loop is still running, stop it first.
    if (msdRetryThread_.joinable()) {
        stopMsdRetryLoop();
        msdRetryStop_.store(false);
    }

    msdRetryInProgress_.store(true);
    msdRetryStartTime_ = std::chrono::steady_clock::now();

    // Clamp to AECS MSD policy (interval <= 2 min, duration <= 60 min)
    int configuredIntervalSec = aecsMgr.getAecsRetryInterval();
    int configuredDurationSec = aecsMgr.getAecsRetryDuration();

    msdRetryIntervalSec_ = configuredIntervalSec;
    msdRetryDurationSec_ = configuredDurationSec;
    msdRetryPhoneId_     = phoneId;

    std::cout << "Starting MSD-over-SMS retry: interval=" << (msdRetryIntervalSec_ / SEC_PER_MIN)
              << " min, duration=" << (msdRetryDurationSec_ / SEC_PER_MIN) << " min..."
              << std::endl;

    PRINT_NOTIFICATION << " AECS information: data transmission in progress" << std::endl;

    msdRetryRawPdus_.clear();
    msdRetryRawPdus_ = aecsMgr.getRetryRawPdu();
    if (msdRetryRawPdus_.empty()) {
        std::cout << "No PDU buffers available for retry" << std::endl;
        msdRetryInProgress_.store(false);
        return;
    }
    // First attempt is scheduled (failure time + interval). Because we are
    // entering the loop due to a failure, the "failure time" is effectively now.
    {
        std::lock_guard<std::mutex> lk(msdRetryMutex_);
        msdNextRetryAt_
            = std::chrono::steady_clock::now() + std::chrono::seconds(msdRetryIntervalSec_);
        msdRetryScheduled_ = true;
    }

    // Start worker thread: wait for scheduled “failure→orig” times
    msdRetryStop_.store(false);
    msdRetryThread_ = std::thread([this] {
        while (!msdRetryStop_.load()) {
            // Check window expiry
            auto now = std::chrono::steady_clock::now();
            auto elapsed
                = std::chrono::duration_cast<std::chrono::seconds>(now - msdRetryStartTime_).count();
            if (elapsed >= msdRetryDurationSec_) {
                std::cout << "MSD retry window expired (" << (msdRetryDurationSec_ / SEC_PER_MIN)
                          << " min). Stopping retries." << std::endl;
                // --- BEGIN: ensure clean state when thread exits naturally ---
                msdRetryInProgress_.store(false);
                {
                    std::lock_guard<std::mutex> lk(msdRetryMutex_);
                    msdRetryScheduled_ = false;
                }
                msdRetryCv_.notify_all();

                markMsdRetryStopped();
                return;
            }
            // Wait for a scheduled (FAILURE + interval) time or stop
            std::unique_lock<std::mutex> lk(msdRetryMutex_);
            msdRetryCv_.wait(lk, [this] { return msdRetryStop_.load() || msdRetryScheduled_; });
            if (msdRetryStop_.load())
                break;
            auto due           = msdNextRetryAt_;
            msdRetryScheduled_ = false;  // consume schedule
            while (!msdRetryStop_.load()) {
                auto now2 = std::chrono::steady_clock::now();
                if (now2 >= due)
                    break;
                msdRetryCv_.wait_for(lk, (due - now2), [this] { return msdRetryStop_.load(); });
                if (msdRetryStop_.load())
                    break;
            }
            if (msdRetryStop_.load())
                break;
            lk.unlock();

            // Re-check window just before origination
            now = std::chrono::steady_clock::now();
            elapsed
                = std::chrono::duration_cast<std::chrono::seconds>(now - msdRetryStartTime_).count();
            if (elapsed >= msdRetryDurationSec_) {
                std::cout << "MSD retry window expired (" << (msdRetryDurationSec_ / SEC_PER_MIN)
                          << " min). Stopping retries." << std::endl;

                markMsdRetryStopped();
                return;
            }
            // Attempt another send
            sendMsdOnce(msdRetryPhoneId_, msdRetryRawPdus_);
        }
    });
}
