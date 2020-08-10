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

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>

#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "L2tpMenu.hpp"

using namespace std;

L2tpMenu::L2tpMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    l2tpManager_ = nullptr;
    initComplete_ = false;
}

L2tpMenu::~L2tpMenu() {
}

bool L2tpMenu::init() {
    bool subSystemStatus = false;
    if (initComplete_ == false) {
        initComplete_ = true;
        auto &dataFactory = telux::data::DataFactory::getInstance();
        l2tpManager_ = dataFactory.getL2tpManager();
        subSystemStatus = l2tpManager_->isSubsystemReady();
        if (not subSystemStatus) {
            std::cout << "\nInitializing L2TP Manager, Please wait" << std::endl;
            std::future<bool> f = l2tpManager_->onSubsystemReady();
            // Wait unconditionally for data subsystem to be ready
            subSystemStatus = f.get();
        }
        std::shared_ptr<ConsoleAppCommand> setConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Set_Configuration",
                {}, std::bind(&L2tpMenu::setConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addTunnel
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Add_Tunnel", {},
                std::bind(&L2tpMenu::addTunnel, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Request_Configuration", {},
                std::bind(&L2tpMenu::requestConfig, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeTunnel
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Remove_Tunnel", {},
                std::bind(&L2tpMenu::removeTunnel, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {setConfig,
            addTunnel, requestConfig, removeTunnel};

        addCommands(commandsList);
    }
    subSystemStatus = l2tpManager_->isSubsystemReady();
    if (subSystemStatus) {
        std::cout << "\nL2TP Manager is ready" << std::endl;
    }
    else {
        std::cout << "\nL2TP Manager is not ready" << std::endl;
        return false;
    }
    ConsoleApp::displayMenu();
    return true;
}


void L2tpMenu::setConfig(std::vector<std::string> inputCommand) {
    std::cout << "Set L2TP Unamanged Tunnel\n";
    telux::common::Status retStat;
    bool enable = true;
    bool enableMss =  false;
    bool enableMtu = false;
    uint32_t mtuSize = 0;
    int inputFlag;
    std::cout << "Enable/Disable L2TP for unmanaged tunnels\n (1-enable, 0-disable): ";
    std::cin >> inputFlag;
    Utils::validateInput(inputFlag);
    if(inputFlag == 0) {
        enable = false;
    }
    else {
        std::cout << "Enable/Disable TCP MSS clampping on L2TP interfaces to avoid segmentation\n"
            "(1-enable, 0-disable): ";
        std::cin >> inputFlag;
        Utils::validateInput(inputFlag);
        if(inputFlag) {
            enableMss = true;
        }
        std::cout << "Enable/Disable MTU size setting on underlying interfaces to avoid "
            "segmentation" << std::endl << "(1-enable, 0-disable): ";
        std::cin >> inputFlag;
        Utils::validateInput(inputFlag);
        if(inputFlag) {
            enableMtu = true;
            std::cout << "Use Default MTU size - 1422 bytes? (1-yes, 0-no): ";
            std::cin >> inputFlag;
            Utils::validateInput(inputFlag);
            if(inputFlag == 0) {
                std::cout << "Enter MTU size : ";
                std::cin >> mtuSize;
                Utils::validateInput(mtuSize);
            }
        }
    }
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Set L2TP Unamanged Tunnel Response is"
                  << (telux::common::ErrorCode::SUCCESS == error ? " successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat = l2tpManager_->setConfig(enable, enableMss, enableMtu, respCb, mtuSize);
    Utils::printStatus(retStat);
}

void L2tpMenu::addTunnel(std::vector<std::string> inputCommand) {
    std::cout << "Set L2TP Configuration\n";
    telux::common::Status retStat;
    L2tpTunnelConfig l2tpTunnelConfig;
    char delimiter = '\n';
    std::cin.get();
    std::cout << "Enter interface name to create L2TP tunnel on: ";
    std::getline(std::cin, l2tpTunnelConfig.locIface, delimiter);

    uint32_t tempInt;
    std::cout << "Enter local tunnel id: ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    l2tpTunnelConfig.locId = tempInt;
    std::cout << "Enter peer tunnel id: ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    l2tpTunnelConfig.peerId = tempInt;
    std::cout << "Enter peer ip version (4-IPv4, 6-IPv6): ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    if (4 == tempInt) {
        l2tpTunnelConfig.ipType = telux::data::IpFamilyType::IPV4;
        std::cin.get();
        std::cout << "Enter peer ipv4 address : ";
        std::getline(std::cin, l2tpTunnelConfig.peerIpv4Addr, delimiter);
    }
    else if (6 == tempInt) {
        l2tpTunnelConfig.ipType = telux::data::IpFamilyType::IPV6;
        std::cin.get();
        std::cout << "Enter peer ipv6 address : ";
        std::getline(std::cin, l2tpTunnelConfig.peerIpv6Addr, delimiter);
    }
    else  {
        std::cout << "Inavlid IP type entered .. exiting ..." <<std::endl;
        return;
    }
    std::cout << "Enter encapsulation protocol (0-IP, 1-UDP): ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    if (0 == tempInt) {
        l2tpTunnelConfig.prot = L2tpProtocol::IP;
    }
    else if (1 == tempInt) {
        l2tpTunnelConfig.prot = L2tpProtocol::UDP;
        std::cout << "Enter local udp port: ";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        l2tpTunnelConfig.localUdpPort = tempInt;
        std::cout << "Enter peer udp port: ";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        l2tpTunnelConfig.peerUdpPort = tempInt;
    }
    else  {
        std::cout << "Inavlid protocol entered .. exiting ..." <<std::endl;
        return;
    }
    std::cout << "Enter number of sessions for this tunnel (max allowed 3): ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);
    if (tempInt > 3) {
        std::cout << "Inavlid number of sessions .. exiting ..." <<std::endl;
        return;
    }
    int num_sessions = tempInt;
    for (int i=0; i<num_sessions; i++) {
        L2tpSessionConfig l2tpSessionConfig;
        std::cout << "Enter local session id for session " << i+1 << " :";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        l2tpSessionConfig.locId = tempInt;
        std::cout << "Enter peer session id for session " << i+1 << " :";
        std::cin >> tempInt;
        Utils::validateInput(tempInt);
        l2tpSessionConfig.peerId = tempInt;
        l2tpTunnelConfig.sessionConfig.emplace_back(l2tpSessionConfig);
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Set L2TP Config Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::NOT_SUPPORTED) {
            std::cout << "L2TP is not enabled, please enable L2TP";
        }
        else if (error == telux::common::ErrorCode::INCOMPATIBLE_STATE) {
            std::cout << "L2TP config can not be enabled...\n";
            std::cout << "Please map VLAN to default PDN first.\n";
        }
        else if (error == telux::common::ErrorCode::NO_EFFECT) {
            std::cout << "L2TP Config already set";
        }
    };
    retStat = l2tpManager_->addTunnel(l2tpTunnelConfig, respCb);
    Utils::printStatus(retStat);
}

void L2tpMenu::requestConfig(std::vector<std::string> inputCommand) {
    auto respCb = [](const L2tpSysConfig &l2tpSysConfig, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        if (error == telux::common::ErrorCode::NOT_SUPPORTED) {
            std::cout << "L2TP Unmanaged tunnel state is not enabled" <<std::endl;
            return;
        }
        std::cout << "CALLBACK: "
                  << "Get L2TP Config Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        std::cout << std::endl;
        std::cout <<  "MTU Config is " <<
            (true == l2tpSysConfig.enableMtu ? "Enabled" : "Disabled") << std::endl;
        if (l2tpSysConfig.mtuSize > 0) {
            std::cout <<  "MTU Size is " << l2tpSysConfig.mtuSize << std::endl;
        }
        std::cout <<  "TCP MSS Config is " <<
            (true == l2tpSysConfig.enableTcpMss ? "Enabled" : "Disabled") << std::endl;
        if (l2tpSysConfig.configList.empty()) {
            std::cout <<  "No Tunnel Configurations Detected" << std::endl;
        }
        else {
            std::cout <<  "Current Tunnel Configurations" << std::endl;
        }
        for (auto tnl : l2tpSysConfig.configList) {
            std::cout <<  "=========== Tunnel Configuration ===========" << std::endl;
            std::cout << "\tPhysical Interface: " << tnl.locIface << std::endl;
            std::cout << "\tLocal Tunnel ID: " << tnl.locId << std::endl;
            std::cout << "\tPeer Tunnel ID: " << tnl.peerId << std::endl;
            if (telux::data::IpFamilyType::IPV4 == tnl.ipType) {
                std::cout << "\tIP Version: IPv4" << std::endl;
                std::cout << "\tPeer IPv4 Address :" << tnl.peerIpv4Addr << std::endl;
            }
            else if (telux::data::IpFamilyType::IPV6 == tnl.ipType) {
                std::cout << "\tIP Version: IPv6" << std::endl;
                std::cout << "\tPeer IPv6 Address :" << tnl.peerIpv6Addr << std::endl;
            }
            else {
                std::cout << "\tIP Version: Unknown" << std::endl;
            }
            if (telux::data::net::L2tpProtocol::IP == tnl.prot) {
                std::cout << "\tEncapsulation Protocol: IP" << std::endl;
            }
            else if (telux::data::net::L2tpProtocol::UDP == tnl.prot) {
                std::cout << "\tEncapsulation Protocol: UDP" << std::endl;
                std::cout << "\tLocal UDP Port : " << tnl.localUdpPort << std::endl;
                std::cout << "\tPeer UDP Port : " << tnl.peerUdpPort << std::endl;
            }
            else {
                std::cout << "\tEncapsulation Protocol: Unknown" << std::endl;
            }
            int cnt = 1;
            for (auto session : tnl.sessionConfig) {
                std::cout << "\tSession: " << cnt << std::endl;
                std::cout << "\t    Session ID : " << session.locId << std::endl;
                std::cout << "\t    Peer Session ID : " << session.peerId << std::endl;
                cnt ++;
            }
        }
        std::cout << std::endl;
    };

    std::cout << "Request L2TP Configuration\n";
    telux::common::Status retStat;
    retStat = l2tpManager_->requestConfig(respCb);
    Utils::printStatus(retStat);
}

void L2tpMenu::removeTunnel(std::vector<std::string> inputCommand) {
    std::cout << "Remove L2TP Tunnel\n";
    telux::common::Status retStat;
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "Remove L2TP Configuration Response"
                  << (telux::common::ErrorCode::SUCCESS == error ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    uint32_t tempInt;
    std::cout << "Enter Tunnel ID to be deleted: ";
    std::cin >> tempInt;
    Utils::validateInput(tempInt);

    retStat = l2tpManager_->removeTunnel(tempInt, respCb);
    Utils::printStatus(retStat);
}
