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

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>

#include <telux/data/DataFactory.hpp>
#include <telux/common/DeviceConfig.hpp>

#include "../../../../common/utils/Utils.hpp"

#include "DataFilterMenu.hpp"
#include "../DataResponseCallback.hpp"
#include "../DataUtils.hpp"

#define PROTO_ICMP 1
#define PROTO_IGMP 2
#define PROTO_TCP 6
#define PROTO_UDP 17
#define PROTO_ESP 50
#define PROTO_ICMP6 58
#define PROTO_RESERVED 255

using namespace std;
using namespace telux::data::net;

DataFilterMenu::DataFilterMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DataFilterMenu::~DataFilterMenu() {

    for (auto &conMgr : dataConnManagerMap_) {
        conMgr.second->deregisterListener(dataListener_[conMgr.first]);
    }
    dataConnManagerMap_.clear();
    dataListener_.clear();

    for (auto &filterMgr : dataFilterManagerMap_) {
        filterMgr.second->deregisterListener(dataFilterListener_[filterMgr.first]);
    }
    dataFilterManagerMap_.clear();
    dataFilterListener_.clear();
}

bool DataFilterMenu::init() {

    bool dfmSubSystemStatus = initDataFilterManagerAndListener(DEFAULT_SLOT_ID);
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        dfmSubSystemStatus |= initDataFilterManagerAndListener(SLOT_ID_2);
    }

    if (!dfmSubSystemStatus) {
        std::cout << "Data Filter initialize failed " << std::endl;
    }
    DataRestrictMode enableMode, disableMode;
    enableMode.filterAutoExit = DataRestrictModeType::DISABLE;
    enableMode.filterMode     = DataRestrictModeType::ENABLE;

    disableMode.filterAutoExit = DataRestrictModeType::DISABLE;
    disableMode.filterMode     = DataRestrictModeType::DISABLE;

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

    std::shared_ptr<ConsoleAppCommand> removeAllFilterCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand("5", "remove_all_data_restrict_filter", {},
            std::bind(&DataFilterMenu::removeAllFilter, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {enableModeCommand,
        disableModeCommand, getFilterModeCommand, addFilterCommand, removeAllFilterCommand};

    addCommands(commandsList);
    ConsoleApp::displayMenu();
    std::cout << "Data Filter init " << dfmSubSystemStatus << std::endl;
    return dfmSubSystemStatus;
}

bool DataFilterMenu::initDataFilterManagerAndListener(SlotId slotId) {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue                                = false;
    std::promise<telux::common::ServiceStatus> promDcm{};

    // Get the DataFactory instances.
    auto &dataFactory    = telux::data::DataFactory::getInstance();
    auto dataConnManager = dataFactory.getDataConnectionManager(
        slotId, [&promDcm](telux::common::ServiceStatus status) { promDcm.set_value(status); });

    if (!dataConnManager) {
        std::cout << "Failed to get DataManager object" << std::endl;
        return false;
    }

    std::cout << "\n\nInitializing Data connection manager subsystem on slot " << slotId
              << ", Please wait ..." << endl;
    subSystemStatus = promDcm.get_future().get();

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nData Connection Manager on slot " << slotId << " is ready" << std::endl;
        retValue = true;
    } else {
        std::cout << "\nData Connection Manager on slot " << slotId << " is not ready" << std::endl;
        return false;
    }

    if (dataConnManagerMap_.find(slotId) == dataConnManagerMap_.end()) {
        dataConnManagerMap_.emplace(slotId, dataConnManager);
        dataListener_.emplace(slotId, std::make_shared<DataListener>(slotId));
        telux::common::Status status
            = dataConnManagerMap_[slotId]->registerListener(dataListener_[slotId]);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Unable to register data connection manager listener on slot " << slotId
                      << std::endl;
        }
    }

    std::promise<telux::common::ServiceStatus> promDfm{};
    // Get data filter manager object
    auto dataFilterMgr = dataFactory.getDataFilterManager(
        slotId, [&promDfm](telux::common::ServiceStatus status) { promDfm.set_value(status); });
    if (dataFilterMgr == nullptr) {
        std::cout << "WARNING: Data Filter feature is not supported." << std::endl;
        return false;
    }

    if (dataFilterMgr) {
        std::cout << "\n\nInitializing Data filter manager subsystem on slot " << slotId
                  << ", Please wait ..." << endl;
        subSystemStatus = promDfm.get_future().get();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Filter Manager on slot " << slotId << " is ready" << std::endl;
            retValue = true;
        } else {
            std::cout << "\nData Filter Manager on slot " << slotId << " is not ready" << std::endl;
            return false;
        }

        // If this is newly created Manager
        if (dataFilterManagerMap_.find(slotId) == dataFilterManagerMap_.end()) {
            dataFilterManagerMap_.emplace(slotId, dataFilterMgr);
            dataFilterListener_.emplace(slotId, std::make_shared<MyDataFilterListener>(slotId));
            auto responseCb
                = std::bind(&DataFilterMenu::commandCallback, this, std::placeholders::_1);
            responseCbMap_.emplace(slotId, responseCb);
            telux::common::Status status
                = dataFilterManagerMap_[slotId]->registerListener(dataFilterListener_[slotId]);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "Unable to register data filter manager listener on slot " << slotId
                          << std::endl;
            }
        }
    }
    return retValue;
}

