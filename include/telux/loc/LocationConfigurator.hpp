/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @file       LocationConfigurator.hpp
 * @brief      Location configurator provides APIs for enabling/disabling
 *             the Constraint TimeUncertainty.
 *
 */

#ifndef LOCATION_CONFIGURATOR_HPP
#define LOCATION_CONFIGURATOR_HPP

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
 * @brief ILocationConfigurator allows for the enablement/disablement of the
 * APIs such as CTunc, PACE, deleteAllAidingData, configureLeverArm, configureConstellations,
 * configureRobustLocation, configureMinGpsWeek, requestMinGpsWeek, deleteAidingData.
 * ILocationConfigurator APIs strictly adheres to the principle of single client per process.
 */
class ILocationConfigurator {
public:

/**
 * This function is called with the response to requestMinGpsWeek API.
 *
 * @param[in] minGpsWeek - minimum gps week.
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibilty.
 *
 */
 using GetMinGpsWeekCallback = std::function<void(uint16_t minGpsWeek,
     telux::common::ErrorCode error)>;

/**
 * Checks the status of location configuration subsystems and returns the result.
 *
 * @returns True if location configuration subsystem is ready for service otherwise false.
 *
 */
  virtual bool isSubsystemReady() = 0;

/**
 * Wait for location configuration subsystem to be ready.
 *
 * @returns  A future that caller can wait on to be notified when location
 *           configuration subsystem is ready.
 *
 */
  virtual std::future<bool> onSubsystemReady() = 0;

/**
 * This API enables or disables the constrained time uncertainty(C-TUNC) feature. When the
 * vehicle is turned off this API helps to put constraint on the time uncertainty. For multiple
 * invocations of this API, client should wait for the command to finish, e.g.: via
 * ResponseCallback recieved before issuing a second configureCTunc command.
 *
 * @param [in] enable - true for enable C-TUNC feature and false for disable C-TUNC
 *                      feature.
 *
 * @param [in] callback - Optional callback to get the response of enablement/disablement of
 *                        C-TUNC.
 *
 * @param [in] timeUncertainty - specifies the time uncertainty threshold that gps engine
 *                              needs to maintain, in unit of milli-seconds.
 *
 * @param [in] energyBudget - specifies the power budget that the GPS engine is allowed to
 *                            spend to maintain the time uncertainty, in the unit of
 *                            100 micro watt second. If the power exceeds the energyBudget then
 *                            this API is disabled. This is a cumulative energy budget.
 *
 * @returns Status of configureCTunc i.e. success or suitable status code.
 *
 */
  virtual telux::common::Status configureCTunc(bool enable, telux::common::ResponseCallback callback
        = nullptr, float timeUncertainty = DEFAULT_TUNC_THRESHOLD, uint32_t energyBudget =
                DEFAULT_TUNC_ENERGY_THRESHOLD) = 0;

 /**
  * This API enables or disables position assisted clock estimator feature. For multiple
  * invocations of this API, client should wait for the command to finish, e.g.: via
  * ResponseCallback recieved before issuing a second configurePACE command.
  *
  * @param [in] enable - to enable/disable position assisted clock estimator feature.
  *
  * @param [in] callback - Optional callback to get the response of enablement/disablement of
  *                        PACE.
  */

  virtual telux::common::Status configurePACE(bool enable, telux::common::ResponseCallback callback
        = nullptr) = 0;

/**
  * This API deletes all form of aiding data from all position engines. This API deletes all
  * assistance data used by GPS engine and force engine to do a cold start for next session.
  * Invoking this API will trigger cold start of all position engines on the device.
  * This will cause significant delay for the position engines to produce next fix and may have
  * other performance impact.
  *
  * @param [in] callback - Optional callback to get the response of delete aiding data.
  *
  */

  virtual telux::common::Status deleteAllAidingData(telux::common::ResponseCallback callback
        = nullptr) = 0;

/**
  * This API sets the lever arm parameters for the vehicle. For multiple invocations of this API
  * client should wait for the command to finish, e.g.: via ResponseCallback recieved before
  * issuing a second configureLeverArm command.
  *
  * @param [in] info - lever arm configuration info regarding below three
  *                   types of lever arm info:
  *                   a: GNSS Antenna w.r.t the origin at the IMU (inertial measurement unit)
  *                   for DR engine
  *                   b: GNSS Antenna w.r.t the origin at the IMU (inertial measurement unit)
  *                   for VEPP engine
  *                   c: VRP (Vehicle Reference Point) w.r.t the origin (at the GNSS Antenna).
  *                   Vehicle manufacturers prefer the position output to be tied to a
  *                   specific point in the vehicle rather than where the antenna is placed
  *                   (midpoint of the rear axle is typical).
  *
  * @param [in] callback - Optional callback to get the response of configure lever arm.
  *
  */

