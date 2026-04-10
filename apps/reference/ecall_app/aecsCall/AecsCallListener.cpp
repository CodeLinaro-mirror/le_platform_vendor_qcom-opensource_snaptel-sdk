/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <iostream>
#include <sstream>

extern "C" {
#include <sys/time.h>
}

#include "AecsCallListener.hpp"
#include "Utils.hpp"
#include "../TelClientUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void AecsCallListener::onIncomingCall(std::shared_ptr<telux::tel::ICall> call) {
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << getCurrentTime() << std::endl;
    PRINT_NOTIFICATION << TelClientUtils::callStateToString(call->getCallState())
             << " on slot Id: " << call->getPhoneId()
             << " and Phone Number: " << call->getRemotePartyNumber()
             << std::endl;

    std::cout << "Enter 2 to answer call" << std::endl;
    std::cout << "Enter 3 to reject call" << std::endl;
}

void AecsCallListener::onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) {
    int phoneId = call->getPhoneId();
    std::string number = call->getRemotePartyNumber();
    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << " Call State: "
                      << TelClientUtils::callStateToString(call->getCallState())
                      << "\n Call Index: " << (int)call->getCallIndex()
                      << ", Call Direction: " << (int)call->getCallDirection()
                      << ", Phone Number: " << number
                      << ", Slot Id: " << phoneId
                      << std::endl;

    auto &mgr = AecsCallManager::getInstance();
    if (call->getCallState() == telux::tel::CallState::CALL_ENDED) {
        // Retry if it is call drop or call origination fail or call failed due
        // to reasons other than Radio off, user or network end the call.
        if (!isAecsCallFailReason(call->getCallEndCause(), phoneId)) {
            mgr.setAecsCallDropStatus(false);
            mgr.setAecsCallFailStatus(false);

        } else if (call->isAecsCallDrop() &&
            (call->getCallDirection() == telux::tel::CallDirection::OUTGOING)) {
            mgr.setAecsCallDropStatus(true);
            // Use OEM/user config for drops
            int intervalSec = mgr.getAecsOemRetryInterval();
            int durationSec = mgr.getAecsOemRetryDuration();

            if (!retryInProgress_.load()) {
                startUnifiedRetry(RetryMode::Drop, phoneId, number, intervalSec, durationSec);
            } else if (retryMode_ != RetryMode::Drop) {
                // Switch running loop to Drop: stop & restart with Drop parameters
                stopRetryLoop();
                startUnifiedRetry(RetryMode::Drop, phoneId, number, intervalSec, durationSec);
            } else {
                // Already retrying -> push next attempt to (END + interval)
                scheduleNextRetryFromNow();
            }
        } else if ((call->getRedialState() == telux::tel::RedialState::MODEM_RETRY_END) &&
            (call->getCallDirection() == telux::tel::CallDirection::OUTGOING)) {
            mgr.setAecsCallFailStatus(true);
            int intervalSec = mgr.getAecsRetryInterval();
            int durationSec = mgr.getAecsRetryDuration();

            if (!retryInProgress_.load()) {
                startUnifiedRetry(RetryMode::Fail, phoneId, number, intervalSec, durationSec);
            } else if (retryMode_ != RetryMode::Fail) {
               // Switch running loop to Fail: stop & restart with Fail parameters
               stopRetryLoop();
               startUnifiedRetry(RetryMode::Fail, phoneId, number, intervalSec, durationSec);
            } else {
                // Already retrying -> push next attempt to (END + interval)
                scheduleNextRetryFromNow();
            }
        } else {
             // retry for AECS call failed reasons
            mgr.setAecsCallFailStatus(true);
            int intervalSec = mgr.getAecsRetryInterval();
            int durationSec = mgr.getAecsRetryDuration();

            if (!retryInProgress_.load()) {
                startUnifiedRetry(RetryMode::Fail, phoneId, number, intervalSec, durationSec);
            } else if (retryMode_ != RetryMode::Fail) {
               // Switch running loop to Fail: stop & restart with Fail parameters
               stopRetryLoop();
               startUnifiedRetry(RetryMode::Fail, phoneId, number, intervalSec, durationSec);
            } else {
                // Already retrying -> push next attempt to (END + interval)
                scheduleNextRetryFromNow();
            }
        }

        if (mgr.isEmergencyMode(phoneId) && (!mgr.getAecsCallDropStatus() &&
            !mgr.getAecsCallFailStatus())) {
            mgr.stopAudioIfNoCalls(phoneId);

            // Stop any running retry loop
            stopRetryLoop();
            // Not a drop/failure termination → close any leftover drop window.
            dropWindowArmed_ = false;
        }

        std::cout << std::endl << std::endl;
        PRINT_NOTIFICATION << getCurrentTime() << " Cause of call termination: "
            << TelClientUtils::callEndCauseToString(call->getCallEndCause()) << std::endl;
    } else if (call->getCallState() == telux::tel::CallState::CALL_ACTIVE &&
        mgr.isEmergencyMode(phoneId)) {
        std::cout << std::endl << std::endl;
        PRINT_NOTIFICATION
            << " AECS information: -voice connection established, voice communication in progress"
            << std::endl;

        // Stop any running retry loop
        stopRetryLoop();

    } else {
        // nothing
    }
}

