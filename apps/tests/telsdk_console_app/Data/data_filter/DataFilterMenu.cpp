/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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

#include "DataFilterMenu.hpp"
#include "../DataResponseCallback.hpp"
#define PROTO_TCP 6
#define PROTO_UDP 17

using namespace std;
using namespace telux::data::net;

DataFilterMenu::DataFilterMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    subSystemStatusUpdated_ = false;
}

DataFilterMenu::~DataFilterMenu() {

    if (dataConnectionManager_) {
        dataConnectionManager_->deregisterListener(dataListener_);
        dataConnectionManager_ = nullptr;
    }

    if (dataFilterMgr_) {
        dataFilterMgr_->deregisterListener(dataFilterListener_);
        dataFilterMgr_ = nullptr;
    }

    if (dataListener_) {
        dataListener_ = nullptr;
    }

    if (dataFilterListener_) {
        dataFilterListener_ = nullptr;
    }
}

bool DataFilterMenu::initializeSDK() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_ = false;
    SlotId slotId = DEFAULT_SLOT_ID;
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    std::promise<telux::common::ServiceStatus> dcmProm{};

    // Get the DataFactory instances.
    auto &dataFactory = telux::data::DataFactory::getInstance();

    dataConnectionManager_ = dataFactory.getDataConnectionManager(slotId,
        [&dcmProm](telux::common::ServiceStatus status) { dcmProm.set_value(status); });

    if (!dataConnectionManager_) {
        std::cout << "Failed to get DataManager object" << std::endl;
        return false;
    }

    if (dataConnectionManager_) {
        std::cout << "\n\nInitializing Data connection manager subsystem on slot " <<
            DEFAULT_SLOT_ID << ", Please wait ..." << endl;
        subSystemStatus = dcmProm.get_future().get();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Connection Manager on slot "<< slotId << " is ready"
                << std::endl;
            dataListener_ = std::make_shared<DataListener>(slotId);
            dataConnectionManager_->registerListener(dataListener_);
        } else {
            std::cout << "\nData Connection Manager on slot "<< slotId << " is not ready"
                << std::endl;
            return false;
        }
    }
    subSystemStatusUpdated_ = false;
    std::promise<telux::common::ServiceStatus> dfsProm{};
    // Get data filter manager object
    dataFilterMgr_ = dataFactory.getDataFilterManager(DEFAULT_SLOT_ID,
        [&dfsProm](telux::common::ServiceStatus status) { dfsProm.set_value(status); });
    if (dataFilterMgr_ == nullptr) {
        std::cout << "WARNING: Data Filter feature is not supported." << std::endl;
        return false;
    }

    if (dataFilterMgr_) {
        subSystemStatus = dataFilterMgr_->getServiceStatus();
        std::cout << "\n\nInitializing Data filter manager subsystem on slot " <<
            DEFAULT_SLOT_ID << ", Please wait ..." << endl;
        subSystemStatus = dfsProm.get_future().get();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Filter Manager on slot "<< slotId << " is ready" << std::endl;
            dataFilterListener_ = std::make_shared<MyDataFilterListener>();
            telux::common::Status status = dataFilterMgr_->registerListener(dataFilterListener_);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Unable to register data filter manager listener" << std::endl;
            }
        } else {
            std::cout << "\nData Filter Manager on slot "<< slotId << " is not ready"
                << std::endl;
            return false;
        }
        responseCb = std::bind(&DataFilterMenu::commandCallback, this, std::placeholders::_1);
    }
    return true;
}

void DataFilterMenu::init() {

    DataRestrictMode enableMode, disableMode;
    enableMode.filterAutoExit = DataRestrictModeType::DISABLE;
    enableMode.filterMode = DataRestrictModeType::ENABLE;

    disableMode.filterAutoExit = DataRestrictModeType::DISABLE;
    disableMode.filterMode = DataRestrictModeType::DISABLE;

    std::shared_ptr<ConsoleAppCommand> enableModeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "enable_data_restrict_mode",
            {}, std::bind(&DataFilterMenu::sendSetDataRestrictMode, this, enableMode)));

    std::shared_ptr<ConsoleAppCommand> disableModeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "disable_data_restrict_mode",
            {}, std::bind(&DataFilterMenu::sendSetDataRestrictMode, this, disableMode)));

    std::shared_ptr<ConsoleAppCommand> getFilterModeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "3", "get_data_restrict_mode", {}, std::bind(&DataFilterMenu::getFilterMode, this)));

    std::shared_ptr<ConsoleAppCommand> addFilterCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "4", "add_data_restrict_filter", {}, std::bind(&DataFilterMenu::addFilter, this)));

    std::shared_ptr<ConsoleAppCommand> removeAllFilterCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5",
            "remove_all_data_restrict_filter", {}, std::bind(&DataFilterMenu::removeAllFilter, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {enableModeCommand,
        disableModeCommand, getFilterModeCommand, addFilterCommand, removeAllFilterCommand};

    addCommands(commandsList);

    if (DataFilterMenu::initializeSDK()) {
        ConsoleApp::displayMenu();
    }
}

void DataFilterMenu::commandCallback(ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Command initiated successfully " << std::endl;
    } else {
        std::cout << " Command failed." << std::endl;
    }
}

void DataFilterMenu::sendSetDataRestrictMode(DataRestrictMode mode) {

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

void DataFilterMenu::getFilterMode() {

    if (dataFilterMgr_ == NULL) {
        std::cout << "Data restrict filter feature is not supported." << std::endl;
        return;
    }

    std::cout << " Sending command to get Data Filter" << std::endl;

    // pass empty interface name as string.
    telux::common::Status status = dataFilterMgr_->requestDataRestrictMode(
        "", &DataFilterModeResponseCb::requestDataRestrictModeResponse);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to send Data Restrict command" << std::endl;
    }
}

IpProtocol DataFilterMenu::getTypeOfFilter(
    DataConfigParser instance, std::map<std::string, std::string> filter) {
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

void DataFilterMenu::addIPParameters(std::shared_ptr<telux::data::IIpFilter> &dataFilter,
    DataConfigParser instance, std::map<std::string, std::string> filterMap) {

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

void DataFilterMenu::addFilter() {

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

    DataConfigParser cfgParser("filter", DEFAULT_DATA_CONFIG_FILE_NAME);
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

void DataFilterMenu::removeAllFilter() {

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
