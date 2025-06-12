/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #ifndef IPSECMENU_APP
 #define IPSECMENU_APP

 #include <algorithm>
 #include <iostream>
 #include <memory>
 #include <string>
 #include <iomanip>


 #include "console_app_framework/ConsoleApp.hpp"

 #include <telux/data/DataDefines.hpp>
 #include <telux/data/DataFactory.hpp>
 #include <telux/data/net/IpsecManager.hpp>

 using namespace telux::data;
 using namespace telux::common;
 using namespace telux::data::net;

class IpsecMenu : public ConsoleApp,
                  public IIpsecListener,
                  public std::enable_shared_from_this<IpsecMenu> {

public:
    // Initialize MENU and SDK
    bool init();

    // Ipsec Manager APIs
    void enableIpsec(std::vector<std::string> inputCommand);
    void disableIpsec(std::vector<std::string> inputCommand);
    void getIpsecEnabled(std::vector<std::string> inputCommand);
    void setTunnelConfig(std::vector<std::string> inputCommand);
    void setTunnelState(std::vector<std::string> inputCommand);
    void deleteTunnel(std::vector<std::string> inputCommand);
    void getTunnelConfig(std::vector<std::string> inputCommand);
    void getTunnelState(std::vector<std::string> inputCommand);
    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    IpsecMenu(std::string appName, std::string cursor);
    ~IpsecMenu();

private:
    void setIKEConfig(telux::data::net::IpsecConfig &ipsecConfig);
    void setHostToHostConfig(telux::data::net::IpsecConfig &ipsecConfig);
    void setSiteToSiteWithoutNATConfig(telux::data::net::IpsecConfig &ipsecConfig);

    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::shared_ptr<telux::data::net::IIpsecManager> ipsecManager_;
    std::mutex mtx_;
    std::condition_variable cv_;
    char delimiter = '\n';
};

 #endif	// IPSECMENU_APP