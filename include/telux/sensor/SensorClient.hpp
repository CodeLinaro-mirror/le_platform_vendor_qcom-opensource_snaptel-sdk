/*
 *  Copyright (c) 2022-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       SensorClient.hpp
 *
 * @brief      Sensor client class provides the APIs to interact with the sensors available in the
 *             system.
 */

#ifndef TELUX_SENSOR_SENSORCLIENT_HPP
#define TELUX_SENSOR_SENSORCLIENT_HPP

#include <vector>
#include <memory>

#include <telux/common/SDKListener.hpp>
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
 * @ref telux::sensor::ISensorClient::selfTest is available.
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
class ISensorEventListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called to notify about available sensor events. Note the following
     * constraints on this listener API
     * It shall not perform time consuming (compute or I/O intensive) operations on this thread
     * It shall not inovke an sensor APIs on this thread due to the underlying concurrency model
     *
     * On platforms with Access control enabled, the client needs to have TELUX_SENSOR_DATA_READ
     * permission for this listener API to be invoked.
     *
     * @param [in] events - List of sensor events
     *
     */
    virtual void onEvent(std::shared_ptr<std::vector<SensorEvent>> events) {
    }

    /**
     * This function is called to notify any change in the configuration of the
     * @ref ISensorClient object this listener is associated with.
     *
     * On platforms with Access control enabled, the client needs to have TELUX_SENSOR_DATA_READ
     * permission for this listener API to be invoked.
     *
     * @param [in] configuration -  The new configuration of the sensor client.
     *                              @ref telux::sensor::SensorConfiguration. Fields that have
     *                              changed can be identified using the @ref
     *                              telux::sensor::SensorConfiguration::updateMask and fields that
     *                              are valid can be identified using @ref
     *                              telux::sensor::SensorConfiguration::validityMask
     *
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
 * @brief ISensorClient interface is used to access the different services provided by the sensor
 * framework to configure, activate and acquire sensor data.
 *
 * Each instance of this class is a unique sensor client to the underlying sensor framework and any
 * number of such clients can exist in a given process. Each of these clients can acquire data from
 * the underlying sensor framework with different configurations.
 */
class ISensorClient {
 public:
    /**
     * Get the information related to sensor
     *
     * @returns information retated to sensor - @ref telux::sensor::SensorInfo
     *
     */
    virtual SensorInfo getSensorInfo() = 0;

    /**
     * Configure the sensor client with desired sampling rate and batch count. Any change in
     * sampling rate or batch count of the sensor will be notified via
     * @ref telux::sensor::ISensorEventListener::onConfigurationUpdate.
     *
     * In case a sensor client needs to be reconfigured after having been activated, the client
     * should be deactivated, configured and activated again as a part of the reconfiguration
     * process.
     *
     * It is always recommended that configuration of a client is done before activating it. If a
     * client is activated without configuration, the client is configured with a default
     * configuration and activated. The default configuration would have the sampling rate set to
     * minimum sampling rate supported @ref telux::sensor::SensorInfo::samplingRates and the batch
     * count set to maximum batch count supported @ref
     * telux::sensor::SensorInfo::maxBatchCountSupported
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_SENSOR_DATA_READ
     * permission to invoke this API successfully.
     *
     * @param[in]   configuration - The desired configuration for the client
     *                              @ref telux::sensor::SensorConfiguration. Ensure the required
     *                              validity mask @ref
     *                              telux::sensor::SensorConfiguration::validityMask is set for the
     *                              configuration.
     *
     * @returns status of configuration request - @ref telux::common::Status
     *
     */
    virtual telux::common::Status configure(SensorConfiguration configuration) = 0;

    /**
     * Get the current configuration of this sensor client
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_SENSOR_DATA_READ
     * permission to invoke this API successfully.
     *
     * @returns the current configuration of the client. @ref
     * telux::sensor::SensorConfiguration::validityMask should be checked to know which of the
     * fields in the returned configuration is valid.
     *
     */
    virtual SensorConfiguration getConfiguration() = 0;

