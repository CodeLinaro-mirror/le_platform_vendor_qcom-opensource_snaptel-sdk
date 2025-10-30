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

/*  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
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
    void onBufferedEvent(
        std::string sensorName, std::shared_ptr<std::vector<SensorEvent>> events, bool isLast) {
        for (SensorEvent s : *(events.get())) {
            SensorUtils::printSensorFeatureBufferedEvent(s);
        }
        std::cout << " Received events from " << sensorName << " count - " << events->size()
                  << " isLast - " << isLast << std::endl;
    }

    void onMotionDetectionEnabled(std::vector<MotionDetectionConfig> motionDetectionConfigs) {
        std::cout << "Received onMotionDetectionEnabled event" << std::endl;
        SensorUtils::printMotionDetectionConfigs(motionDetectionConfigs);
    }

    void onMotionDetectionDisabled() {
        std::cout << "Received onMotionDetectionDisabled event - No active configurations" << std::endl;
    }

    void onMotionDetected(int sensorId, struct iio_event_data event) {
        std::cout << "Received onMotionDetected event" << std::endl;
        std::cout << "Sensor ID: " << sensorId << std::endl;
        std::cout << "Event ID: " << event.id << std::endl;
        std::cout << "Timestamp: " << event.timestamp << std::endl;
    }
};

void SensorFeatureControlMenu::onTcuActivityStateUpdate(TcuActivityState tcuState,
    std::string machineName) {
#ifdef TELSDK_FEATURE_POWER_ENABLED
    std::cout << " TCU Activity state changed for machine "
        << machineName << std::endl;
    SensorUtils::printTcuActivityState(tcuState);

    if ((tcuState == TcuActivityState::SUSPEND) || (tcuState == TcuActivityState::SHUTDOWN)) {
        // enable MLC feature
        for (auto it = enabledFeaturesFifo_.begin(); it != enabledFeaturesFifo_.end(); ++it) {
            enableFeature(*it, false);
        }
        Status ackStatus = tcuActivityMgr_->sendActivityStateAck(StateChangeResponse::ACK,
            tcuState);
        if (ackStatus == Status::SUCCESS) {
            std::cout << " Sent acknowledgement" << std::endl;
        } else {
            std::cout << " Failed to send acknowledgement !" << std::endl;
        }
    } else if (tcuState == TcuActivityState::RESUME) {
        // disable MLC feature
        for (auto it = enabledFeaturesFifo_.begin(); it != enabledFeaturesFifo_.end(); it++) {
            if(enabledFeatures_.find(*it) != enabledFeatures_.end() &&
                enabledFeaturesNonBuffered_.find(*it) == enabledFeaturesNonBuffered_.end()) {
                disableFeature(*it, false);
            }
        }
    }
#endif
}

void SensorFeatureControlMenu::initTcuPowerMgr() {
#ifdef TELSDK_FEATURE_POWER_ENABLED
#ifdef TELUX_FOR_EXTERNAL_AP
    std::cout << " Connecting to REMOTE TCU Activity Manager " << std::endl;
    telux::common::ProcType procType = telux::common::ProcType::REMOTE_PROC;
#else
    std::cout << " Connecting to LOCAL TCU Activity Manager " << std::endl;
    telux::common::ProcType procType = telux::common::ProcType::LOCAL_PROC;
#endif
    std::cout << " Initializing the client as a SLAVE " << std::endl;

    // Get power factory instance
    auto &powerFactory = PowerFactory::getInstance();
    // Get TCU-activity manager object
    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
    tcuActivityMgr_ = powerFactory.getTcuActivityManager(ClientType::SLAVE, procType,
        [&](telux::common::ServiceStatus status) { prom.set_value(status); });
    if (tcuActivityMgr_ == nullptr) {
        std::cout << " ERROR - Failed to get manager instance" << std::endl;
        return;
    }
    // Wait for TCU-activity manager to be ready
    std::cout << " Waiting for TCU Activity Manager to be ready " << std::endl;
    telux::common::ServiceStatus serviceStatus = prom.get_future().get();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " TCU-activity manager is ready" << std::endl;
    } else {
        std::cout << " Failed to initialize TCU-activity manager" << std::endl;
    }

    // Registering a listener for TCU-activity state updates
    // tcuActivityListener_ = std::make_shared<TcuActivityListener>();
    // if (tcuActivityMgr_->registerListener(tcuActivityListener_)
    if (tcuActivityMgr_->registerListener(shared_from_this()) != telux::common::Status::SUCCESS) {
        std::cout << " ERROR - Failed to register for TCU-activity state updates" << std::endl;
    } else {
        std::cout << " Registered Listener for TCU-activity state updates" << std::endl;
    }
#else
    std::cout << " Power manager is not initialized" << std::endl;
#endif // TELSDK_FEATURE_POWER_ENABLED
}

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
        initTcuPowerMgr();
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

    std::shared_ptr<ConsoleAppCommand> enableSensorFeatureFifoCommand
        = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("5", "Enable_Sensor_Feature_On_Suspend", {},
                std::bind(&SensorFeatureControlMenu::enableSensorFeatureFifo, this,
                    std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> skipSensorFeatureOnSuspendCommand
        = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("6", "Skip_Sensor_Feature_On_Suspend", {},
                std::bind(&SensorFeatureControlMenu::skipSensorFeatureOnSuspend, this,
                    std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> listSensorFeaturesQueuedOnSuspendCommand
        = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("7", "List_Sensor_Features_Queued_On_Suspend", {},
                std::bind(&SensorFeatureControlMenu::listSensorFeaturesQueuedOnSuspend, this,
                    std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> listAvailableSensorsCommand
        = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("8", "List_Available_Sensors", {},
                std::bind(&SensorFeatureControlMenu::listAvailableSensors, this,
                    std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> getMotionDetectionConfigLimitsCommand
        = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("9", "Get_Motion_Detection_Config_Limits", {},
                std::bind(&SensorFeatureControlMenu::getMotionDetectionConfigLimits, this,
                    std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> enableMotionDetectionCommand
        = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("10", "Enable_Motion_Detection", {},
                std::bind(&SensorFeatureControlMenu::enableMotionDetection, this,
                    std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> disableMotionDetectionCommand
        = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("11", "Disable_Motion_Detection", {},
                std::bind(&SensorFeatureControlMenu::disableMotionDetection, this,
                    std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> getMotionDetectionConfigsCommand
        = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("12", "Get_Motion_Detection_Configs", {},
                std::bind(&SensorFeatureControlMenu::getMotionDetectionConfigs, this,
                    std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
        = {listSensorFeaturesCommand, enableSensorFeatureCommand, disableSensorFeatureCommand,
            listActiveFeaturesCommand, enableSensorFeatureFifoCommand,
            skipSensorFeatureOnSuspendCommand, listSensorFeaturesQueuedOnSuspendCommand,
            listAvailableSensorsCommand, getMotionDetectionConfigLimitsCommand,
            enableMotionDetectionCommand, disableMotionDetectionCommand,
            getMotionDetectionConfigsCommand};

    ConsoleApp::addCommands(mainMenuCommands);
    ConsoleApp::displayMenu();
}

void SensorFeatureControlMenu::enableSensorFeatureFifo(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_POWER_ENABLED
    std::string name;
    SensorUtils::getInput("Enter feature name: ", name);

    std::vector<SensorFeature> features;
    telux::common::Status status = sensorFeatureManager_->getAvailableFeatures(features);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getAvailableFeatures failed" << std::endl;
        Utils::printStatus(status);
        return;
    }
    for (SensorFeature f : features) {
        if(f.name == name) {
            enabledFeaturesFifo_.emplace(name);
            std::cout << "Enable sensor feature fifo request queued for " << name << std::endl;
            return;
        }
    }
    std::cout << "Enable sensor feature fifo request failed for " << name << std::endl;
#else
    std::cout
        << "Enabling sensor feature on suspend not possible since power feature is not enabled";
#endif  // TELSDK_FEATURE_POWER_ENABLED
}

void SensorFeatureControlMenu::skipSensorFeatureOnSuspend(std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_POWER_ENABLED
    std::string name;
    SensorUtils::getInput("Enter feature name: ", name);
    if (enabledFeaturesFifo_.erase(name) == 0) {  // No element was erased
        std::cout << "Sensor feature " << name << " not found in fifo queue" << std::endl;
    } else {
        std::cout << "Sensor feature fifo request removed for " << name << std::endl;
    }
#else
    std::cout
        << "Enabling sensor feature on suspend not possible since power feature is not enabled";
#endif  // TELSDK_FEATURE_POWER_ENABLED
}

void SensorFeatureControlMenu::listSensorFeaturesQueuedOnSuspend(
    std::vector<std::string> userInput) {
#ifdef TELSDK_FEATURE_POWER_ENABLED
    if (enabledFeaturesFifo_.empty()) {
        std::cout << "No features have been queued to be enabled on suspend" << std::endl;
        return;
    }
    std::cout << "Features queued to be enabled on suspend" << std::endl;
    for (auto it = enabledFeaturesFifo_.begin(); it != enabledFeaturesFifo_.end(); ++it) {
        std::cout << "\t" << (*it) << std::endl;
    }
#else
    std::cout
        << "Enabling sensor feature on suspend not possible since power feature is not enabled";
#endif  // TELSDK_FEATURE_POWER_ENABLED
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
    enableFeature(name, true);
}

void SensorFeatureControlMenu::disableSensorFeature(std::vector<std::string> userInput) {
    std::string name;
    SensorUtils::getInput("Enter feature name: ", name);
    disableFeature(name, true);
}

void SensorFeatureControlMenu::listActiveFeatures(std::vector<std::string> userInput) {
    for (auto it = enabledFeatures_.begin(); it != enabledFeatures_.end(); ++it) {
        std::cout << "\t" << (*it) << std::endl;
    }
}

void SensorFeatureControlMenu::enableFeature(std::string name, bool isLive) {
    telux::common::Status status = sensorFeatureManager_->enableFeature(name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "enableFeature failed: " << name << std::endl;
        Utils::printStatus(status);
        return;
    }
    enabledFeatures_.emplace(name);
    if(isLive) {
      enabledFeaturesNonBuffered_.emplace(name);
    }
    std::cout << "Enable sensor feature request successful for " << name << std::endl;
}

std::set<std::string>::iterator SensorFeatureControlMenu::disableFeature(std::string name, bool isLive) {
    telux::common::Status status = sensorFeatureManager_->disableFeature(name);
    std::set<std::string>::iterator itr = enabledFeatures_.find(name);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "disableFeature failed: " << std::endl;
        Utils::printStatus(status);
        if(itr != enabledFeatures_.end()) {
            itr++;
        }
    } else {
        if(itr != enabledFeatures_.end()) {
            std::cout << "Disable sensor feature request successful for " << name << std::endl;
            itr = enabledFeatures_.erase(itr);
            if (isLive) {
              enabledFeaturesNonBuffered_.erase(name);
            }
        } else {
            std::cout << "Disable sensor feature request failed for " << name << std::endl;
        }
    }
    return itr;
}

void SensorFeatureControlMenu::cleanup() {
    sensorFeatureEventListener_ = nullptr;
    for (auto it = enabledFeatures_.begin(); it != enabledFeatures_.end(); ) {
        it = disableFeature(*it, true);
    }
    sensorFeatureManager_ = nullptr;
    tcuActivityMgr_ = nullptr;
}

void SensorFeatureControlMenu::listAvailableSensors(std::vector<std::string> userInput) {
    std::vector<SensorInfo> info;
    telux::common::Status status = sensorFeatureManager_->getAvailableSensorInfo(info);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getAvailableSensorInfo failed: " << std::endl;
        Utils::printStatus(status);
        return;
    }
    std::cout << "Sensor info request successful" << std::endl;
    for (SensorInfo i : info) {
        SensorUtils::printSensorInfo(i);
    }
}

void SensorFeatureControlMenu::getMotionDetectionConfigLimits(std::vector<std::string> userInput) {
    int sensorId;
    SensorUtils::getInput("Enter sensorId: ", sensorId);
    MotionDetectionConfigLimits motionDetectionConfigLimits = {};
    telux::common::Status ret = sensorFeatureManager_->getMotionDetectionConfigLimits(sensorId,
        motionDetectionConfigLimits);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Request failed with: ";
        Utils::printStatus(ret);
    } else {
        std::cout << "Request success " << std::endl;
        SensorUtils::printMotionDetectionConfigLimits(motionDetectionConfigLimits);
    }
}

void SensorFeatureControlMenu::enableMotionDetection(std::vector<std::string> userInput) {
    MotionDetectionConfig motionDetectionConfig = {};
    SensorUtils::getInput("Enter sensorId: ", motionDetectionConfig.sensorId);
    SensorUtils::getInput("Enter threshold: ", motionDetectionConfig.threshold);
    SensorUtils::getInput("Enter duration: ", motionDetectionConfig.duration);
    SensorUtils::getInput("Enter samplingRate: ", motionDetectionConfig.samplingRate);
    telux::common::Status ret = sensorFeatureManager_->enableMotionDetection(motionDetectionConfig);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Request failed with: ";
        Utils::printStatus(ret);
    } else {
        std::cout << "Request success" << std::endl;
    }
}

void SensorFeatureControlMenu::disableMotionDetection(std::vector<std::string> userInput) {
    int sensorId;
    SensorUtils::getInput("Enter sensorId: ", sensorId);
    telux::common::Status ret = sensorFeatureManager_->disableMotionDetection(sensorId);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Request failed with: ";
        Utils::printStatus(ret);
    } else {
        std::cout << "Request success" << std::endl;
    }
}

void SensorFeatureControlMenu::getMotionDetectionConfigs(std::vector<std::string> userInput) {
    std::vector<MotionDetectionConfig> motionDetectionConfigs;
    telux::common::Status ret =
        sensorFeatureManager_->getMotionDetectionConfigs(motionDetectionConfigs);
    if(ret != telux::common::Status::SUCCESS) {
        std::cout << "Request failed with: ";
        Utils::printStatus(ret);
    } else {
        std::cout << "Request success" << std::endl;
        SensorUtils::printMotionDetectionConfigs(motionDetectionConfigs);
    }
}
