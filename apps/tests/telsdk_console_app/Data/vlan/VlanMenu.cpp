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

#include "VlanMenu.hpp"

using namespace std;

VlanMenu::VlanMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   initComplete_ = false;
}

VlanMenu::~VlanMenu() {
}

bool VlanMenu::init() {
    bool subSystemStatus = false;
    if (initComplete_ == false) {
        initComplete_ = true;
        auto &dataFactory = telux::data::DataFactory::getInstance();
        telux::data::OperationType opType = telux::data::OperationType::DATA_LOCAL;
        auto localVlanMgr = dataFactory.getVlanManager(opType);
        if (localVlanMgr) {
            vlanManagerMap_[opType] = localVlanMgr;
            subSystemStatus = vlanManagerMap_[opType]->isSubsystemReady();
            if (not subSystemStatus) {
                std::cout << "\nInitializing Local VLAN Manager subsystem, Please wait \n";
                std::future<bool> f = vlanManagerMap_[opType]->onSubsystemReady();
                // Wait unconditionally for data subsystem to be ready
                subSystemStatus = f.get();
            }
        }
        opType = telux::data::OperationType::DATA_REMOTE;
        auto remoteVlanMgr = dataFactory.getVlanManager(opType);
        if (remoteVlanMgr) {
            vlanManagerMap_[opType] = remoteVlanMgr;
            subSystemStatus = vlanManagerMap_[opType]->isSubsystemReady();
            if (not subSystemStatus) {
                std::cout << "\nInitializing Remote VLAN Manager subsystem, Please wait\n";
                std::future<bool> f = vlanManagerMap_[opType]->onSubsystemReady();
                // Wait unconditionally for data subsystem to be ready
                subSystemStatus = f.get();
            }
        }
        std::shared_ptr<ConsoleAppCommand> createVlan
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "create_vlan", {},
                std::bind(&VlanMenu::createVlan, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> removeVlan
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "remove_vlan", {},
                std::bind(&VlanMenu::removeVlan, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> queryVlanInfo
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "query_vlan_info", {},
                std::bind(&VlanMenu::queryVlanInfo, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> bindWithProfile
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "bind_with_profile", {},
                std::bind(&VlanMenu::bindWithProfile, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> unbindFromProfile
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "unbind_from_profile", {},
                std::bind(&VlanMenu::unbindFromProfile, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> queryVlanMappingList
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "query_vlan_mapping_list",
                {}, std::bind(&VlanMenu::queryVlanMappingList, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {createVlan, removeVlan,
            queryVlanInfo, bindWithProfile, unbindFromProfile, queryVlanMappingList};

        addCommands(commandsList);
    }
    bool locSubSystemStatus = vlanManagerMap_[
        telux::data::OperationType::DATA_LOCAL]->isSubsystemReady();
    if (locSubSystemStatus) {
        std::cout << "\nLocal VLAN Manager is ready" << std::endl;
    }
    else {
        std::cout << "\nLocal VLAN Manager is not ready" << std::endl;
    }
    bool rmtSubSystemStatus = false;
    if (vlanManagerMap_.find(telux::data::OperationType::DATA_REMOTE) != vlanManagerMap_.end()) {
        rmtSubSystemStatus = vlanManagerMap_[
            telux::data::OperationType::DATA_REMOTE]->isSubsystemReady();
        if (rmtSubSystemStatus) {
            std::cout << "\nRemote VLAN Manager is ready" << std::endl;
        }
        else {
            std::cout << "\nRemote VLAN Manager is not ready" << std::endl;
        }
    }
    ConsoleApp::displayMenu();
    return (locSubSystemStatus | rmtSubSystemStatus);
}

void VlanMenu::createVlan(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Create VLAN \n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    if (vlanManagerMap_.find(opType) == vlanManagerMap_.end()) {
        std::cout << "Invalid entry: Operation is not supported" << std::endl;
        return;
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

    retStat = vlanManagerMap_[opType]->createVlan(config, respCb);
    Utils::printStatus(retStat);
}

void VlanMenu::removeVlan(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Remove VLAN \n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    if (vlanManagerMap_.find(opType) == vlanManagerMap_.end()) {
        std::cout << "Invalid entry: Operation is not supported" << std::endl;
        return;
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
    retStat = vlanManagerMap_[opType]->removeVlan(vlanId, infType, respCb);
    Utils::printStatus(retStat);
}

void VlanMenu::queryVlanInfo(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Query VLAN info\n";
    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    if (vlanManagerMap_.find(opType) == vlanManagerMap_.end()) {
        std::cout << "Invalid entry: Operation is not supported" << std::endl;
        return;
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

    retStat = vlanManagerMap_[opType]->queryVlanInfo(respCb);
    Utils::printStatus(retStat);
}

void VlanMenu::bindWithProfile(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Bind with profile\n";

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    if (vlanManagerMap_.find(opType) == vlanManagerMap_.end()) {
        std::cout << "Invalid entry: Operation is not supported" << std::endl;
        return;
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

    retStat = vlanManagerMap_[opType]->bindWithProfile(
        profileId, vlanId, respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}

void VlanMenu::unbindFromProfile(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Unbind with profile\n";

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    if (vlanManagerMap_.find(opType) == vlanManagerMap_.end()) {
        std::cout << "Invalid entry: Operation is not supported" << std::endl;
        return;
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

   retStat = vlanManagerMap_[opType]->unbindFromProfile(
       profileId, vlanId, respCb, static_cast<SlotId>(slotId));
   Utils::printStatus(retStat);
}

void VlanMenu::queryVlanMappingList(std::vector<std::string> inputCommand) {
    telux::common::Status retStat;
    int operationType;
    bool subSystemStatus = false;

    std::cout << "Query VLAN Mapping List\n";

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    std::cout << "Enter Operation Type (0-LOCAL, 1-REMOTE): ";
    std::cin >> operationType;
    Utils::validateInput(operationType);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(operationType);
    if (vlanManagerMap_.find(opType) == vlanManagerMap_.end()) {
        std::cout << "Invalid entry: Operation is not supported" << std::endl;
        return;
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

    retStat = vlanManagerMap_[opType]->queryVlanMappingList(respCb, static_cast<SlotId>(slotId));
    Utils::printStatus(retStat);
}
