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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021,2023 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * Sample application to demonstrate the use of thermal manager features and APIs
 */

#include <iostream>
#include <memory>
#include <iomanip>
#include <string>
#include <vector>

#include <telux/therm/ThermalFactory.hpp>
#include <telux/therm/ThermalManager.hpp>
#include <telux/therm/ThermalListener.hpp>

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl
const int THERMAL_ZONE_ID = 1;

class ThermalListener : public IThermalListener {
    public:
        // [7] Receive service status notification
        virtual void onServiceStatusChange(ServiceStatus serviceStatus) override {
            PRINT_NOTIFICATION << "Thermal service status: ";
            std::string status;
            switch (serviceStatus) {
                case ServiceStatus::SERVICE_AVAILABLE: {
                    status = "Available";
                    break;
                }
                case ServiceStatus::SERVICE_UNAVAILABLE: {
                    status = "Unavailable";
                    break;
                }
                case ServiceStatus::SERVICE_FAILED: {
                    status = "Failed";
                    break;
                }
                default: {
                    status = "Unknown";
                    break;
                }
            }
            std::cout << status << std::endl;
        }

        // [8] Receive notification when trip update occurs
        //     Receives notification only if it is registered in step 5
        virtual void onTripEvent(std::shared_ptr<ITripPoint> tripPoint, TripEvent tripEvent) override {
            if (tripPoint) {
                PRINT_NOTIFICATION << ": TRIP UPDATE EVENT" << std::endl;
                printTripPointHeader();
                printTripPointInfo(tripPoint, tripEvent);
                return;
            }
            PRINT_NOTIFICATION << ": Invalid trip point" << std::endl;
        }

        // Receive notification when cooling device level changes
        // Receives notification only if it is registered in step 5
        virtual void onCoolingDeviceLevelChange(std::shared_ptr<ICoolingDevice> coolingDevice) override {
            if (coolingDevice) {
                PRINT_NOTIFICATION << ": COOLING DEV LEVEL EVENT" << std::endl;
                printCoolingDeviceHeader();
                printDeviceInfo(coolingDevice);
                return;
            }
            PRINT_NOTIFICATION << ": Invalid cooling device" << std::endl;
        }
};