  virtual telux::common::Status configureLeverArm(const LeverArmConfigInfo& info,
        telux::common::ResponseCallback callback = nullptr) = 0;

/**
  * This API blacklists some constellations or subset of SVs from the constellation from being used
  * by the GNSS engine on modem. For multiple invocations of this API, client should wait for the
  * command to finish, e.g.: via ResponseCallback recieved before issuing a second
  * configureConstellations command. This API call is not incremental and the new settings will
  * completely overwrite the previous call.
  * Supported constellations for this API are GLONASS, QZSS, BEIDOU, GALILEO and SBAS. For other
  * constellations NOTSUPPORTED status will be returned.
  * Nullptr of list will be interpreted as to reset the constellation configuration to
  * device default.
  *
  * @param [in] SvIdBlackList - specify the set of constellations and SVs that should not be used
  *                             by the GNSS engine on modem. Constellations and SVs not specified
  *                             in blacklistedSvList could get used by the GNSS engine on modem.
  *
  * @param [in] callback - Optional callback to get the response of configure constellations.
  *
  */


  virtual telux::common::Status configureConstellations(const SvBlackList& list,
        telux::common::ResponseCallback callback = nullptr) = 0;

/**
  * This API enables/disables robust location feature and enables/disables robust location while
  * device is on E911. This API focuses on detection and reporting GNSS spoofing in position, time
  * and navigation data. When this API is enabled it reports confidence of the GNSS spoofing by the
  * getConformityIndex() API defined in the ILocationInfoEx class.
  *
  * @param [in] enable - true to enable robust location and false to disable robust location.
  *
  * @param [in] enableForE911 - true to enable robust location when the device is on E911 session
  *                             and false to disable on E911 session. This parameter is only valid
  *                             if robust location is enabled.
  *
  * @param [in] callback - Optional callback to get the response of configure robust location.
  *
  * @note Eval: This is a new API and is being evaluated. It is subject to change and could
  *             break backwards compatibility.
  *
  */

  virtual telux::common::Status configureRobustLocation(bool enable,
      bool enableForE911 = false,
          telux::common::ResponseCallback callback = nullptr) = 0;

/**
  * This API configures the minimum GPS week used by the modem GNSS engine. Client should
  * wait for the command to finish, e.g.: via ResponseCallback recieved before issuing
  * a second configureMinGpsWeek command.
  *
  * @param [in] minGpsWeek - minimum GPS week to be used by modem GNSS engine.
  *
  * @param [in] callback - Optional callback to get the response of configure
  *                        minimum GPS week.
  *
  * @returns Status of configureMinGpsWeek i.e. success or suitable status code.
  *
  * @note Eval: This is a new API and is being evaluated. It is subject to change and could
  *             break backwards compatibility.
  *
  */

  virtual telux::common::Status configureMinGpsWeek(uint16_t minGpsWeek,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
  * This API retrieves the minimum GPS week configuration used by the modem GNSS engine.
  *
  * @param [in] cb - callback to retrieve the minimum gps week.
  *
  * @returns Status of requestMinGpsWeek i.e. success or suitable status code.
  *
  * @note Eval: This is a new API and is being evaluated. It is subject to change and could
  *             break backwards compatibility.
  *
  */

  virtual telux::common::Status requestMinGpsWeek(GetMinGpsWeekCallback cb) = 0;

/**
  * This API deletes specified aiding data from all position engines on the device. For
  * example, removing ephemeris data may trigger GNSS engine to do a warm start.
  *
  * @param [in] aidingDataMask - specify the set of aiding data to be deleted from all position
  *                              engines. Currently, only ephemeris deletion is supported.
  *
  * @param [in] callback - Optional callback to get the response of delete aiding data.
  *
  * @returns Status of deleteAidingData i.e. success or suitable status code.
  *
  * @note Eval: This is a new API and is being evaluated. It is subject to change and could
  *             break backwards compatibility.
  *
  */

  virtual telux::common::Status deleteAidingData(AidingData aidingDataMask,
      telux::common::ResponseCallback callback = nullptr) = 0;


/**
 * Destructor of ILocationConfigurator
 */
  virtual ~ILocationConfigurator() {};

};
/** @} */ /* end_addtogroup telematics_location */

} // end of namespace loc

} // end of namespace telux

#endif // LOCATION_MANAGER_HPP