bool AecsCallListener::isAecsCallFailReason(telux::tel::CallEndCause endCause, int phoneId) {
    auto &mgr = AecsCallManager::getInstance();
    /* AECS call retry not requied for below cause codes.
       1. When the device is offline(radio_off) - CallEndCause::RADIO_OFF
       2. When UE(user) ends the call - CallEndCause::CLIENT_END,
       3. When the network or other end(peer) ends the call - CallEndCause::NORMAL
         (in case of PS network), CallEndCause::NORMAL_CALL_CLEARING(in case of CS network)
       4. And if the cause code is unspecified. */
    return (mgr.isEmergencyMode(phoneId) &&
        (endCause != telux::tel::CallEndCause::RADIO_OFF) &&
        (endCause != telux::tel::CallEndCause::CLIENT_END) &&
        (endCause != telux::tel::CallEndCause::NORMAL) &&
        (endCause != telux::tel::CallEndCause::NORMAL_CALL_CLEARING) &&
        (endCause != telux::tel::CallEndCause::ERROR_UNSPECIFIED));
}

std::string AecsCallListener::getCurrentTime() {
    timeval tod;
    gettimeofday(&tod, NULL);
    std::stringstream ss;
    time_t tt = tod.tv_sec;
    char buffer[100];
    std::strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&tt));
    char currTime[120];
    snprintf(currTime, 120, "%s.%ld", buffer, tod.tv_usec / 1000);
    return std::string(currTime);
}

AecsCallCommandCallback::AecsCallCommandCallback(std::string commandName)
    : commandName_(commandName) {
}

void AecsCallCommandCallback::commandResponse(telux::common::ErrorCode error) {
    std::cout << std::endl << std::endl;
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << commandName_ << " operation successful" << std::endl;
    } else {
        PRINT_CB << commandName_ << " operation failed" << std::endl;
    }
    PRINT_CB << commandName_ << " operation - ErrorCode " << (int)error
            << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
}

void AecsDialCallback::makeCallResponse(telux::common::ErrorCode error,
    std::shared_ptr<telux::tel::ICall> call) {
    std::cout << std::endl << std::endl;
    std::cout << "makeAecsCall response ErrorCode: " << int(error)
            << ", description: " << Utils::getErrorCodeAsString(error);
    if (call) {
        std::cout << ", slot id: " << call->getPhoneId() << std::endl;
    } else {
        std::cout << ", call object is null" << std::endl;
    }
}

AecsCallListener::~AecsCallListener() {
    stopRetryLoop();
}

void AecsCallListener::joinRetryThreadIfNeeded() {
    if (retryThread_.joinable() &&
        std::this_thread::get_id() != retryThread_.get_id()) {
        retryThread_.join();
    }
}

void AecsCallListener::markRetryStopped() {
    {
        std::lock_guard<std::mutex> lk(retryMutex_);
        retryStop_.store(true);
        retryScheduled_ = false;
    }
    retryInProgress_.store(false);
    retryMode_ = RetryMode::None;
}

void AecsCallListener::stopRetryLoop() {
    markRetryStopped();
    retryCv_.notify_all();

    // Avoid joining from the retry thread itself
    if (retryThread_.joinable() &&
        std::this_thread::get_id() != retryThread_.get_id()) {
        retryThread_.join();
    }
}

void AecsCallListener::scheduleNextRetryFromNow() {
    // Schedule only if still within the retry window; the worker also re-checks.
    std::lock_guard<std::mutex> lk(retryMutex_);
    auto now = std::chrono::steady_clock::now();
    auto proposedRetry = now + std::chrono::seconds(retryIntervalSec_);

    // Only reschedule if the new time is sooner or no retry is scheduled
    if (!retryScheduled_ || proposedRetry < nextRetryAt_) {
        nextRetryAt_ = proposedRetry;
        retryScheduled_ = true;
        retryCv_.notify_all();
    }
}

