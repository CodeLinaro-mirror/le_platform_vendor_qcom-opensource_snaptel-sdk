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

#include "DataProfileMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

DataProfileMenu::DataProfileMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DataProfileMenu::~DataProfileMenu() {
    myDataProfileListCb_.clear();
    myDataProfileListCbForQuery_.clear();
    myDataCreateProfileCb_.clear();
    myDataProfileCb_.clear();
    myDeleteProfileCb_.clear();
    myModifyProfileCb_.clear();
    myDataProfileCbForGetProfileById_.clear();

    for (auto& profMgr : dataProfileManagerMap_) {
        profMgr.second->deregisterListener(profileListeners_[profMgr.first]);
    }
    dataProfileManagerMap_.clear();
    profileListeners_.clear();
}

bool DataProfileMenu::init() {
    bool dpmSubSystemStatus = initDataProfileManagerAndListener(DEFAULT_SLOT_ID);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        dpmSubSystemStatus |= initDataProfileManagerAndListener(SLOT_ID_2);
    }

    std::shared_ptr<ConsoleAppCommand> reqProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "request_profile_list", {},
            std::bind(&DataProfileMenu::requestProfileList, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> createProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "create_profile", {},
            std::bind(&DataProfileMenu::createProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> deleteProfileMenu = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("3", "delete_profile",
            {"slotId (1-Primary, 2-Secondary)", "profileId", "techPref (0-3GPP, 1-3GPP2)"},
            std::bind(&DataProfileMenu::deleteProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> modifyProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "modify_profile", {},
            std::bind(&DataProfileMenu::modifyProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> queryProfileMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "query_profile", {},
            std::bind(&DataProfileMenu::queryProfile, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> requestProfileByIdMenu
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "request_profile_by_id",
            {"slotId (1-Primary, 2-Secondary)", "profileId", "techPref (0-3GPP, 1-3GPP2)"},
            std::bind(&DataProfileMenu::requestProfileById, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {reqProfile, createProfileMenu,
        deleteProfileMenu, modifyProfileMenu, queryProfileMenu, requestProfileByIdMenu};

    addCommands(commandsList);

    return dpmSubSystemStatus;
}

void DataProfileMenu::displayMenu() {
    bool subSystemStatus = false;
    if (dataProfileManagerMap_.find(DEFAULT_SLOT_ID) != dataProfileManagerMap_.end()) {
        subSystemStatus = dataProfileManagerMap_[DEFAULT_SLOT_ID]->isSubsystemReady();
        if (subSystemStatus) {
            std::cout << "\nData Profile Manager on slot "<< DEFAULT_SLOT_ID <<
            " is ready" << std::endl;
        }
        else {
            std::cout << "\nData Profile Manager on slot "<< DEFAULT_SLOT_ID <<
            " is not ready" << std::endl;
        }
    }
    if (dataProfileManagerMap_.find(SLOT_ID_2) != dataProfileManagerMap_.end()) {
        subSystemStatus = dataProfileManagerMap_[SLOT_ID_2]->isSubsystemReady();
        if (subSystemStatus) {
            std::cout << "\nData Profile Manager on slot "<< SLOT_ID_2 <<
            " is ready" << std::endl;
        }
        else {
            std::cout << "\nData Profile Manager on slot "<< SLOT_ID_2 <<
            " is not ready" << std::endl;
        }
    }
    ConsoleApp::displayMenu();
}

bool DataProfileMenu::initDataProfileManagerAndListener(SlotId slotId) {
    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto profMgr = dataFactory.getDataProfileManager(slotId);

    if (profMgr) {
        // Check if data subsystem is ready
        bool subSystemStatus = profMgr->isSubsystemReady();

        // If data subsystem is not ready, wait for it to be ready
        if (!subSystemStatus) {
            std::cout << "\n\nInitializing Data profile manager subsystem on slot " <<
            slotId << ", Please wait ..." << endl;
            std::future<bool> f = profMgr->onSubsystemReady();
            // Wait unconditionally for data subsystem to be ready
            subSystemStatus = f.get();
        }
        if (subSystemStatus) {
            std::cout << "\nData Profile Manager on slot "<< slotId << " is ready" << std::endl;
        }
        else {
            std::cout << "\nData Profile Manager on slot "<< slotId << " is not ready" << std::endl;
            return false;
        }

        //If this is newly created Manager
        if (dataProfileManagerMap_.find(slotId) == dataProfileManagerMap_.end()) {
            dataProfileManagerMap_.emplace(slotId, profMgr);
            myDataProfileListCb_.emplace(slotId, std::make_shared<MyDataProfilesCallback>());
            myDataProfileListCbForQuery_.emplace(slotId,
                                                 std::make_shared<MyDataProfilesCallback>());
            myDataCreateProfileCb_.emplace(slotId, std::make_shared<MyDataCreateProfileCallback>());
            myDataProfileCb_.emplace(slotId, std::make_shared<MyDataProfileCallback>());
            myDeleteProfileCb_.emplace(slotId, std::make_shared<MyDeleteProfileCallback>());
            myModifyProfileCb_.emplace(slotId, std::make_shared<MyModifyProfileCallback>());
            myDataProfileCbForGetProfileById_.emplace(slotId,
                                                      std::make_shared<MyDataProfileCallback>());
            profileListeners_.emplace(slotId, std::make_shared<MyProfileListener>());

            telux::common::Status status =
                dataProfileManagerMap_[slotId]->registerListener(profileListeners_[slotId]);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Unable to register data profile manager listener on slot " <<
                slotId << std::endl;
            }
        }
        return subSystemStatus;
    } else {
        std::cout << "Profile manager instance is NULL" << std::endl;
        return false;
    }
}

void DataProfileMenu::getProfileParamsFromUser() {
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

void DataProfileMenu::requestProfileList(std::vector<std::string> inputCommand) {
    std::cout << "\nRequest Profile List" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->requestProfileList(
            myDataProfileListCb_[static_cast<SlotId>(slotId)]);

    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request profile list sent successfully" << std::endl;
    } else {
        std::cout << "Request profile list failed, status:" << int(status) << std::endl;
    }
}

void DataProfileMenu::createProfile(std::vector<std::string> inputCommand) {
    std::cout << "\nCreate Profile Request" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    getProfileParamsFromUser();

    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->createProfile(
        params_, myDataCreateProfileCb_[static_cast<SlotId>(slotId)]);

    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Create profile request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send create profile request, Status:" << int(status) << std::endl;
    }
}

void DataProfileMenu::deleteProfile(std::vector<std::string> inputCommand) {
    int slotId, profileId, techPrefId;
    try {
        slotId = std::stoi(inputCommand[1]);
        profileId = std::stoi(inputCommand[2]);
        techPrefId = std::stoi(inputCommand[3]);
    } catch (const std::exception &e) {
        std::cout << "ERROR: Invalid input, please enter numerical values " << std::endl;
        return;
    }
    if (slotId != SLOT_ID_1 && slotId != SLOT_ID_2) {
        std::cout << "Invalid slot id"  << std::endl;
        std::cin.get();
        return;
    }
    std::cout << "\nDeleting Profile " << profileId << " on slotId " << slotId << std::endl;
    telux::data::TechPreference tp = telux::data::TechPreference::UNKNOWN;
    if (techPrefId == 0) {
        tp = telux::data::TechPreference::TP_3GPP;
    } else if (techPrefId == 1) {
        tp = telux::data::TechPreference::TP_3GPP2;
    }
    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->deleteProfile(
        profileId, tp, myDeleteProfileCb_[static_cast<SlotId>(slotId)]);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Delete profile request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send delete profile request, Status:" << int(status) << std::endl;
    }
}

void DataProfileMenu::modifyProfile(std::vector<std::string> inputCommand) {
    std::cout << "\nModify Profile Request" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    int profileId;
    std::cout << "Enter profile Id to Modify : ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    getProfileParamsFromUser();

    telux::common::Status status
        = dataProfileManagerMap_[static_cast<SlotId>(slotId)]->modifyProfile(
            profileId, params_, myModifyProfileCb_[static_cast<SlotId>(slotId)]);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Modify profile request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send Modify profile request, Status:" << int(status) << std::endl;
    }
}

void DataProfileMenu::queryProfile(std::vector<std::string> inputCommand) {
    std::cout << "\nQuery Profile Request" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

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

    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->queryProfile(
        params_, myDataProfileListCbForQuery_[static_cast<SlotId>(slotId)]);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Query profile request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send Query profile request, Status:" << int(status) << std::endl;
    }
}

void DataProfileMenu::requestProfileById(std::vector<std::string> inputCommand) {
    int slotId, profileId, techPrefId;
    try {
        slotId = std::stoi(inputCommand[1]);
        profileId = std::stoi(inputCommand[2]);
        techPrefId = std::stoi(inputCommand[3]);
    } catch (const std::exception &e) {
        std::cout << "ERROR: Invalid input, please enter numerical values " << std::endl;
        return;
    }
    if (slotId != SLOT_ID_1 && slotId != SLOT_ID_2) {
        std::cout << "Invalid slot id"  << std::endl;
        std::cin.get();
        return;
    }

    std::cout << "\nRequest Profile By Id " << profileId << " on slotId " << slotId << std::endl;
    telux::data::TechPreference tp = telux::data::TechPreference::UNKNOWN;
    if (techPrefId == 0) {
        tp = telux::data::TechPreference::TP_3GPP;
    } else if (techPrefId == 1) {
        tp = telux::data::TechPreference::TP_3GPP2;
    }
    telux::common::Status status =
        dataProfileManagerMap_[static_cast<SlotId>(slotId)]->requestProfile(
        profileId, tp, myDataProfileCbForGetProfileById_[static_cast<SlotId>(slotId)]);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request profile by ID request sent successfully" << std::endl;
    } else {
        std::cout << "Failed to send Request profile by ID request, Status:" << int(status)
                  << std::endl;
    }
}
