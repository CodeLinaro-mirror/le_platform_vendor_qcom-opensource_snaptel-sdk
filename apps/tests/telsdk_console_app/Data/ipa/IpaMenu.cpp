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
/*
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 */

#include <algorithm>
#include <sstream>
#include <telux/data/DataFactory.hpp>
#include "Utils.hpp"
#include "IpaMenu.hpp"

using namespace std;

IpaMenu::IpaMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    ipaManager_ = nullptr;
    menuOptionsAdded_ = false;
    subSystemStatusUpdated_ = false;
}

IpaMenu::~IpaMenu() {
}

bool IpaMenu::init() {

    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;

    if (ipaManager_ == nullptr) {
        auto initCb = std::bind(&IpaMenu::onInitComplete, this, std::placeholders::_1);
        auto &dataFactory = telux::data::DataFactory::getInstance();

        ipaManager_ = dataFactory.getIpaManager(initCb);

        if (ipaManager_ == nullptr ) {
            std::cout << "\nError encountered in initializing IPA Manager" << std::endl;
            return false;
        }
        ipaManager_->registerListener(shared_from_this());
    }

    {
        std::unique_lock<std::mutex> lck(mtx_);

        telux::common::ServiceStatus subSystemStatus = ipaManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing IPA Manager, Please wait ..." << std::endl;
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = ipaManager_->getServiceStatus();
        }
        //At this point, initialization should be either AVAILABLE or FAIL
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nIPA Manager is ready" << std::endl;
        } else {
            std::cout << "\nIPA Manager initialization failed" << std::endl;
            ipaManager_ = nullptr;
            ipaManager_->deregisterListener(shared_from_this());
            return false;
        }
    }

    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        std::shared_ptr<ConsoleAppCommand> setIpPassthrough
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "set_ip_passthrough_config", {},
                std::bind(&IpaMenu::setIpPassthrough, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setVlanConfig
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "set_vlan_config", {},
                std::bind(&IpaMenu::setVlanConfig, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setIpCollision
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "set_ip_collision_config", {},
                std::bind(&IpaMenu::setIpCollision, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setFactoryReset
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "factory_reset", {},
                std::bind(&IpaMenu::setFactoryReset, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> monitorLanStatistics
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "monitor_lan_stats_config", {},
                std::bind(&IpaMenu::monitorLanStatistics, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> requestLanStatistics
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "request_lan_stats_config", {},
                std::bind(&IpaMenu::requestLanStatistics, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> updateWlanMode
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "update_wlanMode", {},
                std::bind(&IpaMenu::updateWlanMode, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> updateInterfaceType
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "update_interface_type", {},
                std::bind(&IpaMenu::updateInterfaceType, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setPacketThreshold
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "set_pkt_threshold", {},
                std::bind(&IpaMenu::setPacketThreshold, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setMacBasedSwFiltering
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "set_mac_sw_flt", {},
                std::bind(&IpaMenu::setMacBasedSwFiltering, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setIpBasedSwFiltering
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11", "set_ip_sw_flt", {},
                std::bind(&IpaMenu::setIpBasedSwFiltering, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setInterfaceBasedSwFiltering
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("12", "set_interface_sw_flt", {},
                std::bind(&IpaMenu::setInterfaceBasedSwFiltering, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> enableFileBasedSwFiltering
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("13", "enable_file_sw_flt", {},
                std::bind(&IpaMenu::enableFileBasedSwFiltering, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {setIpPassthrough,
            setVlanConfig, setIpCollision, setFactoryReset, monitorLanStatistics,
            requestLanStatistics, updateWlanMode, updateInterfaceType, setPacketThreshold,
            setMacBasedSwFiltering, setIpBasedSwFiltering, setInterfaceBasedSwFiltering,
            enableFileBasedSwFiltering};

        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void IpaMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

telux::data::IpaDeviceType devType_From_Num(int num) {
    telux::data::IpaDeviceType dev_type = telux::data::IpaDeviceType::IPA_CLIENT_DEVICE_MAX;
    switch (num) {
        case 1:
            dev_type = telux::data::IpaDeviceType::IPA_CLIENT_DEVICE_TYPE_USB;
            break;
        case 2:
            dev_type = telux::data::IpaDeviceType::IPA_CLIENT_DEVICE_TYPE_ETH;
            break;
        case 3:
            dev_type = telux::data::IpaDeviceType::IPA_CLIENT_DEVICE_TYPE_ETH1;
            break;
        case 4:
            dev_type = telux::data::IpaDeviceType::IPA_CLIENT_DEVICE_TYPE_ODU;
            break;
        case 5:
            dev_type = telux::data::IpaDeviceType::IPA_CLIENT_DEVICE_TYPE_WLAN;
            break;
        default:
            break;
    }
    return dev_type;
}

void IpaMenu::setIpPassthrough(std::vector<std::string> inputCommand) {
    telux::data::IpPassthroughConfig ipptconfig {};
    bool enable;
    int dev_type_num;
    telux::data::IpaDeviceType devtype;

    std::cout << "\nPlease enter the IP Passthrough configuration" << std::endl;
    std::cout << "Enable (1/0): ";
    std::cin >> enable;
    ipptconfig.enable = enable;

    std::cout << "Enter exact number for device type: " << std::endl;
    std::cout << "1. USB" << std::endl;
    std::cout << "2. ETH" << std::endl;
    std::cout << "3. ETH1" << std::endl;
    std::cout << "4. ODU" << std::endl;
    std::cout << "5. WLAN" << std::endl;

    std::cin >> dev_type_num;
    devtype = devType_From_Num(dev_type_num);
    if(devtype == telux::data::IpaDeviceType::IPA_CLIENT_DEVICE_MAX){
        std::cout << "Invalid input for device type" << std::endl;
        return;
    }
    ipptconfig.deviceType = devtype;

    std::cout << "Vlan ID: ";
    std::cin >> ipptconfig.vlanId;

    if(enable){
        std::cout << "MAC Addr: ";
        std::cin >> ipptconfig.macAddr;

        std::cout << "Skip Nat( 0- w/ Nat / 1- w/o Nat 1 ) :";
        std::cin >> ipptconfig.skipNat;

        std::cout << "Default PDN( 0-OnDemand / 1-default ) :";
        std::cin >> ipptconfig.defaultPdn;
    }

    std::cout << "Rmnet interface:";
    std::cin >> ipptconfig.iface;

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->setIpPassthrough(ipptconfig);

    std::cout << "\nsetIpPassthrough Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::setVlanConfig(std::vector<std::string> inputCommand) {
    telux::data::IpaVlanConfigMode mode;
    std::vector<std::string> ifaces;
    std::string line, intf;

    std::cout << "\nPlease enter the Set VLAN configuration" << std::endl;

    std::cout << "Enter interface names separated by spaces:" << std::endl;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, line);
    std::istringstream iss(line);

    while (iss >> intf)
        ifaces.push_back(intf);

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->setVlanConfig(mode, ifaces);

    std::cout << "\nsetVlanConfig Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::setIpCollision(std::vector<std::string> inputCommand) {
    telux::data::IpCollisionConfig ipCollisionCfg {};

    std::cout << "\nPlease enter the IP Collision configuration" << std::endl;
    std::cout << "Enable (1/0):";
    std::cin >> ipCollisionCfg.enable;

    std::cout << "VlanId ( 0-NonVlan ):";
    std::cin >> ipCollisionCfg.vlanId;

    std::cout << "PDN( 0-default / 1-without NAT ):";
    std::cin >> ipCollisionCfg.defaultPdn;

    std::cout << "Interface:";
    std::cin >> ipCollisionCfg.iface;

    std::cout << "Rmnet intf IPv4 Address: ";
    std::cin >> ipCollisionCfg.ipAddress;

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->setIpCollision(ipCollisionCfg);

    std::cout << "\nsetIpCollision Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::setFactoryReset(std::vector<std::string> inputCommand) {

    std::cout << "\nProcessing Factory Reset" << std::endl;
    telux::common::ErrorCode retCode = ipaManager_->setFactoryReset();

    std::cout << "\nsetFactoryReset Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::monitorLanStatistics(std::vector<std::string> inputCommand) {
    std::vector<telux::data::LanStatisticsConfig>lanStats;
    int num;

    std::cout << "\nPlease enter the Lan Stats configuration" << std::endl;
    std::cout<<"\nNo. of clients to monitor Lan stats"<<std::endl;
    std::cin>>num;

    while(num--){
        telux::data::LanStatisticsConfig params;
        std::cout << "Connect Status (1/0): ";
        std::cin >> params.enable;

        std::cout << "Interface name :";
        std::cin >> params.devName;

        std::cout << "Client Mac address: ";
        std::cin >> params.macAddr;

        lanStats.push_back(params);
    }

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->monitorLanStatistics(lanStats);

    std::cout << "\nmonitorLanStatistics Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::requestLanStatistics(std::vector<std::string> inputCommand) {
    telux::data::GetLanStatisticsConfig lanStatsQuery {};
    int dev_type_num;
    telux::data::IpaDeviceType devtype;

    std::cout << "\nPlease enter inputs for Lan Stats Query" << std::endl;

    std::cout << "Enter exact number for device type: " << std::endl;
    std::cout << "1. USB" << std::endl;
    std::cout << "2. ETH" << std::endl;
    std::cout << "3. ETH1" << std::endl;
    std::cout << "4. ODU" << std::endl;
    std::cout << "5. WLAN" << std::endl;

    std::cin >> dev_type_num;
    devtype = devType_From_Num(dev_type_num);
    if(devtype == telux::data::IpaDeviceType::IPA_CLIENT_DEVICE_MAX){
        std::cout << "Invalid input for device type" << std::endl;
        return;
    }
    lanStatsQuery.deviceType = devtype;

    std::cout << "No. Of Clients( 0-specific/1-All Clients ) :";
    std::cin >> lanStatsQuery.allClients;

    std::cout << "Disconnect Status(0/1) :";
    std::cin >> lanStatsQuery.disconnectStatus;

    std::cout << "Reset(0/1): ";
    std::cin >> lanStatsQuery.reset;

    if(lanStatsQuery.allClients == 0){
        std::cout << "Client Mac address: ";
        std::cin >> lanStatsQuery.macAddr;
    }

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->requestLanStatistics(lanStatsQuery);

    std::cout << "\nrequestLanStatistics Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::updateWlanMode(std::vector<std::string> inputCommand) {
    telux::data::IpaXmlWlanMode Mode;
    int mode_in;
    std::string iface;

    std::cout << "\nProvide input to update wlan mode" << std::endl;

    std::cout << "Iface:";
    std::cin >> iface;

    std::cout << "Mode(0-Full / 1-Internet): ";
    std::cin >> mode_in;
    if(mode_in == 0){
        Mode = telux::data::IpaXmlWlanMode::IPA_XML_MODE_FULL;
    } else if(mode_in == 1) {
        Mode = telux::data::IpaXmlWlanMode::IPA_XML_MODE_INTERNET;
    } else {
        std::cout << "Invalid input for Mode" << std::endl;
        return;
    }

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->updateWlanMode(iface, Mode);

    std::cout << "\nupdateWlanMode Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::updateInterfaceType(std::vector<std::string> inputCommand) {
    telux::data::IpaXmlInterfaceCategory Mode;
    int mode_in;
    std::string iface;

    std::cout << "\nProvide input to update interface type" << std::endl;

    std::cout << "Iface:";
    std::cin >> iface;

    std::cout << "Mode(0-Odu/ 1-Wlan/ 2-Wan): ";
    std::cin >> mode_in;
    if(mode_in == 0){
        Mode = telux::data::IpaXmlInterfaceCategory::IPA_XML_ODU;
    } else if(mode_in == 1) {
        Mode = telux::data::IpaXmlInterfaceCategory::IPA_XML_WLAN;
    } else if(mode_in == 2) {
        Mode = telux::data::IpaXmlInterfaceCategory::IPA_XML_WAN;
    } else {
        std::cout << "Invalid input for Mode" << std::endl;
        return;
    }

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->updateInterfaceType(iface, Mode);

    std::cout << "\nupdateInterfaceType Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::setPacketThreshold(std::vector<std::string> inputCommand) {
    telux::data::PacketThresholdConfig pktThreshCfg {};

    std::cout << "\nProvide input to set Packet Threshold" << std::endl;
    std::cout << "Enable (0/1): ";
    std::cin >> pktThreshCfg.enable;
    if(pktThreshCfg.enable == 1){
        std::cout << "Threshold: ";
        std::cin >> pktThreshCfg.threshold;
    }

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->setPacketThreshold(pktThreshCfg);

    std::cout << "\nsetPacketThreshold Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::setMacBasedSwFiltering(std::vector<std::string> inputCommand) {
    bool enable;
    std::vector<std::string> macAddrs;
    std::string line, addr;

    std::cout << "\nPlease enter input for MAC based filtering" << std::endl;
    std::cout << "Enable (1/0): ";
    std::cin >> enable;

    std::cout << "Enter MAC addresses separated by spaces:";
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, line);
    std::istringstream iss(line);

    while (iss >> addr)
        macAddrs.push_back(addr);

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->setMacBasedSwFiltering(enable, macAddrs);

    std::cout << "\nsetMacBasedSwFiltering Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::setIpBasedSwFiltering(std::vector<std::string> inputCommand) {
    bool enable;
    std::vector<telux::data::IpAddressSegment> ipAddrs;
    int num=0;

    std::cout << "\nPlease enter input for IP based filtering" << std::endl;
    std::cout << "Enable (1/0): ";
    std::cin >> enable;

    std::cout << "Enter no. of IP addresse segments :";
    std::cin >> num;
    while(num--){
        telux::data::IpAddressSegment ip;
        std::cout << "enter start addr: ";
        std::cin >> ip.startAddr;
        std::cout << "enter end addr: ";
        std::cin >> ip.endAddr;

        ipAddrs.push_back(ip);
    }

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->setIpBasedSwFiltering(enable, ipAddrs);

    std::cout << "\nsetIpBasedSwFiltering Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::setInterfaceBasedSwFiltering(std::vector<std::string> inputCommand) {
    bool enable;
    telux::data::IfaceBitMaskConfig bitmask {};
    std::string line, iface;

    std::cout << "\nPlease enter input for interface based filtering" << std::endl;
    std::cout << "Enable (1/0): ";
    std::cin >> enable;

    std::cout << "Enter Interfaces input as bitset:";
    std::cout << "1 (0001) :  eth0";
    std::cout << "3 (0011) :  wlan0 | eth0";
    std::cout << "14 (1110) :  wlan2 | wlan1 | wlan0";
    std::cin >> bitmask.ifaceBitMask;

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->setInterfaceBasedSwFiltering(enable, bitmask);

    std::cout << "\nsetInterfaceBasedSwFiltering Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void IpaMenu::enableFileBasedSwFiltering(std::vector<std::string> inputCommand) {
    std::string fpath;

    std::cout << "\nEnter File path for filtering" << std::endl;
    std::cin >> fpath;

    std::cout<<"Going to set\n";
    telux::common::ErrorCode retCode = ipaManager_->enableFileBasedSwFiltering(fpath);

    std::cout << "\nenableFileBasedSwFiltering Response"
	<< (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
	<< ". ErrorCode: " << static_cast<int>(retCode)
	<< ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}
