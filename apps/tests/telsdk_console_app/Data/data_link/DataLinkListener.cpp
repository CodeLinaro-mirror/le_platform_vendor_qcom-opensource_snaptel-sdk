/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:

 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>

#include "DataLinkListener.hpp"
#include "../DataUtils.hpp"

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

std::string DataLinkListener::ethModeTypeToString(telux::data::EthModeType ethModeType) {
    std::string mode ="";
    switch(ethModeType) {
        case telux::data::EthModeType::ETHMODE_USXGMII_10G:
            mode = " USXGMII_10G";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_5G:
            mode = " USXGMII_5G";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_2_5G:
            mode = " USXGMII_2_5G";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_1G:
            mode = " USXGMII_1G";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_100M:
            mode = " USXGMII_100M";
            break;
        case telux::data::EthModeType::ETHMODE_USXGMII_10M:
            mode = " USXGMII_10M";
            break;
        case telux::data::EthModeType::ETHMODE_SGMII_2_5G:
            mode = " SGMII_2_5G";
            break;
        case telux::data::EthModeType::ETHMODE_SGMII_1G:
            mode = " SGMII_1G";
            break;
        case telux::data::EthModeType::ETHMODE_SGMII_100M:
            mode = " SGMII_100M";
            break;
        default:
            mode = " Unknown ETH mode";
            break;
    }
    return mode;
}



std::string DataLinkListener::linkModeChangeStatusToString(telux::data::LinkModeChangeStatus status) {
    std::string stat ="";
    switch(status) {
        case telux::data::LinkModeChangeStatus::ACCEPTED:
            stat = " ACCEPTED";
            break;
        case telux::data::LinkModeChangeStatus::COMPLETED:
            stat = " COMPLETED";
            break;
        case telux::data::LinkModeChangeStatus::FAILED:
            stat = " FAILED";
            break;
        case telux::data::LinkModeChangeStatus::REJECTED:
            stat = " REJECTED";
            break;
        case telux::data::LinkModeChangeStatus::TIMEOUT:
            stat = " TIMEOUT";
            break;

        default:
            stat = " Unknown ETH status";
            break;
    }
    return stat;
}

void DataLinkListener::onEthModeChangeRequest(telux::data::EthModeType ethModeType) {
    PRINT_NOTIFICATION <<
        " ** Data Link onEthModeChangeRequest **\n" <<
        ethModeTypeToString(ethModeType) << std::endl;
}

void DataLinkListener::onEthModeChangeTransactionStatus(telux::data::EthModeType ethModeType,
    telux::data::LinkModeChangeStatus status) {
    std::string stat = linkModeChangeStatusToString(status);

    PRINT_NOTIFICATION << " ** Data Link onEthModeChangeTransactionStatus **\n"
                       << ethModeTypeToString(ethModeType)
                       << " ,status : " << static_cast<int>(status) << " "
                       << stat << std::endl;
}