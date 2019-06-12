/*
 *  Copyright (c) 2019 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iomanip>
#include <iostream>
#include <vector>

#include "ThermalHelper.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

std::string ThermalHelper::convertTripTypeToStr(telux::therm::TripType type) {
    std::string tripType;
    switch (type) {
    case telux::therm::TripType::CRITICAL:
        tripType = "CRITICAL";
        break;
    case telux::therm::TripType::HOT:
        tripType = "HOT";
        break;
    case telux::therm::TripType::PASSIVE:
        tripType = "PASSIVE";
        break;
    case telux::therm::TripType::ACTIVE:
        tripType = "ACTIVE";
        break;
    case telux::therm::TripType::CONFIGURABLE_HIGH:
        tripType = "CONFIGURABLE_HIGH";
        break;
    case telux::therm::TripType::CONFIGURABLE_LOW:
        tripType = "CONFIGURABLE_LOW";
        break;
    default:
        tripType = "UNKNOWN";
        break;
    }
    return tripType;
}

std::string ThermalHelper::tripPointToString(
    std::shared_ptr<telux::therm::ITripPoint> &tripInfo, std::string &trip) {
    std::string tripTempPoints;
    if (trip == "CRITICAL")
        tripTempPoints
            += "C" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (trip == "HOT")
        tripTempPoints
            += "H" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (trip == "ACTIVE")
        tripTempPoints
            += "A" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (trip == "PASSIVE")
        tripTempPoints
            += "P" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (trip == "CONFIGURABLE_HIGH")
        tripTempPoints
            += "CH" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else if (trip == "CONFIGURABLE_LOW")
        tripTempPoints
            += "CL" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    else
        tripTempPoints
            += "U" + std::string("(") + std::to_string(tripInfo->getThresholdTemp()) + ")";
    return tripTempPoints;
}

void ThermalHelper::printBindingInfo(std::shared_ptr<telux::therm::IThermalZone> &tzInfo) {
    std::vector<telux::therm::BoundCoolingDevice> boundCoolingDeviceList
        = tzInfo->getBoundCoolingDevices();
    int boundCdevSize = boundCoolingDeviceList.size();
    if (boundCdevSize > 0) {
        std::cout << std::endl;
        std::cout << "Binding Info: " << std::endl;
        std::cout << std::setw(2) << "+--------------------------------------------------+"
                  << std::endl;
        std::cout << std::setw(5) << "|" << std::setw(10) << "Cooling Dev Id  " << std::setw(10)
                  << "|" << std::setw(20) << "Trip Points" << std::setw(10) << "|" << std::endl;
        std::cout << std::setw(2) << "+--------------------------------------------------+"
                  << std::endl;
        for (auto j = 0; j < boundCdevSize; j++) {
            std::string thresholdPoints;
            int noOfBoundTripPoints = boundCoolingDeviceList[j].bindingInfo.size();
            if (noOfBoundTripPoints > 0) {
                for (auto k = 0; k < noOfBoundTripPoints; k++) {
                    std::string trip
                        = convertTripTypeToStr(boundCoolingDeviceList[j].bindingInfo[k]->getType());
                    thresholdPoints
                        += tripPointToString(boundCoolingDeviceList[j].bindingInfo[k], trip);
                }
                std::cout << std::left << std::setw(7) << " " << std::setw(3)
                          << boundCoolingDeviceList[j].coolingDeviceId << std::setw(15) << " "
                          << std::setw(30) << thresholdPoints << std::setw(20) << std::endl;
            } else {
                std::cout << "No trip points bound!" << std::endl;
            }
        }
    } else {
        std::cout << "No bound cooling devices found!" << std::endl;
    }
}

void ThermalHelper::printThermalZoneInfo(std::shared_ptr<telux::therm::IThermalZone> &tzInfo) {
    std::vector<std::shared_ptr<telux::therm::ITripPoint>> tripInfo;
    tripInfo = tzInfo->getTripPoints();
    std::string tripPoints;
    if (tripInfo.size() > 0) {
        for (size_t i = 0; i < tripInfo.size(); ++i) {
            std::string trip = convertTripTypeToStr(tripInfo[i]->getType());
            tripPoints += tripPointToString(tripInfo[i], trip);
        }
    }

    std::cout << std::left << std::setw(4) << " " << std::setw(3) << tzInfo->getId()
              << std::setw(10) << " " << std::setw(25) << tzInfo->getDescription() << std::setw(7)
              << " " << std::setw(5) << tzInfo->getCurrentTemp() << std::setw(12) << " "
              << std::setw(5) << tzInfo->getPassiveTemp() << std::setw(5) << " " << std::setw(30)
              << tripPoints << std::setw(20);
    std::cout << std::endl;
}

void ThermalHelper::printCoolingDevInfo(std::shared_ptr<telux::therm::ICoolingDevice> &cdevInfo) {
    std::cout << std::left << std::setw(5) << " " << std::setw(3) << cdevInfo->getId()
              << std::setw(7) << " " << std::setw(20) << cdevInfo->getDescription() << std::setw(7)
              << " " << std::setw(5) << cdevInfo->getMaxCoolingLevel() << std::setw(15) << " "
              << std::setw(5) << cdevInfo->getCurrentCoolingLevel() << std::endl;
}