/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AECSSMSLISTENER_HPP
#define AECSSMSLISTENER_HPP

#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SmsManager.hpp>

class AecsSmsListener : public telux::tel::ISmsListener,
                        public std::enable_shared_from_this<AecsSmsListener> {
public:
    void onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> message) override;
    void onIncomingSms(int phoneId, std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs)
        override;
    void onDeliveryReport(int phoneId, int msgRef, std::string receiverAddress,
        telux::common::ErrorCode error) override;
    ~AecsSmsListener();

    // Called from callbacks when a send attempt fails (delivery).
    void onMsdAttemptFailed(int phoneId);

private:

    // ---- MSD retry worker state ----
    std::atomic<bool> msdRetryInProgress_{false};
    std::atomic<bool> msdRetryStop_{false};

    std::thread msdRetryThread_;
    std::mutex msdRetryMutex_;
    std::condition_variable msdRetryCv_;

    // Timing + policy (already present, keep using them)
    std::chrono::steady_clock::time_point msdRetryStartTime_{};
    int msdRetryIntervalSec_{0};
    int msdRetryDurationSec_{0};
    int msdRetryPhoneId_{INVALID_SLOT_ID};

    // Payload to resend (already present in cpp)
    std::vector<telux::tel::PduBuffer> msdRetryRawPdus_;

    void stopMsdRetryLoop();
    std::shared_ptr<telux::tel::ISmsManager> getSmsManagerForPhoneId(int phoneId);
    void retryMsdOverSms(int phoneId);
    void sendMsdOnce(int phoneId, std::vector<telux::tel::PduBuffer> rawPdus);

    // schedule next retry based on FAILURE time ---
    std::chrono::steady_clock::time_point msdNextRetryAt_{};
    bool msdRetryScheduled_{false};
    int msdLastSentPhoneId_{-1}; // Remember phoneId used for last send

    void scheduleNextMsdRetryFromNow();
};

#endif  // AECSSMSLISTENER_HPP
