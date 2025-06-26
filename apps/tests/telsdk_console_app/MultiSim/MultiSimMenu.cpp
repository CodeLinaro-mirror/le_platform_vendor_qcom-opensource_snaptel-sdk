/*
 *  Copyright (c) 2020 The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * MultiSimMenu provides menu options to invoke MultiSim functions such as
 * requestHighCapability.
 */

#include <chrono>
#include <iostream>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include <Utils.hpp>

#include "MultiSimMenu.hpp"
#include "MyMultiSimHandler.hpp"
#include "MyMultiSimListener.hpp"

#define SLOT_1 1
#define SLOT_2 2

MultiSimMenu::MultiSimMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

MultiSimMenu::~MultiSimMenu() {
    if (multiSimMgr_) {
        multiSimMgr_->deregisterListener(multiSimListener_);
        multiSimMgr_ = nullptr;
    }

    if (multiSimListener_) {
        multiSimListener_ = nullptr;
    }
}

bool MultiSimMenu::init() {

    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    std::promise<telux::common::ServiceStatus> prom;
    //  Get the PhoneFactory and MultiSimManager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    multiSimMgr_       = phoneFactory.getMultiSimManager(
        [&](telux::common::ServiceStatus status) { prom.set_value(status); });
    if (!multiSimMgr_) {
        std::cout << "ERROR - MultiSimManger is null \n";
        return false;
    }
    telux::common::ServiceStatus multiSimMgrStatus = multiSimMgr_->getServiceStatus();
    if (multiSimMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "MultiSimManger subsystem is not ready, Please wait \n";
    }
    multiSimMgrStatus = prom.get_future().get();
    if (multiSimMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        endTime                                   = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Subsystem to ready : " << elapsedTime.count() << "s\n"
                  << std::endl;
        std::cout << "MultiSimManger subsystem is ready \n";
        multiSimListener_            = std::make_shared<MyMultiSimListener>();
        telux::common::Status status = multiSimMgr_->registerListener(multiSimListener_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "ERROR - Failed to register listener" << std::endl;
        }
        std::shared_ptr<ConsoleAppCommand> getSlotCountCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Get_slot_count", {},
                std::bind(&MultiSimMenu::getSlotCount, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> requestHighCapabilityCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Request_high_capability",
                {}, std::bind(&MultiSimMenu::requestHighCapability, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setHighCapabilityCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Set_high_capability", {},
                std::bind(&MultiSimMenu::setHighCapability, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> setActiveSlotCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Switch_Active_slot", {},
        std::bind(&MultiSimMenu::switchActiveSlot, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> configureLogicalSlotMappingCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5",
                "Configure_Logical_slot_mapping", {},
        std::bind(&MultiSimMenu::configureLogicalSlotMapping, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getLogicalSlotMappingCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6",
                "Get_Logical_slot_mapping", {},
        std::bind(&MultiSimMenu::getLogicalSlotMapping, this, std::placeholders::_1)));
        std::shared_ptr<ConsoleAppCommand> getPhysicalSlotCommand
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7",
                "Get_Physical_slot_status", {},
        std::bind(&MultiSimMenu::getPhysicalSlotStatus, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListMultiSimMenu
            = { getSlotCountCommand, requestHighCapabilityCommand, setHighCapabilityCommand,
                setActiveSlotCommand, configureLogicalSlotMappingCommand,
                getLogicalSlotMappingCommand, getPhysicalSlotCommand};

        addCommands(commandsListMultiSimMenu);
        ConsoleApp::displayMenu();
    } else {
        std::cout << "Unable to initialise MultiSimManger subsystem " << std::endl;
        return false;
    }
    return true;
}

void MultiSimMenu::getSlotCount(std::vector<std::string> userInput) {
    if (multiSimMgr_) {
        int slotCount = -1;
        auto ret      = multiSimMgr_->getSlotCount(slotCount);
        if (ret == telux::common::Status::SUCCESS) {
            std::cout << "Slot Count: " << slotCount << std::endl;
        } else {
            std::cout << "Get Slot Count failed with status: " << static_cast<int>(ret)
                      << std::endl;
        }
    } else {
        std::cout << "ERROR - MultiSimManger is null" << std::endl;
    }
}

void MultiSimMenu::requestHighCapability(std::vector<std::string> userInput) {
    if (multiSimMgr_) {
        auto ret = multiSimMgr_->requestHighCapability(
            MyMultiSimCallback::requestHighCapabilityResponse);
        std::cout << (ret == telux::common::Status::SUCCESS
                          ? "Request High Capability request is successful \n"
                          : "Request High Capability failed")
                  << '\n';
    } else {
        std::cout << "ERROR - MultiSimManger is null" << std::endl;
    }
}

void MultiSimMenu::setHighCapability(std::vector<std::string> userInput) {
    do {
        if (multiSimMgr_) {
            char delimiter     = '\n';
            std::string slotId = "";
            std::cout << "Enter SlotId (1-Primary, 2-Secondary) : ";
            std::getline(std::cin, slotId, delimiter);
            int opt = -1;
            if (!slotId.empty()) {
                try {
                    opt = std::stoi(slotId);
                } catch (const std::exception &e) {
                    std::cout << "ERROR: Invalid input, enter numerical value " << opt << std::endl;
                    break;
                }
            } else {
                std::cout << "ERROR: Input cannot be empty string " << std::endl;
                break;
            }
            auto ret = multiSimMgr_->setHighCapability(
                opt, MyMultiSimCallback::setHighCapabilityResponse);
            std::cout << (ret == telux::common::Status::SUCCESS
                              ? "Set High capability request is successful \n"
                              : "Set High capability rate request failed")
                      << '\n';
        } else {
            std::cout << "ERROR - MultiSimManger is null" << std::endl;
        }
    } while (0);
}

void MultiSimMenu::switchActiveSlot(std::vector<std::string> userInput) {
    // Blocking this command in DSDA configuration, to avoid using it unintentionally, as this is
    // intended for DSSA(Dual Sim Single Active) configuration
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        std::cout << " ERROR: Invalid operation" << std::endl;
        return;
    }
    if (multiSimMgr_) {
        char delimiter     = '\n';
        std::string slotId = "";
        std::cout << "Enter SlotId (1-Primary, 2-Secondary) : ";
        std::getline(std::cin, slotId, delimiter);
        int opt = -1;
        if (!slotId.empty()) {
            try {
                opt = std::stoi(slotId);
            } catch (const std::exception &e) {
                std::cout << "ERROR: Invalid input, enter numerical value " << opt << std::endl;
                return;
            }
        } else {
            std::cout << "ERROR: Input cannot be empty string " << std::endl;
            return;
        }
        SlotId slot = SlotId::INVALID_SLOT_ID;
        if (opt == 1) {
            slot = SlotId::SLOT_ID_1;
        } else if (opt == 2) {
            slot = SlotId::SLOT_ID_2;
        } else {
            std::cout << "ERROR: Invalid input " << std::endl;
            return;
        }
        auto ret = multiSimMgr_->switchActiveSlot(slot, MyMultiSimCallback::setActiveSlotResponse);
        std::cout
            << (ret == telux::common::Status::SUCCESS ? "Set active slot request is successful \n"
                                                      : "Set active slot request failed")
            << '\n';
    } else {
        std::cout << "ERROR - MultiSimManger is null" << std::endl;
    }
}


void MultiSimMenu::configureLogicalSlotMapping(std::vector<std::string> userInput) {
    if (!multiSimMgr_) {
        std::cout << "ERROR - MultiSimManger is null" << std::endl;
        return;
    }

    // Get slot count to guide the user
    int slotCount = -1;
    auto getCountStatus = multiSimMgr_->getSlotCount(slotCount);
    if (getCountStatus != telux::common::Status::SUCCESS || slotCount <= 0) {
        std::cout << "ERROR - Unable to retrieve slot count; status: "
                  << static_cast<int>(getCountStatus) << std::endl;
        return;
    }

    std::cout << " Logical slot count: " << slotCount << std::endl;

    // Prepare mapping: LogicalSlotId -> LogicalSlotMapInfo
    std::map<LogicalSlotId, telux::tel::LogicalSlotMapInfo> mapInfo;

    for (int i = 1; i <= slotCount; ++i) {
        LogicalSlotId logicalId = logicalIdFromIndex(i);
        telux::tel::LogicalSlotMapInfo info{};
        char delimiter = '\n';
        std::string physSlotStr;

        std::cout << "Enter physical slot for LogicalSlotId " << i
                  << " (1-Primary, 2-Secondary, ...): ";
        std::getline(std::cin, physSlotStr, delimiter);

        int physSlot = -1;
        try {
            physSlot = std::stoi(physSlotStr);
        } catch (const std::exception &e) {
            std::cout << "ERROR: Invalid input, enter numerical value" << std::endl;
            return;
        }

        if (physSlot < SLOT_1 || physSlot > SLOT_2) {
            std::cout << "ERROR: Invalid physical slot index" << std::endl;
            return;
        }

        int portId = -1;
        std::string portIdStr;

        std::cout << "Enter the port identifier for the LogicalSlotId:\n"
          << "- For MEP B, the port identifiers start from 0 (e.g., 0, 1...)\n"
          << "- For MEP A1, the port identifiers start from 1 (e.g., 1, 2...)";
        std::getline(std::cin, portIdStr, delimiter);
        try {
            portId = std::stoi(portIdStr);
        } catch (const std::exception &e) {
            std::cout << "ERROR: Invalid input, enter numerical value" << std::endl;
            return;
        }

        // Validate port ID range
        if (portId < 0) {
            std::cout << "ERROR: Port identifier must be non-negative" << std::endl;
            return;
        }

        info.physicalSlot = static_cast<PhysicalSlotId>(physSlot);
        info.portId = portId;
        mapInfo.emplace(logicalId, info);
    }
    auto ret = multiSimMgr_->configureLogicalSlotMapping(
        mapInfo, MyMultiSimCallback::configureLogicalSlotMappingResponse);

    std::cout << (ret == telux::common::Status::SUCCESS
            ? "Configure logical slot mapping request is successful"
            : "Configure logical slot mapping request failed")
            << " with status: " << static_cast<int>(ret);
}

LogicalSlotId MultiSimMenu::logicalIdFromIndex(int idx) {
    switch (idx) {
        case 1: return LogicalSlotId::SLOT_ID_1;
        case 2: return LogicalSlotId::SLOT_ID_2;
        default: return LogicalSlotId::INVALID_SLOT_ID;
    }
}

void MultiSimMenu::getPhysicalSlotStatus(std::vector<std::string> userInput) {
    if (!multiSimMgr_) {
        std::cout << "ERROR - MultiSimManager is null" << std::endl;
        return;
    }
    std::map<PhysicalSlotId, telux::tel::SimSlotStatus> slotStatus;
    auto ret = multiSimMgr_->getPhysicalSlotStatus(slotStatus);
    if(ret == telux::common::ErrorCode::SUCCESS) {
        if (slotStatus.empty()) {
            std::cout << "No slot status entries.\n";
            return;
        }
        for(auto it = slotStatus.begin(); it != slotStatus.end(); ++it) {
            auto slotId = it->first;
            auto status = it->second;
            std::cout << " Physical SlotId: " << static_cast<int>(slotId)
                << ", SlotState: " << MyMultiSimHelper::slotStateToString(status.slotState)
                << ", CardState: " << MyMultiSimHelper::cardStateToString(status.cardState)
                << ", CardError: " << MyMultiSimHelper::cardErrorToString(status.cardError)
                << ", IsMEP: " << (status.isMep ? "true" : "false")
                << ", mode = "  << MyMultiSimHelper::mepModeToString(status.mepSlotInfo.mode)
                << std::endl;

            if (status.isMep) {
                const auto &ports = status.mepSlotInfo.port;
                if (!ports.empty()) {
                    std::cout << " MEP Port Details: " << std::endl;
                    for (const telux::tel::PortInfo &p : ports) {
                        std::cout << " LogicalSlotId: " << static_cast<int>(p.slotId)
                            << ", ICCID: "    << (p.iccId.empty() ? "<none>" : p.iccId)
                            << ", PortState: " << MyMultiSimHelper::portStateToString(p.state)
                            << std::endl;
                    }
                } else {
                    std::cout << " MEP Port details not found " << std::endl;
                }
            } else {
                std::cout << " MEP is not active on this physical slot. " << std::endl;
            }
        }
    } else {
        std::cout << " Configure logical slot mapping request failed with status code: "
            << static_cast<int>(ret) << std::endl;
    }
}

void MultiSimMenu::getLogicalSlotMapping(std::vector<std::string> userInput) {
    if (!multiSimMgr_) {
        std::cout << "ERROR - MultiSimManager is null" << std::endl;
        return;
    }
    std::map<LogicalSlotId, telux::tel::LogicalSlotMapInfo> mapInfo;
    auto ret = multiSimMgr_->getLogicalSlotMapping(mapInfo);
    if(ret == telux::common::ErrorCode::SUCCESS) {
        if (mapInfo.empty()) {
            std::cout << "Logical slot mapping: <empty>" << std::endl;
            return;
        }

        for (auto it = mapInfo.begin(); it != mapInfo.end(); ++it) {
            LogicalSlotId logicalId = it->first;
            const telux::tel::LogicalSlotMapInfo &info = it->second;

            std::cout << "LogicalSlotId: " << static_cast<int>(logicalId)
                    << ", PhysicalSlotId: " << static_cast<int>(info.physicalSlot)
                    << ", PortId: " << info.portId
                    << std::endl;
        }
    } else {
        std::cout << "getLogicalSlotMapping request failed with status code: "
            << static_cast<int>(ret) << std::endl;
    }
}
