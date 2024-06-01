/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <memory>

#include "../common/Logger.hpp"

#include "ThermalZone.hpp"

namespace telux {
namespace therm {

TripPoint::TripPoint()
   : type_(TripType::UNKNOWN)
   , temp_(INVALID_THERMAL_TEMP)
   , hysteresis_(INVALID_THERMAL_TEMP)
   , tripId_(INVALID_VALUE)
   , tZoneId_(INVALID_VALUE) {
    LOG(INFO, __FUNCTION__);
}

TripType TripPoint::getType() const {
    return type_;
}

int TripPoint::getThresholdTemp() const {
    return temp_;
}

int TripPoint::getHysteresis() const {
    return hysteresis_;
}

int TripPoint::getTripId() const {
    return tripId_;
}

int TripPoint::getTZoneId() const {
    return tZoneId_;
}

bool TripPoint::operator==(const ITripPoint &rHs) const {
    if ((getType() == rHs.getType()) && (getThresholdTemp() == rHs.getThresholdTemp())
        && (getHysteresis() == rHs.getHysteresis())) {
        return true;
    }
    return false;
}

std::string TripPoint::toString() {
    std::stringstream ss;
    ss << " Trip type: " << static_cast<int>(type_) << ", Trip temp: " << temp_
       << ", Hysteresis: " << hysteresis_ << ", Trip id: " << tripId_ << ", Tzone id: " << tZoneId_;
    return ss.str();
}

void TripPoint::setType(TripType type) {
    type_ = type;
}

void TripPoint::setThresholdTemp(int temp) {
    temp_ = temp;
}

void TripPoint::setHysteresis(int hysteresis) {
    hysteresis_ = hysteresis;
}

void TripPoint::setTripId(int tripId) {
    tripId_ = tripId;
}

void TripPoint::setTZoneId(int tZoneId) {
    tZoneId_ = tZoneId;
}

ThermalZone::ThermalZone()
   : tzSensorInstance_(INVALID_VALUE)
   , thermalZoneType_("")
   , sensorTemp_(INVALID_THERMAL_TEMP)
   , passiveTemp_(INVALID_THERMAL_TEMP) {
    LOG(DEBUG, __FUNCTION__);
}

int ThermalZone::getId() const {
    return tzSensorInstance_;
}

std::string ThermalZone::getDescription() const {
    return thermalZoneType_;
}

int ThermalZone::getCurrentTemp() const {
    return sensorTemp_;
}

int ThermalZone::getPassiveTemp() const {
    return passiveTemp_;
}

std::vector<std::shared_ptr<ITripPoint>> ThermalZone::getTripPoints() const {
    return tripInfo_;
}

std::string ThermalZone::toString() {
    std::stringstream ss;
    ss << " Tzone Id: " << tzSensorInstance_ << ", Tzone name: " << thermalZoneType_
       << ", Current temp: " << sensorTemp_ << ", Passive temp: " << passiveTemp_ << ",";
    for (auto trip : getTripPoints()) {
        ss << std::static_pointer_cast<TripPoint>(trip)->toString();
    }

    for (auto boundCdev : getBoundCoolingDevices()) {
        ss << "Bound cdev Id: " << boundCdev.coolingDeviceId;
        for (auto boundTrip : boundCdev.bindingInfo) {
            ss << ", Trip type: " << static_cast<int>(boundTrip->getType())
               << ", Trip temp: " << boundTrip->getThresholdTemp()
               << ", Hysteresis: " << boundTrip->getHysteresis();
        }
    }
    return ss.str();
}

std::vector<BoundCoolingDevice> ThermalZone::getBoundCoolingDevices() const {
    return boundCoolingDev_;
}

void ThermalZone::setId(int instance) {
    tzSensorInstance_ = instance;
}

void ThermalZone::setDescription(std::string type) {
    thermalZoneType_ = type;
}

void ThermalZone::setCurrentTemp(int temp) {
    sensorTemp_ = temp;
}

void ThermalZone::setPassiveTemp(int passiveTemp) {
    passiveTemp_ = passiveTemp;
}

void ThermalZone::setTripPoints(std::vector<std::shared_ptr<TripPoint>> tripInfo) {
    for (auto trip : tripInfo) {
        tripInfo_.emplace_back(trip);
    }
}

void ThermalZone::setBoundCoolingDevices(std::vector<BoundCoolingDevice> boundCoolingDev) {
    for (auto boundCdev : boundCoolingDev) {
        boundCoolingDev_.emplace_back(boundCdev);
    }
}

}  // end of namespace therm
}  // end of namespace telux
