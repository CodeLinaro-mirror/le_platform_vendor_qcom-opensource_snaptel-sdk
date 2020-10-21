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
#include <telux/common/DeviceConfig.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "FirewallMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

#define PROTO_ICMP 1
#define PROTO_IGMP 2
#define PROTO_TCP 6
#define PROTO_UDP 17
#define PROTO_ESP 50

FirewallMenu::FirewallMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    firewallManager_ = nullptr;
    initComplete_ = false;
}

FirewallMenu::~FirewallMenu() {
}

bool FirewallMenu::init() {
    bool subSystemStatus = false;
    if (initComplete_ == false) {
        initComplete_ = true;
        auto &dataFactory = telux::data::DataFactory::getInstance();
        auto localFirewallMgr = dataFactory.getFirewallManager(
            telux::data::OperationType::DATA_LOCAL);
        if (localFirewallMgr) {
            firewallManager_ = localFirewallMgr;
        }
        auto remoteFirewallMgr = dataFactory.getFirewallManager(
            telux::data::OperationType::DATA_REMOTE);
        if (remoteFirewallMgr) {
            firewallManager_ = remoteFirewallMgr;
        }
        if(firewallManager_ == nullptr ) {
            std::cout << "\nUnable to create Firewall Manager ... " << std::endl;
            return false;
        }
        subSystemStatus = firewallManager_->isSubsystemReady();
        if (not subSystemStatus) {
            std::cout << "\nInitializing Firewall Manager subsystem, Please wait" << std::endl;
            std::future<bool> f = firewallManager_->onSubsystemReady();
            // Wait unconditionally for data subsystem to be ready
            subSystemStatus = f.get();
        }
        std::shared_ptr<ConsoleAppCommand> setFirewall
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "set_firewall", {},
                std::bind(&FirewallMenu::setFirewall, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestFirewallStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "request_firewall_status", {},
                std::bind(&FirewallMenu::requestFirewallStatus, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> addFirewallEntry
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "add_firewall_entry", {},
                std::bind(&FirewallMenu::addFirewallEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeFirewallEntry
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "remove_firewall_entry", {},
                std::bind(&FirewallMenu::removeFirewallEntry, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestFirewallEntries
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "request_firewall_entries", {},
                std::bind(&FirewallMenu::requestFirewallEntries, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> enableDmz
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "enable_dmz", {},
            std::bind(&FirewallMenu::enableDmz, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> disableDmz
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "disable_dmz",{},
            std::bind(&FirewallMenu::disableDmz, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestDmzEntry
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "request_dmz_entry", {},
                std::bind(&FirewallMenu::requestDmzEntry, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {setFirewall,
            requestFirewallStatus, addFirewallEntry, removeFirewallEntry, requestFirewallEntries,
            enableDmz, disableDmz, requestDmzEntry};

        addCommands(commandsList);
    }
    subSystemStatus = firewallManager_->isSubsystemReady();
    if (subSystemStatus) {
        std::cout << "\nFirewall Manager is ready" << std::endl;
    }
    else {
        std::cout << "\nFirewall Manager is not ready" << std::endl;
        return false;
    }
    ConsoleApp::displayMenu();
    return true;
}

void FirewallMenu::parseProtoInfo(std::shared_ptr<IIpFilter> filter,
    telux::data::IpProtocol protocol, int &srcPort, int &dstPort, int &srcPortRange,
        int &dstPortRange, std::string &protoStr) {

    if (protocol == PROTO_TCP) {
        auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(filter);
        if(tcpFilter) {
            TcpInfo tcpInfo = tcpFilter->getTcpInfo();
            srcPort = tcpInfo.src.port;
            srcPortRange = tcpInfo.src.range;
            dstPort = tcpInfo.dest.port;
            dstPortRange = tcpInfo.dest.range;
            protoStr = "TCP";
        } else {
            std::cout << " TCP filter is NULL so couldn't get TCP info\n ";
        }
    } else if (protocol == PROTO_UDP) {
        auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(filter);
        if(udpFilter) {
            UdpInfo udpInfo = udpFilter->getUdpInfo();
            srcPort = udpInfo.src.port;
            srcPortRange = udpInfo.src.range;
            dstPort = udpInfo.dest.port;
            dstPortRange = udpInfo.dest.range;
            protoStr = "UDP";
        } else {
            std::cout << " UDP filter is NULL so couldn't get UDP info\n ";
        }
    } else if (protocol == PROTO_ICMP) {
        protoStr = "ICMP";
    } else if (protocol == PROTO_IGMP) {
        protoStr = "IGMP";
    } else if (protocol == PROTO_ESP) {
        protoStr = "ESP";
    } else {
       std::cout << "Error: invalid protocol \n ";
    }
    return;
}

void FirewallMenu::setFirewall(std::vector<std::string> inputCommand) {
    bool fwEnable = false;
    bool allowPackets = false;
    telux::common::Status retStat;

    std::cout << "Set Firewall\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int enableFwFlag;
    std::cout << "Enter Enable Firewall (1 - On, 0 - Off): ";
    std::cin >> enableFwFlag;
    Utils::validateInput(enableFwFlag);
    if (enableFwFlag) {
        fwEnable = true;
    }

    if (fwEnable) {
        int allowPacketsFlag;
        std::cout << "Enter Packets Allowed (1 - Accept, 0 - Drop): ";
        std::cin >> allowPacketsFlag;
        Utils::validateInput(allowPacketsFlag);
        if (allowPacketsFlag) {
            allowPackets = true;
        }
    } else {
        allowPackets = false;
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setFirewall Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = firewallManager_->setFirewall(
        profileId, fwEnable, allowPackets, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void FirewallMenu::requestFirewallStatus(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "request Firewall Status\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto respCb = [](bool enable, bool allowPackets, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestFirewallStatus Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        std::cout << "Firewall " << (enable ? "is enabled" : "not enabled") << "\n";
        if (enable) {
            std::cout << "Firewall enabled to "
                      << (allowPackets ? "Accept Packets" : "Drop packets") << "\n";
        }
    };

    retStat = firewallManager_->requestFirewallStatus(profileId,respCb,static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void FirewallMenu::getIPV4ParamsFromUser(telux::data::IpProtocol proto,
    std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp) {
    std::string srcAddr = "", srcSubnetMask = "", destAddr = "", destSubnetMask = "";
    std::string tosVal = "", tosMask = "";
    char delimiter = '\n';

    int option;
    std::cout << "Do you want to enter IPV4 source address and subnet mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option);
    if (option == 1) {
        std::cin.get();
        std::cout << "Enter IPv4 Source address: ";
        std::getline(std::cin, srcAddr, delimiter);
        std::cout << "Enter IPv4 Source subnet mask: ";
        std::getline(std::cin, srcSubnetMask, delimiter);
    }

    std::cout << "Do you want to enter IPV4 destination address and subnet mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option);
    if (option == 1) {
        std::cin.get();
        std::cout << "Enter IPv4 Destination address: ";
        std::getline(std::cin, destAddr, delimiter);
        std::cout << "Enter IPv4 Destination subnet mask: ";
        std::getline(std::cin, destSubnetMask, delimiter);
    }

    std::cout << "Do you want to enter IPV4 TOS value and TOS mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option);
    if (option == 1) {
        std::cin.get();
        std::cout << "Enter Type of service value [0 to 255]: ";
        std::getline(std::cin, tosVal, delimiter);
        std::cout << "Enter Type of service mask [0 to 255]: ";
        std::getline(std::cin, tosMask, delimiter);
    }

    IPv4Info info;
    info.srcAddr = srcAddr;
    info.srcSubnetMask = srcSubnetMask;
    info.destAddr = destAddr;
    info.destSubnetMask = destSubnetMask;
    if (tosVal.empty()) {
        info.value = (uint8_t)0;
    } else {
        info.value = (uint8_t)atoi(tosVal.c_str());
    }
    if (tosMask.empty()) {
        info.mask = (uint8_t)0;
    } else {
        info.mask = (uint8_t)atoi(tosMask.c_str());
    }
    info.nextProtoId = proto;

    if (proto == 253) {
        info.nextProtoId = 6;
        ipFilter->setIPv4Info(info);
        info.nextProtoId = 17;
        ipFilterTcpUdp->setIPv4Info(info);
    } else {
        ipFilter->setIPv4Info(info);
    }
}

void FirewallMenu::getIPV6ParamsFromUser(telux::data::IpProtocol proto,
    std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp) {
    std::string srcAddr = "", destAddr = "";
    int srcPrefixLen = 0, dstPrefixLen = 0;
    int trfVal = 0, trfMask = 0, flowLabel = 0;
    char delimiter = '\n';

    int option;
    std::cout << "Do you want to enter IPV6 source address and subnet mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option);
    if (option == 1) {
        std::cin.get();
        std::cout << "Enter IPv6 Source address: ";
        std::getline(std::cin, srcAddr, delimiter);
        std::cout << "Enter IPv6 Source prefix length: ";
        std::cin >> srcPrefixLen;
        Utils::validateInput(srcPrefixLen);
        std::cin.get();
    }

    std::cout << "Do you want to enter IPv6 destination address and subnet mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option);
    if (option == 1) {
        std::cin.get();
        std::cout << "Enter IPv6 Destination address: ";
        std::getline(std::cin, destAddr, delimiter);
        std::cout << "Enter IPv6 Destination prefix length: ";
        std::cin >> dstPrefixLen;
        Utils::validateInput(dstPrefixLen);
        std::cin.get();
    }

    std::cout << "Do you want to enter IPV6 Traffic Class value and mask: [1-YES 0-NO]:";
    std::cin >> option;
    Utils::validateInput(option);
    if (option == 1) {
        std::cout << "Enter IPv6 Traffic class value: ";
        std::cin >> trfVal;
        Utils::validateInput(trfVal);

        std::cout << "Enter IPv6 Traffic class mask: ";
        std::cin >> trfMask;
        Utils::validateInput(trfMask);

        std::cout << "Enter IPv6 flow label : ";
        std::cin >> flowLabel;
        Utils::validateInput(flowLabel);
    }


    int natEnabled;
    std::cout << "Enter IPv6 nat enabled (1-Enable, 0-Disabled): ";
    std::cin >> natEnabled;
    Utils::validateInput(natEnabled);

    IPv6Info info;
    info.srcAddr = srcAddr;
    info.destAddr = destAddr;
    info.srcPrefixLen = (uint8_t)srcPrefixLen;
    info.dstPrefixLen = (uint8_t)dstPrefixLen;
    info.nextProtoId = proto;
    info.val = (uint8_t)trfVal;
    info.mask = (uint8_t)trfMask;
    info.flowLabel = (uint32_t)flowLabel;
    info.natEnabled = (uint8_t)natEnabled;

    if (proto == 253) {
        info.nextProtoId = 6;
        ipFilter->setIPv6Info(info);
        info.nextProtoId = 17;
        ipFilterTcpUdp->setIPv6Info(info);
    } else {
        ipFilter->setIPv6Info(info);
    }
}

void FirewallMenu::getProtocolParamsFromUser(std::string proto, std::string &srcPort,
    std::string &srcRange, std::string &destPort, std::string &destRange) {
    char delimiter = '\n';
    int option;
    std::cout << "Do you want to enter Source Port and Range [1-YES 0-NO]";
    std::cin >> option;
    Utils::validateInput(option);
    if (option == 1) {
        std::cin.get();
        std::cout << "Enter "<< proto <<" source port: ";
        std::getline(std::cin, srcPort, delimiter);
        std::cout << "Enter "<< proto <<" source range: ";
        std::getline(std::cin, srcRange, delimiter);
    }
    std::cout << "Do you want to enter Destination Port and Range [1-YES 0-NO]";
    std::cin >> option;
    Utils::validateInput(option);
    if (option == 1) {
        std::cin.get();
        std::cout << "Enter "<< proto <<" destination port: ";
        std::getline(std::cin, destPort, delimiter);
        std::cout << "Enter "<< proto <<" destination range: ";
        std::getline(std::cin, destRange, delimiter);
    }
}

void FirewallMenu::getProtocolParams(telux::data::IpProtocol proto,
    std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp) {
    switch (proto) {
    case 6:  // TCP
    {
        TcpInfo tcpInfo;
        std::string srcPort = "", srcRange = "";
        std::string destPort = "", destRange = "";

        getProtocolParamsFromUser("TCP", srcPort, srcRange, destPort, destRange);
        tcpInfo.src.port = srcPort.empty()?(uint16_t)0 : (uint16_t)atoi(srcPort.c_str());
        tcpInfo.src.range = srcRange.empty()? (uint16_t)0 : (uint16_t)atoi(srcRange.c_str());
        tcpInfo.dest.port = destPort.empty()?(uint16_t)0 : (uint16_t)atoi(destPort.c_str());
        tcpInfo.dest.range = destRange.empty()?(uint16_t)0 : (uint16_t)atoi(destRange.c_str());

        auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(ipFilter);
        if(tcpFilter) {
            tcpFilter->setTcpInfo(tcpInfo);
        }
    } break;
    case 17:  // UDP
    {
        UdpInfo info;
        std::string srcPort = "", srcRange = "";
        std::string destPort = "", destRange = "";

        getProtocolParamsFromUser("UDP", srcPort, srcRange, destPort, destRange);
        info.src.port = srcPort.empty()?(uint16_t)0 : (uint16_t)atoi(srcPort.c_str());
        info.src.range = srcRange.empty()? (uint16_t)0 : (uint16_t)atoi(srcRange.c_str());
        info.dest.port = destPort.empty()?(uint16_t)0 : (uint16_t)atoi(destPort.c_str());
        info.dest.range = destRange.empty()?(uint16_t)0 : (uint16_t)atoi(destRange.c_str());

        auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(ipFilter);
        if(udpFilter) {
            udpFilter->setUdpInfo(info);
        }
    } break;
    case 253:  // TCP_UDP
    {
        TcpInfo tcpInfo;
        UdpInfo udpInfo;
        std::string srcPort = "", srcRange = "";
        std::string destPort = "", destRange = "";

        getProtocolParamsFromUser("", srcPort, srcRange, destPort, destRange);
        tcpInfo.src.port = srcPort.empty()?(uint16_t)0 : (uint16_t)atoi(srcPort.c_str());
        tcpInfo.src.range = srcRange.empty()? (uint16_t)0 : (uint16_t)atoi(srcRange.c_str());
        tcpInfo.dest.port = destPort.empty()?(uint16_t)0 : (uint16_t)atoi(destPort.c_str());
        tcpInfo.dest.range = destRange.empty()?(uint16_t)0 : (uint16_t)atoi(destRange.c_str());

        udpInfo.src.port = srcPort.empty()?(uint16_t)0 : (uint16_t)atoi(srcPort.c_str());
        udpInfo.src.range = srcRange.empty()? (uint16_t)0 : (uint16_t)atoi(srcRange.c_str());
        udpInfo.dest.port = destPort.empty()?(uint16_t)0 : (uint16_t)atoi(destPort.c_str());
        udpInfo.dest.range = destRange.empty()?(uint16_t)0 : (uint16_t)atoi(destRange.c_str());

        auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(ipFilter);
        if(tcpFilter) {
            tcpFilter->setTcpInfo(tcpInfo);
        }

        auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(ipFilterTcpUdp);
        if(udpFilter) {
            udpFilter->setUdpInfo(udpInfo);
        }
    } break;
    default:
        break;
    }
}

void FirewallMenu::addFirewallEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "add Firewall Entry\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int fwDirection;
    std::cout << "Enter Firewall Direction (1-Uplink, 2-Downlink): ";
    std::cin >> fwDirection;
    Utils::validateInput(fwDirection);
    telux::data::Direction fwDir = static_cast<telux::data::Direction>(fwDirection);

    char delimiter = '\n';
    std::string protoStr;
    std::cin.get();
    std::cout << "Enter Protocol (TCP, UDP, TCP_UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);
    telux::data::IpProtocol proto = DataUtils::getProtcol(protoStr);

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType);
    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    std::shared_ptr<telux::data::net::IFirewallEntry> fwEntry = nullptr;
    // To handle creation of TCP_UDP firewall entry
    std::shared_ptr<telux::data::net::IFirewallEntry> fwEntryTcpUdp = nullptr;
    auto &dataFactory = telux::data::DataFactory::getInstance();

    if (proto == 253) {
        fwEntry = dataFactory.getNewFirewallEntry(6, fwDir, ipFamType);
        fwEntryTcpUdp = dataFactory.getNewFirewallEntry(17, fwDir, ipFamType);
    } else {
        fwEntry = dataFactory.getNewFirewallEntry(proto, fwDir, ipFamType);
    }

    std::shared_ptr<IIpFilter> ipFilter = fwEntry->getIProtocolFilter();
    std::shared_ptr<IIpFilter> ipFilterTcpUdp = nullptr;
    if (proto == 253) {
        ipFilterTcpUdp = fwEntryTcpUdp->getIProtocolFilter();
    }

    if (fwEntry) {
        if (ipFamilyType == 4) {
            getIPV4ParamsFromUser(proto,ipFilter, ipFilterTcpUdp);
        }
        if (ipFamilyType == 6) {
            getIPV6ParamsFromUser(proto,ipFilter, ipFilterTcpUdp);
        }
        getProtocolParams(proto,ipFilter, ipFilterTcpUdp);
    } else {
        std::cout << "\nERROR: unable to get firewall entry instance\n";
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "addFirewallEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = firewallManager_->addFirewallEntry(
        profileId, fwEntry, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);

    if (proto == 253) {
        retStat = firewallManager_->addFirewallEntry(
        profileId, fwEntryTcpUdp, respCb, static_cast<SlotId>(slotId));
        Utils::printStatus(retStat);
    }
}

void FirewallMenu::requestFirewallEntries(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "request Firewall Entry\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto respCb = [this](
        std::vector<shared_ptr<IFirewallEntry>> entries,
            telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestFirewallEntries Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        std::cout << "Found " << entries.size() << " entries\n";
        this->fwEntries_ = entries;
        this->displayFirewallEntry();
    };

    retStat = firewallManager_->requestFirewallEntries(profileId, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void FirewallMenu::displayFirewallEntry() {
    std::cout << std::setw(2)
        << "+-----------------------------------------------------------------------"
        << "------------------------------------------------------------------------"
        << "-+"
        << std::endl;
    std::cout << "|    Handle    | "
        << "Direction | "
        << "IPv4 Src Address | "
        << "       IPv6 Src Address        | "
        << "Protocol | "
        << "Src Port | "
        << "Src PortRange | "
        << "Dst Port | "
        << "Dst PortRange  | " << std::endl;
    std::cout << std::setw(2)
        << "+-----------------------------------------------------------------------"
        << "------------------------------------------------------------------------"
        << "-+"
        << std::endl;

    for (uint8_t i = 0; i < fwEntries_.size(); i++) {
        std::shared_ptr<IIpFilter> ipfilter = fwEntries_[i]->getIProtocolFilter();
        IPv4Info ipv4Info = ipfilter->getIPv4Info();
        IPv6Info ipv6Info = ipfilter->getIPv6Info();
        IpProtocol proto = ipfilter->getIpProtocol();
        int srcPort, destPort, srcPortRange, dstPortRange;
        srcPort = destPort = srcPortRange = dstPortRange = 0;
        std::string protoStr;
        parseProtoInfo(ipfilter, proto, srcPort, destPort, srcPortRange, dstPortRange, protoStr);
        std::string dir  = (static_cast<uint32_t>(
                    fwEntries_[i]->getDirection()) == 1)? "UPLINK":"DOWNLINK";
        ipv4Info.srcAddr = (ipv4Info.srcAddr.empty()) ? "Any":ipv4Info.srcAddr;
        ipv6Info.srcAddr = (ipv6Info.srcAddr.empty()) ? "Any":ipv6Info.srcAddr ;
        protoStr = (protoStr.empty())? "Any":protoStr;

        std::cout << std::left << std::setw(2) << "  " << std::setw(13)
            << fwEntries_[i]->getHandle()
            << "  " << std::setw(12) << dir
            << "  " << std::setw(18) << ipv4Info.srcAddr
            << std::setw(32) << ipv6Info.srcAddr << "  "
            << std::setw(9)  << protoStr << " "
            << std::setw(9)  << ((srcPort)? std::to_string(srcPort):"Any") << " "
            << std::setw(14) << ((srcPortRange)? std::to_string(srcPortRange):"Any") << " "
            << std::setw(12) << ((destPort)? std::to_string(destPort):"Any") << " "
            << std::setw(18) << ((dstPortRange)? std::to_string(dstPortRange):"Any") << std::endl;

//            << "  " << std::setw(18) << ipv4Info.srcAddr
//            << std::setw(32) << ipv6Info.srcAddr << "  "
//            << std::setw(9) << protoStr<< " "
//            << std::setw(9) << srcPort << " "
//            << std::setw(14) << srcPortRange << " "
//            << std::setw(12) << destPort << " "
//            << std::setw(18) << dstPortRange << std::endl;
    }
}

void FirewallMenu::removeFirewallEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "remove Firewall Entry\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int entryHandle;
    std::cout << "Enter handle of firewall entry to be removed: ";
    std::cin >> entryHandle;
    Utils::validateInput(entryHandle);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "removeFirewallEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    retStat = firewallManager_->removeFirewallEntry(
        profileId, entryHandle, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void FirewallMenu::enableDmz(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;

    std::cout << "Add DMZ\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    char delimiter = '\n';
    std::string ipAddr;
    std::cin.get();
    std::cout << "Enter IP address: ";
    std::getline(std::cin, ipAddr, delimiter);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "enableDmz Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat = firewallManager_->enableDmz(profileId, ipAddr, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void FirewallMenu::disableDmz(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "Remove DMZ\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    char delimiter = '\n';
    int ipType;
    std::cin.get();
    std::cout << "Enter IP Type (4-IPv4, 6-IPv6): ";
    std::cin >> ipType;
    Utils::validateInput(ipType);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "disableDmz Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    retStat = firewallManager_->disableDmz(profileId,
        static_cast<telux::data::IpFamilyType>(ipType), respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void FirewallMenu::requestDmzEntry(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    std::cout << "request Dmz Entries\n";
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto respCb = [](std::vector<std::string> dmzEntries, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestDmzEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (dmzEntries.size() > 0) {
            std::cout << "=============================================\n";
            for (auto entry : dmzEntries) {
                std::cout << "address: " << entry
                    << "\n=============================================\n";
            }
        }
    };

    retStat = firewallManager_->requestDmzEntry(profileId, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

