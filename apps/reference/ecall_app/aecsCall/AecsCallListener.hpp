/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AECSCALLLISTENER_HPP
#define AECSCALLLISTENER_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/CallListener.hpp>

#include "AecsCallManager.hpp"

class AecsCallListener : public telux::tel::ICallListener {
 public:
    void onIncomingCall(std::shared_ptr<telux::tel::ICall> call) override;
    void onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) override;
    std::string getCallStateString(telux::tel::CallState cs);
    std::string getCallEndCauseString(telux::tel::CallEndCause causeCode);
    std::string getCurrentTime();
    ~AecsCallListener();

 private:
    enum class RetryMode {
        None,
        Drop,
        Fail
    };

    // Unified worker
    void startUnifiedRetry(
        RetryMode mode, int phoneId, const std::string &number, int intervalSec, int durationSec);
    void stopRetryLoop();

    // Retry state (shared)
    std::chrono::steady_clock::time_point retryStart_;
    int retryIntervalSec_ = 0;
    int retryDurationSec_ = 0;
    int retryPhoneId_     = INVALID_PHONE_ID;
    std::string retryNumber_;
    std::atomic<bool> retryInProgress_{false};
    std::atomic<bool> retryStop_{false};
    std::thread retryThread_;
    std::mutex retryMutex_;
    std::condition_variable retryCv_;
    RetryMode retryMode_{RetryMode::None};

    // schedule next retry based on call END time ---
    std::chrono::steady_clock::time_point nextRetryAt_{};
    bool retryScheduled_{false};

    // Schedule a retry at (now + retryIntervalSec_), and wake the worker.
    void scheduleNextRetryFromNow();

    // Persist AECS drop window across temporary CALL_ACTIVE states.
    std::chrono::steady_clock::time_point dropWindowDeadline_{};
    bool dropWindowArmed_{false};
    bool isAecsCallFailReason(telux::tel::CallEndCause endCause, int phoneId);
    void markRetryStopped();
    void joinRetryThreadIfNeeded();
};

class AecsDialCallback {
 public:
    static void makeCallResponse(
        telux::common::ErrorCode error, std::shared_ptr<telux::tel::ICall> call);
};

class AecsCallCommandCallback : public telux::common::ICommandResponseCallback {
 public:
    AecsCallCommandCallback(std::string commandName);
    void commandResponse(telux::common::ErrorCode error) override;

 private:
    std::string commandName_;
};

#endif  // AECSCALLLISTENER_HPP
