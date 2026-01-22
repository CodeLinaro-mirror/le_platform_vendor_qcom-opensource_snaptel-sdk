/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef THERMALLISTENER_HPP
#define THERMALLISTENER_HPP

#include <telux/therm/ThermalListener.hpp>

using namespace telux::common;
using namespace telux::therm;

class ThermalListener : public telux::therm::IThermalListener {
 public:
    void onServiceStatusChange(ServiceStatus status) override;
    void onCoolingDeviceLevelChange(std::shared_ptr<ICoolingDevice> coolingDevice) override;
    void onTripEvent(std::shared_ptr<ITripPoint> tripPoint, TripEvent tripEvent) override;

    ThermalListener();
    ~ThermalListener();
};

#endif  // THERMALLISTENER_HPP
