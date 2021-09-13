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

/**
 * @file       SensorFeatureManager.hpp
 *
 * @brief      Sensor Feature Manager class provides APIs to interact with the sensor framework to
 *             control the features offered by the framework.
 */

#ifndef TELUX_SENSOR_SENSORFEATUREMANAGER_HPP
#define TELUX_SENSOR_SENSORFEATUREMANAGER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/sensor/SensorDefines.hpp>

namespace telux {
namespace sensor {

/** @addtogroup telematics_sensor_feature_control
 * @{ */

/**
 * @brief ISensorFeatureEventListener interface is used to receive notifications related to
 * sensor feature events
 *
 * The listener method can be invoked from multiple different threads.
 * Client needs to make sure that implementation is thread-safe.
 */
class ISensorFeatureEventListener {
 public:
    /**
     * This function is called to notify about sensor feature events
     *
     * @param [in] id - The sensor feature event that got triggered
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual void onEvent(SensorFeatureEvent event) {
    }

    /**
     * The destructor for the sensor feature event listener
     */
    virtual ~ISensorFeatureEventListener() {
    }
};

/**
 * @brief   Sensor Feature Manager class provides APIs to interact with the sensor framework to
 *          list the available features, enable them or disable them. The availability of sensor
 *          features depends on the capabilities of the underlying hardware.
 */
class ISensorFeatureManager {
 public:
    /**
     * Checks the status of sensor sub-system and returns the result.
     *
     * @returns the status of sensor sub-system status @ref telux::common::ServiceStatus
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Request the sensor framework to provide the available features. The feature could be offered
     * by the sensor framework or the underlying hardware.
     *
     * @param [out] features    List of sensor features the sensor framework offers
     *
     * @returns                 status of the request @ref telux::common::Status
     *
     * @note                    Eval: This is a new API and is being evaluated. It is subject to
     *                          change and could break backwards compatibility.
     */
    virtual telux::common::Status getAvailableFeatures(std::vector<SensorFeature> &features) = 0;

    /**
     * Enable the requested feature
     *
     * @param [in] name         The name of the feature to be enabled. Enabling an already enabled
     *                          feature would result in the API returning
     *                          @ref telux::common::Status::SUCCESS.
     *
     * @returns                 status of the request @ref telux::common::Status
     *
     * @note                    Eval: This is a new API and is being evaluated. It is subject to
     *                          change and could break backwards compatibility.
     */
    virtual telux::common::Status enableFeature(std::string name) = 0;

    /**
     * Disable the requested feature
     *
     * @param [in] name         The name of the feature to be disabled. Disabling an already
     *                          disabled feature would result in the API returning
     *                          @ref telux::common::Status::SUCCESS.
     *
     * @returns                 status of the request @ref telux::common::Status
     *
     * @note                    Eval: This is a new API and is being evaluated. It is subject to
     *                          change and could break backwards compatibility.
     */
    virtual telux::common::Status disableFeature(std::string name) = 0;

    /**
     * Register a listener for sensor feature related events
     *
     * @returns status of registration request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<ISensorFeatureEventListener> listener) = 0;

    /**
     * Deregister a sensor feature event listener
     *
     * @returns status of deregistration request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<ISensorFeatureEventListener> listener) = 0;

    /**
     * Destructor for ISensorFeatureManager
     */
    virtual ~ISensorFeatureManager(){};
};

/** @} */ /* end_addtogroup telematics_sensor_feature_control */
}  // namespace sensor
}  // namespace telux

#endif  // TELUX_SENSOR_SENSORFEATUREMANAGER_HPP
