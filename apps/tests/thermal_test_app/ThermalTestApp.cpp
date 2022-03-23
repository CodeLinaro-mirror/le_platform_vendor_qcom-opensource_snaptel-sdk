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
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <memory>
#include <iomanip>
#include <csignal>

#include <telux/common/Version.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/therm/ThermalFactory.hpp>
#include "../../common/utils/Utils.hpp"

#include "ThermalHelper.hpp"
#include "ThermalTestApp.hpp"
#include "ThermalListener.hpp"

using namespace telux::common;

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

std::shared_ptr<ThermalTestApp> thermalTestApp_ = nullptr;
auto sdkVersion = telux::common::Version::getSdkVersion();
std::string sdkReleaseName = telux::common::Version::getReleaseName();
std::string APP_NAME = "Thermal Test App v" + std::to_string(sdkVersion.major) + "."
                       + std::to_string(sdkVersion.minor) + "." + std::to_string(sdkVersion.patch)
                       + "\n" + "Release name: " + sdkReleaseName;

ThermalTestApp::ThermalTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

ThermalTestApp::~ThermalTestApp() {
    thermalManagerMap_.clear();
}

void signalHandler(int signum) {
    thermalTestApp_->signalHandler(signum);
}

void ThermalTestApp::signalHandler(int signum) {
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    cleanup();
    exit(1);
}

void ThermalTestApp::cleanup() {
    for (auto thermalManager : thermalManagerMap_) {
        auto procType = thermalManager.first;
        auto manager = thermalManager.second;
        if (manager) {
            Status status = manager->deregisterListener(thermalListenerMap_[procType]);
            if (status == Status::SUCCESS) {
                std::cout << "Deregister for Thermal Listener succeed." << std::endl;
            } else {
                std::cout << "Deregister for Thermal Listener failed." << std::endl;
            }
        }
    }
    thermalManagerMap_.clear();
}

