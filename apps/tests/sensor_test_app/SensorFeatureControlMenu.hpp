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

#ifndef SENSORFEATURECONTROLMENU_HPP
#define SENSORFEATURECONTROLMENU_HPP

#include <vector>
#include <memory>
#include <set>
#include <string>

#include <telux/sensor/SensorFactory.hpp>
#include "ConsoleApp.hpp"
#include "SensorClient.hpp"

using namespace telux::sensor;
using namespace telux::common;

class SensorFeatureControlMenu : public ConsoleApp {
 public:
    SensorFeatureControlMenu(
        std::string appName, std::string cursor, SensorTestAppArguments commandLineArgs);
    ~SensorFeatureControlMenu();
    telux::common::ServiceStatus init(bool shouldInitConsole);
    void cleanup();
    void parseArgs(int argc, char **argv);

 private:
    void initConsole();
    telux::common::ServiceStatus initSensorFeatureManager();
    void listSensorFeatures(std::vector<std::string> userInput);
    void enableSensorFeature(std::vector<std::string> userInput);
    void disableSensorFeature(std::vector<std::string> userInput);
    void listActiveFeatures(std::vector<std::string> userInput);
    void cleanupReinit(std::vector<std::string> userInput);
    void disableFeature(std::string name);

    // Structure instance to store the command line args passed
    SensorTestAppArguments commandLineArgs_;
    // Instance of the sensor feature manager and corresponding event listener
    std::shared_ptr<ISensorFeatureManager> sensorFeatureManager_;
    std::shared_ptr<ISensorFeatureEventListener> sensorFeatureEventListener_;
    std::set<std::string> enabledFeatures_;
};

#endif  // SENSORFEATURECONTROLMENU_HPP
