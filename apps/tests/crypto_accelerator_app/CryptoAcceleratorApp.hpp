/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOACCELERATORAPP_HPP
#define CRYPTOACCELERATORAPP_HPP

#include <telux/sec/CryptoAcceleratorManager.hpp>

#include "common/console_app_framework/ConsoleApp.hpp"

class CryptoAcceleratorApp : public ConsoleApp {
 public:
    CryptoAcceleratorApp(std::string appName, std::string cursor);
    ~CryptoAcceleratorApp();

    void init(void);
    void cryptoOperationMenu(telux::sec::Mode mode);
};

#endif  // CRYPTOACCELERATORAPP_HPP
