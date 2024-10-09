/*
 *  Copyright (c) 2017-2020, The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2021-2022, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       LocationListener.hpp
 *
 * @brief      Interface for location service listener object. Client needs to
 * implement this
 *             interface to get location service notifications like location
 * update,
 *             satellite vehicle info etc.
 *             The methods in listener can be invoked from multiple threads.
 *             Client needs to make sure that implementation is thread-safe.
 *
 */

#ifndef LOCATIONLISTENER_HPP
#define LOCATIONLISTENER_HPP

#include "telux/loc/LocationDefines.hpp"
#include <memory>

namespace telux {

namespace loc {
// forward declarations
class ILocationInfoBase;
class ILocationInfoEx;
class IGnssSVInfo;

/** @addtogroup telematics_location
* @{ */

/**
 * @brief Listener class for getting location updates and satellite vehicle
 * information.
 *
 * The methods in listener can be invoked from multiple different
 * threads. Client needs to make sure that implementation is thread-safe.
 *
 */
class ILocationListener {
public:

/**
 * This function is called when device receives location update. When there are
 * multiple engines running on the system, the received location information is
 * fused report from all engines.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] locationInfo - Location information  like latitude, longitude,
 * timeInfo other information such as heading, altitude and velocity etc.
 *
 */
  virtual void onBasicLocationUpdate(
      const std::shared_ptr<ILocationInfoBase> &locationInfo) {}

/**
 * This function is called when device receives Gnss location update. When there
 * are multiple engines running on the system, the received location information
 * is fused report from all engines.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] locationInfo - Contains richer set of location information
 * like latitude, longitude, timeInfo, heading, altitude, velocity and other
 * information such as deviations, elliptical accuracies etc.
 *
 */
  virtual void onDetailedLocationUpdate(
      const std::shared_ptr<ILocationInfoEx> &locationInfo) {}

/**
 * This function is called when device receives multiple Gnss location update from
 * the different engine types requested, which are SPE/PPE/FUSED.
 * This API will be called ONLY if we use startDetailedEngineReports.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] locationEngineInfo - Contains a list of location infos. Each element in
 * the list corresponds to one of SPE/PPE/FUSED.
 *
 */
  virtual void onDetailedEngineLocationUpdate(
      const std::vector<std::shared_ptr<ILocationInfoEx> > &locationEngineInfo) {}

/**
 * This function is called when device receives GNSS satellite information.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] gnssSVInfo - GNSS satellite information
 *
 */
  virtual void onGnssSVInfo(const std::shared_ptr<IGnssSVInfo> &gnssSVInfo) {}

/**
 * This function is called when device receives GNSS data information
 * like jammer metrics and automatic gain control for satellite signal type.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] info - GNSS signal info
 *
 */
  virtual void onGnssSignalInfo(const std::shared_ptr<IGnssSignalInfo> &info) {}

/**
 * This function is called when device receives GNSS NMEA sentences.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] timestamp - Timestamp
 * @param [in] nmea - Nmea sentence
 */
  virtual void onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) {}

/**
 * This function is called when device receives signal measurement information
 * such as satellite vehicle pseudo range, satellite vehicle clock time, carrier phase
 * measurement etc.
 * The frequency at which this API is called is determined by what was requested
 * @ref telux::loc::GnssReportType::MEASUREMENT or @ref telux::loc::GnssReportType::HIGH_RATE_MEASUREMENT in
 * @ref ILocationManager::startDetailedReports and
 * @ref ILocationManager::startDetailedEngineReports.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] measurementInfo - GNSS measurement information
 *
 */
  virtual void onGnssMeasurementsInfo(const telux::loc::GnssMeasurements &measurementInfo) {}

/**
 * This function is called during a disaster/crisis to update the
 * disaster/crisis reports.
 *
 * @param [in] dcReportInfo - GNSS disaster/crisis report information.
 *             This includes the report type and data payload received from the GNSS engine.
 */
  virtual void onGnssDisasterCrisisInfo(const telux::loc::GnssDisasterCrisisReport &dcReportInfo){}

/**
 * This function is called when the capabilities of the location stack gets updated.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] capabilityInfo - @ref telux::loc::LocCapability, capability information
 *
 */
  virtual void onCapabilitiesInfo(const telux::loc::LocCapability capabilityInfo) {}


/**
 * Destructor of ILocationListener
 */
  virtual ~ILocationListener() {}
};

class ILocationSystemInfoListener {
public:
/**
 * This function is called when device receives location related system information
 * such as leap second change.
 *
 * On platforms with Access control enabled, the client needs to have TELUX_LOC_DATA permission
 * for this listener API to be invoked.
 *
 * @param [in] locationSystemInfo - contains location system information such as
 *                                  current leap seconds change
 */
  virtual void onLocationSystemInfo(const LocationSystemInfo &locationSystemInfo) {}

/**
 * Destructor of ILocationSystemInfoListener
 */
  virtual ~ILocationSystemInfoListener() {}

};

/**
 * @brief ILocationConfigListener interface is used to receive notifications related to
 * configuration events.
 *
 * The listener method can be invoked from multiple different threads.
 * Client needs to make sure that implementation is thread-safe.
 */
class ILocationConfigListener {
  public:
    /**
     * The API is invoked when there is any update in the Xtra assistance data.
     *
     * Clients need to register for this indication
     * via @ref LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS.
     *
     * @param [in] xtraStatus - Xtra assistant data's current status, validity
     *                          and whether it is enabled.
     */
    virtual void onXtraStatusUpdate(const XtraStatus xtraStatus) {}

