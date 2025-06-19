/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a Ethernet Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to setEthernetConfig, requestEthernetConfig, setMacsecConfig,
 * requestMacsecConfig.
 */

#ifndef ETHERNETMENU_HPP
#define ETHERNETMENU_HPP

#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>

#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/EthernetManager.hpp>

using namespace telux::data::net;
using namespace telux::common;

#define MAX_IFACE_NAME_SIZE 16

class EthernetMenu : public ConsoleApp ,
                 public IEthernetListener,
                 public std::enable_shared_from_this<EthernetMenu> {
 public:
    // initialize menu and sdk
    bool init();

    // Ethernet Manager APIs
    void setEthernetConfig(std::vector<std::string> inputCommand);
    void requestEthernetConfig(std::vector<std::string> inputCommand);
    void enableMacsec(std::vector<std::string> inputCommand);
    void disableMacsec(std::vector<std::string> inputCommand);
    void requestMacsecConfig(std::vector<std::string> inputCommand);

    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    EthernetMenu(std::string appName, std::string cursor);
    ~EthernetMenu();
 private:
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::shared_ptr<telux::data::net::IEthernetManager> ethernetManager_;
    //ethConfigValid_ variable is set to true when requestEthernetConfig receives valid eth Config
    bool ethConfigValid_;
    bool ethConfigStatusUpdated_;

    void displayEthernetNicConfig(const telux::data::net::EthConfig &ethConfig);
    void displayMacSecConfig(const telux::data::net::MacsecConfig &macsecConfig);
    bool setEthNicIfaceName(std::string &ethIface);
    int  setEthNicType(telux::data::net::EthNetworkType &ethNicType);

    void setMacsecConfig(telux::data::net::MacsecConfig &macsecConfig);
    void setMacsecOpr(telux::data::net::MacsecOp      &state);
    void setMacsecMode(telux::data::net::MacsecMode      &mode);
};
#endif /*ETHERNETMENU_HPP*/
