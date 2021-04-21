/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
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

#include <future>
#include <iostream>
#include <limits>
#include <vector>
#include <condition_variable>

#include <telux/sensor/SensorFactory.hpp>

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m"

class SensorEventListener : public telux::sensor::ISensorEventListener {
 public:
    SensorEventListener(telux::sensor::SensorInfo info)
       : info_(info) {
    }

    // [11] Receive sensor events. This notification is received every time the configured batch
    // count is available with the sensor framework
    virtual void onEvent(std::shared_ptr<std::vector<telux::sensor::SensorEvent>> events) override {
        PRINT_NOTIFICATION << ": Received " << events->size()
                           << " events from sensor: " << info_.name << std::endl;
        for (telux::sensor::SensorEvent s : *(events.get())) {
            printSensorEvent(s);
        }
    }

    // [9] Receive configuration updates
    virtual void onConfigurationUpdate(telux::sensor::SensorConfiguration configuration) override {
        PRINT_NOTIFICATION << ": Received configuration update from sensor: " << info_.name << ": ["
                           << configuration.samplingRate << ", " << configuration.batchCount << " ]"
                           << std::endl;
    }

 private:
    bool isUncalibratedSensor(telux::sensor::SensorType type) {
        return ((type == telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED)
                || (type == telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED));
    }
    void printSensorEvent(telux::sensor::SensorEvent &s) {
        if (isUncalibratedSensor(info_.type)) {
            PRINT_NOTIFICATION << ": name " << info_.name << ": " << s.timestamp << ", "
                               << s.uncalibrated.data.x << ", " << s.uncalibrated.data.y << ", "
                               << s.uncalibrated.data.z << ", " << s.uncalibrated.bias.x << ", "
                               << s.uncalibrated.bias.y << ", " << s.uncalibrated.bias.z
                               << std::endl;
        } else {
            PRINT_NOTIFICATION << ": name " << info_.name << ": " << s.timestamp << ", "
                               << s.calibrated.x << ", " << s.calibrated.y << ", " << s.calibrated.z
                               << std::endl;
        }
    }
    telux::sensor::SensorInfo info_;
};

std::string getSensorType(telux::sensor::SensorType type) {
    switch (type) {
        case (telux::sensor::SensorType::GYROSCOPE): {
            return "Gyroscope";
        }
        case (telux::sensor::SensorType::ACCELEROMETER): {
            return "Accelerometer";
        }
        case (telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED): {
            return "Uncalibrated Gyroscope";
        }
        case (telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED): {
            return "Uncalibrated Accelerometer";
        }
        default: {
            return "Unknown sensor type";
        }
    }
}

void printSensorInfo(telux::sensor::SensorInfo info) {
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

std::string getSensorName(
    std::vector<telux::sensor::SensorInfo> &sensorInfo, telux::sensor::SensorType type) {
    for (auto &info : sensorInfo) {
        if (info.type == type) {
            return info.name;
        }
    }
    return "";
}

float getMinimumSamplingRate(telux::sensor::SensorInfo info) {
    printSensorInfo(info);
    float min = std::numeric_limits<float>::infinity();
    for (float r : info.samplingRates) {
        if (r < min) {
            min = r;
        }
    }
    return min;
}

int main(int argc, char **argv) {
    std::cout << "********* sensor sample app *********" << std::endl;
    // [1] Get sensor factory instance
    auto &sensorFactory = telux::sensor::SensorFactory::getInstance();

    // [2] Prepare a callback to sensor factory which is called when the initialization of the
    // sensor sub-system is completed
    std::promise<telux::common::ServiceStatus> p;
    auto initCb = [&p](telux::common::ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
    };

    // [3] Get the sensor manager
    std::shared_ptr<telux::sensor::ISensorManager> sensorManager
        = sensorFactory.getSensorManager(initCb);
    if (sensorManager == nullptr) {
        std::cout << "sensor manager is nullptr" << std::endl;
        exit(1);
    }
    std::cout << "obtained sensor manager" << std::endl;

    // [4] Wait until initialization is complete
    p.get_future().get();
    if (sensorManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Sensor service not available" << std::endl;
        exit(1);
    }

    // [5] Get information on available sensors and their characteristics like name, supported
    // sampling rates among other information
    std::cout << "Sensor service is now available" << std::endl;
    std::vector<telux::sensor::SensorInfo> sensorInfo;
    telux::common::Status status = sensorManager->getAvailableSensorInfo(sensorInfo);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get information on available sensors" << static_cast<int>(status)
                  << std::endl;
        exit(1);
    }
    std::cout << "Received sensor information" << std::endl;
    for (auto info : sensorInfo) {
        printSensorInfo(info);
    }

    // [6] Get the desired sensor
    std::string gyroName
        = getSensorName(sensorInfo, telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED);
    std::shared_ptr<telux::sensor::ISensor> gyroScope;
    std::cout << "Getting sensor with name " << gyroName << std::endl;
    status = sensorManager->getSensor(gyroScope, gyroName);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to get gyroscope sensor" << std::endl;
        exit(1);
    }

    // [7] Create a dedicated listener per sensor and register the listener to get notifications
    // about sensor configuration updates, sensor events
    std::shared_ptr<SensorEventListener> sensorEventListener
        = std::make_shared<SensorEventListener>(gyroScope->getSensorInfo());
    gyroScope->registerListener(sensorEventListener);

    // [8] Configure the sensor with the desired configuration, with the required validityMask set
    telux::sensor::SensorConfiguration config;
    config.samplingRate = getMinimumSamplingRate(gyroScope->getSensorInfo());
    config.batchCount = gyroScope->getSensorInfo().maxBatchCountSupported;
    std::cout << "Configuring gyroscope with samplingRate, batchCount [" << config.samplingRate
              << ", " << config.batchCount << "]" << std::endl;
    config.validityMask.set(telux::sensor::SensorConfigParams::SAMPLING_RATE);
    config.validityMask.set(telux::sensor::SensorConfigParams::BATCH_COUNT);
    status = gyroScope->configure(config);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to configure gyroscope" << std::endl;
        exit(1);
    }

    // [10] Activate the sensor
    status = gyroScope->activate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to activate gyroscope" << std::endl;
        exit(1);
    }
    std::cout << "\n\nWait to receive further notifications OR press ENTER to exit \n\n";
    std::cin.ignore();

    // [12] Deactivate the sensor
    status = gyroScope->deactivate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Failed to deactivate gyroscope" << std::endl;
        exit(1);
    }

    // [13] Delete the sensor object
    gyroScope = nullptr;

    // [14] When sensor manager is no longer required, delete the sensor manager object
    sensorManager = nullptr;

    return 0;
}
