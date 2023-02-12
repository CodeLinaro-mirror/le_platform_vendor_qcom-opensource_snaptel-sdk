/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "DataSettingsListener.hpp"
#include "../DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

 DataSettingsListener::DataSettingsListener() {
 }

void DataSettingsListener::onServiceStatusChange(
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
        " ** Data Settings onServiceStatusChange " <<  " **\n" << stat << std::endl;
}
