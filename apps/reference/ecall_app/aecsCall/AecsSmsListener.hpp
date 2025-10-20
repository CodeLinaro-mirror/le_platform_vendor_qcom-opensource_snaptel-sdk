/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AECSSMSLISTENER_HPP
#define AECSSMSLISTENER_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SmsManager.hpp>

class AecsSmsListener : public telux::tel::ISmsListener {
 public:
    void onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> message) override;
    void onIncomingSms(
        int phoneId, std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs) override;
    void onDeliveryReport(int phoneId, int msgRef, std::string receiverAddress,
        telux::common::ErrorCode error) override;
};

class AecsSmsCommandCallback : public telux::common::ICommandResponseCallback {
 public:
    void commandResponse(telux::common::ErrorCode error) override;
    static void sendSmsResponse(std::vector<int> msgRefs, telux::common::ErrorCode errorCode);
};

class AecsSmsDeliveryCallback : public telux::common::ICommandResponseCallback {
 public:
    void commandResponse(telux::common::ErrorCode error) override;
};
#endif  // AECSSMSLISTENER_HPP