bool ThermalTestApp::init() {
    bool initStatus = false;
    int cid = -1;

    do {
        ThermalTestApp::getInput(
            "Select the application processor for operations(1-LOCAL/2-REMOTE/3-BOTH): ", cid);
        if (cid == 1) {
            initStatus = initThermalManager(telux::common::ProcType::LOCAL_PROC);
        } else if (cid == 2) {
            initStatus = initThermalManager(telux::common::ProcType::REMOTE_PROC);
        } else if (cid == 3) {
            initThermalManager(telux::common::ProcType::LOCAL_PROC);
            initStatus |= initThermalManager(telux::common::ProcType::REMOTE_PROC);
        } else {
            std::cout << " Invalid input:  " << cid << ", please re-enter" << std::endl;
        }
    } while ((cid != 1) && (cid != 2) && (cid != 3));

    if (!initStatus) {
        return false;
    }

    std::shared_ptr<ConsoleAppCommand> thermalZonesCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "thermal_zones", {},
            std::bind(&ThermalTestApp::getThermalZones, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> coolingDevicesCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "cooling_devices", {},
            std::bind(&ThermalTestApp::getCoolingDevices, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> thermalZoneByIdCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "thermal_zone_by_id", {},
            std::bind(&ThermalTestApp::getThermalZoneById, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> coolingDeviceByIdCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "cooling_device_by_id", {},
            std::bind(&ThermalTestApp::getCoolingDeviceById, this, std::placeholders::_1)));
    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListThermalSubMenu
        = {thermalZonesCommand, coolingDevicesCommand, thermalZoneByIdCommand,
            coolingDeviceByIdCommand};
    addCommands(commandsListThermalSubMenu);
    ConsoleApp::displayMenu();
    return true;
}

bool ThermalTestApp::initThermalManager(telux::common::ProcType procType) {
    // Get thermal factory instance
    auto &thermalFactory = telux::therm::ThermalFactory::getInstance();
    // Get thermal manager instance
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    auto thermalManager = thermalFactory.getThermalManager(
        [&](ServiceStatus status) { prom.set_value(status); }, procType);

    if (thermalManager == nullptr) {
        std::cout << " ERROR - Failed to get thermal manager instance \n";
        return false;
    }

    thermalListenerMap_.emplace(procType, std::make_shared<ThermalListener>());
    Status status = thermalManager->registerListener(thermalListenerMap_[procType]);
    if (status == Status::SUCCESS) {
        std::cout << "Register for Thermal Listener succeed." << std::endl;
    } else {
        std::cout << "Register for Thermal Listener failed." << std::endl;
    }

    std::cout << " thermal manager instance returned for proc type:" << static_cast<int>(procType);
    thermalManagerMap_.emplace(procType, thermalManager);
    // Wait for thermal manager to be ready
    ServiceStatus mgrStatus = prom.get_future().get();
    if (mgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Thermal Subsystem is ready " << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize Thermal subsystem" << std::endl;
        return false;
    }
    return true;
}

void ThermalTestApp::printThermalZoneHeader() {
    std::cout << "*** Thermal zones ***" << std::endl;
    std::cout << std::setw(2)
              << "+---------------------------------------------------------------------------"
                 "--------------------+"
              << std::endl;
    std::cout << std::setw(3) << "| Tzone Id | " << std::setw(10) << "Type  " << std::setw(35)
              << " | Current Temp  " << std::setw(5) << "|  Passive Temp  |" << std::setw(20)
              << " Trip Points  " << std::endl;
    std::cout << std::setw(2)
              << "+---------------------------------------------------------------------------"
                 "--------------------+"
              << std::endl;
}

int ThermalTestApp::readAndValidateProcType() {
    int operationType = -1;
    do {
        ThermalTestApp::getInput(" Enter operation type (0 - LOCAL, 1 - REMOTE)", operationType);
        if ((operationType < 0) || (operationType > 1)) {
            std::cout << " Invalid input:  " << operationType << ", please re-enter" << std::endl;
        }
    } while ((operationType != 0) && (operationType != 1));
    return operationType;
}

void ThermalTestApp::getThermalZones(std::vector<std::string> userInput) {

    int operationType = readAndValidateProcType();
    telux::common::ProcType procType = static_cast<telux::common::ProcType>(operationType);
    if (thermalManagerMap_.find(procType) == thermalManagerMap_.end()) {
        std::cout << " Thermal manager is not ready for operation type: " << operationType;
        return;
    }
    std::vector<std::shared_ptr<telux::therm::IThermalZone>> zoneInfo
        = thermalManagerMap_[procType]->getThermalZones();
    if (zoneInfo.size() > 0) {
        printThermalZoneHeader();
        for (size_t index = 0; index < zoneInfo.size(); index++) {
            if (zoneInfo[index]) {
                ThermalHelper::printThermalZoneInfo(zoneInfo[index]);
            } else {
                std::cout << "No thermal zone found at index: " << index << std::endl;
            }
        }
    }
}

void ThermalTestApp::getThermalZoneById(std::vector<std::string> userInput) {
    int thermalZoneId = -1;
    std::cout << "Enter thermal zone id: ";
    ThermalTestApp::getInput("Enter thermal zone id: ", thermalZoneId);
    int operationType = readAndValidateProcType();
    telux::common::ProcType procType = static_cast<telux::common::ProcType>(operationType);
    if (thermalManagerMap_.find(procType) == thermalManagerMap_.end()) {
        std::cout << " Thermal manager is not ready for operation type: " << operationType;
        return;
    }
    std::cout << "Thermal zone Id: " << thermalZoneId << std::endl;
    std::shared_ptr<telux::therm::IThermalZone> tzInfo
        = thermalManagerMap_[procType]->getThermalZone(thermalZoneId);
    if (tzInfo != nullptr) {
        printThermalZoneHeader();
        ThermalHelper::printThermalZoneInfo(tzInfo);
        ThermalHelper::printBindingInfo(tzInfo);
    } else {
        std::cout << "No thermal zone found for Id: " << thermalZoneId << std::endl;
    }
}

void ThermalTestApp::printCoolingDeviceHeader() {
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

void ThermalTestApp::getCoolingDevices(std::vector<std::string> userInput) {
    int operationType = readAndValidateProcType();
    telux::common::ProcType procType = static_cast<telux::common::ProcType>(operationType);
    if (thermalManagerMap_.find(procType) == thermalManagerMap_.end()) {
        std::cout << " Thermal manager is not ready for operation type: " << operationType;
        return;
    }
    std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> coolingDevice
        = thermalManagerMap_[procType]->getCoolingDevices();
    if (coolingDevice.size() > 0) {
        printCoolingDeviceHeader();
        for (size_t index = 0; index < coolingDevice.size(); index++) {
            if (coolingDevice[index]) {
                ThermalHelper::printCoolingDevInfo(coolingDevice[index]);
            } else {
                std::cout << "No cooling devices found at index: " << index << std::endl;
            }
        }
    } else {
        std::cout << "No cooling devices found!" << std::endl;
    }
}

void ThermalTestApp::getCoolingDeviceById(std::vector<std::string> userInput) {
    int coolingDevId = -1;
    ThermalTestApp::getInput("Enter cooling device Id: ", coolingDevId);
    int operationType = readAndValidateProcType();
    telux::common::ProcType procType = static_cast<telux::common::ProcType>(operationType);
    if (thermalManagerMap_.find(procType) == thermalManagerMap_.end()) {
        std::cout << " Thermal manager is not ready for operation type: " << operationType;
        return;
    }
    std::cout << "Cooling device Id: " << coolingDevId << std::endl;
    std::shared_ptr<telux::therm::ICoolingDevice> cdev
        = thermalManagerMap_[procType]->getCoolingDevice(coolingDevId);
    if (cdev != nullptr) {
        printCoolingDeviceHeader();
        ThermalHelper::printCoolingDevInfo(cdev);
    } else {
        std::cout << "No cooling device found for Id: " << coolingDevId << std::endl;
    }
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    thermalTestApp_ = std::make_shared<ThermalTestApp>(APP_NAME, "Therm> ");
    signal(SIGINT, signalHandler);
    // initialize commands and display
    if (!thermalTestApp_->init()) {
        return 1;
    }
    return thermalTestApp_->mainLoop();  // Main loop to continuously read and execute commands
}
