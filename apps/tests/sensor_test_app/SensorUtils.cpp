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
 * @file       SensorUtils.cpp
 *
 * @brief      Sensor Utility class
 */

#include <iostream>
#include <ctime>

#include "SensorUtils.hpp"
#include "../../common/utils/Utils.hpp"
#include <telux/sensor/SensorDefines.hpp>

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"

std::string SensorUtils::getSensorType(SensorType type) {
    switch (type) {
        case (SensorType::GYROSCOPE): {
            return "Gyroscope";
        }
        case (SensorType::ACCELEROMETER): {
            return "Accelerometer";
        }
        case (SensorType::GYROSCOPE_UNCALIBRATED): {
            return "Uncalibrated Gyroscope";
        }
        case (SensorType::ACCELEROMETER_UNCALIBRATED): {
            return "Uncalibrated Accelerometer";
        }
        default: {
            return "Unknown sensor type";
        }
    }
}

bool SensorUtils::isUncalibratedSensor(SensorType type) {
    return ((type == SensorType::GYROSCOPE_UNCALIBRATED)
            || (type == SensorType::ACCELEROMETER_UNCALIBRATED));
}

void SensorUtils::printSensorInfo(SensorInfo info) {
    std::cout << "ID: " << info.id << ", type: " << getSensorType(info.type)
              << ", name: " << info.name << ", vendor: " << info.vendor << ", Sampling rates: [ ";
    for (auto rate : info.samplingRates) {
        std::cout << rate << ", ";
    }
    std::cout << "\b\b ], Max sampling rate: " << info.maxSamplingRate
              << ", Max count: " << info.maxBatchCountSupported
              << ", Min count: " << info.minBatchCountSupported << ", Range: " << info.range
              << std::endl;
}

std::string SensorUtils::getSupportedRates(SensorInfo info) {
    std::string supportedRates = "[ ";
    for (float f : info.samplingRates) {
        supportedRates.append(std::to_string(f)).append(", ");
    }
    supportedRates.append("\b\b ], <= ").append(std::to_string(info.maxSamplingRate));
    return supportedRates;
}

std::string SensorUtils::getBatchCountLimits(SensorInfo info) {
    std::string batchCountLimits = "[ ";
    batchCountLimits.append(std::to_string(info.minBatchCountSupported) + ", "
                            + std::to_string(info.maxBatchCountSupported) + " ]");
    return batchCountLimits;
}

SensorConfiguration SensorUtils::getSensorConfig(std::shared_ptr<SensorClient> s) {
    // If sensor type == GYRO | ACCELERO, get sampling rate and batch count
    std::shared_ptr<ISensor> sensor = s->getSensor();
    SensorType type = sensor->getSensorInfo().type;
    if ((type == SensorType::GYROSCOPE) || (type == SensorType::ACCELEROMETER)
        || ((type == SensorType::GYROSCOPE_UNCALIBRATED)
            || (type == SensorType::ACCELEROMETER_UNCALIBRATED))) {
        float samplingRate;
        float batchCount;
        std::string supportedRates = getSupportedRates(sensor->getSensorInfo());
        std::string batchCountLimits = getBatchCountLimits(sensor->getSensorInfo());
        SensorUtils::getInput("Enter sampling rate " + supportedRates + ": ", samplingRate);
        SensorUtils::getInput("Enter batch count " + batchCountLimits + ": ", batchCount);

        // Set the sensor configuration
        SensorConfiguration s;
        s.samplingRate = samplingRate;
        s.batchCount = batchCount;
        s.validityMask.set(SensorConfigParams::SAMPLING_RATE);
        s.validityMask.set(SensorConfigParams::BATCH_COUNT);
        return s;
    }
    return SensorConfiguration();
}

std::shared_ptr<SensorClient> SensorUtils::getSensor(
    int cid, std::vector<std::shared_ptr<SensorClient>> &sensors) {

    std::shared_ptr<SensorClient> sensor = nullptr;
    for (auto s : sensors) {
        if (s->id_ == cid) {
            sensor = s;
            break;
        }
    }
    if (sensor == nullptr) {
        std::cout << "Sensor with client ID " << cid << " not available" << std::endl;
    }
    return sensor;
}

void SensorUtils::printSensorEvent(SensorType type, SensorEvent &s, std::string &tag) {
    if (isUncalibratedSensor(type)) {
        print_notification << tag << s.timestamp << ", " << s.uncalibrated.data.x << ", "
                           << s.uncalibrated.data.y << ", " << s.uncalibrated.data.z << ", "
                           << s.uncalibrated.bias.x << ", " << s.uncalibrated.bias.y << ", "
                           << s.uncalibrated.bias.z << std::endl;
    } else {
        print_notification << tag << s.timestamp << ", " << s.calibrated.x << ", " << s.calibrated.y
                           << ", " << s.calibrated.z << std::endl;
    }
}

void SensorUtils::printSensorFeatureInfo(SensorFeature feature) {
    std::cout << "Feature name: " << feature.name << std::endl;
}

void SensorUtils::printSensorFeatureEvent(SensorFeatureEvent event) {
    print_notification << "Sensor feature event " << event.id << " from feature " << event.name
                       << " @ " << event.timestamp << std::endl;
}