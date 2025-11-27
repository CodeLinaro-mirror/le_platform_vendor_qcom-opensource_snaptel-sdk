/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This file hosts the implemenation of the ThermalListener class, which is notified of
 * service change events in the thermal
 */

#include <iostream>

#include "ThermalListener.hpp"
#include "ThermalHelper.hpp"

#define PRINT_NOTIFICATION_SERVICE std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_NOTIFICATION_DATA std::cout << "\033[1;32mNOTIFICATION: \033[0m"

ThermalListener::ThermalListener() {
}

ThermalListener::~ThermalListener() {
}

void ThermalListener::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION_SERVICE << ": Thermal Service Status : UNAVAILABLE" << std::endl;
    } else if (status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION_SERVICE << ": Thermal Service Status : AVAILABLE" << std::endl;
    }
}

void ThermalListener::onCoolingDeviceLevelChange(std::shared_ptr<ICoolingDevice> coolingDevice) {
    std::cout << std::endl;
    if (coolingDevice) {
        PRINT_NOTIFICATION_DATA << ": COOLING DEV LEVEL EVENT" << std::endl;
        ThermalHelper::printCoolingDeviceHeader();
        ThermalHelper::printCoolingDevInfo(coolingDevice);
        return;
    }
    PRINT_NOTIFICATION_DATA << ": Invalid cooling device" << std::endl;
}

void ThermalListener::onTripEvent(std::shared_ptr<ITripPoint> tripPoint, TripEvent tripEvent) {
    std::cout << std::endl;
    if (tripPoint) {
        PRINT_NOTIFICATION_DATA << ": TRIP UPDATE EVENT" << std::endl;
        ThermalHelper::printTripPointHeader();
        ThermalHelper::printTripPointInfo(tripPoint, tripEvent);
        return;
    }
    PRINT_NOTIFICATION_DATA << ": Invalid trip point" << std::endl;
}