    /**
     * The API is invoked when the Location service is ready to start receiving the location
     * reports using the @ref ILocationConfigurator::injectLocationData API.
     *
     * Clients need to register for this indication
     * via @ref LocConfigIndicationsType::LOC_CONF_IND_LOC_INJ_REQ.
     *
     * @param [in] timeInMilliSeconds - contains the time in milliseconds which represents
     *                                  the Max rate at which the
     *                                  @ref LocationConfigurator::injectLocationData API can
     *                                  be called.
     */
    virtual void onStartInjection(const uint32_t timeInMilliSeconds) {}

    /**
     * The API is invoked to notify the Location service's intent to stop injecting
     * the location reports using the @ref ILocationConfigurator::injectLocationData API.
     *
     * Clients need to register for this indication
     * via @ref LocConfigIndicationsType::LOC_CONF_IND_LOC_INJ_REQ.
     *
     */
    virtual void onStopInjection() {}

    /**
     * Destructor of ILocationConfigListener
     */
    virtual ~ILocationConfigListener() {}
};

/**
 * @deprecated - Use @ref ILocationConfigListener class.
 *
 */
class ILocationInjectionListener {
  public:
    /**
     * This function is called when the Location service is ready to start receiving the location
     * reports using the @ref LocationConfigurator::injectLocationData API.
     *
     * @deprecated - Use @ref ILocationConfigListener::onStartInjection API.
     *
     * @param [in] timeInMillis - contains the time in milliseconds which represents the Max rate of
     *                            at which the @ref LocationConfigurator::injectLocationData API can
     *                            be called.
     */
    virtual void onStartInjection(const uint32_t timeInMilliSeconds) {}

    /**
     * This function notifies the Location service's intent to stop injecting the location reports
     * using the @ref LocationConfigurator::injectLocationData API.
     *
     * @deprecated - Use @ref ILocationConfigListener::onStopInjection API.
     *
     */
    virtual void onStopInjection() {}

    /**
     * Destructor of ILocationInjectionListener
     */
    virtual ~ILocationInjectionListener() {}

};

/** @} */ /* end_addtogroup telematics_location */
}         // end of namespace loc

} // end of namespace telux

#endif // LOCATIONLISTENER_HPP
