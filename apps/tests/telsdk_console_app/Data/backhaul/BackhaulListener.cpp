/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "BackhaulListener.hpp"
#include "../DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

BackhaulListener::BackhaulListener() {
}

void BackhaulListener::onServiceStatusChange(telux::common::ServiceStatus status) {
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
        " ** Backhaul onServiceStatusChange **\n" << stat << std::endl;
}

void BackhaulListener::onBackhaulStatusChange(telux::data::net::BackhaulStatusInfo 
                       backhaulStatusInfo) {

    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << __FUNCTION__
        << " Backhaul Listener:: onBackhaulStatusChange" << std::endl;

           if ((backhaulStatusInfo.isV4BackhaulAvailable  == false) &&
               (backhaulStatusInfo.isV6BackhaulAvailable == false) &&
               (backhaulStatusInfo.isEthPduAvailable == false)) {
             std::cout <<"No Backhaul \n";
           }
           else {
               std::cout<< DataUtils::backhaulToString(backhaulStatusInfo.backhaulType) <<" Backhual\n";
               std::cout<<"IPV4 " << (backhaulStatusInfo.isV4BackhaulAvailable ? \
                                  "Connected": "Disconnected") <<"\n";
               std::cout<<"IPV6 " << (backhaulStatusInfo.isV6BackhaulAvailable ? \
                                  "Connected": "Disconnected") <<"\n";
               std::cout<<"ETH PDU " << (backhaulStatusInfo.isEthPduAvailable ? \
                                     "Connected": "Disconnected") <<"\n";
           }
}
