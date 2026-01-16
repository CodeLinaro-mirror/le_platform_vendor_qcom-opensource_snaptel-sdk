/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOOPERATIONMENU_HPP
#define CRYPTOOPERATIONMENU_HPP

#include "common/console_app_framework/ConsoleApp.hpp"

#include "CommandProcessor.hpp"

class CryptoOperationMenu : public ConsoleApp {

 public:
    CryptoOperationMenu(std::string appName, std::string cursor);
    ~CryptoOperationMenu();

    telux::common::ErrorCode init(telux::sec::Mode mode);

    void getHexStringAsByteArrayFromUsr(
        const std::string choiceToDisplay, std::vector<uint8_t> &usrEntry);

    void getChoiceNumberFromUsr(const std::string choicesToDisplay, const uint32_t minVal,
        const uint32_t maxVal, uint32_t &selection);

    void getUniqueIdFromUser(uint32_t &uniqueId);

    void getCurveFromUser(telux::sec::ECCCurve &curve);

    void getPriorityFromUser(telux::sec::RequestPriority &priority);

    void getTimeoutFromUser(uint32_t &timeout);

    void verify(void);

    void calculate(void);

 private:
    std::shared_ptr<CommandProcessor> cmdProcessor_;
    telux::sec::Mode mode_;
};

#endif  // CRYPTOOPERATIONMENU_HPP
