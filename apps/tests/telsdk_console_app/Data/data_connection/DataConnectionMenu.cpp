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

#include "DataConnectionMenu.hpp"

using namespace std;

DataConnectionMenu::DataConnectionMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DataConnectionMenu::~DataConnectionMenu() {
    for (auto& conMgr : dataConnectionManagerMap_) {
        conMgr.second->deregisterListener(dataListeners_[conMgr.first]);
    }
    dataConnectionManagerMap_.clear();
    dataListeners_.clear();
}

bool DataConnectionMenu::init() {
    bool dcmSubSystemStatus = initConnectionManagerAndListener(DEFAULT_SLOT_ID);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        dcmSubSystemStatus |= initConnectionManagerAndListener(SLOT_ID_2);
    }

    std::shared_ptr<ConsoleAppCommand> startDataCall
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "start_data_call", {},
            std::bind(&DataConnectionMenu::startDataCall, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> stopDataCall
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "stop_data_call", {},
            std::bind(&DataConnectionMenu::stopDataCall, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> reqDataCallStats
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "request_datacall_statistics",
            {}, std::bind(&DataConnectionMenu::requestDataCallStatistics, this,
            std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> resetDataCallStats
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "reset_datacall_statistics",
            {}, std::bind(&DataConnectionMenu::resetDataCallStatistics, this,
            std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> reqDataCallList
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "request_datacall_list", {}, std::bind(static_cast<void(DataConnectionMenu::*)()>(
            &DataConnectionMenu::requestDataCallList), this)));
    std::shared_ptr<ConsoleAppCommand> setDefaultProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "6", "set_default_profile", {}, std::bind(
            &DataConnectionMenu::setDefaultProfile, this)));
    std::shared_ptr<ConsoleAppCommand> getDefaultProfile
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "7", "get_default_profile", {}, std::bind(
            &DataConnectionMenu::getDefaultProfile, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {startDataCall, stopDataCall,
        reqDataCallStats, resetDataCallStats, reqDataCallList, setDefaultProfile, getDefaultProfile};

    addCommands(commandsList);
    return dcmSubSystemStatus;
}

void DataConnectionMenu::displayMenu() {
    bool subSystemStatus = false;
    if (dataConnectionManagerMap_.find(DEFAULT_SLOT_ID) != dataConnectionManagerMap_.end()) {
        subSystemStatus = dataConnectionManagerMap_[DEFAULT_SLOT_ID]->isSubsystemReady();
        if (subSystemStatus) {
            std::cout << "\nData Connection Manager on slot "<< DEFAULT_SLOT_ID <<
            " is ready" << std::endl;
        }
        else {
            std::cout << "\nData Connection Manager on slot "<< DEFAULT_SLOT_ID <<
            " is not ready" << std::endl;
        }
    }
    if (dataConnectionManagerMap_.find(SLOT_ID_2) != dataConnectionManagerMap_.end()) {
        subSystemStatus = dataConnectionManagerMap_[SLOT_ID_2]->isSubsystemReady();
        if (subSystemStatus) {
            std::cout << "\nData Connection Manager on slot "<< SLOT_ID_2 <<
            " is ready" << std::endl;
        }
        else {
            std::cout << "\nData Connection Manager on slot "<< SLOT_ID_2 <<
            " is not ready" << std::endl;
        }
    }
    ConsoleApp::displayMenu();
}

bool DataConnectionMenu::initConnectionManagerAndListener(SlotId slotId){
    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto conMgr = telux::data::DataFactory::getInstance().getDataConnectionManager(slotId);

    // Check if data subsystem is ready
    bool subSystemStatus = conMgr->isSubsystemReady();
    if (!subSystemStatus) {
        std::cout << "\n\nInitializing Data subsystem on slot " << slotId <<
            ", Please wait ..." << std::endl;
        std::future<bool> f = conMgr->onSubsystemReady();
        // Wait unconditionally for data subsystem to be ready
        subSystemStatus = f.get();
    }
    if (subSystemStatus) {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is ready" << std::endl;
    }
    else {
        std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready" << std::endl;
    }

    //If this is newly created Manager
    if (dataConnectionManagerMap_.find(slotId) == dataConnectionManagerMap_.end()) {
        dataConnectionManagerMap_.emplace(slotId, conMgr);
        auto dataListener = std::make_shared<DataListener>();
        if (dataListener == nullptr) {
            std::cout <<
            "ERROR - Unable to allocate listeners .. terminate application" << std::endl;
            exit(1);
        }
        dataListeners_.emplace(slotId, dataListener);
        dataConnectionManagerMap_[slotId]->registerListener(dataListeners_[slotId]);

        //Update dataListener_'s data call list
        requestDataCallList(OperationType::DATA_LOCAL, slotId,
            std::bind(&DataListener::initDataCallListResponseCb, dataListeners_[slotId],
            std::placeholders::_1, std::placeholders::_2));
        requestDataCallList(OperationType::DATA_REMOTE, slotId,
            std::bind(&DataListener::initDataCallListResponseCb, dataListeners_[slotId],
            std::placeholders::_1, std::placeholders::_2));
    }
    return subSystemStatus;
}

