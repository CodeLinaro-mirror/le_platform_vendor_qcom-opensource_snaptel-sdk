/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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

#include <Utils.hpp>

#include "DataUtils.hpp"
#include "DataMenu.hpp"
#include "DataResponseCallback.hpp"
#define PROTO_ICMP 1
#define PROTO_IGMP 2
#define PROTO_TCP 6
#define PROTO_UDP 17
#define PROTO_ESP 50

using namespace std;
using namespace telux::data::net;

DataMenu::DataMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DataMenu::~DataMenu() {

    myDataProfileListCb_ = nullptr;
    myDataProfileListCb_ = nullptr;
    myDataProfileListCbForQuery_ = nullptr;
    myDataCreateProfileCb_ = nullptr;
    myDataProfileCb_ = nullptr;
    myDeleteProfileCb_ = nullptr;
    myModifyProfileCb_ = nullptr;
    myDataProfileCbForGetProfileById_ = nullptr;

    if (dataConnectionManager_) {
        dataConnectionManager_->deregisterListener(dataListener_);
        dataConnectionManager_ = nullptr;
    }

    if (dataProfileManager_) {
        dataProfileManager_->deregisterListener(profileListener_);
        dataProfileManager_ = nullptr;
    }

    if (dataFilterMgr_) {
        dataFilterMgr_->deregisterListener(dataFilterListener_);
        dataFilterMgr_ = nullptr;
    }

    if (profileListener_) {
        profileListener_ = nullptr;
    }

    if (dataListener_) {
        dataListener_ = nullptr;
    }

    if (dataFilterListener_) {
        dataFilterListener_ = nullptr;
    }
}

