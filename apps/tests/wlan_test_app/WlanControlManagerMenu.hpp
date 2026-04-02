/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLANCONTROLMANAGERMENU_HPP
#define WLANCONTROLMANAGERMENU_HPP

#include <iostream>
#include <memory>
#include <map>
#include <mutex>
#include <condition_variable>

#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/WlanControlManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"
#include "WlanUtils.hpp"
#include "../../common/utils/Utils.hpp"

class WlanControlManagerMenu : public ConsoleApp,
                               public telux::wlan::IWlanControlListener,
                               public std::enable_shared_from_this<WlanControlManagerMenu> {
 public:
    WlanControlManagerMenu(std::string appName, std::string cursor);
    ~WlanControlManagerMenu();

    /**
     * Blocking call that instantiates the manager and waits for initialization
     * to complete. Returns true if the manager is ready for service.
     */
    bool isSubSystemReady();

    /**
     * Register menu commands. Must be called after isSubSystemReady() returns true.
     */
    bool init();
    void showMenu();

    // Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    void getInterfaceStatus(std::vector<std::string> userInput);
    void setStaIpConfig(std::vector<std::string> userInput);
    void getStaIpConfig(std::vector<std::string> userInput);
    void setApInterworking(std::vector<std::string> userInput);
    void getApInterworking(std::vector<std::string> userInput);

    // IWlanControlListener overrides
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onApStatusChanged(const std::vector<telux::wlan::ApStatus> &status) override;
    void onStationStatusChanged(const std::vector<telux::wlan::StaStatus> &staStatus) override;

 private:
    bool menuOptionsAdded_;
    std::shared_ptr<telux::wlan::IWlanControlManager> wlanControlManager_ = nullptr;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
};

#endif  // WLANCONTROLMANAGERMENU_HPP