void DataConnectionMenu::startDataCall(std::vector<std::string> inputCommand) {
    std::cout << "\nStart data call" << std::endl;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    telux::common::Status retStat = telux::common::Status::SUCCESS;
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

    retStat =
        dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->startDataCall(profileId, ipFamType,
        MyDataCallResponseCallback::startDataCallResponseCallBack, opType);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::stopDataCall(std::vector<std::string> inputCommand) {
    std::cout << "\nStop data call" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

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
    retStat =
        dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->stopDataCall(profileId, ipFamType,
        MyDataCallResponseCallback::stopDataCallResponseCallBack, opType);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::requestDataCallStatistics(std::vector<std::string> inputCommand) {
    std::cout << "\nRequest DataCall Statistics" << std::endl;

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;
    Utils::validateInput(profileId);

    auto dataCall = dataListeners_[static_cast<SlotId>(slotId)]->getDataCall(
        static_cast<SlotId>(slotId), profileId);
    if (dataCall) {
        dataCall->requestDataCallStatistics(
            &DataCallStatisticsResponseCb::requestStatisticsResponse);
    } else {
        std::cout << "Unable to find DataCall, Please start_data_call" << std::endl;
    }
}

void DataConnectionMenu::resetDataCallStatistics(std::vector<std::string> inputCommand) {
    std::cout << "\nReset DataCall Statistics" << std::endl;

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    int profileId;
    std::cout << "Enter Profile Id: ";
    std::cin >> profileId;

    auto dataCall = dataListeners_[static_cast<SlotId>(slotId)]->getDataCall(
        static_cast<SlotId>(slotId), profileId);
    if (dataCall) {
        dataCall->resetDataCallStatistics(&DataCallStatisticsResponseCb::resetStatisticsResponse);
    } else {
        std::cout << "Unable to find DataCall, Please start_data_call" << std::endl;
    }
}

void DataConnectionMenu::requestDataCallList(OperationType operationType,
    SlotId slotId, DataCallListResponseCb cb) {
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    if (dataConnectionManagerMap_[slotId]) {
        telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
        retStat = dataConnectionManagerMap_[slotId]->requestDataCallList(opType,cb);
    }
}

void DataConnectionMenu::requestDataCallList() {
    std::cout << "\nRequest DataCall List" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);

    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->requestDataCallList(
        opType,MyDataCallResponseCallback::dataCallListResponseCb);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::setDefaultProfile() {
    std::cout << "\nSet Default Profile" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

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

    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->setDefaultProfile(
        opType, profileId, respCb);
    Utils::printStatus(retStat);
}

void DataConnectionMenu::getDefaultProfile() {
    std::cout << "\nGet Default Profile" << std::endl;
    telux::common::Status retStat = telux::common::Status::SUCCESS;
    SlotId slotId = DEFAULT_SLOT_ID;
    int profileId;

    int operationType;
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);

    // Callback
    auto respCb = [](int profileId, SlotId slotId, telux::common::ErrorCode error) {
        std::cout << std::endl << std::endl;
        std::cout << "CALLBACK: "
                    << "GetDefaultProfile Response"
                    << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                    << ". ErrorCode: " << static_cast<int>(error)
                    << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Slot Id: " << slotId << endl
                      << "Profile Id: " << profileId << endl;
        }
    };

    retStat = dataConnectionManagerMap_[static_cast<SlotId>(slotId)]->getDefaultProfile(
        opType, respCb);
    Utils::printStatus(retStat);
}
