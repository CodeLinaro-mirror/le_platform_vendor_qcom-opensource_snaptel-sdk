/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "StatsListener.hpp"
#include "../DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

StatsListener::StatsListener() {
}

void StatsListener::onServiceStatusChange(telux::common::ServiceStatus status) {
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
        " ** Stats onServiceStatusChange **\n" << stat << std::endl;
}


void StatsListener::onClientDataUsageResetImminent(
        const std::vector<telux::data::ClientDataUsage> clientDataUsageStats,
        telux::data::UsageResetReason reason) {

    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << __FUNCTION__
        << " Stats Listener:: onClientDataUsageResetImminent"
        << " reason: " << DataUtils::usageResetReasonToString(reason) << std::endl;

    for (const telux::data::ClientDataUsage x : clientDataUsageStats) {
            std::cout<<"\n IPV4: "<< x.v4Addr << std::endl;
            for(const auto& addr: x.v6Addr){
                std::cout<< addr << std::endl;
            }
            std::cout<<"\n Mac Address "<< x.macAddress << std::endl;
            std::cout<<"\n Bytes Rx: "<< x.usage.bytesRx << std::endl;
            std::cout<<"\n Bytes Tx: "<< x.usage.bytesTx << std::endl;
        }
    }

}