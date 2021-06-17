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
 * @file        SensorClient.cpp
 *
 * @brief       This file hosts the implementation for the sensor client to configure and acquire
 *              data from the sensor framework
 */

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <future>

#include "SensorClient.hpp"

#include "SensorUtils.hpp"
#include "../../common/utils/Utils.hpp"
#include <telux/common/Version.hpp>

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"

SensorClient::SensorClient(int id, std::shared_ptr<ISensor> sensor, bool verboseNotification)
   : id_(id)
   , sensor_(sensor)
   , verboseNotification_(verboseNotification)
   , lastBatchReceivedAt_(0) {
    tag_ = std::string("[")
               .append(SensorUtils::getSensorType(sensor_->getSensorInfo().type))
               .append(", Sensor ID: ")
               .append(std::to_string(sensor_->getSensorInfo().id))
               .append(", Client ID: ")
               .append(std::to_string(id_))
               .append("] ");
}

void SensorClient::init() {
    sensor_->registerListener(shared_from_this());
}

void SensorClient::cleanup() {
    sensor_->deregisterListener(shared_from_this());
}

SensorClient::~SensorClient() {
    sensor_->deactivate();
    sensor_ = nullptr;
}

void SensorClient::printInfo() {
    std::cout << "\tClient ID: " << id_ << std::endl;
    SensorUtils::printSensorInfo(sensor_->getSensorInfo(), true);
    SensorConfiguration configuration = sensor_->getConfiguration();
    std::cout << "\n\tConfiguration: [";
    if (configuration.validityMask.test(SensorConfigParams::SAMPLING_RATE)) {
        std::cout << std::fixed << std::setprecision(2) << configuration.samplingRate;
    } else {
        std::cout << "NA";
    }
    std::cout << ", "
              << (configuration.validityMask.test(SensorConfigParams::BATCH_COUNT)
                         ? std::to_string(configuration.batchCount)
                         : "NA")
              << "]" << std::endl
              << std::endl;
}

void SensorClient::onEvent(std::shared_ptr<std::vector<SensorEvent>> events) {
    uint64_t receivedTimeStamp = Utils::getNanosecondsSinceBoot();
    float jitter = 0;

    // Calculate jitter in milliseconds
    if (lastBatchReceivedAt_ > 0) {
        jitter = 1.0 * (receivedTimeStamp - lastBatchReceivedAt_) / 1000000;
    }
    uint64_t eventTimeStamp = 0;
    uint32_t count = 0;
    float samplingRateAggregate = 0.0;
    for (SensorEvent s : *(events.get())) {
        float samplingRate = 0.0;
        if (eventTimeStamp > 0) {
            ++count;
            // Instantaneous sampling rate, calculated between consecutive samples
            samplingRate = 1.0 / (s.timestamp - eventTimeStamp) * 1000000000;
        }
        if (verboseNotification_) {
            SensorUtils::printSensorEvent(sensor_->getSensorInfo().type, s, samplingRate, tag_);
        }
        samplingRateAggregate += samplingRate;
        eventTimeStamp = s.timestamp;
    }

    print_notification << tag_ << receivedTimeStamp << ": Received " << events->size()
                       << " events, time since previous batch: " << std::fixed << jitter
                       << "ms, average calculated sampling rate: " << samplingRateAggregate / count
                       << " Hz" << std::endl;
    lastBatchReceivedAt_ = receivedTimeStamp;
}
void SensorClient::onConfigurationUpdate(SensorConfiguration configuration) {
    print_notification << tag_ << "Received configuration update: [" << configuration.samplingRate
                       << ", " << configuration.batchCount << "]" << std::endl;
}

void SensorClient::configure(SensorConfiguration config) {
    telux::common::Status status = sensor_->configure(config);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "sensor configuration failed: ";
        Utils::printStatus(status);
        return;
    }
    std::cout << tag_ << "Sensor configuration successful" << std::endl;
}

void SensorClient::activate() {
    telux::common::Status status = sensor_->activate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "sensor activation failed: ";
        Utils::printStatus(status);
        return;
    }
    std::cout << tag_ << "Sensor activation successful" << std::endl;
}

void SensorClient::deactivate() {
    telux::common::Status status = sensor_->deactivate();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "sensor deactivation failed: ";
        Utils::printStatus(status);
        return;
    }
    std::cout << tag_ << "Sensor deactivation successful" << std::endl;
}

void SensorClient::enableLowPowerMode() {
    telux::common::Status status = sensor_->enableLowPowerMode();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "low power mode enable request failed: ";
        Utils::printStatus(status);
        return;
    }
    std::cout << tag_ << "Low power mode enable request successful" << std::endl;
}

void SensorClient::disableLowPowerMode() {
    telux::common::Status status = sensor_->disableLowPowerMode();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << tag_ << "low power mode disable request failed: ";
        Utils::printStatus(status);
        return;
    }
    std::cout << tag_ << "Low power mode disable request successful" << std::endl;
}
