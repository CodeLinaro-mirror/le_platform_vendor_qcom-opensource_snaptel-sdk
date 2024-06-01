/*
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "CoolingDevice.hpp"

#define INVALID -1

namespace telux {
namespace therm {

CoolingDevice::CoolingDevice()
   : coolingDevInstance_(INVALID)
   , coolingDevType_("")
   , maxCoolingLevel_(INVALID)
   , currentCoolingLevel_(INVALID) {
    LOG(INFO, __FUNCTION__);
}

int CoolingDevice::getId() const {
    return coolingDevInstance_;
}

std::string CoolingDevice::getDescription() const {
    return coolingDevType_;
}

int CoolingDevice::getMaxCoolingLevel() const {
    return maxCoolingLevel_;
}

int CoolingDevice::getCurrentCoolingLevel() const {
    return currentCoolingLevel_;
}

std::string CoolingDevice::toString() {
    std::stringstream ss;
    ss << " cdev Id: " << coolingDevInstance_ << ", cdev name: " << coolingDevType_
       << ", max cooling level: " << maxCoolingLevel_
       << ", cur cooling level: " << currentCoolingLevel_;
    return ss.str();
}

void CoolingDevice::setId(int instance) {
    coolingDevInstance_ = instance;
}

void CoolingDevice::setDescription(std::string type) {
    coolingDevType_ = type;
}

void CoolingDevice::setMaxCoolingLevel(int maxCoolingLevel) {
    maxCoolingLevel_ = maxCoolingLevel;
}

void CoolingDevice::setCurrentCoolingLevel(int currentCoolingLevel) {
    currentCoolingLevel_ = currentCoolingLevel;
}

}  // end of namespace therm
}  // end of namespace telux