void DataFilterMenu::commandCallback(ErrorCode errorCode) {
    if (errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << " Command initiated successfully " << std::endl;
    } else {
        std::cout << " Command failed." << std::endl;
    }
}

void DataFilterMenu::sendSetDataRestrictMode(DataRestrictMode mode) {

    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataFilterManagerMap_.find(static_cast<SlotId>(slotId)) == dataFilterManagerMap_.end()) {
        std::cout << "\nData Filter Manager on slot " << slotId << " is not ready" << std::endl;
        return;
    }

    if (mode.filterMode == DataRestrictModeType::ENABLE) {
        std::cout << " Sending command to enable Data Filter" << std::endl;
        std::cout << "Auto Exit Filter (0-DISABLE, 1-ENABLE): ";
        int filterAutoExitInput = 0;
        std::cin >> filterAutoExitInput;
        Utils::validateInput(
            filterAutoExitInput, {static_cast<int>(DataRestrictModeType::DISABLE),
                                     static_cast<int>(DataRestrictModeType::ENABLE)});

        if (filterAutoExitInput) {
            std::cout << " ENABLE Auto Exit Filter " << std::endl;
            mode.filterAutoExit = DataRestrictModeType::ENABLE;
        } else {
            std::cout << " DISABLE Auto Exit Filter " << std::endl;
            mode.filterAutoExit = DataRestrictModeType::DISABLE;
        }
    } else if (mode.filterMode == DataRestrictModeType::DISABLE) {
        std::cout << " Sending command to disable Data Filter" << std::endl;
    }

    telux::common::Status status = telux::common::Status::FAILED;
    status = dataFilterManagerMap_[static_cast<SlotId>(slotId)]->setDataRestrictMode(
        mode, responseCbMap_[static_cast<SlotId>(slotId)]);

    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to send Data Restrict command" << std::endl;
    }
}

void DataFilterMenu::getFilterMode() {
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataFilterManagerMap_.find(static_cast<SlotId>(slotId)) == dataFilterManagerMap_.end()) {
        std::cout << "\nData Filter Manager on slot " << slotId << " is not ready" << std::endl;
        return;
    }
    std::cout << " Sending command to get Data Filter" << std::endl;
    telux::common::Status status
        = dataFilterManagerMap_[static_cast<SlotId>(slotId)]->requestDataRestrictMode(
            &DataFilterModeResponseCb::requestDataRestrictModeResponse);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to send Data Restrict command" << std::endl;
    }
}

