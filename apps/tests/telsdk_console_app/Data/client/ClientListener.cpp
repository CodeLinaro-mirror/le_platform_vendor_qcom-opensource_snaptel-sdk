/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "ClientListener.hpp"
#include "../DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

ClientListener::ClientListener() {
}

void ClientListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat = "";
    switch (status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat = " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }

    PRINT_NOTIFICATION << " ** Client onServiceStatusChange **\n" << stat << std::endl;
}

void ClientListener::onDeviceDataUsageResetImminent(
    const std::vector<telux::data::DeviceDataUsage> devicesDataUsage,
    telux::data::UsageResetReason reason) {

    std::cout << std::endl << std::endl;
    PRINT_NOTIFICATION << __FUNCTION__ << " Client Listener:: onDeviceDataUsageResetImminent"
                       << " reason: " << DataUtils::usageResetReasonToString(reason) << std::endl;

    for (const telux::data::DeviceDataUsage x : devicesDataUsage) {
        std::cout << "macAddress: " << x.macAddress << std::endl;
        std::cout << "bytesRx: " << x.usage.bytesRx << std::endl;
        std::cout << "bytesTx: " << x.usage.bytesTx << std::endl << std::endl;
    }
}
