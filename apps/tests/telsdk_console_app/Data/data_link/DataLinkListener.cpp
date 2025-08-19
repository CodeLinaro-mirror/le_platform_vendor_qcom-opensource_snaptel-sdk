/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "DataLinkListener.hpp"
#include "../DataUtils.hpp"

using namespace telux::data;
using namespace telux::common;

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

DataLinkListener::DataLinkListener(){
    std::cout << "DataLinkListener constructed" << std::endl;
}

DataLinkListener::~DataLinkListener(){
    std::cout << "DataLinkListener destructed" << std::endl;
}

void DataLinkListener::onServiceStatusChange(
    telux::common::ServiceStatus status) {

    std::string stat ="";
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
        " ** Data Link onServiceStatusChange **\n" << stat << std::endl;
}

void DataLinkListener::onLinkStatusChange(const LinkStatusInfo& info) {
    PRINT_NOTIFICATION << "onLinkStatusChange \n";

    std::cout << " Interface Type:" << DataUtils::interfaceToString(info.ifaceType)
    << "\n InterfaceName: " << info.ifaceName
    << "\n LinkStatus: " << DataUtils::linkStatusToString(info.status) <<std::endl;
}
