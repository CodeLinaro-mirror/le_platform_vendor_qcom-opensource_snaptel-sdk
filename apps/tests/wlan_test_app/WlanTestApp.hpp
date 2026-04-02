/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLANTESTAPP_HPP
#define WLANTESTAPP_HPP

#include <iostream>
#include <memory>

#include <telux/common/Version.hpp>
#include "WlanDeviceManagerMenu.hpp"
#include "WlanApInterfaceManagerMenu.hpp"
#include "WlanStaInterfaceManagerMenu.hpp"
#include "WlanControlManagerMenu.hpp"

#include "console_app_framework/ConsoleApp.hpp"

class WlanTestApp : public ConsoleApp {
 public:
    /**
     * Initialize commands and SDK
     */
    bool init();

    WlanTestApp(std::string appName, std::string cursor);
    ~WlanTestApp();

    void wlanDeviceManagerMenu(std::vector<std::string> inputCommand);
    void wlanApInterfaceManagerMenu(std::vector<std::string> inputCommand);
    void wlanStaInterfaceManagerMenu(std::vector<std::string> inputCommand);
    void wlanControlManagerMenu(std::vector<std::string> inputCommand);

 private:
    bool initWlan();
    std::shared_ptr<WlanDeviceManagerMenu> wlanDeviceManagerMenu_             = nullptr;
    std::shared_ptr<WlanApInterfaceManagerMenu> wlanApInterfaceManagerMenu_   = nullptr;
    std::shared_ptr<WlanStaInterfaceManagerMenu> wlanStaInterfaceManagerMenu_ = nullptr;
    std::shared_ptr<WlanControlManagerMenu> wlanControlManagerMenu_           = nullptr;
};
#endif