void AecsCallListener::startUnifiedRetry(RetryMode mode, int phoneId, const std::string &number,
    int intervalSec, int durationSec) {

    // Ensure previous retry thread is fully cleaned up
    joinRetryThreadIfNeeded();

    auto now = std::chrono::steady_clock::now();

    // For AECS "Drop" retries, preserve the original window if it was already started.
    if (mode == RetryMode::Drop) {
        if (!dropWindowArmed_) {
            // First drop in this session: arm the window.
            dropWindowDeadline_ = now + std::chrono::seconds(durationSec);
            dropWindowArmed_ = true;
        } else {
            // Subsequent drops: use the remaining window.
            auto remaining = std::chrono::duration_cast<std::chrono::seconds>
                (dropWindowDeadline_ - now).count();
            if (remaining <= 0) {
                std::cout << "AECS drop retry window already expired. Skipping retries."
                    << std::endl;
                return;
            }
            durationSec = static_cast<int>(remaining);
        }
    } else {
        // Failure mode has its own window semantics; do not carry over the drop window.
        dropWindowArmed_ = false;
    }
    retryMode_ = mode;
    retryPhoneId_ = phoneId;
    retryNumber_ = number;
    retryIntervalSec_ = intervalSec;
    retryDurationSec_ = durationSec;
    retryStart_ = now;
    retryStop_.store(false);
    retryInProgress_.store(true);

    // schedule the FIRST retry from THIS call end moment
    {
       std::lock_guard<std::mutex> lk(retryMutex_);
       nextRetryAt_ = std::chrono::steady_clock::now() + std::chrono::seconds(retryIntervalSec_);
       retryScheduled_ = true;
    }

    std::cout << "Starting AECS retry (mode="
              << (mode == RetryMode::Drop ? "Drop" : "Fail")
              << ") interval=" << (retryIntervalSec_ / SEC_PER_MIN)
              << " min, duration=" << (retryDurationSec_ / SEC_PER_MIN) << " min..."
              << std::endl;

    retryThread_ = std::thread([this] {
        auto &mgr = AecsCallManager::getInstance();
        while (!retryStop_.load()) {
            // Check window expiry
            auto now = std::chrono::steady_clock::now();
            auto elapsed =
                std::chrono::duration_cast<std::chrono::seconds>(now - retryStart_).count();
            if (elapsed >= retryDurationSec_) {
                std::cout << "AECS " << (retryMode_ == RetryMode::Drop ? "drop" : "failure")
                          << " retry window expired. Stopping retries." << std::endl;
                if (retryMode_ == RetryMode::Drop) {
                    // Drop window is finished; disarm it.
                    dropWindowArmed_ = false;
                }
                markRetryStopped();
                return;
            }

            std::unique_lock<std::mutex> lk(retryMutex_);
            retryCv_.wait(lk, [this] {
            return retryStop_.load() || retryScheduled_;
        });
        if (retryStop_.load()) break;

        // Copy and clear the schedule
        auto due = nextRetryAt_;
        retryScheduled_ = false;

        // Wait until 'due' (or stop), then originate
        while (!retryStop_.load()) {
            auto now2 = std::chrono::steady_clock::now();
            if (now2 >= due) break;
            retryCv_.wait_for(lk, (due - now2), [this]{ return retryStop_.load(); });
            if (retryStop_.load()) break;
        }
        if (retryStop_.load()) break;
        lk.unlock();

        // Re-check window just before origination
        now = std::chrono::steady_clock::now();
        elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - retryStart_).count();
        if (elapsed >= retryDurationSec_) {
            std::cout << "AECS " << (retryMode_ == RetryMode::Drop ? "drop" : "failure")
                  << " retry window expired. Stopping retries.." << std::endl;

            if (retryMode_ == RetryMode::Drop) {
                // Drop window is finished; disarm it.
                dropWindowArmed_ = false;
            }
            markRetryStopped();
            return;
        }

        std::cout << std::endl << std::endl;
            PRINT_NOTIFICATION << " AECS information: - "
                << (retryMode_ == RetryMode::Drop
                     ? "voice connection is dropped, retry the voice connection"
                     : "voice connection failure, retrying the voice connection")
                << std::endl;

        telux::common::Status st =
            mgr.getCallManager()->makeAecsCall(retryPhoneId_, retryNumber_,
                    AecsDialCallback::makeCallResponse);
        std::cout << "AECS " << (retryMode_ == RetryMode::Drop ? "drop" : "failure")
                   << " retry requested, status = " << (int)st << std::endl;
        }
    });
}