std::string convertTripTypeToStr(telux::therm::TripType type) {
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

std::string tripPointToString(
    std::shared_ptr<telux::therm::ITripPoint> &tripInfo, std::string &tripTempPoints) {
    if (!tripInfo) {
        std::cout << "Invalid trip point" << std::endl;
        return std::string();
    }
    std::string trip = convertTripTypeToStr(tripInfo->getType());
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

void printBindingInfo(std::shared_ptr<telux::therm::IThermalZone> &tzInfo) {
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
                    thresholdPoints = tripPointToString(
                        boundCoolingDeviceList[j].bindingInfo[k], thresholdPoints);
                    if (!thresholdPoints.size()) { return; }
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

void printZoneInfo(std::shared_ptr<telux::therm::IThermalZone> &tzInfo) {
    std::vector<std::shared_ptr<telux::therm::ITripPoint>> tripInfo;
    if (!tzInfo) {
        std::cout << "Invalid thermal zone" << std::endl;
        return;
    }

    tripInfo = tzInfo->getTripPoints();
    std::string tripPoints;
    if (tripInfo.size() > 0) {
        for (size_t i = 0; i < tripInfo.size(); ++i) {
            tripPoints = tripPointToString(tripInfo[i], tripPoints);
            if (!tripPoints.size()) { return; }
        }
    }

    std::cout << std::left << std::setw(4) << " " << std::setw(3) << tzInfo->getId()
              << std::setw(10) << " " << std::setw(25) << tzInfo->getDescription() << std::setw(7)
              << " " << std::setw(5) << tzInfo->getCurrentTemp() << std::setw(12) << " "
              << std::setw(5) << tzInfo->getPassiveTemp() << std::setw(5) << " " << std::setw(30)
              << tripPoints << std::setw(20);
    std::cout << std::endl;
}

void printDeviceInfo(std::shared_ptr<telux::therm::ICoolingDevice> &cdevInfo) {
    if (!cdevInfo) {
        std::cout << "Invalid cooling device" << std::endl;
        return;
    }
    std::cout << std::left << std::setw(5) << " " << std::setw(3) << cdevInfo->getId()
              << std::setw(7) << " " << std::setw(20) << cdevInfo->getDescription() << std::setw(7)
              << " " << std::setw(5) << cdevInfo->getMaxCoolingLevel() << std::setw(15) << " "
              << std::setw(5) << cdevInfo->getCurrentCoolingLevel() << std::endl;
}

void printTripPointInfo(std::shared_ptr<telux::therm::ITripPoint> &tripPointInfo,
        TripEvent event) {
    std::string tripPoints;
    std::string trip = convertTripTypeToStr(tripPointInfo->getType());
    tripPoints += tripPointToString(tripPointInfo, trip);
    std::cout
        << std::left << std::setw(3) << " " << std::setw(2) << tripPointInfo->getTZoneId()
        << std::setw(10) << " " << std::setw(2) << tripPointInfo->getTripId() << std::setw(10)
        << " " << std::setw(6) << tripPointInfo->getThresholdTemp() << std::setw(13) << " "
        << std::setw(10) << tripPointInfo->getHysteresis() << std::setw(9) << " " << std::setw(2)
        << ((event == TripEvent::CROSSED_UNDER) ? "CROSSED_UNDER" : "CROSSED_OVER ") << std::setw(5)
        << " " << std::setw(2) << tripPoints << std::endl;
}

void printThermalZoneHeader() {
    std::cout << "*** Thermal zones ***" << std::endl;
    std::cout << std::setw(2)
              << "+---------------------------------------------------------------------------"
                 "--------------------+"
              << std::endl;
    std::cout << std::setw(3) << "| Tzone Id | " << std::setw(25) << " Type  " << std::setw(5)
              << " | Current Temp  " << std::setw(5) << "|  Passive Temp  |" << std::setw(20)
              << " Trip Points  " << std::endl;
    std::cout << std::setw(2)
              << "+---------------------------------------------------------------------------"
                 "--------------------+"
              << std::endl;
}

void printCoolingDeviceHeader() {
    std::cout << "*** Cooling Devices ***" << std::endl;
    std::cout << std::setw(2)
              << "+--------------------------------------------------------------------------+"
              << std::endl;
    std::cout << std::setw(3) << " | CDev Id " << std::setw(20) << " | CDev Type " << std::setw(5)
              << " | Max Cooling State |" << std::setw(5) << " Current Cooling State |"
              << std::endl;
    std::cout << std::setw(2)
              << "+--------------------------------------------------------------------------+"
              << std::endl;
}

void printTripPointHeader() {
    std::cout << "*** Trip point ***" << std::endl;
    std::cout << std::setw(2)
              << "+---------------------------------------------------------------------------"
                 "--------------------+"
              << std::endl;
    std::cout << std::setw(3) << "| Tzone Id | " << std::setw(10) << "Trip Id | " << std::setw(15)
              << "  Threshold Temp  |"
              << " " << std::setw(8) << "  Hysteresis Temp  |"
              << " " << std::setw(8) << "  Trip Event  |"
              << " " << std::setw(10) << "  Trip Point  |" << std::endl;
    std::cout << std::setw(2)
              << "+---------------------------------------------------------------------------"
                 "--------------------+"
              << std::endl;
}

int main(int argc, char **argv) {
    std::cout << "********* thermal zone info *********" << std::endl;

    // [1] Get thermal factory instance
    auto &thermalFactory = telux::therm::ThermalFactory::getInstance();

    // [2] Prepare initialization callback that is invoked when the thermal sub-system
    //     initialization is complete
    std::promise<telux::common::ServiceStatus> p;
    auto initCb = [&p](telux::common::ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
    };

    // [3] Get thermal manager object
    std::shared_ptr<telux::therm::IThermalManager> thermalMgr
        = thermalFactory.getThermalManager(initCb);
    if (!thermalMgr) {
        std::cout << "Thermal manager is nullptr" << std::endl;
        return -1;
    }

    // [4] Wait for the initialization callback and check the service status
    telux::common::ServiceStatus serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Thermal manager initialization failed" << std::endl;
        return -2;
    }

    // [5] Create the listener object
    std::shared_ptr<ThermalListener> thermalListener
        = std::make_shared<ThermalListener>();

    // [6] To register only trip update notification, likewise it can be registered
    //     for only cooling device state changes notification.
    thermalMgr->registerListener(thermalListener, 1 << TNT_TRIP_UPDATE);

    // [9] To de-register only trip update notification, likewise it can be de-registered
    //     for only cooling device state changes notification. The SSR notification will not
    //     de-registered by default except mask: 0xFFFF.
    thermalMgr->deregisterListener(thermalListener, 1 << TNT_TRIP_UPDATE);

    // [10] Send get thermal zones request using thermal manager object
    std::vector<std::shared_ptr<telux::therm::IThermalZone>> zoneInfo
        = thermalMgr->getThermalZones();
    if (zoneInfo.size() > 0) {
        printThermalZoneHeader();
        for (size_t index = 0; index < zoneInfo.size(); index++) {
            printZoneInfo(zoneInfo[index]);
        }
    } else {
        std::cout << "No thermal zones found!" << std::endl;
    }

    // [11] Send get cooling devices request using thermal manager object
    std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> coolingDevice
        = thermalMgr->getCoolingDevices();
    if (coolingDevice.size() > 0) {
        printCoolingDeviceHeader();

        for (size_t index = 0; index < coolingDevice.size(); index++) {
            printDeviceInfo(coolingDevice[index]);
        }
    } else {
        std::cout << "No cooling devices found!" << std::endl;
    }

    // [12] Send request to get thermal zone for specific id using thermal manager object
    int thermalZoneId = THERMAL_ZONE_ID;
    std::cout << "Thermal zone info by Id: " << thermalZoneId << std::endl;
    std::shared_ptr<telux::therm::IThermalZone> tzInfo = thermalMgr->getThermalZone(thermalZoneId);
    if (tzInfo != nullptr) {
        printThermalZoneHeader();
        printZoneInfo(tzInfo);
        printBindingInfo(tzInfo);
    }
    std::cout << "\n\nPress ENTER to exit \n\n";
    std::cin.ignore();

    // [13] Cleanup when we don't need to listen to anything and when exit the application.
    //      Here the APP is de-registering all notifications. However, the client can choose
    //      to de-register specific as well. For example, to deregister only trip update
    //      notifications, the client may provide mask: 0x0001 or mask: 0x0002 to deregister only
    //      the notification for change in cdev level.
    thermalMgr->deregisterListener(thermalListener);
    thermalListener = nullptr;
    thermalMgr = nullptr;
    return 0;
}