bool DataMenu::initializeSDK() {
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();

    dataConnectionManager_ = telux::data::DataFactory::getInstance().getDataConnectionManager();

    // Check if data subsystem is ready
    bool subSystemStatus = dataConnectionManager_->isSubsystemReady();

    // If data subsystem is not ready, wait for it to be ready
    if (!subSystemStatus) {
        std::cout << "\n\nData subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = dataConnectionManager_->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    dataProfileManager_ = dataFactory.getDataProfileManager();

    // Check if data subsystem is ready
    subSystemStatus = dataProfileManager_->isSubsystemReady();

    // If data subsystem is not ready, wait for it to be ready
    if (!subSystemStatus) {
        std::cout << "\n\nData profile manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = dataProfileManager_->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    // Exit the application, if SDK is unable to initialize data subsystems
    if (subSystemStatus) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                  << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
        exit(0);
    }

    if (subSystemStatus) {
        dataListener_ = std::make_shared<DataListener>();

        if (dataListener_) {
            dataConnectionManager_->registerListener(dataListener_);

            //Update dataListener_'s data call list
            requestDataCallList(OperationType::DATA_LOCAL,
                                std::bind(&DataListener::initDataCallListResponseCb, dataListener_,
                                          std::placeholders::_1, std::placeholders::_2));
            requestDataCallList(OperationType::DATA_REMOTE,
                                std::bind(&DataListener::initDataCallListResponseCb, dataListener_,
                                          std::placeholders::_1, std::placeholders::_2));
        }
    }

    // Get data filter manager object
    dataFilterMgr_ = dataFactory.getDataFilterManager();
    if (dataFilterMgr_ == NULL) {
        std::cout << "WARNING: Data Filter feature is not supported." << std::endl;
    }

    if (dataFilterMgr_ != NULL) {
        // Check data filter manager service status
        bool isReady = dataFilterMgr_->isReady();
        if (!isReady) {
            std::cout << " Data filter services are not ready, waiting for it to be ready "
                      << std::endl;
            std::future<bool> f = dataFilterMgr_->onReady();
            isReady = f.get();
        }

        if (isReady) {
            std::cout << " Data Filter services are ready !" << std::endl;
        } else {
            std::cout << " *** ERROR - Unable to initialize data filter services" << std::endl;
            return -1;
        }

        responseCb = std::bind(&DataMenu::commandCallback, this, std::placeholders::_1);
    }

    myDataProfileListCb_ = std::make_shared<MyDataProfilesCallback>();
    myDataProfileListCbForQuery_ = std::make_shared<MyDataProfilesCallback>();
    myDataCreateProfileCb_ = std::make_shared<MyDataCreateProfileCallback>();
    myDataProfileCb_ = std::make_shared<MyDataProfileCallback>();
    myDeleteProfileCb_ = std::make_shared<MyDeleteProfileCallback>();
    myModifyProfileCb_ = std::make_shared<MyModifyProfileCallback>();
    myDataProfileCbForGetProfileById_ = std::make_shared<MyDataProfileCallback>();
    profileListener_ = std::make_shared<MyProfileListener>();

    if (dataFilterMgr_ != NULL) {
        dataFilterListener_ = std::make_shared<MyDataFilterListener>();
    }

    telux::common::Status status = dataProfileManager_->registerListener(profileListener_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Unable to register data profile manager listener" << std::endl;
    }

    if (dataFilterMgr_ != NULL) {
        status = dataFilterMgr_->registerListener(dataFilterListener_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Unable to register data filter manager listener" << std::endl;
        }
    }

    return true;
}

void DataMenu::init() {

    std::shared_ptr<ConsoleAppCommand> startDataCall
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "start_data_call", {},
            std::bind(&DataMenu::startDataCall, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> stopDataCall
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "stop_data_call", {},
            std::bind(&DataMenu::stopDataCall, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> reqDataCallStats
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "request_datacall_statistics",
            {}, std::bind(&DataMenu::requestDataCallStatistics, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> resetDataCallStats
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "reset_datacall_statistics",
            {}, std::bind(&DataMenu::resetDataCallStatistics, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> reqDataCallList
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "request_datacall_list", {},
            std::bind(static_cast<void(DataMenu::*)()>(&DataMenu::requestDataCallList), this)));
    std::shared_ptr<ConsoleAppCommand> setDefaultProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "set_default_profile\n", {}, std::bind(&DataMenu::setDefaultProfile, this)));

    DataRestrictMode enableMode, disableMode;
    enableMode.filterAutoExit = DataRestrictModeType::DISABLE;
    enableMode.filterMode = DataRestrictModeType::ENABLE;

    disableMode.filterAutoExit = DataRestrictModeType::DISABLE;
    disableMode.filterMode = DataRestrictModeType::DISABLE;

    std::shared_ptr<ConsoleAppCommand> enableModeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "enable_data_restrict_mode",
            {}, std::bind(&DataMenu::sendSetDataRestrictMode, this, enableMode)));

    std::shared_ptr<ConsoleAppCommand> disableModeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "disable_data_restrict_mode",
            {}, std::bind(&DataMenu::sendSetDataRestrictMode, this, disableMode)));

    std::shared_ptr<ConsoleAppCommand> getFilterModeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "9", "get_data_restrict_mode", {}, std::bind(&DataMenu::getFilterMode, this)));

    std::shared_ptr<ConsoleAppCommand> addFilterCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "10", "add_data_restrict_filter", {}, std::bind(&DataMenu::addFilter, this)));

    std::shared_ptr<ConsoleAppCommand> removeAllFilterCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11",
            "remove_all_data_restrict_filter", {}, std::bind(&DataMenu::removeAllFilter, this)));

    std::shared_ptr<ConsoleAppCommand> reqStaticNatEntries
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("12", "request_static_nat_entries",
            {}, std::bind(&DataMenu::requestStaticNatEntries, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> addStaticNatEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("13", "add_static_nat", {},
            std::bind(&DataMenu::addStaticNatEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> removeStaticNatEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("14", "remove_static_nat", {},
            std::bind(&DataMenu::removeStaticNatEntry, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> requestFirewallStatus
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("15", "request_firewall_status", {},
            std::bind(&DataMenu::requestFirewallStatus, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setFirewall
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("16", "set_firewall", {},
            std::bind(&DataMenu::setFirewall, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> addFirewallEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("17", "add_firewall_entry", {},
            std::bind(&DataMenu::addFirewallEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestFirewallEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("18", "request_firewall_entry", {},
            std::bind(&DataMenu::requestFirewallEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> removeFirewallEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("19", "remove_firewall_entry", {},
            std::bind(&DataMenu::removeFirewallEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> enableDmz
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "20", "enable_dmz", {}, std::bind(&DataMenu::enableDmz, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> disableDmz
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "21", "disable_dmz",{}, std::bind(&DataMenu::disableDmz, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestDmzEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("22", "request_dmz_entry", {},
            std::bind(&DataMenu::requestDmzEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> createVlan
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("23", "create_vlan", {},
            std::bind(&DataMenu::createVlan, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> removeVlan
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("24", "remove_vlan", {},
            std::bind(&DataMenu::removeVlan, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> queryVlanInfo
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("25", "query_vlan_info", {},
            std::bind(&DataMenu::queryVlanInfo, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> bindWithProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("26", "bind_with_profile", {},
            std::bind(&DataMenu::bindWithProfile, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> unbindFromProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("27", "unbind_from_profile", {},
            std::bind(&DataMenu::unbindFromProfile, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> queryVlanMappingList
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("28", "query_vlan_mapping_list\n",
            {}, std::bind(&DataMenu::queryVlanMappingList, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> enableSocks
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("29", "socks_enablement\n",
            {}, std::bind(&DataMenu::enableSocks, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> bridgeMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("30", "Bridge_Menu\n",
            {}, std::bind(&DataMenu::bridgeMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> reqProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("100", "request_profile_list", {},
            std::bind(&DataMenu::requestProfileList, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> createProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("101", "create_profile", {},
            std::bind(&DataMenu::createProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> deleteProfileMenu = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("102", "delete_profile", {"profileId", "techPref (0-3GPP, 1-3GPP2)"},
            std::bind(&DataMenu::deleteProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> modifyProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("103", "modify_profile", {},
            std::bind(&DataMenu::modifyProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> queryProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("104", "query_profile", {},
            std::bind(&DataMenu::queryProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> requestProfileByIdMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("105", "request_profile_by_id",
            {"profileId", "techPref (0-3GPP, 1-3GPP2)"},
            std::bind(&DataMenu::requestProfileById, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {startDataCall, stopDataCall,
        reqDataCallStats, resetDataCallStats, reqDataCallList, setDefaultProfile, enableModeCommand,
        disableModeCommand, getFilterModeCommand, addFilterCommand, removeAllFilterCommand,
        reqStaticNatEntries, addStaticNatEntry, removeStaticNatEntry, requestFirewallStatus,
        setFirewall, addFirewallEntry, requestFirewallEntry, removeFirewallEntry, enableDmz,
        disableDmz, requestDmzEntry, createVlan, removeVlan, queryVlanInfo, bindWithProfile,
        unbindFromProfile, queryVlanMappingList, enableSocks, bridgeMenuCommand, reqProfile,
        createProfileMenu, deleteProfileMenu, modifyProfileMenu, queryProfileMenu,
        requestProfileByIdMenu};

    addCommands(commandsList);

    if (DataMenu::initializeSDK()) {
        ConsoleApp::displayMenu();
    }
}

void DataMenu::startDataCall(std::vector<std::string> inputCommand) {
    std::cout << "\nStart data call" << std::endl;
    int profileId;
    std::cout << "Enter Profile Id : ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType);

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);

    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    dataConnectionManager_->startDataCall(profileId, ipFamType,
        MyDataCallResponseCallback::startDataCallResponseCallBack, opType);
}

void DataMenu::stopDataCall(std::vector<std::string> inputCommand) {
    std::cout << "\nStop data call" << std::endl;
    int profileId;
    std::cout << "Enter Profile Id : ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType);

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);

    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    dataConnectionManager_->stopDataCall(profileId, ipFamType,
        MyDataCallResponseCallback::stopDataCallResponseCallBack, opType);
}

void DataMenu::requestDataCallStatistics(std::vector<std::string> inputCommand) {
    std::cout << "\nRequest DataCall Statistics" << std::endl;

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto dataCall = dataListener_->getDataCall(profileId);
    if (dataCall) {
        dataCall->requestDataCallStatistics(
            &DataCallStatisticsResponseCb::requestStatisticsResponse);
    } else {
        std::cout << "Unable to find DataCall, Please start_data_call" << std::endl;
    }
}

void DataMenu::resetDataCallStatistics(std::vector<std::string> inputCommand) {
    std::cout << "\nReset DataCall Statistics" << std::endl;

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;

    auto dataCall = dataListener_->getDataCall(profileId);
    if (dataCall) {
        dataCall->resetDataCallStatistics(&DataCallStatisticsResponseCb::resetStatisticsResponse);
    } else {
        std::cout << "Unable to find DataCall, Please start_data_call" << std::endl;
    }
}

void DataMenu::requestDataCallList(OperationType operationType, DataCallListResponseCb cb) {
    if (dataConnectionManager_) {
        telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
        dataConnectionManager_->requestDataCallList(opType,cb);
    }
}

void DataMenu::requestDataCallList() {
    std::cout << "\nRequest DataCall List" << std::endl;
    if (dataConnectionManager_) {
        int operationType;
        std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
        std::cin >> operationType;
        Utils::validateInput(operationType);

        telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
        if (telux::common::Status::NOTIMPLEMENTED == dataConnectionManager_->requestDataCallList(
            opType,MyDataCallResponseCallback::dataCallListResponseCb)) {
            std::cout << "Feature Not Supported" << std::endl;
        }
    }
}

void DataMenu::setDefaultProfile() {
    std::cout << "\nSet Default Profile" << std::endl;
    if (dataConnectionManager_) {
        int operationType;
        std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
        std::cin >> operationType;
        Utils::validateInput(operationType);
        telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

        int profileId;
        std::cout << "Enter Profile Id: ";
        std::cin >> profileId;
        Utils::validateInput(profileId);

        // Callback
        auto respCb = [](telux::common::ErrorCode error) {
            std::cout << std::endl << std::endl;
            std::cout << "CALLBACK: "
                      << "setDefaultProfile Response"
                      << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                      << ". ErrorCode: " << static_cast<int>(error)
                      << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        };

        if (telux::common::Status::NOTIMPLEMENTED == dataConnectionManager_->setDefaultProfile(
            opType, profileId, respCb)) {
            std::cout << "Feature Not Supported" << std::endl;
        }
    }
}
void DataMenu::commandCallback(ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Command initiated successfully " << std::endl;
    } else {
        std::cout << " Command failed." << std::endl;
    }
}

void DataMenu::sendSetDataRestrictMode(DataRestrictMode mode) {

    if (dataFilterMgr_ == NULL) {
        std::cout << "Data restrict filter feature is not supported." << std::endl;
        return;
    }

    char delimiter = '\n';
    std::string profileIdInput;
    std::cout << "Enter Profile Id : ";
    std::getline(std::cin, profileIdInput, delimiter);

    int profileId = -1;
    if (!profileIdInput.empty()) {
        try {
            profileId = std::stoi(profileIdInput);
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << profileId
                      << std::endl;
            return;
        }
    } else {
        profileId = PROFILE_ID_MAX;
    }

    std::string ipFamilyTypeInput;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::getline(std::cin, ipFamilyTypeInput, delimiter);

    telux::data::IpFamilyType ipFamType = IpFamilyType::UNKNOWN;
    if (!ipFamilyTypeInput.empty()) {
        try {
            ipFamType = static_cast<telux::data::IpFamilyType>(std::stoi(ipFamilyTypeInput));
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values "
                      << static_cast<int>(ipFamType) << std::endl;
            return;
        }
    }

    if (mode.filterMode == DataRestrictModeType::ENABLE) {
        std::cout << " Sending command to enable Data Filter" << std::endl;
    } else if (mode.filterMode == DataRestrictModeType::DISABLE) {
        std::cout << " Sending command to disable Data Filter" << std::endl;
    }

    mode.filterAutoExit = DataRestrictModeType::DISABLE;
    telux::common::Status status = telux::common::Status::FAILED;

    if (profileId == PROFILE_ID_MAX && ipFamType == IpFamilyType::UNKNOWN) {
        status = dataFilterMgr_->setDataRestrictMode(mode, responseCb);
    } else {
        status = dataFilterMgr_->setDataRestrictMode(mode, responseCb, profileId, ipFamType);
    }

    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to send Data Restrict command" << std::endl;
    }
}

void DataMenu::getFilterMode() {

    if (dataFilterMgr_ == NULL) {
        std::cout << "Data restrict filter feature is not supported." << std::endl;
        return;
    }

    std::string interfaceName;
    std::cout << "Enter Network Interface Name: ";
    std::cin >> interfaceName;
    Utils::validateInput(interfaceName);

    std::cout << " Sending command to get Data Filter" << std::endl;

    telux::common::Status status = dataFilterMgr_->requestDataRestrictMode(
        interfaceName, &DataFilterModeResponseCb::requestDataRestrictModeResponse);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to send Data Restrict command" << std::endl;
    }
}

IpProtocol DataMenu::getTypeOfFilter(
    ConfigParser instance, std::map<std::string, std::string> filter) {
    IpProtocol type = PROTO_UDP;
    if (instance.getValue(filter, "FILTER_PROTOCOL_TYPE") != "") {
        std::string protoType = instance.getValue(filter, "FILTER_PROTOCOL_TYPE");
        if (strcmp(protoType.c_str(), "UDP") == 0) {
            type = PROTO_UDP;
        } else if (strcmp(protoType.c_str(), "TCP") == 0) {
            type = PROTO_TCP;
        }
        std::cout << "Set TCP Port and Range combination" << std::endl;
    }
    return type;
}

void DataMenu::addIPParameters(std::shared_ptr<telux::data::IIpFilter> &dataFilter,
    ConfigParser instance, std::map<std::string, std::string> filterMap) {

    if (instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS") != ""
        || instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS") != "") {
        telux::data::IPv4Info ipv4Info_ = {};
        if (instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS") != "") {
            ipv4Info_.srcAddr = instance.getValue(filterMap, "SOURCE_IPV4_ADDRESS");
        }
        if (instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS") != "") {
            ipv4Info_.destAddr = instance.getValue(filterMap, "DESTINATION_IPV4_ADDRESS");
        }
        dataFilter->setIPv4Info(ipv4Info_);
    }

    if (instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS") != ""
        || instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS") != "") {
        telux::data::IPv6Info ipv6Info_ = {};
        if (instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS") != "") {
            ipv6Info_.srcAddr = instance.getValue(filterMap, "SOURCE_IPV6_ADDRESS");
        }
        if (instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS") != "") {
            ipv6Info_.destAddr = instance.getValue(filterMap, "DESTINATION_IPV6_ADDRESS");
        }
        dataFilter->setIPv6Info(ipv6Info_);
    }
}

void DataMenu::addFilter() {

    if (dataFilterMgr_ == NULL) {
        std::cout << "Data restrict filter feature is not supported." << std::endl;
        return;
    }

    char delimiter = '\n';
    std::string profileIdInput;
    std::cout << "Enter Profile Id : ";
    std::getline(std::cin, profileIdInput, delimiter);

    int profileId = -1;
    if (!profileIdInput.empty()) {
        try {
            profileId = std::stoi(profileIdInput);
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << profileId
                      << std::endl;
            return;
        }
    } else {
        profileId = PROFILE_ID_MAX;
    }

    std::string ipFamilyTypeInput;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::getline(std::cin, ipFamilyTypeInput, delimiter);

    telux::data::IpFamilyType ipFamType = IpFamilyType::UNKNOWN;
    if (!ipFamilyTypeInput.empty()) {
        try {
            ipFamType = static_cast<telux::data::IpFamilyType>(std::stoi(ipFamilyTypeInput));
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values "
                      << static_cast<int>(ipFamType) << std::endl;
            return;
        }
    } else {
        ipFamType = IpFamilyType::UNKNOWN;
    }

    ConfigParser cfgParser("filter", DEFAULT_CONFIG_FILE_NAME);
    std::vector<std::map<std::string, std::string>> vectorFilter = cfgParser.getFilters();

    std::cout << "Total Filter = " << vectorFilter.size() << std::endl;

    // Get data factory instance
    auto &dataFilterFactory = DataFactory::getInstance();

    for (uint8_t i = 0; i < vectorFilter.size(); i++) {

        IpProtocol typeOfFilter = getTypeOfFilter(cfgParser, vectorFilter[i]);
        std::shared_ptr<telux::data::IIpFilter> dataFilter;

        if (typeOfFilter == PROTO_TCP) {

            std::cout << "Creating TCP filter " << std::endl;

            // Get data filter manager object
            dataFilter = dataFilterFactory.getNewIpFilter(PROTO_TCP);
            addIPParameters(dataFilter, cfgParser, vectorFilter[i]);
            auto tcpRestrictFilter = std::dynamic_pointer_cast<ITcpFilter>(dataFilter);

            PortInfo srcPort = {};
            PortInfo destPort = {};

            srcPort.port = 0;
            srcPort.range = 0;
            destPort.port = 0;
            destPort.range = 0;
            telux::data::TcpInfo tcpInfo_ = {};

            if (cfgParser.getValue(vectorFilter[i], "TCP_SOURCE_PORT") != ""
                || cfgParser.getValue(vectorFilter[i], "TCP_SOURCE_PORT_RANGE") != "") {
                srcPort.port = std::stoi(cfgParser.getValue(vectorFilter[i], "TCP_SOURCE_PORT"));
                srcPort.range
                    = std::stoi(cfgParser.getValue(vectorFilter[i], "TCP_SOURCE_PORT_RANGE"));
                tcpInfo_.src = srcPort;
            }

            if (cfgParser.getValue(vectorFilter[i], "TCP_DESTINATION_PORT") != ""
                || cfgParser.getValue(vectorFilter[i], "TCP_DESTINATION_PORT_RANGE") != "") {
                destPort.port
                    = std::stoi(cfgParser.getValue(vectorFilter[i], "TCP_DESTINATION_PORT"));
                destPort.range
                    = std::stoi(cfgParser.getValue(vectorFilter[i], "TCP_DESTINATION_PORT_RANGE"));
                tcpInfo_.dest = destPort;
            }
            if (tcpRestrictFilter) {
                tcpRestrictFilter->setTcpInfo(tcpInfo_);
            } else {
                std::cout << " *** ERROR - Invalid tcp filter" << std::endl;
                return;
            }
        } else if (typeOfFilter == PROTO_UDP) {
            std::cout << "Creating UDP filter " << std::endl;

            // Get data filter manager object
            dataFilter = dataFilterFactory.getNewIpFilter(PROTO_UDP);
            addIPParameters(dataFilter, cfgParser, vectorFilter[i]);

            auto udpRestrictFilter = std::dynamic_pointer_cast<IUdpFilter>(dataFilter);

            PortInfo srcPort;
            PortInfo destPort;

            srcPort.port = 0;
            srcPort.range = 0;
            destPort.port = 0;
            destPort.range = 0;
            telux::data::UdpInfo udpInfo_ = {};

            if (cfgParser.getValue(vectorFilter[i], "UDP_SOURCE_PORT") != ""
                || cfgParser.getValue(vectorFilter[i], "UDP_SOURCE_PORT_RANGE") != "") {
                srcPort.port = std::stoi(cfgParser.getValue(vectorFilter[i], "UDP_SOURCE_PORT"));
                srcPort.range
                    = std::stoi(cfgParser.getValue(vectorFilter[i], "UDP_SOURCE_PORT_RANGE"));
                udpInfo_.src = srcPort;
            }

            if (cfgParser.getValue(vectorFilter[i], "UDP_DESTINATION_PORT") != ""
                || cfgParser.getValue(vectorFilter[i], "UDP_DESTINATION_PORT_RANGE") != "") {
                destPort.port
                    = std::stoi(cfgParser.getValue(vectorFilter[i], "UDP_DESTINATION_PORT"));
                destPort.range
                    = std::stoi(cfgParser.getValue(vectorFilter[i], "UDP_DESTINATION_PORT_RANGE"));
                udpInfo_.dest = destPort;
            }
            if (udpRestrictFilter) {
                udpRestrictFilter->setUdpInfo(udpInfo_);
            } else {
                std::cout << " *** ERROR - Invalid udp filter" << std::endl;
                return;
            }
        } else {
            std::cout << " *** ERROR - Invalid conf file parameters" << std::endl;
            return;
        }
        std::cout << " Sending command to Add Data Filter" << std::endl;
        telux::common::Status status = telux::common::Status::FAILED;

        if (profileId == PROFILE_ID_MAX && ipFamType == IpFamilyType::UNKNOWN) {
            status = dataFilterMgr_->addDataRestrictFilter(dataFilter, responseCb);
        } else {
            status = dataFilterMgr_->addDataRestrictFilter(
                dataFilter, responseCb, profileId, ipFamType);
        }
        if (status != telux::common::Status::SUCCESS) {
            std::cout << " *** ERROR - Failed to send Data Restrict command" << std::endl;
        }
    }
}

void DataMenu::removeAllFilter() {

    if (dataFilterMgr_ == NULL) {
        std::cout << "Data restrict filter feature is not supported." << std::endl;
        return;
    }
    std::cout << "\nRemove data filters" << std::endl;

    char delimiter = '\n';
    std::string profileIdInput;
    std::cout << "Enter Profile Id : ";
    std::getline(std::cin, profileIdInput, delimiter);

    int profileId = -1;
    if (!profileIdInput.empty()) {
        try {
            profileId = std::stoi(profileIdInput);
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << profileId
                      << std::endl;
            return;
        }
    } else {
        profileId = PROFILE_ID_MAX;
    }

    std::string ipFamilyTypeInput;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::getline(std::cin, ipFamilyTypeInput, delimiter);

    telux::data::IpFamilyType ipFamType = IpFamilyType::UNKNOWN;
    if (!ipFamilyTypeInput.empty()) {
        try {
            ipFamType = static_cast<telux::data::IpFamilyType>(std::stoi(ipFamilyTypeInput));
        } catch (const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values "
                      << static_cast<int>(ipFamType) << std::endl;
            return;
        }
    } else {
        ipFamType = IpFamilyType::UNKNOWN;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    if (profileId == PROFILE_ID_MAX && ipFamType == IpFamilyType::UNKNOWN) {
        status = dataFilterMgr_->removeAllDataRestrictFilters(responseCb);
    } else {
        status = dataFilterMgr_->removeAllDataRestrictFilters(responseCb, profileId, ipFamType);
    }
    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to send remove Data Filter command" << std::endl;
        return;
    }
}

void DataMenu::getProfileParamsFromUser() {
    char delimiter = '\n';
    int techPref;
    std::cout << "Enter Tech Preference (0-3GPP, 1-3GPP2): ";
    std::cin >> techPref;
    Utils::validateInput(techPref);

    std::cin.get();
    std::string profileName;
    std::cout << "Enter profileName : ";
    std::getline(std::cin, profileName, delimiter);

    std::string apnName;
    std::cout << "Enter APN : ";
    std::getline(std::cin, apnName, delimiter);

    std::string username;
    std::cout << "Enter userName : ";
    std::getline(std::cin, username, delimiter);

    std::string password;
    std::cout << "Enter password : ";
    std::getline(std::cin, password, delimiter);

    int authType;
    std::cout << "Enter Authentication Protocol Type : \n0-None \n1-PAP \n2-CHAP"
                 "\n3-PAP_CHAP\n";
    std::cin >> authType;
    Utils::validateInput(authType);

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType);

    params_.profileName = profileName;
    params_.techPref = static_cast<telux::data::TechPreference>(techPref);
    params_.authType = static_cast<telux::data::AuthProtocolType>(authType);
    params_.ipFamilyType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    params_.apn = apnName;
    params_.userName = username;
    params_.password = password;
}

void DataMenu::requestProfileList(std::vector<std::string> inputCommand) {
    telux::common::Status status = dataProfileManager_->requestProfileList(myDataProfileListCb_);

    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request profile list sent successfully" << std::endl;
    } else {
        std::cout << "Request profile list failed, status:" << int(status) << std::endl;
    }
}

void DataMenu::createProfile(std::vector<std::string> inputCommand) {
    getProfileParamsFromUser();

    telux::common::Status status
        = dataProfileManager_->createProfile(params_, myDataCreateProfileCb_);

    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Create profile request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send create profile request, Status:" << int(status) << std::endl;
    }
}

void DataMenu::deleteProfile(std::vector<std::string> inputCommand) {
    int profileId, techPrefId;
    try {
        profileId = std::stoi(inputCommand[1]);
        techPrefId = std::stoi(inputCommand[2]);
    } catch (const std::exception &e) {
        std::cout << "ERROR: Invalid input, please enter numerical values " << std::endl;
        return;
    }
    telux::data::TechPreference tp = telux::data::TechPreference::UNKNOWN;
    if (techPrefId == 1) {
        tp = telux::data::TechPreference::TP_3GPP;
    } else if (techPrefId == 2) {
        tp = telux::data::TechPreference::TP_3GPP2;
    }
    telux::common::Status status
        = dataProfileManager_->deleteProfile(profileId, tp, myDeleteProfileCb_);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Delete profile request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send delete profile request, Status:" << int(status) << std::endl;
    }
}

void DataMenu::modifyProfile(std::vector<std::string> inputCommand) {
    int profileId;
    std::cout << "Enter profile Id to Modify : ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    getProfileParamsFromUser();

    telux::common::Status status
        = dataProfileManager_->modifyProfile(profileId, params_, myModifyProfileCb_);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Modify profile request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send Modify profile request, Status:" << int(status) << std::endl;
    }
}

void DataMenu::queryProfile(std::vector<std::string> inputCommand) {
    char delimiter = '\n';
    int techPref;
    std::cout << "Enter Tech Preference (0-3GPP, 1-3GPP2): ";
    std::cin >> techPref;
    Utils::validateInput(techPref);

    std::cin.get();
    std::string profileName;
    std::cout << "Enter profileName: ";
    std::getline(std::cin, profileName, delimiter);

    std::string apnName;
    std::cout << "Enter APN: ";
    std::getline(std::cin, apnName, delimiter);

    std::string username;
    std::cout << "Enter username: ";
    std::getline(std::cin, username, delimiter);

    std::string password;
    std::cout << "Enter password: ";
    std::getline(std::cin, password, delimiter);

    int authType;
    std::cout << "Enter Authentication Protocol Type : \n0-None \n1-PAP"
                 "\n2-CHAP \n3-PAP_CHAP\n";
    std::cin >> authType;
    Utils::validateInput(authType);

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPV6, 10-IPV4V6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType);

    params_.profileName = profileName;
    params_.techPref = static_cast<telux::data::TechPreference>(techPref);
    params_.authType = static_cast<telux::data::AuthProtocolType>(authType);
    params_.ipFamilyType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
    params_.apn = apnName;
    params_.userName = username;
    params_.password = password;

    telux::common::Status status
        = dataProfileManager_->queryProfile(params_, myDataProfileListCbForQuery_);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Query profile request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send Query profile request, Status:" << int(status) << std::endl;
    }
}

void DataMenu::requestProfileById(std::vector<std::string> inputCommand) {
    int profileId, techPrefId;
    try {
        profileId = std::stoi(inputCommand[1]);
        techPrefId = std::stoi(inputCommand[2]);
    } catch (const std::exception &e) {
        std::cout << "ERROR: Invalid input, please enter numerical values " << std::endl;
        return;
    }

    telux::data::TechPreference tp = telux::data::TechPreference::UNKNOWN;
    if (techPrefId == 0) {
        tp = telux::data::TechPreference::TP_3GPP;
    } else if (techPrefId == 1) {
        tp = telux::data::TechPreference::TP_3GPP2;
    }
    telux::common::Status status
        = dataProfileManager_->requestProfile(profileId, tp, myDataProfileCbForGetProfileById_);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request profile by ID request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send Request profile by ID request, Status:" << int(status)
                  << std::endl;
    }
}

void DataMenu::addStaticNatEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::INatManager> natMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Add Static NAT entry\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    natMgr = dataFactory.getNatManager(opType);
    subSystemStatus = natMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\n\nNat Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = natMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    char delimiter = '\n';
    std::string privIpAddr;
    std::cin.get();
    std::cout << "Enter Private IP address: ";
    std::getline(std::cin, privIpAddr, delimiter);

    int privPort;
    std::cout << "Enter Private port: ";
    std::cin >> privPort;
    Utils::validateInput(privPort);

    int globPort;
    std::cout << "Enter Global port: ";
    std::cin >> globPort;
    Utils::validateInput(globPort);

    std::string protoStr;
    std::cin.get();
    std::cout << "Enter Protocol (TCP, UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);

    telux::data::IpProtocol proto = getProtcol(protoStr);
    struct NatConfig natConfig;
    natConfig.addr = privIpAddr;
    natConfig.port = (uint16_t)privPort;
    natConfig.globalPort = (uint16_t)globPort;
    natConfig.proto = (uint8_t)proto;

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "addStaticNatEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    natMgr->addStaticNatEntry(profileId, natConfig, respCb);
}

void DataMenu::removeStaticNatEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::INatManager> natMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Remove Static NAT entry\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    natMgr = dataFactory.getNatManager(opType);
    subSystemStatus = natMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\n\nNat Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = natMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    char delimiter = '\n';
    std::string privIpAddr;
    std::cin.get();
    std::cout << "Enter Private IP address: ";
    std::getline(std::cin, privIpAddr, delimiter);

    int privPort;
    std::cout << "Enter Private port: ";
    std::cin >> privPort;
    Utils::validateInput(privPort);

    int globPort;
    std::cout << "Enter Global port: ";
    std::cin >> globPort;
    Utils::validateInput(globPort);

    std::string protoStr;
    std::cin.get();
    std::cout << "Enter Protocol (TCP, UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);

    telux::data::IpProtocol proto = getProtcol(protoStr);
    struct NatConfig natConfig;
    natConfig.addr = privIpAddr;
    natConfig.port = (uint16_t)privPort;
    natConfig.globalPort = (uint16_t)globPort;
    natConfig.proto = (uint8_t)proto;

    // Callback
    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "removeStaticNatEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    natMgr->removeStaticNatEntry(profileId, natConfig, respCb);
}

void DataMenu::requestStaticNatEntries(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::INatManager> natMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "List Static NAT entries\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    natMgr = dataFactory.getNatManager(opType);
    subSystemStatus = natMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\n\nNat Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = natMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    auto respCb = [](const std::vector<NatConfig> &snatEntries, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestStaticNatEntries Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (snatEntries.size() > 0) {
            std::cout << "==========================================\n";
        }
        for (auto entry : snatEntries) {
            std::cout << "Private IP address: " << entry.addr << "\nPrivate port: " << entry.port
                      << "\nGlobal port: " << entry.globalPort
                      << "\nProtocol: " << DataUtils::protocolToString(entry.proto)
                      << "\n==========================================\n";
        }
    };
    natMgr->requestStaticNatEntries(profileId, respCb);
}

telux::data::IpProtocol DataMenu::getProtcol(std::string protoStr) {
    std::string protoStrToCompare = protoStr;
    std::transform(protoStrToCompare.begin(), protoStrToCompare.end(), protoStrToCompare.begin(),
        [](unsigned char ch) { return std::tolower(ch); });

    // Lazy initialization
    if (protoMap_.empty()) {
        protoMap_["udp"] = 17;
        protoMap_["tcp"] = 6;
        protoMap_["igmp"] = 2;
        protoMap_["icmp"] = 1;
        protoMap_["esp"] = 50;
    }
    if (protoMap_.find(protoStrToCompare) != std::end(protoMap_)) {
        return protoMap_[protoStrToCompare];
    }

    std::cout << "Error: invalid protocol \n ";
    return 0;
}

void DataMenu::parseProtoInfo(std::shared_ptr<IIpFilter> filter,
    telux::data::IpProtocol protocol, int &srcPort, int &dstPort, int &srcPortRange,
        int &dstPortRange, std::string &protoStr) {

    if (protocol == PROTO_TCP) {
        auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(filter);
        TcpInfo tcpInfo = tcpFilter->getTcpInfo();
        srcPort = tcpInfo.src.port;
        srcPortRange = tcpInfo.src.range;
        dstPort = tcpInfo.dest.port;
        dstPortRange = tcpInfo.dest.range;
        protoStr = "TCP";
    } else if (protocol == PROTO_UDP) {
        auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(filter);
        UdpInfo udpInfo = udpFilter->getUdpInfo();
        srcPort = udpInfo.src.port;
        srcPortRange = udpInfo.src.range;
        dstPort = udpInfo.dest.port;
        dstPortRange = udpInfo.dest.range;
        protoStr = "UDP";
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


void DataMenu::setFirewall(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool subSystemStatus = false;
    bool fwEnable = false;
    bool allowPackets = false;

    std::cout << "Set Firewall\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    int enableFwFlag;
    std::cout << "Enter Enable Firewall (1 - On, 0 - Off): ";
    std::cin >> enableFwFlag;
    Utils::validateInput(enableFwFlag);
    if (enableFwFlag) {
        fwEnable = true;
    }

    int allowPacketsFlag;
    std::cout << "Enter Packets Allowed (1 - Accept, 0 - Drop): ";
    std::cin >> allowPacketsFlag;
    Utils::validateInput(allowPacketsFlag);
    if (allowPacketsFlag) {
        allowPackets = true;
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "setFirewall Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    firewallMgr->setFirewall(profileId, fwEnable, allowPackets, respCb);
}

void DataMenu::requestFirewallStatus(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "request Firewall Status\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

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

    firewallMgr->requestFirewallStatus(profileId, respCb);
}

void DataMenu::addFirewallEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool subSystemStatus = false;
    std::cout << "add Firewall Entry\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    int fwDirection;
    std::cout << "Enter Firewall Direction (1-Uplink, 2-Downlink): ";
    std::cin >> fwDirection;
    Utils::validateInput(fwDirection);
    telux::data::Direction fwDir = static_cast<telux::data::Direction>(fwDirection);

    char delimiter = '\n';
    std::string protoStr;
    std::cin.get();
    std::cout << "Enter Protocol (TCP, UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);
    telux::data::IpProtocol proto = getProtcol(protoStr);

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6): ";
    std::cin >> ipFamilyType;
    Utils::validateInput(ipFamilyType);
    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);

    std::shared_ptr<telux::data::net::IFirewallEntry> fwEntry
        = dataFactory.getNewFirewallEntry(proto, fwDir, ipFamType);

    std::shared_ptr<IIpFilter> ipFilter = fwEntry->getIProtocolFilter();

    if (fwEntry) {
        // Entry IPv4 info
        if (ipFamilyType == 4) {
            std::string srcAddr;
            std::cin.get();
            std::cout << "Enter IPv4 Source address: ";
            std::getline(std::cin, srcAddr, delimiter);

            std::string srcSubnetMask;
            std::cout << "Enter IPv4 Source subnet mask: ";
            std::getline(std::cin, srcSubnetMask, delimiter);

            std::string destAddr;
            std::cout << "Enter IPv4 Destination address: ";
            std::getline(std::cin, destAddr, delimiter);

            std::string destSubnetMask;
            std::cout << "Enter IPv4 Destination subnet mask: ";
            std::getline(std::cin, destSubnetMask, delimiter);

            int tosVal;
            std::cout << "Enter Type of service value: ";
            std::cin >> tosVal;
            Utils::validateInput(tosVal);

            int tosMask;
            std::cout << "Enter Type of service mask: ";
            std::cin >> tosMask;
            Utils::validateInput(tosMask);

            IPv4Info info;
            info.srcAddr = srcAddr;
            info.srcSubnetMask = srcSubnetMask;
            info.destAddr = destAddr;
            info.destSubnetMask = destSubnetMask;
            info.value = (uint8_t)tosVal;
            info.mask = (uint8_t)tosMask;
            info.nextProtoId = proto;

            ipFilter->setIPv4Info(info);
        }

        // Entry IPv6 info
        if (ipFamilyType == 6) {
            std::string srcAddr;
            std::cin.get();
            std::cout << "Enter IPv6 Source address: ";
            std::getline(std::cin, srcAddr, delimiter);

            std::string destAddr;
            std::cout << "Enter IPv6 Destination address: ";
            std::getline(std::cin, destAddr, delimiter);

            int trfVal;
            std::cout << "Enter IPv6 Traffic class value: ";
            std::cin >> trfVal;
            Utils::validateInput(trfVal);

            int trfMask;
            std::cout << "Enter IPv6 Traffic class mask: ";
            std::cin >> trfMask;
            Utils::validateInput(trfMask);

            int flowLabel;
            std::cout << "Enter IPv6 flow label : ";
            std::cin >> flowLabel;
            Utils::validateInput(flowLabel);

            int natEnabled;
            std::cout << "Enter IPv6 nat enabled (1-Enable, 0-Disabled): ";
            std::cin >> natEnabled;
            Utils::validateInput(natEnabled);

            IPv6Info info;
            info.srcAddr = srcAddr;
            info.destAddr = destAddr;
            info.nextProtoId = proto;
            info.val = (uint8_t)trfVal;
            info.mask = (uint8_t)trfMask;
            info.flowLabel = (uint32_t)flowLabel;
            info.natEnabled = (uint8_t)natEnabled;

            ipFilter->setIPv6Info(info);
        }

        switch (proto) {
        case 6:  // TCP
        {
            TcpInfo tcpInfo;
            int srcPort;
            std::cout << "Enter TCP source port: ";
            std::cin >> srcPort;
            Utils::validateInput(srcPort);

            int srcRange;
            std::cout << "Enter TCP source range: ";
            std::cin >> srcRange;
            Utils::validateInput(srcRange);

            int destPort;
            std::cout << "Enter TCP destination port: ";
            std::cin >> destPort;
            Utils::validateInput(destPort);

            int destRange;
            std::cout << "Enter TCP destination range: ";
            std::cin >> destRange;
            Utils::validateInput(destRange);

            tcpInfo.src.port = (uint16_t)srcPort;
            tcpInfo.src.range = (uint16_t)srcRange;
            tcpInfo.dest.port = (uint16_t)destPort;
            tcpInfo.dest.range = (uint16_t)destRange;

            auto tcpFilter = std::dynamic_pointer_cast<ITcpFilter>(ipFilter);
            if(tcpFilter) {
                tcpFilter->setTcpInfo(tcpInfo);
            }
        } break;
        case 17:  // UDP
        {
            UdpInfo info;
            int srcPort;
            std::cout << "Enter UDP source port: ";
            std::cin >> srcPort;
            Utils::validateInput(srcPort);

            int srcRange;
            std::cout << "Enter UDP source range: ";
            std::cin >> srcRange;
            Utils::validateInput(srcRange);

            int destPort;
            std::cout << "Enter UDP destination port: ";
            std::cin >> destPort;
            Utils::validateInput(destPort);

            int destRange;
            std::cout << "Enter UDP destination range: ";
            std::cin >> destRange;
            Utils::validateInput(destRange);

            info.src.port = (uint16_t)srcPort;
            info.src.range = (uint16_t)srcRange;
            info.dest.port = (uint16_t)destPort;
            info.dest.range = (uint16_t)destRange;

            auto udpFilter = std::dynamic_pointer_cast<IUdpFilter>(ipFilter);
            if(udpFilter) {
                udpFilter->setUdpInfo(info);
            }
        } break;
        default:
            break;
        }
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

    firewallMgr->addFirewallEntry(profileId, fwEntry, respCb);
}

void DataMenu::requestFirewallEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "request Firewall Entry\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    auto respCb = [this](
        std::vector<std::shared_ptr<IFirewallEntry>> entries, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestFirewallEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        std::cout << "Found " << entries.size() << " entries\n";
        this->fwEntries_ = entries;
        this->displayFirewallEntry();
    };

    firewallMgr->requestFirewallEntry(profileId, respCb);
}

void DataMenu::displayFirewallEntry() {
    std::cout << std::setw(2)
        << "+------------------------------------------------------------------"
        << "-------------------------------------------------------------------"
        << "-+"
        << std::endl;
    std::cout << "| ID# | "
        << "Direction | "
        << "IPv4 Src Address | "
        << "       IPv6 Src Address        | "
        << "Protocol | "
        << "Src Port | "
        << "Src PortRange | "
        << "Dst Port | "
        << "Dst PortRange | " << std::endl;
    std::cout << std::setw(2)
        << "+------------------------------------------------------------------"
        << "-------------------------------------------------------------------"
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
        std::cout << std::left << std::setw(2) << "  " << std::setw(5) << i + 1
            << "  " << std::setw(12) << dir
            << "  " << std::setw(18) << ipv4Info.srcAddr
            << std::setw(32) << ipv6Info.srcAddr << "  "
            << std::setw(9) << protoStr<< " "
            << std::setw(9) << srcPort << " "
            << std::setw(14) << srcPortRange << " "
            << std::setw(12) << destPort << " "
            << std::setw(18) << dstPortRange << std::endl;
    }
}

void DataMenu::removeFirewallEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "remove Firewall Entry\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    int fwDirection;
    std::cout << "Enter Firewall Direction (1-Uplink, 2-Downlink): ";
    std::cin >> fwDirection;
    Utils::validateInput(fwDirection);
    telux::data::Direction fwDir = static_cast<telux::data::Direction>(fwDirection);

    char delimiter = '\n';
    std::string protoStr;
    std::cin.get();
    std::cout << "Enter Protocol (TCP, UDP, ICMP, ESP): ";
    std::getline(std::cin, protoStr, delimiter);
    telux::data::IpProtocol proto = getProtcol(protoStr);

    int ipFamilyType;
    std::cout << "Enter Ip Family (4-IPv4, 6-IPv6): ";
    std::cin >> ipFamilyType;
    std::cout << ipFamilyType;
    Utils::validateInput(ipFamilyType);
    telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "removeFirewallEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    auto iter = std::find_if(
        std::begin(fwEntries_), std::end(fwEntries_), [=](std::shared_ptr<IFirewallEntry> e) {
            return (e->getDirection() == fwDir && e->getIpFamilyType() == ipFamType
                    && e->getIProtocolFilter()->getIpProtocol() == proto);
        });
    if (iter != std::end(fwEntries_)) {
        firewallMgr->removeFirewallEntry(profileId, *iter, respCb);
    } else {
        std::cout << " Invalid input, execute remove_firewall_entry command \n";
    }
}

void DataMenu::enableDmz(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Add DMZ\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

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
    firewallMgr->enableDmz(profileId, ipAddr, respCb);
}

void DataMenu::disableDmz(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Remove DMZ\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

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
    firewallMgr->disableDmz(profileId, static_cast<telux::data::IpFamilyType>(ipType), respCb);
}

void DataMenu::requestDmzEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "request Dmz Entries\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    auto respCb = [](std::vector<std::string> dmzEntries, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "requestDmzEntry Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;

        if (dmzEntries.size() > 0) {
            std::cout << "=============================================\n";
        }
        for (auto entry : dmzEntries) {
            std::cout << "address: " << entry
                      << "\n=============================================\n";
        }
    };

    firewallMgr->requestDmzEntry(profileId, respCb);
}

void DataMenu::createVlan(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Create VLAN \n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    vlanMgr = dataFactory.getVlanManager(opType);
    subSystemStatus = vlanMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nVLAN Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = vlanMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    int ifaceType;
    std::cout << "Enter Interface Type\n (1-WLAN, 2-ETH, 3-ECM, 4-RNDIS, 5-MHI): ";
    std::cin >> ifaceType;
    Utils::validateInput(ifaceType);
    telux::data::InterfaceType infType = static_cast<telux::data::InterfaceType>(ifaceType);

    int vlanId;
    std::cout << "Enter VLAN Id: ";
    std::cin >> vlanId;
    Utils::validateInput(vlanId);

    int acc;
    std::cout << "Enter acceleration  (0-false, 1-true): ";
    std::cin >> acc;
    Utils::validateInput(acc);
    bool isAccelerated = false;
    if (acc) {
        isAccelerated = true;
    }

    auto respCb = [](bool isAccelerated, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "createVlan Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        std::cout << "Acceleration " << (isAccelerated ? "is allowed" : "is not allowed") << "\n";
    };

    VlanConfig config;
    config.iface = infType;
    config.vlanId = vlanId;
    config.isAccelerated = isAccelerated;

    vlanMgr->createVlan(config, respCb);
}
void DataMenu::removeVlan(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Remove VLAN \n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    vlanMgr = dataFactory.getVlanManager(opType);
    subSystemStatus = vlanMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nVLAN Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = vlanMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    int ifaceType;
    std::cout << "Enter Interface Type\n (1-WLAN, 2-ETH, 3-ECM, 4-RNDIS, 5-MHI): ";
    std::cin >> ifaceType;
    Utils::validateInput(ifaceType);
    telux::data::InterfaceType infType = static_cast<telux::data::InterfaceType>(ifaceType);

    int vlanId;
    std::cout << "Enter VLAN Id: ";
    std::cin >> vlanId;
    Utils::validateInput(vlanId);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "removeVlan Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };
    vlanMgr->removeVlan(vlanId, infType, respCb);
}
void DataMenu::queryVlanInfo(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Query VLAN info\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    vlanMgr = dataFactory.getVlanManager(opType);
    subSystemStatus = vlanMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nVLAN Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = vlanMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    auto respCb = [](const std::vector<VlanConfig> &configs, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "queryVlanInfo Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        for (auto c : configs) {
            std::cout << "iface: " << (int)c.iface << ", vlanId: " << c.vlanId
                      << ", accelerated: " << (int)c.isAccelerated << "\n";
        }
    };

    vlanMgr->queryVlanInfo(respCb);
}

void DataMenu::enableSocks(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::ISocksManager> socksMgr;
    int operationType;
    int enableEntry;
    bool subSystemStatus = false;

    std::cout << "Enable/Disable Socks Proxy\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    std::cout << "Enter Enablement Type (0-Disable, 1-Enable): ";
    std::cin >> enableEntry;
    Utils::validateInput(enableEntry);
    if (enableEntry < 0 || enableEntry >1) {
        std::cout << "Invalid Entry. Please try again ...\n";
        return;
    }
    bool enablement = (enableEntry == 0 ? false : true);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    socksMgr = dataFactory.getSocksManager(opType);
    subSystemStatus = socksMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nSocksManager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = socksMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "enableSocks Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    socksMgr->enableSocks(enablement,respCb);
}

void DataMenu::bridgeMenu(std::vector<std::string> userInput) {
    BridgeMenu bridgeMenu("Software Bridge Menu", "bridge> ");
    if(0 == bridgeMenu.init()) {
        bridgeMenu.mainLoop();
    }
    ConsoleApp::displayMenu();
}

void DataMenu::bindWithProfile(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Bind with profile\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    vlanMgr = dataFactory.getVlanManager(opType);
    subSystemStatus = vlanMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nVLAN Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = vlanMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int vlanId;
    std::cout << "Enter Vlan Id: ";
    std::cin >> vlanId;
    Utils::validateInput(vlanId);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "bindWithProfile Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    vlanMgr->bindWithProfile(profileId, vlanId, respCb);
}

void DataMenu::unbindFromProfile(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Unbind with profile\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    vlanMgr = dataFactory.getVlanManager(opType);
    subSystemStatus = vlanMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nVLAN Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = vlanMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    int vlanId;
    std::cout << "Enter Vlan Id: ";
    std::cin >> vlanId;
    Utils::validateInput(vlanId);

    auto respCb = [](telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "bindWithProfile Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    };

    vlanMgr->unbindFromProfile(profileId, vlanId, respCb);
}

void DataMenu::queryVlanMappingList(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Query VLAN Mapping List\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    vlanMgr = dataFactory.getVlanManager(opType);
    subSystemStatus = vlanMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nVLAN Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = vlanMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }

    auto respCb = [](
        const std::list<std::pair<int, int>> &mapping, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                  << "queryVlanMappingList Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error)
                  << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        for (auto c : mapping) {
            std::cout << "profId: " << (int)c.first << ", vlanId: " << c.second << "\n";
        }
    };

    vlanMgr->queryVlanMappingList(respCb);
}
