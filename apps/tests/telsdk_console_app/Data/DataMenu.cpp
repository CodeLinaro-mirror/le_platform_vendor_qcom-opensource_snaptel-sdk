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

#include "../../common/utils/Utils.hpp"

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

    if (profileListener_) {
        profileListener_ = nullptr;
    }

    if (dataListener_) {
        dataListener_ = nullptr;
    }
}

bool DataMenu::initializeSDK() {
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();

    dataConnectionManager_ = telux::data::DataFactory::getInstance().getDataConnectionManager();

    // Check if data subsystem is ready
    bool dcmSubSystemStatus = dataConnectionManager_->isSubsystemReady();
    dataListener_ = std::make_shared<DataListener>();
    if (dataListener_ == nullptr) {
        std::cout << "ERROR - Unable to allocate listeners .. terminate application" << std::endl;
        exit(1);
    }
    dataConnectionManager_->registerListener(dataListener_);

    // If data subsystem is not ready, wait for it to be ready
    if (!dcmSubSystemStatus) {
        std::cout << "\n\nData subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = dataConnectionManager_->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        dcmSubSystemStatus = f.get();
    }

    dataProfileManager_ = dataFactory.getDataProfileManager();

    // Check if data subsystem is ready
    bool dpmSubSystemStatus = dataProfileManager_->isSubsystemReady();

    // If data subsystem is not ready, wait for it to be ready
    if (!dpmSubSystemStatus) {
        std::cout << "\n\nData profile manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = dataProfileManager_->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        dpmSubSystemStatus = f.get();
    }

    // Check if the SDK is able to initialize data subsystems
    if ((dcmSubSystemStatus) && (dpmSubSystemStatus)) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                  << std::endl;
    } else {
        std::cout << "Unable to initialize subSystem" << std::endl;
    }

    if ((dcmSubSystemStatus) && (dpmSubSystemStatus)) {
        //Update dataListener_'s data call list
        requestDataCallList(OperationType::DATA_LOCAL,
                            std::bind(&DataListener::initDataCallListResponseCb, dataListener_,
                                        std::placeholders::_1, std::placeholders::_2));
        requestDataCallList(OperationType::DATA_REMOTE,
                            std::bind(&DataListener::initDataCallListResponseCb, dataListener_,
                                        std::placeholders::_1, std::placeholders::_2));
    }

    myDataProfileListCb_ = std::make_shared<MyDataProfilesCallback>();
    myDataProfileListCbForQuery_ = std::make_shared<MyDataProfilesCallback>();
    myDataCreateProfileCb_ = std::make_shared<MyDataCreateProfileCallback>();
    myDataProfileCb_ = std::make_shared<MyDataProfileCallback>();
    myDeleteProfileCb_ = std::make_shared<MyDeleteProfileCallback>();
    myModifyProfileCb_ = std::make_shared<MyModifyProfileCallback>();
    myDataProfileCbForGetProfileById_ = std::make_shared<MyDataProfileCallback>();
    profileListener_ = std::make_shared<MyProfileListener>();

    telux::common::Status status = dataProfileManager_->registerListener(profileListener_);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Unable to register data profile manager listener" << std::endl;
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
            "6", "set_default_profile", {}, std::bind(&DataMenu::setDefaultProfile, this)));

    std::shared_ptr<ConsoleAppCommand> dataFilterMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Data_Filter",
            {}, std::bind(&DataMenu::openDataFilterMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> reqStaticNatEntries
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "request_static_nat_entries",
            {}, std::bind(&DataMenu::requestStaticNatEntries, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> addStaticNatEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "add_static_nat", {},
            std::bind(&DataMenu::addStaticNatEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> removeStaticNatEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "remove_static_nat", {},
            std::bind(&DataMenu::removeStaticNatEntry, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> requestFirewallStatus
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11", "request_firewall_status", {},
            std::bind(&DataMenu::requestFirewallStatus, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setFirewall
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("12", "set_firewall", {},
            std::bind(&DataMenu::setFirewall, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> addFirewallEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("13", "add_firewall_entry", {},
            std::bind(&DataMenu::addFirewallEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestFirewallEntries
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("14", "request_firewall_entry", {},
            std::bind(&DataMenu::requestFirewallEntries, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> removeFirewallEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("15", "remove_firewall_entry", {},
            std::bind(&DataMenu::removeFirewallEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> enableDmz
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "16", "enable_dmz", {}, std::bind(&DataMenu::enableDmz, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> disableDmz
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "17", "disable_dmz",{}, std::bind(&DataMenu::disableDmz, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestDmzEntry
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("18", "request_dmz_entry", {},
            std::bind(&DataMenu::requestDmzEntry, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> createVlan
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("19", "create_vlan", {},
            std::bind(&DataMenu::createVlan, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> removeVlan
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("20", "remove_vlan", {},
            std::bind(&DataMenu::removeVlan, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> queryVlanInfo
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("21", "query_vlan_info", {},
            std::bind(&DataMenu::queryVlanInfo, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> bindWithProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("22", "bind_with_profile", {},
            std::bind(&DataMenu::bindWithProfile, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> unbindFromProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("23", "unbind_from_profile", {},
            std::bind(&DataMenu::unbindFromProfile, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> queryVlanMappingList
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("24", "query_vlan_mapping_list",
            {}, std::bind(&DataMenu::queryVlanMappingList, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> enableSocks
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("25", "socks_enablement",
            {}, std::bind(&DataMenu::enableSocks, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> bridgeMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("26", "Bridge_Menu",
            {}, std::bind(&DataMenu::bridgeMenu, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> l2tpMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("31", "L2tp_Menu",
            {}, std::bind(&DataMenu::l2tpMenu, this, std::placeholders::_1)));

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
        reqDataCallStats, resetDataCallStats, reqDataCallList, setDefaultProfile, dataFilterMenu,
        reqStaticNatEntries, addStaticNatEntry, removeStaticNatEntry, requestFirewallStatus,
        setFirewall, addFirewallEntry, requestFirewallEntries, removeFirewallEntry, enableDmz,
        disableDmz, requestDmzEntry, createVlan, removeVlan, queryVlanInfo, bindWithProfile,
        unbindFromProfile, queryVlanMappingList, enableSocks, bridgeMenuCommand, l2tpMenuCommand,
        reqProfile, createProfileMenu, deleteProfileMenu, modifyProfileMenu, queryProfileMenu,
        requestProfileByIdMenu};

    addCommands(commandsList);

    if (DataMenu::initializeSDK()) {
        ConsoleApp::displayMenu();
    }
}

void DataMenu::openDataFilterMenu(std::vector<std::string> userInput) {
    DataFilterMenu dataFilterMenu("Data Filter Menu", "data_filter> ");
    dataFilterMenu.init();
    dataFilterMenu.mainLoop();
    ConsoleApp::displayMenu();
}

void DataMenu::startDataCall(std::vector<std::string> inputCommand) {
    std::cout << "\nStart data call" << std::endl;
    telux::common::Status retStat;
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
    retStat = dataConnectionManager_->startDataCall(profileId, ipFamType,
        MyDataCallResponseCallback::startDataCallResponseCallBack, opType);
    Utils::printStatus(retStat);
}

void DataMenu::stopDataCall(std::vector<std::string> inputCommand) {
    std::cout << "\nStop data call" << std::endl;
    telux::common::Status retStat;
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
    retStat = dataConnectionManager_->stopDataCall(profileId, ipFamType,
        MyDataCallResponseCallback::stopDataCallResponseCallBack, opType);
    Utils::printStatus(retStat);
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
    telux::common::Status retStat;
    if (dataConnectionManager_) {
        telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
        retStat = dataConnectionManager_->requestDataCallList(opType,cb);
        Utils::printStatus(retStat);
    }
}

void DataMenu::requestDataCallList() {
    std::cout << "\nRequest DataCall List" << std::endl;
    telux::common::Status retStat;
    if (dataConnectionManager_) {
        int operationType;
        std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
        std::cin >> operationType;
        Utils::validateInput(operationType);

        telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
        retStat = dataConnectionManager_->requestDataCallList(
            opType,MyDataCallResponseCallback::dataCallListResponseCb);
        Utils::printStatus(retStat);
    }
}

void DataMenu::setDefaultProfile() {
    std::cout << "\nSet Default Profile" << std::endl;
    telux::common::Status retStat;
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

        retStat = dataConnectionManager_->setDefaultProfile(opType, profileId, respCb);
        Utils::printStatus(retStat);
    }
}
void DataMenu::commandCallback(ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Command initiated successfully " << std::endl;
    } else {
        std::cout << " Command failed." << std::endl;
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
    if (techPrefId == 0) {
        tp = telux::data::TechPreference::TP_3GPP;
    } else if (techPrefId == 1) {
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
    telux::common::Status retStat;
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

    retStat = natMgr->addStaticNatEntry(profileId, natConfig, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::removeStaticNatEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::INatManager> natMgr;
    telux::common::Status retStat;
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

    retStat = natMgr->removeStaticNatEntry(profileId, natConfig, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::requestStaticNatEntries(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::INatManager> natMgr;
    telux::common::Status retStat;
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
    retStat = natMgr->requestStaticNatEntries(profileId, respCb);
    Utils::printStatus(retStat);
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
        protoMap_["tcp_udp"] = 253;
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

std::shared_ptr<telux::data::net::IFirewallManager>
    DataMenu::getFirewallManagerInstance(telux::data::OperationType opType) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr = nullptr;
    bool subSystemStatus = false;

    auto &dataFactory = telux::data::DataFactory::getInstance();
    firewallMgr = dataFactory.getFirewallManager(opType);
    subSystemStatus = firewallMgr->isSubsystemReady();
    if (not subSystemStatus) {
        std::cout << "\nFirewall Manager subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = firewallMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }
    return firewallMgr;
}

void DataMenu::setFirewall(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    bool fwEnable = false;
    bool allowPackets = false;
    telux::common::Status retStat;

    std::cout << "Set Firewall\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    firewallMgr = getFirewallManagerInstance(opType);
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

    retStat = firewallMgr->setFirewall(profileId, fwEnable, allowPackets, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::requestFirewallStatus(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    telux::common::Status retStat;

    std::cout << "request Firewall Status\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    firewallMgr = getFirewallManagerInstance(opType);
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

    retStat = firewallMgr->requestFirewallStatus(profileId, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::getIPV4ParamsFromUser(telux::data::IpProtocol proto,
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

void DataMenu::getIPV6ParamsFromUser(telux::data::IpProtocol proto,
    std::shared_ptr<IIpFilter> ipFilter, std::shared_ptr<IIpFilter> ipFilterTcpUdp) {
    std::string srcAddr = "", destAddr = "";
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
        std::cout << "Enter IPv6 Destination address: ";
        std::getline(std::cin, destAddr, delimiter);
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

void DataMenu::getProtocolParamsFromUser(std::string proto, std::string &srcPort,
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

void DataMenu::getProtocolParams(telux::data::IpProtocol proto,
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

void DataMenu::addFirewallEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    telux::common::Status retStat;
    std::cout << "add Firewall Entry\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    firewallMgr = getFirewallManagerInstance(opType);
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
    telux::data::IpProtocol proto = getProtcol(protoStr);

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

    retStat = firewallMgr->addFirewallEntry(profileId, fwEntry, respCb);
    Utils::printStatus(retStat);

    if (proto == 253) {
        retStat = firewallMgr->addFirewallEntry(profileId, fwEntryTcpUdp, respCb);
        Utils::printStatus(retStat);
    }
}

void DataMenu::requestFirewallEntries(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    telux::common::Status retStat;

    std::cout << "request Firewall Entry\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    firewallMgr = getFirewallManagerInstance(opType);
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

    retStat = firewallMgr->requestFirewallEntries(profileId, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::displayFirewallEntry() {
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
        std::cout << std::left << std::setw(2) << "  " << std::setw(13)
            << fwEntries_[i]->getHandle()
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
    telux::common::Status retStat;

    std::cout << "remove Firewall Entry\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    firewallMgr = getFirewallManagerInstance(opType);
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

    retStat = firewallMgr->removeFirewallEntry(profileId, entryHandle, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::enableDmz(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    telux::common::Status retStat;

    std::cout << "Add DMZ\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    firewallMgr = getFirewallManagerInstance(opType);
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
    retStat = firewallMgr->enableDmz(profileId, ipAddr, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::disableDmz(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    telux::common::Status retStat;

    std::cout << "Remove DMZ\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    firewallMgr = getFirewallManagerInstance(opType);
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
    retStat = firewallMgr->disableDmz(profileId, static_cast<telux::data::IpFamilyType>(ipType), respCb);
    Utils::printStatus(retStat);
}

void DataMenu::requestDmzEntry(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IFirewallManager> firewallMgr;
    int operationType;
    telux::common::Status retStat;

    std::cout << "request Dmz Entries\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    firewallMgr = getFirewallManagerInstance(opType);
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

    retStat = firewallMgr->requestDmzEntry(profileId, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::createVlan(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
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

    retStat = vlanMgr->createVlan(config, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::removeVlan(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
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
    retStat = vlanMgr->removeVlan(vlanId, infType, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::queryVlanInfo(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    telux::common::Status retStat;
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
        if (configs.size() == 0) {
            std::cout << "No VLAN Entries Configured" << "\n";
        } else {
            for (auto c : configs) {
                std::cout << "iface: " << (int)c.iface << ", vlanId: " << c.vlanId
                    << ", accelerated: " << (int)c.isAccelerated << "\n";
            }
        }
    };

    retStat = vlanMgr->queryVlanInfo(respCb);
    Utils::printStatus(retStat);
}

void DataMenu::enableSocks(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::ISocksManager> socksMgr;
    telux::common::Status retStat;
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

    retStat = socksMgr->enableSocks(enablement,respCb);
    Utils::printStatus(retStat);
}

void DataMenu::bridgeMenu(std::vector<std::string> userInput) {
    BridgeMenu bridgeMenu("Software Bridge Menu", "bridge> ");
    if(0 == bridgeMenu.init()) {
        bridgeMenu.mainLoop();
    }
    ConsoleApp::displayMenu();
}

void DataMenu::l2tpMenu(std::vector<std::string> userInput) {
    L2tpMenu l2tpMenu("L2TP Menu", "l2tp> ");
    if(0 == l2tpMenu.init()) {
        l2tpMenu.mainLoop();
    }
    ConsoleApp::displayMenu();
}

void DataMenu::bindWithProfile(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    telux::common::Status retStat;
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

    retStat = vlanMgr->bindWithProfile(profileId, vlanId, respCb);
    Utils::printStatus(retStat);
}

void DataMenu::unbindFromProfile(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    telux::common::Status retStat;
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

   retStat = vlanMgr->unbindFromProfile(profileId, vlanId, respCb);
   Utils::printStatus(retStat);
}

void DataMenu::queryVlanMappingList(std::vector<std::string> inputCommand) {
    std::shared_ptr<telux::data::net::IVlanManager> vlanMgr;
    telux::common::Status retStat;
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

    retStat = vlanMgr->queryVlanMappingList(respCb);
    Utils::printStatus(retStat);
}
