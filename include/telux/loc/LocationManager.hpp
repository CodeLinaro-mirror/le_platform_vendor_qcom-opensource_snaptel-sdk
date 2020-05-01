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

/**
 * @file       LocationManager.hpp
 * @brief      Location manager provides APIs to get position reports
 *             and satellite vehicle information updates. The reports
 *             specific to particular location engine can also be obtained
 *             by choosing the required engine report.
 *
 */

#ifndef LOCATION_MANAGER_HPP
#define LOCATION_MANAGER_HPP

#include <future>
#include <memory>

#include "telux/common/CommonDefines.hpp"
#include "telux/loc/LocationDefines.hpp"
#include "telux/loc/LocationListener.hpp"

namespace telux {

namespace loc {

/** @addtogroup telematics_location
* @{ */

/**
 * @brief ILocationManager provides interface to register and remove listeners.
 * It also allows to set and get configuration/ criteria for position reports.
 * The new APIs(registerListenerEx, deRegisterListenerEx, startDetailedReports,
 * startBasicReports) and old/deprecated APIs(registerListener, removeListener,
 * setPositionReportTimeout, setHorizontalAccuracyLevel, setMinIntervalForReports)
 * should not be used interchangebly, either the new APIs should be used or the
 * old APIs should be used.
 *
 */
class ILocationManager {
public:

/**
 * This function is called with the response to getEnergyConsumedInfoUpdate API.
 *
 * @param[in] energyConsumed - Information regarding energy consumed by Gnss engine.
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibilty.
 *
 */
 using GetEnergyConsumedCallback = std::function<void(telux::loc::GnssEnergyConsumedInfo
     energyConsumed, telux::common::ErrorCode error)>;

/**
 * Checks the status of location subsystems and returns the result.
 *
 * @returns True if location subsystem is ready for service otherwise false.
 *
 */
  virtual bool isSubsystemReady() = 0;

/**
 * Wait for location subsystem to be ready.
 *
 * @returns  A future that caller can wait on to be notified when location
 *           subsystem is ready.
 *
 */
  virtual std::future<bool> onSubsystemReady() = 0;

/**
 * Register a listener for specific updates from location manager like
 * location, jamming info and satellite vehicle info. If enhanced position,
 * using Dead Reckoning etc., is enabled, enhanced fixes will be provided.
 * Otherwise raw GNSS fixes will be provided.
 * The position reports will start only when startDetailedReports or
 * startBasicReports is invoked.
 *
 * @param [in] listener - Pointer of ILocationListener object that processes
 *                        the notification.
 *
 * @returns Status of registerListener i.e success or suitable status code.
 *
 *
 */
  virtual telux::common::Status
      registerListenerEx(std::weak_ptr<ILocationListener> listener) = 0;

/**
 * Remove a previously registered listener.
 *
 * @param [in] listener - Previously registered ILocationListener that needs
 *                        to be removed.
 *
 * @returns Status of removeListener success or suitable status code
 *
 */
  virtual telux::common::Status
      deRegisterListenerEx(std::weak_ptr<ILocationListener> listener) = 0;

/**
 * Starts the richer location reports by configuring the time between them as
 * the interval. Any of the 3 APIs that is startDetailedReports or startDetailedEngineReports
 * or startBasicReports can be called one after the other irrespective of order, without
 * calling stopReports in between any of them and the API which is called last will be honored
 * for providing the callbacks. If multiple clients invoke this API with different interval,
 * then all the clients will be benefited with interval which is smallest among all the intervals.
 *
 * This Api enables the onDetailedLocationUpdate, onGnssSVInfo,
 * onGnssSignalInfo, onGnssNmeaInfo and onGnssMeasurementsInfo Apis on the listener.
 *
 * @param [in] interval - Minimum time interval between two consecutive
 * reports in milliseconds.
 *
 * E.g. If minInterval is 1000 milliseconds, reports will be provided with a
 * periodicity of 1 second or more depending on the number of applications
 * listening to location updates.
 *
 * @param [in] callback - Optional callback to get the response of set
 *             minimum interval for reports.
 *
 * @returns Status of startDetailedReports i.e. success or suitable status
 * code.
 *
 */
  virtual telux::common::Status
      startDetailedReports(uint32_t interval,
                           telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * Starts a session which may provide richer default combined position reports
 * and position reports from other engines. The fused position report type will
 * always be supported if at least one engine in the system is producing valid report.
 * Any of the 3 APIs that is startDetailedReports or startDetailedEngineReports
 * or startBasicReports can be called one after the other irrespective of order, without
 * calling stopReports in between any of them and the API which is called last will be
 * honored for providing the callbacks. If multiple clients invoke this API with different
 * interval, then all the clients will be benefited with interval which is smallest among
 * all the intervals.
 * This Api enables the onDetailedLocationUpdate, onGnssSVInfo,
 * onGnssSignalInfo, onGnssNmeaInfo and onGnssMeasurementsInfo Apis on the listener.
 *
 * @param [in] interval - Minimum time interval between two consecutive
 * reports in milliseconds.
 *
 * E.g. If minInterval is 1000 milliseconds, reports will be provided with a
 * periodicity of 1 second or more depending on the number of applications
 * listening to location updates.
 *
 * @param [in] engineType - The type of engine requested for fixes such as
 * SPE or PPE or FUSED. The FUSED includes all the engines that are running to
 * generate the fixes such as reports from SPE, PPE and DRE.
 *
 * @param [in] callback - Optional callback to get the response of set
 *             minimum interval for reports.
 *
 * @returns Status of startDetailedEngineReports i.e. success or suitable status
 * code.
 *
 */
  virtual telux::common::Status
      startDetailedEngineReports(uint32_t interval, LocReqEngine engineType,
                           telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * Starts the Location report by configuring the time and distance between
 * the consecutive reports. Any of the 3 APIs that is startDetailedReports or
 * startDetailedEngineReports or startBasicReports can be called one after the other
 * irrespective of order, without calling stopReports in between any of them and the
 * API which is called last will be honored for providing the callbacks. If multiple
 * clients invoke this API with different interval, then all the clients will be benefited
 * with interval which is smallest among all the intervals.
 *
 * This Api enables the onBasicLocationUpdate Api on the listener.
 *
 * @param [in] distanceInMeters - distanceInMeters between two
 * consecutive reports in meters.
 * intervalInMs - Minimum time interval between two consecutive
 * reports in milliseconds.
 *
 * E.g. If intervalInMs is 1000 milliseconds and distanceInMeters is 100m,
 * reports will be provided according to the condition that happens first. So we need to
 * provide both the parameters for evaluating the report.
 *
 * The underlying system may have a minimum distance threshold(e.g. 1 meter).
 * Effective distance will not be smaller than this lower bound.
 *
 * The effective distance may have a granularity level higher than 1 m, e.g.
 * 5 m. So distanceInMeters being 59 may be honored at 60 m, depending on the system.
 *
 * Where there is another application in the system having a session with
 * shorter distance, this client may benefit and receive reports at that distance.
 *
 * @param [in] callback - Optional callback to get the response of set
 *                        minimum distance for reports.
 *
 * @returns Status of startBasicReports i.e. success or suitable status code.
 *
 */
  virtual telux::common::Status
      startBasicReports(uint32_t distanceInMeters, uint32_t intervalInMs,
                        telux::common::ResponseCallback callback = nullptr) = 0;
/**
 * This API registers a ILocationSystemInfoListener listener and will receive information related
 * to location system that are not tied with location fix session, e.g.: next leap second event.
 * The startBasicReports, startDetailedReports, startDetailedEngineReports does not need to be
 * called before calling this API, in order to receive updates.
 *
 * @param [in] listener - Pointer of ILocationSystemInfoListener object.
 *
 * @param [in] callback - Optional callback to get the response of location
 *                        system info.
 *
 * @returns Status of getLocationSystemInfo i.e success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 *
 */
  virtual telux::common::Status
      registerForSystemInfoUpdates(std::weak_ptr<ILocationSystemInfoListener> listener,
          telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API removes a previously registered listener and will also stop receiving informations
 * related to location system for that particular listener.
 *
 * @param [in] listener - Previously registered ILocationSystemInfoListener that needs to be
 *                        removed.
 *
 * @param [in] callback - Optional callback to get the response of location
 *                        system info.
 *
 * @returns Status of deRegisterForSystemInfoUpdates success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 *
 */
  virtual telux::common::Status
      deRegisterForSystemInfoUpdates(std::weak_ptr<ILocationSystemInfoListener> listener,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API receives information on energy consumed by modem GNSS engine. If this API
 * is called on this object while this is already a pending request, then it will overwrite
 * the callback to be invoked and the callback from the previous invocation will not be
 * called.
 *
 * @param [in] cb - callback to get the information of Gnss energy consumed.
 *
 * @returns Status of requestEnergyConsumedInfo i.e success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *       break backwards compatibility.
 */
  virtual telux::common::Status requestEnergyConsumedInfo(GetEnergyConsumedCallback cb) = 0;

/**
 * This API will stop reports started using startDetailedReports or startBasicReports
 * or registerListener or setMinIntervalForReports.
 *
 * @param [in] callback - Optional callback to get the response of stop reports.
 *
 * @returns Status of stopReports i.e. success or suitable status code.
 *
 */
  virtual telux::common::Status
      stopReports(telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * Register a listener for specific updates from location manager like location and satellite
 * vehicle info.
 * As soon as the first listener is registered, position fixes will start being reported.
 *
 * @param [in] listener - Pointer of ILocationListener object that processes the notification.
 *
 * @returns Status of registerListener i.e success or suitable status code.
 *
 * @deprecated API is not going to be supported in future releases. Clients should stop using
 * this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
 */
  virtual telux::common::Status
      registerListener(std::weak_ptr<ILocationListener> listener) = 0;

/**
 * Remove a previously registered listener.
 *
 * @param [in] listener - Previously registered ILocationListener that needs to be removed
 *
 * @returns Status of removeListener success or suitable status code
 *
 * @deprecated API is not going to be supported in future releases. Clients should stop using
 * this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
 */
  virtual telux::common::Status
      removeListener(std::weak_ptr<ILocationListener> listener) = 0;

/**
 * Configures position report timeout.
 * LocationManager tries to determine the position until the position report timeout has
 * elapsed. If the final position cannot be determined before the timeout period, it returns
 * a position report with session status marked as SessionStatus::TIMEOUT instead of
 * SessionStatus::SUCCESS. Position report timeout is an app spectific setting.
 * E.g. If this API is called with timeout parameter value 5000, the LocationManager tries to
 * determine the position before 5 seconds. If position report is determined, a position
 * report will be returned with sessionStatus=SessionStatus::SUCCESS. Otherwise, a
 * position report will be sent with sessionStatus=SessionStatus::TIMEOUT after 5 seconds.
 *
 * @param [in] timeout  - Maximum time to get a position report in milliseconds.
 * @param [in] callback - Optional callback to get the response of set
 *                        position report time out
 *
 * @returns Status as SUCCESS for setPositionReportTimeout.
 *
 * @deprecated API is not going to be supported in future releases. Clients should stop using
 * this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
 */
  virtual telux::common::Status setPositionReportTimeout(
      uint32_t timeout, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) = 0;

/**
 * Configuring horizontal accuracy level.
 * If the final position cannot be determined with the required horizontal accuracy level
 * within the timeout period specified in the setPositionReportTimeout API, a timeout fix
 * will be provided. Refer to setPositionReportTimeout API for more details.
 *
 * @param [in] accuracy - @ref HorizontalAccuracyLevel
 * @param [in] callback - Optional callback to get the response of set
 *                        horizontal accuracy level
 *
 * @returns Status as SUCCESS of setHorizontalAccuracyLevel.
 *
 * @deprecated API is not going to be supported in future releases. Clients should stop using
 * this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
 */
  virtual telux::common::Status setHorizontalAccuracyLevel(
      HorizontalAccuracyLevel accuracy = HorizontalAccuracyLevel::LOW,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) = 0;

/**
 * Configuring minimum time interval between two consecutive position reports.
 *
 * @param [in] minInterval - Minimum time interval between two consecutive reports
 *                           in milliseconds.
 *                           E.g. If minInterval is 1000 milliseconds, reports will be
 *                           provided with a periodicity of 1 second or more depending
 *                           on the number of applications listening to location updates.
 * @param [in] callback - Optional callback to get the response of set
 *                        minimum interval for reports.
 *
 * @returns Status of setMinIntervalForReports i.e. success or suitable status code.
 *
 *
 * @deprecated API is not going to be supported in future releases. Clients should stop using
 * this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
 */
  virtual telux::common::Status setMinIntervalForReports(
      uint32_t minInterval,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr) = 0;

/**
 * Get timeout of a position report.
 *
 * @returns Maximum time to get a position report.
 *
 *
 * @deprecated API is not going to be supported in future releases. Clients should stop using
 * this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
 */
  virtual uint32_t getPositionReportTimeout() = 0;

/**
 * Get horizontal accuracy level of a location fix.
 *
 * @returns @ref HorizontalAccuracyLevel.
 *
 *
 * @deprecated API is not going to be supported in future releases. Clients should stop using
 * this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
 */
  virtual HorizontalAccuracyLevel getHorizontalAccuracyLevel() = 0;

/**
 * Get the time interval between final reports.
 *
 * @returns Minimum time interval between final position reports.
 *
 *
 * @deprecated API is not going to be supported in future releases. Clients should stop using
 * this API. Once an API has been marked as Deprecated, the API could be removed in future releases.
 */
  virtual uint32_t getMinIntervalForFinalReports() = 0;

/**
 * Destructor of ILocationManager
 */
  virtual ~ILocationManager() {}
  ;
};
/** @} */ /* end_addtogroup telematics_location */

} // end of namespace loc

} // end of namespace telux

#endif // LOCATION_MANAGER_HPP
