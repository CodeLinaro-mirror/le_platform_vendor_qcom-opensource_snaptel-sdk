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
 * @file       Sensor.hpp
 *
 * @brief      Sensor class provides the APIs to interact with the sensors available in the system.
 */

#ifndef TELUX_SENSOR_SENSOR_HPP
#define TELUX_SENSOR_SENSOR_HPP

#include <vector>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/sensor/SensorDefines.hpp>

namespace telux {
namespace sensor {

/** @addtogroup telematics_sensor_control
 * @{ */

/**
 * @brief   This file hosts the sensor interfaces to configure, activate or get data from the
 *          individual sensors available - Gyroscope, Accelerometer, etc.
 */

/**
 * This function is invoked when a result for a self-test initiated using
 * @ref telux::sensor::ISensor::selfTest is available .
 *
 * @param [in] result The result of the self test - @ref telux::common::ErrorCode
 *
 */
using SelfTestResultCallback = std::function<void(telux::common::ErrorCode result)>;

/**
 * @brief ISensorEventListener interface is used to receive notifications related to
 * sensor events and configuration updates
 *
 * The listener method can be invoked from multiple different threads.
 * Client needs to make sure that implementation is thread-safe.
 */
class ISensorEventListener {
 public:
    /**
     * This function is called to notify about available sensor events. Note the following
     * constraints on this listener API
     * It shall not perform time consuming (compute or I/O intensive) operations on this thread
     * It shall not inovke an sensor APIs on this thread due to the underlying concurrency model
     *
     * @param [in] events - List of sensor events
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onEvent(std::shared_ptr<std::vector<SensorEvent>> events) {
    }

    /**
     * This function is called to notify any change in sensor configuration.
     *
     * @param [in] configuration -  The new configuration for the sensor
     *                              @ref telux::sensor::SensorConfiguration. Fields that have
     *                              changed can be identified using the @ref
     *                              telux::sensor::SensorConfiguration::updateMask and fields that
     *                              are valid can be identified using @ref
     *                              telux::sensor::SensorConfiguration::validityMask
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     *
     */
    virtual void onConfigurationUpdate(SensorConfiguration configuration) {
    }

    /**
     * The destructor for the sensor event listener
     */
    virtual ~ISensorEventListener() {
    }
};
/**
 * @brief ISensor interface is used to access the different services provided by the sensor to
 * configure, activate and get sensor data.
 */
class ISensor {
 public:
    /**
     * Get the information related to sensor
     *
     * @returns information retated to sensor - @ref telux::sensor::SensorInfo
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual SensorInfo getSensorInfo() = 0;

    /**
     * Configure sensor with desired sampling rate and batch count. Any change in
     * sampling rate or batch count of the sensor will be notified via
     * @ref telux::sensor::ISensorEventListener::onConfigurationUpdate.
     *
     * In case a sensor needs to be reconfigured after having been activated, the sensor
     * should be deactivated, configured and activated again as a part of the reconfiguration
     * process.
     *
     * It is always recommended that configuration of a sensor is done before activating it. If a
     * sensor is activated without configuration, the sensor is configured with a default
     * configuration and activated. The default configuration would have the sampling rate set to
     * minimum sampling rate supported @ref telux::sensor::SensorInfo::samplingRates and the batch
     * count set to maximum batch count supported @ref
     * telux::sensor::SensorInfo::maxBatchCountSupported
     *
     * @param[in]   configuration - The desired configuration for the sensor
     *                              @ref telux::sensor::SensorConfiguration. Ensure the required
     *                              validity mask @ref
     *                              telux::sensor::SensorConfiguration::validityMask is set for the
     *                              configuration.
     *
     * @returns status of configuration request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status configure(SensorConfiguration configuration) = 0;

    /**
     * Get the current configuration of this sensor
     *
     * @returns the current configuration of the sensor. @ref
     * telux::sensor::SensorConfiguration::validityMask should be checked to know which of the
     * fields in the returned configuration is valid.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual SensorConfiguration getConfiguration() = 0;

    /**
     * Activate the sensor. Once activated, any available sensor event will be notified via
     * @ref telux::sensor::ISensorEventListener::onEvent
     *
     * It is always recommended that configuration of a sensor is done before activating it. If a
     * sensor is activated without configuration, the sensor is configured with a default
     * configuration and activated. The default configuration would have the sampling rate set to
     * minimum sampling rate supported @ref telux::sensor::SensorInfo::samplingRates and the batch
     * count set to maximum batch count supported @ref
     * telux::sensor::SensorInfo::maxBatchCountSupported. Activating an already activated sensor
     * would result in the API returning @ref telux::common::Status::SUCCESS.
     *
     * @returns status of activation request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status activate() = 0;

    /**
     * Deactivate the sensor. Once deactivated, no further sensor events will be notified via
     * @ref telux::sensor::ISensorEventListener::onEvent. Deactivating an already inactive sensor
     * would result in the API returning @ref telux::common::Status::SUCCESS.
     *
     * @returns status of deactivation request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status deactivate() = 0;

    /**
     * Request the sensor to operate in low power mode. The sensor should be in deactivated state to
     * exercise this API. The success of this request depends on the capabilities of the
     * underlying hardware.
     *
     * @returns status of request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status enableLowPowerMode() = 0;

    /**
     * Request the sensor to exit low power mode. The sensor should be in deactivated state to
     * exercise this API. The success of this request depends on the capabilities of the
     * underlying hardware.
     *
     * @returns status of request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status disableLowPowerMode() = 0;

    /**
     * Initiate self test on this sensor
     *
     * If there are active data acquisition sessions corresponding to this sensor, these
     * will be paused and the self test is initiated. Once the self test is complete the sensor data
     * sessions will be restored.
     *
     * @param[in]   selfTestType - The type of self test to be performed - @ref
     *                             telux::sensor::SelfTestType
     * @param[in]   SelfTestResultCallback - Callback to get the result of the self test initiated
     *
     * @returns status of the request - @ref telux::common::Status. Note that the result of the self
     *          test done by the sensor is provided via the callback - @ref
     *          telux::sensor::SelfTestResultCallback
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status selfTest(
        SelfTestType selfTestType, SelfTestResultCallback cb) = 0;

    /**
     * Register a listener for sensor related events
     *
     * @returns status of registration request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<ISensorEventListener> listener) = 0;

    /**
     * Deregister a sensor event listener
     *
     * @returns status of deregistration request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<ISensorEventListener> listener) = 0;

    /**
     * Destructor for ISensor
     */
    virtual ~ISensor(){};
};

/** @} */ /* end_addtogroup telematics_sensor_control */
}  // namespace sensor
}  // namespace telux

#endif  // TELUX_SENSOR_SENSOR_HPP
