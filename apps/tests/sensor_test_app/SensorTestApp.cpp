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

/**
 * @file       sensorTestApp.cpp
 *
 * @brief      This is entry class for test application for sensor,
 *             It allows one to interactively invoke most of the public APIs in sensor.
 */

#include <getopt.h>
#include <iostream>
#include <memory>

extern "C" {
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
}

#include <telux/sensor/SensorFactory.hpp>
#include "SensorTestApp.hpp"
#include "SensorControlMenu.hpp"
#include "SensorFeatureControlMenu.hpp"
#include <telux/common/Version.hpp>
#include "../../common/utils/Utils.hpp"

std::shared_ptr<SensorTestApp> sensorTestApp;

SensorTestApp::SensorTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor)
   , verboseNotification_(false) {
}

SensorTestApp::~SensorTestApp() {
}

telux::common::ServiceStatus SensorTestApp::init() {
    initConsole();
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

void SensorTestApp::initConsole() {
    std::shared_ptr<ConsoleAppCommand> sensorControlMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Sensor_Control", {},
            std::bind(&SensorTestApp::sensorControlMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> sensorFeatureControlMenuCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Sensor_Feature_Control", {},
            std::bind(&SensorTestApp::sensorFeatureControlMenu, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
        = {sensorControlMenuCommand, sensorFeatureControlMenuCommand};

    ConsoleApp::addCommands(mainMenuCommands);
    ConsoleApp::displayMenu();
}

void SensorTestApp::sensorControlMenu(std::vector<std::string> userInput) {
    SensorControlMenu sensorControlMenu(
        "Sensor control menu", "sensor_control> ", verboseNotification_);
    if (sensorControlMenu.init(true) != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Failed to initialize sensor control menu" << std::endl;
        return;
    }
    sensorControlMenu.mainLoop();
    displayMenu();
}

void SensorTestApp::sensorFeatureControlMenu(std::vector<std::string> userInput) {
    SensorFeatureControlMenu sensorFeatureControl(
        "Sensor feature control menu", "sensor_feature_control> ", verboseNotification_);
    if (sensorFeatureControl.init(true) != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Failed to initialize sensor feature control menu" << std::endl;
        return;
    }
    sensorFeatureControl.mainLoop();
    displayMenu();
}

void SensorTestApp::printHelp(std::string programName) {
    std::cout << "Usage: " << programName << " [-nh]" << std::endl
              << std::endl
              << "-n        Enable detailed notification information" << std::endl
              << "-h        This help" << std::endl;
}

void SensorTestApp::parseArgs(int argc, char **argv) {
    int c = -1;
    static const struct option long_options[]
        = {{"notification configuration", no_argument, 0, 'n'}, {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}};
    int option_index = 0;
    c = getopt_long(argc, argv, "nh", long_options, &option_index);
    if (c == -1) {
        return;
    }
    do {
        switch (c) {
            case 'n': {
                std::cout << "Enabling verbose notification" << std::endl;
                verboseNotification_ = true;
                break;
            }
            case 'h': {
                printHelp(argv[0]);
                exit(0);
            }
        }
        c = getopt_long(argc, argv, "nh", long_options, &option_index);
    } while (c != -1);
}

int main(int argc, char **argv) {
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string appName = "Sensor test app - SDK v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch);
    sensorTestApp = std::make_shared<SensorTestApp>(appName, "sensor> ");
    sensorTestApp->parseArgs(argc, argv);
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "sensors"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    // initialize commands and display
    if (sensorTestApp->init() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        return -1;
    }
    sensorTestApp->mainLoop();  // Main loop to continuously read and execute commands
    return 0;
}
