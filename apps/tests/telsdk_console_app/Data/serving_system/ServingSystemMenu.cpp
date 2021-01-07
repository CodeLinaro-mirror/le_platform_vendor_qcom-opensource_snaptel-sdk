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

#include <telux/common/DeviceConfig.hpp>
#include <telux/data/DataFactory.hpp>
#include "../../../../common/utils/Utils.hpp"

#include "ServingSystemMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

#define PRINT_NOTIFICATION std::cout << "\n\033[1;35mNOTIFICATION: \033[0m"

std::string getDrbStatusString(telux::data::DrbStatus stat) {
    string statusStr = "UNKNOWN";
    switch (stat) {
        case DrbStatus::DORMANT:
            statusStr = "DORMANT";
            break;
        case DrbStatus::ACTIVE:
            statusStr = "ACTIVE";
            break;
        case DrbStatus::UNKNOWN:
        default:
            break;
    }
    return statusStr;
}

DataServingSystemMenu::DataServingSystemMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    dataServingSystemManagers_.clear();
    addMenuCmds_ = false;
    subSystemStatusUpdated_ = false;
    dataServingSystemListeners_[DEFAULT_SLOT_ID] =
        std::make_shared<ServingSystemListenerOnDefaultSlotId>();
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        dataServingSystemListeners_[SLOT_ID_2] =
            std::make_shared<ServingSystemListenerOnSlotId2>();
    }
}

DataServingSystemMenu::~DataServingSystemMenu() {
}

bool DataServingSystemMenu::init() {
    bool initStat = initServingSystemManagerAndListener(DEFAULT_SLOT_ID);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        initStat |= initServingSystemManagerAndListener(SLOT_ID_2);
    }

    if (addMenuCmds_ == false) {
        addMenuCmds_ = true;
        std::shared_ptr<ConsoleAppCommand> getDrbStatus
            = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "get_drb_status", {},
                std::bind(&DataServingSystemMenu::getDrbStatus, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {getDrbStatus};
        addCommands(commandsList);
    }

    ConsoleApp::displayMenu();
    return initStat;
}

bool DataServingSystemMenu::initServingSystemManagerAndListener(SlotId slotId) {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue = false;
    subSystemStatusUpdated_ = false;
    auto initCb = std::bind(&DataServingSystemMenu::onInitCompleted, this,
        std::placeholders::_1);

    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto ServingSystemMgr =
        dataFactory.getServingSystemManager(slotId, initCb);
    if(ServingSystemMgr) {
        subSystemStatus = ServingSystemMgr->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
            std::cout << "\nInitializing Serving Manager on Slot "
                      << static_cast<int>(slotId) << ", Please wait..." << std::endl;
            std::unique_lock<std::mutex> lck(mtx_);
            cv_.wait(lck, [this]{return this->subSystemStatusUpdated_;});
            subSystemStatus = ServingSystemMgr->getServiceStatus();
        }
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nServing System Manager on slot "
                      << static_cast<int>(slotId)  << " is ready" << std::endl;
            retValue = true;
        }
        else {
            std::cout << "\nServing System Manager on slot "
                      << static_cast<int>(slotId)  << " is not ready" << std::endl;
            //If manager exist - deregister and remove it
            if (dataServingSystemManagers_.find(slotId) != dataServingSystemManagers_.end()) {
                dataServingSystemManagers_[slotId]->deregisterListener(
                    dataServingSystemListeners_[slotId]);
                dataServingSystemManagers_.erase(slotId);
            }
            retValue = false;
        }

        //If it is new manager and initialization passed
        if ((retValue == true) &&
            (dataServingSystemManagers_.find(slotId) == dataServingSystemManagers_.end())) {
            dataServingSystemManagers_.emplace(slotId, ServingSystemMgr);
            dataServingSystemManagers_[slotId]->registerListener(
                dataServingSystemListeners_[slotId]);
        }
    }
    return retValue;
}

void DataServingSystemMenu::onInitCompleted(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

void DataServingSystemMenu::getDrbStatus(std::vector<std::string> inputCommand) {
    std::cout << "Get DRB Status\n";
    telux::common::Status retStat;

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }

    if (dataServingSystemManagers_.find(static_cast<SlotId>(slotId)) ==
        dataServingSystemManagers_.end()) {
        std::cout << "Serving System Manager on SlotId: " << slotId << " is not ready" << std::endl;
        return;
    }

    telux::data::DrbStatus stat =
        dataServingSystemManagers_[static_cast<SlotId>(slotId)]->getDrbStatus();
    std::cout << "Current Drb Status is : " << getDrbStatusString(stat) << std::endl;
}

void ServingSystemListenerOnDefaultSlotId::onServiceStatusChange(
    telux::common::ServiceStatus status) {

    std::string stat;
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }

    PRINT_NOTIFICATION <<
        " ** Data ServingSystem onServiceStatusChange Slot: " << static_cast<int>(DEFAULT_SLOT_ID)
        <<  " **\n" << stat << std::endl;
}

void ServingSystemListenerOnDefaultSlotId::onDrbStatusChanged(
    telux::data::DrbStatus status) {
    PRINT_NOTIFICATION <<
        " Serving System Listener - received Drb status: " << getDrbStatusString(status)
        << " on SlotId: " << static_cast<int>(DEFAULT_SLOT_ID) << std::endl << std::endl;
}

void ServingSystemListenerOnSlotId2::onServiceStatusChange(
    telux::common::ServiceStatus status) {
    std::string stat;
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }

    PRINT_NOTIFICATION <<
        " ** Data ServingSystem onServiceStatusChange Slot: " <<  static_cast<int>(SLOT_ID_2)
        <<  " **\n" << stat << std::endl;
}

void ServingSystemListenerOnSlotId2::onDrbStatusChanged(telux::data::DrbStatus status) {
    PRINT_NOTIFICATION <<
        " Serving System Listener - received Drb status: " << getDrbStatusString(status)
        << " on SlotId: " << static_cast<int>(SLOT_ID_2) << std::endl << std::endl;
}
