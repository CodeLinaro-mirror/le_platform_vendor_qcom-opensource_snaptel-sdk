/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLANDEVICEMANAGERMENU_HPP
#define WLANDEVICEMANAGERMENU_HPP

#include <iostream>
#include <memory>

#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>
#include <condition_variable>

#include "console_app_framework/ConsoleApp.hpp"
#include "WlanUtils.hpp"
#include "../../common/utils/Utils.hpp"

class WlanDeviceManagerMenu : public ConsoleApp ,
                              public telux::wlan::IWlanListener,
                              public std::enable_shared_from_this<WlanDeviceManagerMenu> {

 public:
    WlanDeviceManagerMenu(std::string appName, std::string cursor);
    ~WlanDeviceManagerMenu();

    bool isSubSystemReady();
    bool init();

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    void enableWlan(std::vector<std::string> userInput);
    void setMode(std::vector<std::string> userInput);
    void getConfig(std::vector<std::string> userInput);
    void getStatus(std::vector<std::string> userInput);
    void setDynamicMode(std::vector<std::string> userInput);
    void getCurrentConfig(std::vector<std::string> userInput);

    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onEnableChanged(bool enable) override;
 private:
    bool menuOptionsAdded_;
    std::shared_ptr<telux::wlan::IWlanDeviceManager> wlanDeviceManager_ = nullptr;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
#endif
