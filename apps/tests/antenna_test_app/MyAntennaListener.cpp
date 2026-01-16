/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This file hosts the implemenation of the AntennaListener class, which is notified of
 * antenna events in the platform
 */

#include <iostream>

#include "MyAntennaListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void MyAntennaListener::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : UNAVAILABLE" << std::endl;
    } else if (status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : AVAILABLE" << std::endl;
    }
}

void MyAntennaListener::onActiveAntennaChange(int antIndex) {
    std::cout << "\n";
    PRINT_NOTIFICATION << "Received active Antenna Config Change " << std::endl;
    PRINT_NOTIFICATION << "New Antenna Index: " << antIndex << std::endl;
}
