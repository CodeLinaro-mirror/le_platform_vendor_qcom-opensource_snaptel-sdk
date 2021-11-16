/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       SensorControlMenu.cpp
 *
 * @brief      This file hosts tests for sensor feature control.
 */

#include <algorithm>
#include <chrono>
#include <getopt.h>
#include <iostream>
#include <memory>
#include <future>

#include <telux/sensor/SensorFactory.hpp>
#include "SensorFeatureControlMenu.hpp"
#include "SensorUtils.hpp"
#include "../../common/utils/Utils.hpp"

class SensorFeatureEventListener : public telux::sensor::ISensorFeatureEventListener {
 public:
    void onEvent(SensorFeatureEvent event) {
        SensorUtils::printSensorFeatureEvent(event);
    }
};

SensorFeatureControlMenu::SensorFeatureControlMenu(
    std::string appName, std::string cursor, SensorTestAppArguments commandLineArgs)
   : ConsoleApp(appName, cursor)
   , commandLineArgs_(commandLineArgs) {
}

SensorFeatureControlMenu::~SensorFeatureControlMenu() {
    cleanup();
}

telux::common::ServiceStatus SensorFeatureControlMenu::initSensorFeatureManager() {
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    std::promise<ServiceStatus> prom;
    //  Get the SensorFactory and SensorFeatureManager instances.
    auto &sensorFactory = telux::sensor::SensorFactory::getInstance();
    sensorFeatureManager_ = sensorFactory.getSensorFeatureManager(
        [&prom](telux::common::ServiceStatus status) { prom.set_value(status); });
    if (!sensorFeatureManager_) {
        std::cout << "Failed to get SensorFeatureManager object" << std::endl;
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }
    //  Check if sensor subsystem is ready
    //  If sensor subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = sensorFeatureManager_->getServiceStatus();
    if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nSensor subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }
    //  Exit the application, if SDK is unable to initialize sensor subsystems
    if (managerStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Sensor Subsystems to ready : " << elapsedTime.count() << "s"
                  << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize sensor subsystem" << std::endl;
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }

    sensorFeatureEventListener_ = std::make_shared<SensorFeatureEventListener>();
    if (sensorFeatureManager_->registerListener(sensorFeatureEventListener_)
        != telux::common::Status::SUCCESS) {
        std::cout << "Registration with sensor feature manager failed!!" << std::endl;
    }

    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::ServiceStatus SensorFeatureControlMenu::init(bool shouldInitConsole) {
    telux::common::ServiceStatus serviceStatus = initSensorFeatureManager();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        if (shouldInitConsole) {
            initConsole();
        }
    }
    return serviceStatus;
}

void SensorFeatureControlMenu::initConsole() {
    std::shared_ptr<ConsoleAppCommand> listSensorFeaturesCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "List_Sensor_Features", {},
            std::bind(&SensorFeatureControlMenu::listSensorFeatures, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> enableSensorFeatureCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Enable_Sensor_Feature", {},
            std::bind(
                &SensorFeatureControlMenu::enableSensorFeature, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> disableSensorFeatureCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Disable_Sensor_Feature", {},
            std::bind(
                &SensorFeatureControlMenu::disableSensorFeature, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> listActiveFeaturesCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "List_Active_Features", {},
            std::bind(&SensorFeatureControlMenu::listActiveFeatures, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands = {listSensorFeaturesCommand,
        enableSensorFeatureCommand, disableSensorFeatureCommand, listActiveFeaturesCommand};

    ConsoleApp::addCommands(mainMenuCommands);
    ConsoleApp::displayMenu();
}

void SensorFeatureControlMenu::listSensorFeatures(std::vector<std::string> userInput) {
    std::vector<SensorFeature> features;
    telux::common::Status status = sensorFeatureManager_->getAvailableFeatures(features);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getAvailableFeatures failed: " << std::endl;
        Utils::printStatus(status);
        return;
    }
    std::cout << "Sensor feature request successful" << std::endl;
    for (SensorFeature f : features) {
        SensorUtils::printSensorFeatureInfo(f);
    }
}

void SensorFeatureControlMenu::enableSensorFeature(std::vector<std::string> userInput) {
    std::string name;
    SensorUtils::getInput("Enter feature name: ", name);
    telux::common::Status status = sensorFeatureManager_->enableFeature(name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "enableFeature failed: " << std::endl;
        Utils::printStatus(status);
        return;
    }
    enabledFeatures_.emplace(name);
    std::cout << "Enable sensor feature request successful for " << name << std::endl;
}

void SensorFeatureControlMenu::disableSensorFeature(std::vector<std::string> userInput) {
    std::string name;
    SensorUtils::getInput("Enter feature name: ", name);
    disableFeature(name);
}

void SensorFeatureControlMenu::listActiveFeatures(std::vector<std::string> userInput) {
    for (auto it = enabledFeatures_.begin(); it != enabledFeatures_.end(); ++it) {
        std::cout << "\t" << (*it) << std::endl;
    }
}

void SensorFeatureControlMenu::disableFeature(std::string name) {
    telux::common::Status status = sensorFeatureManager_->disableFeature(name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "disableFeature failed: " << std::endl;
        Utils::printStatus(status);
        return;
    }
    enabledFeatures_.erase(name);
    std::cout << "Disable sensor feature request successful for " << name << std::endl;
}

void SensorFeatureControlMenu::cleanup() {
    sensorFeatureEventListener_ = nullptr;
    for (auto it = enabledFeatures_.begin(); it != enabledFeatures_.end(); ++it) {
        disableFeature(*it);
    }
    sensorFeatureManager_ = nullptr;
}
