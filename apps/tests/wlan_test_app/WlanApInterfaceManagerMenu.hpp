/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLANAPINTERFACEMANAGERMENU_HPP
#define WLANAPINTERFACEMANAGERMENU_HPP

#include <iostream>
#include <memory>

#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/ApInterfaceManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"
#include "WlanUtils.hpp"
#include "../../common/utils/Utils.hpp"

class WlanApInterfaceManagerMenu : public ConsoleApp ,
                                   public telux::wlan::IApListener,
                                   public std::enable_shared_from_this<WlanApInterfaceManagerMenu> {
 public:
    WlanApInterfaceManagerMenu(std::string appName, std::string cursor);
    ~WlanApInterfaceManagerMenu();

    /**
     * Initialize commands
     */
    bool init();
    void showMenu();

    void setConfig(std::vector<std::string> userInput);
    void getConfig(std::vector<std::string> userInput);
    void getConnectedDevices(std::vector<std::string> userInput);
    void getConnectedDevicesStats(std::vector<std::string> userInput);
    void getStatus(std::vector<std::string> userInput);
    void manageApService(std::vector<std::string> userInput);

    void onApBandChanged(telux::wlan::BandType radio) override;
    void onApDeviceStatusChanged(telux::wlan::ApDeviceConnectionEvent event,
        std::vector<telux::wlan::DeviceIndInfo> info) override;

    // Deprecated APIs
    void onApDeviceStatusChanged(telux::wlan::ApDeviceConnectionEvent event,
        std::vector<telux::wlan::DeviceInfo> info) override;
 private:
    bool menuOptionsAdded_;
    std::shared_ptr<telux::wlan::IApInterfaceManager> wlanApInterfaceManager_ = nullptr;

};
#endif
