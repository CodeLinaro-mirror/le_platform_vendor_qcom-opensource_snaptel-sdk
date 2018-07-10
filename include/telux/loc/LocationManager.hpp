/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
*             and satellite vehicle information updates. It also allows
*             to configure the location engine.
*
* @note       Eval: This is a new API and is being evaluated.It is subject to change
*             and could break backwards compatibility.
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
 * @brief ILocationManager provides interface to register and remove listeners. It also
 *        allows to set and get configuration/ criteria for position reports.
 * @note    Eval: This is a new API and is being evaluated.It is subject to change
 *          and could break backwards compatibility.
 */
class ILocationManager {
public:
   /**
    * Checks the status of location subsystems and returns the result.
    *
    * @returns True if location subsystem is ready for service otherwise false.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual bool isSubsystemReady() = 0;

   /**
    * Wait for location subsystem to be ready.
    *
    * @returns  A future that caller can wait on to be notified when location
    *           subsystem is ready.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * Register a listener for specific updates from location manager like location and satellite
    * vehicle info. If enhanced position, using Dead Reckoning etc., is enabled, enhanced fixes
    * will be provided. Otherwise raw GNSS fixes will be provided.
    *
    * @param [in] listener - Pointer of ILocationListener object that processes the notification
    *
    * @returns Status of registerListener i.e success or suitable status code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<ILocationListener> listener) = 0;

   /**
    * Remove a previously registered listener.
    *
    * @param [in] listener - Previously registered ILocationListener that needs to be removed
    *
    * @returns Status of removeListener success or suitable status code
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status removeListener(std::weak_ptr<ILocationListener> listener) = 0;

   /**
    * Configures position report timeout.
    * LocationManager tries to determine the position until the position report timeout has
    * elapsed. If the final position cannot be determined before the timeout period, it returns
    * a position report with session status marked as SessionStatus::TIMEOUT instead of
    * SessionStatus::SUCCESS. Position report timeout is an app spectific setting.
    * E.g. If this API is called with timeout parameter value 5000, the LocationManager tries to
    *      determine the position before 5 seconds. If position report is determined, a position
    *      report will be returned with sessionStatus=SessionStatus::SUCCESS. Otherwise, a
    *      position report will be sent with sessionStatus=SessionStatus::TIMEOUT after 5 seconds.
    *
    * @param [in] timeout  - Maximum time to get a position report in milliseconds.
    * @param [in] callback - Optional callback to get the response of set
    *                        position report time out
    *
    * @returns Status of setPositionReportTimeout i.e. success or suitable status code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status setPositionReportTimeout(
      uint32_t timeout, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

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
    * @returns Status of setHorizontalAccuracyLevel i.e. success or suitable status code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status setHorizontalAccuracyLevel(
      HorizontalAccuracyLevel accuracy = HorizontalAccuracyLevel::LOW,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Configuring minimum time interval between two consecutive position reports.
    *
    * @param [in] minInterval - Minimum time interval between two consecutive reports
    *                           in milliseconds.
    *                           E.g. If minInterval is 1000 milliseconds, reports will be
    *                           provided with a periodicity of 1 second or more depending
    *                           on the number of applications listening to location updates.
    * @param [in] callback - Optional callback to get the response of set
    *             minimum interval for reports.
    *
    * @returns Status of setMinIntervalForReports i.e. success or suitable status code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status setMinIntervalForReports(
      uint32_t minInterval,
      std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Get timeout of a position report.
    *
    * @returns Maximum time to get a position report.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual uint32_t getPositionReportTimeout() = 0;

   /**
    * Get horizontal accuracy level of a location fix.
    *
    * @returns @ref HorizontalAccuracyLevel.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual HorizontalAccuracyLevel getHorizontalAccuracyLevel() = 0;

   /**
    * Get the time interval between final reports.
    *
    * @returns Minimum time interval between final position reports.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual uint32_t getMinIntervalForFinalReports() = 0;

   /**
    * Destructor of ILocationManager
    */
   virtual ~ILocationManager(){};
};
/** @} */ /* end_addtogroup telematics_location */

}  // end of namespace loc

}  // end of namespace telux

#endif  // LOCATION_MANAGER_HPP
