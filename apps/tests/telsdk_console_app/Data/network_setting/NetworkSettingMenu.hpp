/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a NetworkSetting Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to Port Trigger, Alg Update, DataPathOpt Status.
 */

#ifndef NETWORKSETTINGMENU_HPP
#define NETWORKSETTINGMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>


#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/NetworkSettingManager.hpp>
#define MAX_IFACE_NAME_SIZE 16

using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class NetworkSettingMenu : public ConsoleApp ,
                 public INetworkSettingListener,
                 public std::enable_shared_from_this<NetworkSettingMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // NetworkSettingManager APIs
    void addPortTriggerEntry(std::vector<std::string> inputCommand);
    void requestPortTriggerEntry(std::vector<std::string> inputCommand);
    void deletePortTriggerEntry(std::vector<std::string> inputCommand);
    void updateAlg(std::vector<std::string> inputCommand);
    void setDataPathOptStatus(std::vector<std::string> inputCommand);
    void requestDataPathOptStatus(std::vector<std::string> inputCommand);

    // New API functions
    void addSWIpChannelConfig(std::vector<std::string> inputCommand);
    void removeSWIpChannelConfig(std::vector<std::string> inputCommand);
    void requestSWIpChannelConfig(std::vector<std::string> inputCommand);
    void allowIpFamily(std::vector<std::string> inputCommand);
    void addDHCPReservationRecord(std::vector<std::string> inputCommand);
    void editDHCPReservationRecord(std::vector<std::string> inputCommand);
    void requestDHCPReservationRecords(std::vector<std::string> inputCommand);
    void deleteDHCPReservationRecord(std::vector<std::string> inputCommand);
    void activateLAN(std::vector<std::string> inputCommand);

    int getIpFamilyTypeV4V6();
    bool setIfaceName(std::string &interfaceName);

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    NetworkSettingMenu(std::string appName, std::string cursor);
    ~NetworkSettingMenu();
 private:
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::shared_ptr<telux::data::net::INetworkSettingManager> networkSettingManager_;
    std::mutex mtx_;
    std::condition_variable cv_;
};
#endif