    /**
     * Activate the sensor client. Once activated, any available sensor event will be notified via
     * @ref telux::sensor::ISensorEventListener::onEvent
     *
     * It is always recommended that configuration of a client is done before activating it. If a
     * client is activated without configuration, the client is configured with the default
     * configuration and activated. The default configuration would have the sampling rate set to
     * minimum sampling rate supported @ref telux::sensor::SensorInfo::samplingRates and the batch
     * count set to maximum batch count supported @ref
     * telux::sensor::SensorInfo::maxBatchCountSupported. Activating an already activated sensor
     * would result in the API returning @ref telux::common::Status::SUCCESS.
     *
     * Activating this sensor client would not impact other inactive sensor clients.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_SENSOR_DATA_READ
     * permission to invoke this API successfully.
     *
     * @returns status of activation request - @ref telux::common::Status
     *
     */
    virtual telux::common::Status activate() = 0;

    /**
     * Deactivate the sensor client. Once deactivated, no further sensor events will be notified via
     * @ref telux::sensor::ISensorEventListener::onEvent. Deactivating an already inactive sensor
     * would result in the API returning @ref telux::common::Status::SUCCESS.
     *
     * Deactivating this sensor client would not impact other active sensor clients.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_SENSOR_DATA_READ
     * permission to invoke this API successfully.
     *
     * @returns status of deactivation request - @ref telux::common::Status
     *
     */
    virtual telux::common::Status deactivate() = 0;

    /**
     * Initiate self test on this sensor
     *
     * If there are active data acquisition sessions corresponding to this sensor, these
     * will be paused and the self test is initiated. Once the self test is complete the sensor data
     * sessions will be restored.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_SENSOR_PRIVILEGED_OPS
     * permission to invoke this API successfully.
     *
     * @param[in]   selfTestType - The type of self test to be performed - @ref
     *                             telux::sensor::SelfTestType
     * @param[in]   cb - Callback to get the result of the self test initiated
     *
     * @returns status of the request - @ref telux::common::Status. Note that the result of the self
     *          test done by the sensor is provided via the callback - @ref
     *          telux::sensor::SelfTestResultCallback
     *
     */
    virtual telux::common::Status selfTest(
        SelfTestType selfTestType, SelfTestResultCallback cb) = 0;

    /**
     * Register a listener for sensor related events
     *
     * @returns status of registration request - @ref telux::common::Status
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<ISensorEventListener> listener) = 0;

    /**
     * Deregister a sensor event listener
     *
     * @returns status of deregistration request - @ref telux::common::Status
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<ISensorEventListener> listener) = 0;

    /**
     * Destructor for ISensorClient.
     *
     * Internally, the sensor client object is first deactivated and then destroyed.
     *
     */
    virtual ~ISensorClient(){};

    /**
     * Deprecated APIs
     *
     */

    /**
     * Request the sensor to operate in low power mode. The sensor should be in deactivated state to
     * exercise this API. The success of this request depends on the capabilities of the
     * underlying hardware.
     *
     * @returns status of request - @ref telux::common::Status
     *
     * @deprecated This API is no longer supported.
     *
     */
    virtual telux::common::Status enableLowPowerMode() = 0;

    /**
     * Request the sensor to exit low power mode. The sensor should be in deactivated state to
     * exercise this API. The success of this request depends on the capabilities of the
     * underlying hardware.
     *
     * @returns status of request - @ref telux::common::Status
     *
     * @deprecated This API is no longer supported.
     *
     */
    virtual telux::common::Status disableLowPowerMode() = 0;
};

// Note that the class ISensor is an alias for ISensorClient and ISensor would deprecated and
// eventually removed. As of now, it is retained for backward compatibility.
using ISensor = ISensorClient;

/** @} */ /* end_addtogroup telematics_sensor_control */
}  // namespace sensor
}  // namespace telux

#endif // TELUX_SENSOR_SENSORCLIENT_HPP
