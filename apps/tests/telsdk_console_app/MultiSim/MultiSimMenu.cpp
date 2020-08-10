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

/**
 * MultiSimMenu provides menu options to invoke MultiSim functions such as
 * requestHighCapability.
 */

#include <chrono>
#include <iostream>

#include <telux/tel/PhoneFactory.hpp>
#include <Utils.hpp>

#include "MultiSimMenu.hpp"
#include "MyMultiSimHandler.hpp"
#include "MyMultiSimListener.hpp"

MultiSimMenu::MultiSimMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

MultiSimMenu::~MultiSimMenu() {
    if(multiSimMgr_) {
        multiSimMgr_->deregisterListener(multiSimListener_);
        multiSimMgr_ = nullptr;
    }

    if(multiSimListener_) {
        multiSimListener_ = nullptr;
    }
}

void MultiSimMenu::init() {

    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    //  Get the PhoneFactory and MultiSimManager instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    multiSimMgr_ = phoneFactory.getMultiSimManager();

    if(multiSimMgr_) {
        //  Check if MultiSim subsystem is ready
        bool subSystemStatus = multiSimMgr_->isSubsystemReady();

        //  If MultiSim subsystem is not ready, wait for it to be ready
        if(!subSystemStatus) {
            std::cout << "\n\nMultiSim subsystem is not ready, Please wait" << std::endl;
            std::future<bool> f = multiSimMgr_->onSubsystemReady();
            // If we want to wait unconditionally for MultiSim subsystem to be ready
            subSystemStatus = f.get();
        }

        //  Exit the application, if SDK is unable to initialize MultiSim subsystem
        if(subSystemStatus) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            std::cout << "Elapsed Time for Subsystem to ready : " << elapsedTime.count() << "s\n"
                      << std::endl;
            multiSimListener_ = std::make_shared<MyMultiSimListener>();
            telux::common::Status status = multiSimMgr_->registerListener(multiSimListener_);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "ERROR - Failed to register listener" << std::endl;
            }
        } else {
            std::cout << "ERROR - Unable to initialize subsystem" << std::endl;
            exit(0);
        }
    } else {
        std::cout << "ERROR - MultiSimManger is null" << std::endl;
    }

    std::shared_ptr<ConsoleAppCommand> getSlotCountCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Get_slot_count", {},
        std::bind(&MultiSimMenu::getSlotCount, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> requestHighCapabilityCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Request_high_capability", {},
        std::bind(&MultiSimMenu::requestHighCapability, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setHighCapabilityCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Set_high_capability", {},
        std::bind(&MultiSimMenu::setHighCapability, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setActiveSlotCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set_Active_slot", {},
        std::bind(&MultiSimMenu::switchActiveSlot, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getSlotsStatusCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Get_slots_status", {},
        std::bind(&MultiSimMenu::requestsSlotStatus, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListMultiSimMenu
        = { getSlotCountCommand, requestHighCapabilityCommand, setHighCapabilityCommand,
            setActiveSlotCommand, getSlotsStatusCommand};

    addCommands(commandsListMultiSimMenu);
    ConsoleApp::displayMenu();
}

void MultiSimMenu::getSlotCount(std::vector<std::string> userInput) {
    if(multiSimMgr_) {
        int slotCount = -1;
        auto ret = multiSimMgr_->getSlotCount(slotCount);
        if(ret == telux::common::Status::SUCCESS) {
            std::cout << "Slot Count: " << slotCount << std::endl;
        } else {
            std::cout << "Get Slot Count failed with status: "
            << static_cast<int>(ret) << std::endl;
        }
    } else {
        std::cout << "ERROR - MultiSimManger is null" << std::endl;
    }
}

void MultiSimMenu::requestHighCapability(std::vector<std::string> userInput) {
    if(multiSimMgr_) {
        auto ret = multiSimMgr_->requestHighCapability(
            MyMultiSimCallback::requestHighCapabilityResponse);
        std::cout
            << (ret == telux::common::Status::SUCCESS
                ? "Request High Capability request is successful \n"
                : "Request High Capability failed")
            << '\n';
    } else {
        std::cout << "ERROR - MultiSimManger is null" << std::endl;
    }
}

void MultiSimMenu::setHighCapability(std::vector<std::string> userInput) {
    do {
        if(multiSimMgr_) {
            char delimiter = '\n';
            std::string slotId = "";
            std::cout
                << "Enter SlotId (1-Primary, 2-Secondary) : ";
            std::getline(std::cin, slotId, delimiter);
            int opt = -1;
            if(!slotId.empty()) {
                try {
                    opt = std::stoi(slotId);
                } catch(const std::exception &e) {
                    std::cout
                        << "ERROR: Invalid input, enter numerical value " << opt << std::endl;
                    break;
                }
            } else {
                std::cout << "ERROR: Input cannot be empty string " << std::endl;
                break;
            }
            auto ret = multiSimMgr_->setHighCapability(
                opt, MyMultiSimCallback::setHighCapabilityResponse);
            std::cout
                << (ret == telux::common::Status::SUCCESS
                   ? "Set High capability request is successful \n"
                   : "Set High capability rate request failed")
                << '\n';
       } else {
           std::cout << "ERROR - MultiSimManger is null" << std::endl;
       }
   } while(0);
}

void MultiSimMenu::switchActiveSlot(std::vector<std::string> userInput) {
    if(multiSimMgr_) {
        char delimiter = '\n';
        std::string slotId = "";
        std::cout  << "Enter SlotId (1-Primary, 2-Secondary) : ";
        std::getline(std::cin, slotId, delimiter);
        int opt = -1;
        if(!slotId.empty()) {
            try {
                opt = std::stoi(slotId);
            } catch(const std::exception &e) {
                std::cout
                    << "ERROR: Invalid input, enter numerical value " << opt << std::endl;
                return;
            }
        } else {
            std::cout << "ERROR: Input cannot be empty string " << std::endl;
            return;
        }
        SlotId slot = SlotId::INVALID_SLOT_ID;
        if(opt == 1) {
            slot = SlotId::SLOT_ID_1;
        } else if(opt == 2) {
            slot = SlotId::SLOT_ID_2;
        } else {
            std::cout << "ERROR: Invalid input " << std::endl;
            return;
        }
        auto ret = multiSimMgr_->switchActiveSlot(slot,
                                    MyMultiSimCallback::setActiveSlotResponse);
        std::cout << (ret == telux::common::Status::SUCCESS
               ? "Set active slot request is successful \n"
               : "Set active slot request failed") << '\n';
   } else {
       std::cout << "ERROR - MultiSimManger is null" << std::endl;
   }
}

void MultiSimMenu::requestsSlotStatus(std::vector<std::string> userInput) {
    if(multiSimMgr_) {
        auto ret = multiSimMgr_->requestSlotStatus(MyMultiSimCallback::requestsSlotsStatusResponse);
        std::cout << (ret == telux::common::Status::SUCCESS
                ? "Slots status request is successful \n"
                : "Slots status request failed") << '\n';
    } else {
        std::cout << "ERROR - MultiSimManger is null" << std::endl;
    }
}