IpProtocol DataFilterMenu::getTypeOfFilter(
    DataConfigParser instance, std::map<std::string, std::string> filter) {
    IpProtocol type = PROTO_RESERVED;
    if (instance.getValue(filter, "FILTER_PROTOCOL_TYPE") != "") {
        std::string protoType = instance.getValue(filter, "FILTER_PROTOCOL_TYPE");
        if (strcmp(protoType.c_str(), "UDP") == 0) {
            type = PROTO_UDP;
        } else if (strcmp(protoType.c_str(), "TCP") == 0) {
            type = PROTO_TCP;
        } else if (strcmp(protoType.c_str(), "ICMP") == 0) {
            type = PROTO_ICMP;
        } else if (strcmp(protoType.c_str(), "IGMP") == 0) {
            type = PROTO_IGMP;
        } else if (strcmp(protoType.c_str(), "ESP") == 0) {
            type = PROTO_ESP;
        } else if (strcmp(protoType.c_str(), "ICMP6") == 0) {
            type = PROTO_ICMP6;
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

int DataFilterMenu::getPortInfo(DataConfigParser cfgParser,
    std::map<std::string, std::string> pairMap, std::string key, std::string errorStr) {
    int value = std::stoi(cfgParser.getValue(pairMap, key));

    if (value > std::numeric_limits<unsigned short>::max()
        || value < std::numeric_limits<unsigned short>::min()) {
        throw invalid_argument(errorStr);
    }
    return value;
}

void DataFilterMenu::addFilter() {
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataFilterManagerMap_.find(static_cast<SlotId>(slotId)) == dataFilterManagerMap_.end()) {
        std::cout << "\nData Filter Manager on slot " << slotId << " is not ready" << std::endl;
        return;
    }
    // Get data factory instance
    std::vector<std::shared_ptr<telux::data::IIpFilter>> dataFilters;
    int value = 0;
    std::cout << "Add data filters (0 - Read from file, 1 - Manual input): ";
    std::cin >> value;
    Utils::validateInput(value, {0, 1});
    if (value) {
        if (!getDataFiltersInput(dataFilters)) {
            return;
        }
    } else {
        std::string filePath = DEFAULT_DATA_CONFIG_FILE_NAME;
        std::cout << "Add data filters ( 0 - Default \"/etc/Datafilter.conf\","
                  << " 1 - Provide file path): ";
        std::cin >> value;
        Utils::validateInput(value, {0, 1});
        if (value) {
            std::cin >> filePath;
        }
        if (!getDataFiltersConfig(slotId, filePath, dataFilters)) {
            return;
        }
    }

    std::cout << " Number of filters to add: " << dataFilters.size() << std::endl;
    std::cout << " Sending command to Add Data Filters" << std::endl;
    telux::common::Status status = telux::common::Status::FAILED;

    status = dataFilterManagerMap_[static_cast<SlotId>(slotId)]->addDataRestrictFilters(
        dataFilters, responseCbMap_[static_cast<SlotId>(slotId)]);

    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to send Data Restrict command" << std::endl;
    }
}

bool DataFilterMenu::getDataFiltersConfig(int slotId, std::string filePath,
    std::vector<std::shared_ptr<telux::data::IIpFilter>> &dataFilters) {

    DataConfigParser cfgParser("filter", filePath);
    std::vector<std::map<std::string, std::string>> vectorFilter = cfgParser.getFilters();
    auto &dataFilterFactory                                      = DataFactory::getInstance();
    std::cout << "Total filters in config = " << vectorFilter.size() << std::endl;

    for (uint8_t i = 0; i < vectorFilter.size(); i++) {
        SlotId filterSlotId = getSlotIdOfFilter(cfgParser, vectorFilter[i]);
        if (static_cast<SlotId>(slotId) != filterSlotId)
            continue;
        IpProtocol typeOfFilter = getTypeOfFilter(cfgParser, vectorFilter[i]);
        std::shared_ptr<telux::data::IIpFilter> dataFilter;

        if (typeOfFilter == PROTO_TCP) {

            std::cout << "Creating TCP filter " << std::endl;

            // Get data filter manager object
            dataFilter = dataFilterFactory.getNewIpFilter(PROTO_TCP);
            addIPParameters(dataFilter, cfgParser, vectorFilter[i]);
            auto tcpRestrictFilter = std::dynamic_pointer_cast<ITcpFilter>(dataFilter);

            telux::data::TcpInfo tcpInfo_ = {};
            tcpInfo_.src.range            = 0;
            tcpInfo_.dest.range           = 0;
            try {
                if (cfgParser.getValue(vectorFilter[i], "TCP_SOURCE_PORT") != "") {
                    tcpInfo_.src.port = getPortInfo(
                        cfgParser, vectorFilter[i], "TCP_SOURCE_PORT", "TCP port value");
                    if (cfgParser.getValue(vectorFilter[i], "TCP_SOURCE_PORT_RANGE") != "") {
                        tcpInfo_.src.range = getPortInfo(cfgParser, vectorFilter[i],
                            "TCP_SOURCE_PORT_RANGE", "TCP Port range value");
                    }
                }

                if (cfgParser.getValue(vectorFilter[i], "TCP_DESTINATION_PORT") != "") {
                    tcpInfo_.dest.port = getPortInfo(
                        cfgParser, vectorFilter[i], "TCP_DESTINATION_PORT", "TCP port value");
                    if (cfgParser.getValue(vectorFilter[i], "TCP_DESTINATION_PORT_RANGE") != "") {
                        tcpInfo_.dest.range = getPortInfo(cfgParser, vectorFilter[i],
                            "TCP_DESTINATION_PORT_RANGE", "TCP port range vlaue");
                    }
                }
            } catch (const std::exception &e) {
                std::cout << " *** ERROR - Invalid " << e.what() << ", expected in range (0-65535)"
                          << std::endl;
                return false;
            }
            if (tcpRestrictFilter) {
                tcpRestrictFilter->setTcpInfo(tcpInfo_);
            } else {
                std::cout << " *** ERROR - Invalid tcp filter" << std::endl;
                return false;
            }
        } else if (typeOfFilter == PROTO_UDP) {
            std::cout << "Creating UDP filter " << std::endl;

            // Get data filter manager object
            dataFilter = dataFilterFactory.getNewIpFilter(PROTO_UDP);
            addIPParameters(dataFilter, cfgParser, vectorFilter[i]);

            auto udpRestrictFilter = std::dynamic_pointer_cast<IUdpFilter>(dataFilter);

            telux::data::UdpInfo udpInfo_ = {};
            udpInfo_.src.range            = 0;
            udpInfo_.dest.range           = 0;
            try {
                if (cfgParser.getValue(vectorFilter[i], "UDP_SOURCE_PORT") != "") {
                    udpInfo_.src.port = getPortInfo(
                        cfgParser, vectorFilter[i], "UDP_SOURCE_PORT", "UDP port value");
                    if (cfgParser.getValue(vectorFilter[i], "UDP_SOURCE_PORT_RANGE") != "") {
                        udpInfo_.src.range = getPortInfo(cfgParser, vectorFilter[i],
                            "UDP_SOURCE_PORT_RANGE", "UDP Port range value");
                    }
                }

                if (cfgParser.getValue(vectorFilter[i], "UDP_DESTINATION_PORT") != "") {
                    udpInfo_.dest.port = getPortInfo(
                        cfgParser, vectorFilter[i], "UDP_DESTINATION_PORT", "UDP port value");
                    if (cfgParser.getValue(vectorFilter[i], "UDP_DESTINATION_PORT_RANGE") != "") {
                        udpInfo_.dest.range = getPortInfo(cfgParser, vectorFilter[i],
                            "UDP_DESTINATION_PORT_RANGE", "UDP port range vlaue");
                    }
                }
            } catch (const std::exception &e) {
                std::cout << " *** ERROR - Invalid " << e.what() << ", expected in range (0-65535)"
                          << std::endl;
                return false;
            }
            if (udpRestrictFilter) {
                udpRestrictFilter->setUdpInfo(udpInfo_);
            } else {
                std::cout << " *** ERROR - Invalid udp filter" << std::endl;
                return false;
            }
        } else {
            std::cout << " *** ERROR - Invalid conf file parameters" << std::endl;
            return false;
        }
        dataFilters.emplace_back(dataFilter);
    }
    return true;
}

void DataFilterMenu::getIPInfo(std::shared_ptr<telux::data::IIpFilter> &dataFilter) {
    int sourceOption = 0;
    std::cout << " Enter IP family (4-IPv4, 6-IPv6, 0-Skip): ";
    std::cin >> sourceOption;
    Utils::validateInput(sourceOption, {4, 6, 0});

    if (sourceOption == 4) {
        telux::data::IPv4Info ipv4Info = {};
        std::string ipv4Addr = "", ipv4SubnetMask = "";
        char delimiter = '\n';
        std::cout << " Do you want to enter a source IPv4 address? (1-Yes, 0-Skip): ";
        std::cin >> sourceOption;
        Utils::validateInput(sourceOption, {1, 0});
        if (sourceOption) {
            std::cout << "Enter source IPv4 address: ";
            std::getline(std::cin, ipv4Info.srcAddr, delimiter);
        }

        std::cout << " Do you want to enter a destination IPv4 address? (1-Yes, 0-Skip): ";
        std::cin >> sourceOption;
        Utils::validateInput(sourceOption, {1, 0});
        if (sourceOption) {
            std::cout << "Enter destination IPv4 address: ";
            std::getline(std::cin, ipv4Info.destAddr, delimiter);
        }
        dataFilter->setIPv4Info(ipv4Info);
    } else if (sourceOption == 6) {
        telux::data::IPv6Info ipv6Info = {};
        char delimiter                 = '\n';
        std::string ipv6Addr           = "";
        std::cout << "Enter source IPv6 address: ";
        std::getline(std::cin, ipv6Info.srcAddr, delimiter);

        std::cout << "Enter destination IPv6 address: ";
        std::getline(std::cin, ipv6Info.destAddr, delimiter);
        dataFilter->setIPv6Info(ipv6Info);
    }
}

void DataFilterMenu::getPortInfo(int &srcPort, int &srcRange, int &destPort, int &destRange) {
    std::cout << " Enter source port (or 0-Skip): ";
    std::cin >> srcPort;
    Utils::validateInput(srcPort);
    if (srcPort > std::numeric_limits<unsigned short>::max()
        || srcPort < std::numeric_limits<unsigned short>::min()) {
        throw invalid_argument(" Invalid source port");
    }
    std::cout << " Enter source port range (or 0-Skip): ";
    std::cin >> srcRange;
    Utils::validateInput(srcRange);
    if (srcRange > std::numeric_limits<unsigned short>::max()
        || srcRange < std::numeric_limits<unsigned short>::min()) {
        throw invalid_argument(" Invalid source port range");
    }

    std::cout << " Enter destination port (or 0-Skip): ";
    std::cin >> destPort;
    Utils::validateInput(destPort);
    if (destPort > std::numeric_limits<unsigned short>::max()
        || destPort < std::numeric_limits<unsigned short>::min()) {
        throw invalid_argument(" Invalid destination port");
    }
    std::cout << " Enter destination port range (or 0-Skip): ";
    std::cin >> destRange;
    Utils::validateInput(destRange);
    if (destRange > std::numeric_limits<unsigned short>::max()
        || destRange < std::numeric_limits<unsigned short>::min()) {
        throw invalid_argument(" Invalid destination port range");
    }
}

bool DataFilterMenu::getDataFiltersInput(
    std::vector<std::shared_ptr<telux::data::IIpFilter>> &dataFilters) {
    auto &dataFilterFactory = DataFactory::getInstance();
    std::vector<std::shared_ptr<IFirewallEntry>> fwEntries;
    int continueNewDataFilterInput = 1;

    do {
        char delimiter = '\n';
        std::string protoStr;
        std::cout << "Enter Protocol (TCP, UDP): ";
        std::getline(std::cin, protoStr, delimiter);
        telux::data::IpProtocol typeOfFilter = DataUtils::getProtcol(protoStr);
        std::shared_ptr<telux::data::IIpFilter> dataFilter;

        if (typeOfFilter == PROTO_TCP) {
            std::cout << "Creating TCP filter " << std::endl;
            // Get data filter manager object
            dataFilter = dataFilterFactory.getNewIpFilter(PROTO_TCP);
            getIPInfo(dataFilter);

            auto tcpRestrictFilter       = std::dynamic_pointer_cast<ITcpFilter>(dataFilter);
            telux::data::TcpInfo tcpInfo = {};
            int srcPort = 0, srcRange = 0, destPort = 0, destRange = 0;
            try {
                getPortInfo(srcPort, srcRange, destPort, destRange);
            } catch (const std::exception &e) {
                std::cout << " *** ERROR - Invalid " << e.what() << ", expected in range (0-65535)"
                          << std::endl;
                return false;
            }
            tcpInfo.src.port   = srcPort;
            tcpInfo.src.range  = srcRange;
            tcpInfo.dest.port  = destPort;
            tcpInfo.dest.range = destRange;

            if (tcpRestrictFilter) {
                tcpRestrictFilter->setTcpInfo(tcpInfo);
            } else {
                std::cout << " *** ERROR - Invalid tcp filter" << std::endl;
                return false;
            }
        } else if (typeOfFilter == PROTO_UDP) {
            std::cout << "Creating UDP filter " << std::endl;

            // Get data filter manager object
            dataFilter = dataFilterFactory.getNewIpFilter(PROTO_UDP);
            getIPInfo(dataFilter);

            auto udpRestrictFilter       = std::dynamic_pointer_cast<IUdpFilter>(dataFilter);
            telux::data::UdpInfo udpInfo = {};
            int srcPort = 0, srcRange = 0, destPort = 0, destRange = 0;
            try {
                getPortInfo(srcPort, srcRange, destPort, destRange);
            } catch (const std::exception &e) {
                std::cout << " *** ERROR - Invalid " << e.what() << ", expected in range (0-65535)"
                          << std::endl;
                return false;
            }
            udpInfo.src.port   = srcPort;
            udpInfo.src.range  = srcRange;
            udpInfo.dest.port  = destPort;
            udpInfo.dest.range = destRange;

            if (udpRestrictFilter) {
                udpRestrictFilter->setUdpInfo(udpInfo);
            } else {
                std::cout << " *** ERROR - Invalid udp filter" << std::endl;
                return false;
            }
        } else {
            std::cout << " *** ERROR - Invalid conf file parameters" << std::endl;
            return false;
        }
        dataFilters.emplace_back(dataFilter);
        std::cout << "\n 0 - Done, 1 - Add more data filters " << std::endl;

        std::cin >> continueNewDataFilterInput;
        Utils::validateInput(continueNewDataFilterInput, {0, 1});
    } while (continueNewDataFilterInput);
    return true;
}

SlotId DataFilterMenu::getSlotIdOfFilter(
    DataConfigParser instance, std::map<std::string, std::string> filter) {
    SlotId slotId = DEFAULT_SLOT_ID;
    if (instance.getValue(filter, "SLOT_ID") != "") {
        std::string protoType = instance.getValue(filter, "SLOT_ID");
        if (strcmp(protoType.c_str(), "2") == 0) {
            slotId = SLOT_ID_2;
        }
    }
    std::cout << " SlotId : " << slotId << std::endl;
    return slotId;
}

void DataFilterMenu::removeAllFilter() {
    int slotId = DEFAULT_SLOT_ID;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        slotId = Utils::getValidSlotId();
    }
    if (dataFilterManagerMap_.find(static_cast<SlotId>(slotId)) == dataFilterManagerMap_.end()) {
        std::cout << "\nData Filter Manager on slot " << slotId << " is not ready" << std::endl;
        return;
    }
    std::cout << "\nRemove data filters" << std::endl;
    telux::common::Status status = telux::common::Status::FAILED;
    status = dataFilterManagerMap_[static_cast<SlotId>(slotId)]->removeAllDataRestrictFilters(
        responseCbMap_[static_cast<SlotId>(slotId)]);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << " *** ERROR - Failed to send remove Data Filter command" << std::endl;
        return;
    }
}
