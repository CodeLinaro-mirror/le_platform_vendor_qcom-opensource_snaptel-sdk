/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * This is a Firewall Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to set Firewall and DMZ features
 * interfaces.
 */

#ifndef FIREWALLMENU_HPP
#define FIREWALLMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>


#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class FirewallMenu : public ConsoleApp {
 public:
    // initialize menu and sdk
    bool init();

    // Firewall Manager APIs
    void setFirewall(std::vector<std::string> inputCommand);
    void requestFirewallStatus(std::vector<std::string> inputCommand);
    void addFirewallEntry(std::vector<std::string> inputCommand);
    void requestFirewallEntries(std::vector<std::string> inputCommand);
    void removeFirewallEntry(std::vector<std::string> inputCommand);
    void enableDmz(std::vector<std::string> inputCommand);
    void disableDmz(std::vector<std::string> inputCommand);
    void requestDmzEntry(std::vector<std::string> inputCommand);

    FirewallMenu(std::string appName, std::string cursor);
    ~FirewallMenu();
 private:
    bool initComplete_;
    // get IPV4 Firewall params from user and set IPV4Info
    void getIPV4ParamsFromUser(telux::data::IpProtocol proto,
        std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp);
    // get IPV6 Firewall params from user and set IPV6Info
    void getIPV6ParamsFromUser(telux::data::IpProtocol proto,
        std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp);
    // get Transport Firewall params from user and set TCP/UDP Info
    void getProtocolParams(telux::data::IpProtocol proto,
        std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp);
    void getProtocolParamsFromUser (std::string proto, std::string &srcPort,
        std::string &srcRange, std::string &destPort, std::string &destRange);
    void parseProtoInfo(std::shared_ptr<IIpFilter> filter, telux::data::IpProtocol protocol,
        int &srcPort, int &destPort, int &srcPortRange, int &dstPortRange, std::string &protoStr);
    void displayFirewallEntry();

    std::shared_ptr<telux::data::net::IFirewallManager> firewallManager_;
    std::vector<std::shared_ptr<IFirewallEntry>> fwEntries_;
};
#endif
