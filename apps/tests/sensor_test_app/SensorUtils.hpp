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

#ifndef SENSORUTILS_HPP
#define SENSORUTILS_HPP

#include <memory>
#include <limits>
#include <telux/sensor/SensorDefines.hpp>
#include <telux/sensor/Sensor.hpp>

#include "SensorClient.hpp"

using namespace telux::sensor;

class SensorUtils {
 public:
    static std::string getSensorType(SensorType type);
    static void printSensorInfo(SensorInfo info, bool more = false, std::ostream &os = std::cout);
    static std::string getSupportedRates(SensorInfo info);
    static std::string getBatchCountLimits(SensorInfo info);
    static SensorConfiguration getSensorConfig(std::shared_ptr<SensorClient> s);
    static std::shared_ptr<SensorClient> getSensor(
        int cid, std::vector<std::shared_ptr<SensorClient>> &sensors);
    template <typename T>
    static void getInput(std::string prompt, T &input) {
        std::cout << prompt;
        std::cin >> input;
        bool valid = false;
        do {
            if (std::cin.good()) {
                valid = true;
            } else {
                // If an error occurs then an error flag is set and future attempts to get
                // input will fail. Clear the error flag on cin.
                std::cin.clear();
                // Extracts characters from the previous input sequence and discards them,
                // until entire stream have been extracted, or one compares equal to newline.
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input, please re-enter" << std::endl;
                std::cout << prompt;
                std::cin >> input;
            }
        } while (!valid);
    }
    static void printSensorEvent(SensorType type, SensorEvent &s, std::string &tag);
    static bool isUncalibratedSensor(SensorType type);
    static void printSensorFeatureInfo(SensorFeature feature);
    static void printSensorFeatureEvent(SensorFeatureEvent event);
};

#endif  // SENSORUTILS_HPP
