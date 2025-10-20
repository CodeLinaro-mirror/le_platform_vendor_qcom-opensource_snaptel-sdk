/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AECSCALL_HPP
#define AECSCALL_HPP

#include "console_app_framework/ConsoleApp.hpp"
#include "AecsCallListener.hpp"
#include "AecsSmsListener.hpp"
#include "AecsCallManager.hpp"

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2

// Retry configuration
static constexpr int DEFAULT_RETRY_INTERVAL_SEC = 120;  // 2 minutes
static constexpr int DEFAULT_RETRY_DURATION_SEC = 3600;  // 60 minutes

/* TODO: Inline helper to determine if a phone number corresponds to an AECS call.
   Returns true if the number starts with the AECS prefix "*99". */
inline bool AECS_CALL(const std::string &number) {
    static const std::string prefix = "*99";
    return number.compare(0, prefix.size(), prefix) == 0;
}

class AecsCall : public ConsoleApp {
 public:
    // initialize menu
    bool init();
    AecsCall(std::string appName, std::string cursor);
    ~AecsCall();
    void makeCallResponse(telux::common::ErrorCode error, std::shared_ptr<telux::tel::ICall>);

 private:
    int getInputPhoneId();
    void dialAecsCall(std::vector<std::string> userInput);
    void acceptCall(std::vector<std::string> userInput);
    void getAllCalls(std::vector<std::string> userInput);
    void rejectCall(std::vector<std::string> userInput);
    void sendAecsMessage(std::vector<std::string> userInput);
    void retryDroppedAecsCall(std::vector<std::string> userInput);
    void retryFailedAecsCall(std::vector<std::string> userInput);
    void retryMsdOverSms(std::vector<std::string> userInput);
    void hangup(std::vector<std::string> userInput);
    void setEmergencyMode(std::vector<std::string> userInput);

    bool menuOptionsAdded_;
    std::shared_ptr<telux::tel::ICallManager> callMgr_;
    std::shared_ptr<telux::tel::ICallListener> callListener_;
    std::shared_ptr<AecsCallCommandCallback> aecsHangupCb_;
    std::shared_ptr<AecsCallCommandCallback> aecsAnswerCb_;
    std::shared_ptr<AecsCallCommandCallback> aecsRejectCb_;

    std::shared_ptr<AecsSmsCommandCallback> aecsSmsCmdCb_;
    std::shared_ptr<AecsSmsDeliveryCallback> aecsSmsDeliveryCb_;
    std::shared_ptr<telux::tel::ISmsListener> smsListener_;
    std::vector<std::shared_ptr<telux::tel::ISmsManager>> smsMgrs_;
};
#endif
