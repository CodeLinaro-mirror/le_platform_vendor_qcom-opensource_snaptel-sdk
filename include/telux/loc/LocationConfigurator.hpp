/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @brief ILocationConfigurator allows general engine configurations (example: TUNC, PACE etc),
 * configuration of specific engines like SPE (example: minSVElevation, minGPSWeek etc) or DRE,
 * deletion of warm and cold aiding data, NMEA configuration and support for XTRA feature.
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
 * This function is called with the response to requestMinSVElevation API.
 *
 * @param[in] minSVElevation - minimum SV Elevation angle in units of degree.
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibilty.
 *
 */
 using GetMinSVElevationCallback = std::function<void(uint8_t minSVElevation,
     telux::common::ErrorCode error)>;

/** This function is called with the response to requestRobustLocation API.
 *
 * @param[in] rLConfig - robust location settings information.
 *
 *  @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 *  @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibilty.
 *
 */
using GetRobustLocationCallback = std::function<void(const telux::loc::
     RobustLocationConfiguration rLConfig, telux::common::ErrorCode error)>;

/** This function is called with the response to requestXtraStatus API.
 *
 * @param[in] xtraStatus - Information pertaining to Xtra assistance data.
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 */
 using GetXtraStatusCallback = std::function<void(const telux::loc::XtraStatus xtraStatus,
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
  * When resetToDefault is false then the list is expected to contain the constellations or SVs
  * that should be blacklisted. An empty list could be specified to allow all constellations/SVs
  * (i.e. none will be blacklisted) in determining the fix.
  * When resetToDefault is set to true, the device will revert to the default list of SV/
  * constellations to be blacklisted.
  *
  * @param [in] list - specify the set of constellations and SVs that should not be used
  *                    by the GNSS engine on modem. Constellations and SVs not specified
  *                    in blacklistedSvList could get used by the GNSS engine on modem.
  *
  * @param [in] callback - Optional callback to get the response of configure constellations.
  *
  * @param [in] resetToDefault - when set to true, the device will revert to the default list of
  *                              SV/constellation to be blacklisted. When set to false, list will
  *                              be inspected to determine what should be blacklisted.
  *
  */


  virtual telux::common::Status configureConstellations(const SvBlackList& list,
        telux::common::ResponseCallback callback = nullptr, bool resetToDefault = false) = 0;

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
  * This API retrieves the robust location settings used by the GNSS engine.
  *
  * @param [in] cb - callback to retrieve robust location information.
  *
  * @returns Status of requestRobustLocation i.e. success or suitable status code.
  *
  * @note Eval: This is a new API and is being evaluated. It is subject to change and could
  *             break backwards compatibility.
  *
  */

  virtual telux::common::Status requestRobustLocation(GetRobustLocationCallback cb) = 0;

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
  * This API configures the minimum SV elevation angle setting used by the GNSS standard position
  * engine. Configuring minimum SV elevation setting will not cause position engine to stop
  * tracking low elevation SVs. This elevation mask is used to filter out the SVs that are used in
  * determining the position. SVs with an elevation below this setting will be excluded from
  * position determination. So configuring this to a large angle will filter out most of the SVs
  * and will have adverse affects on the performance of the position engine.
  *
  * This setting does not impact the SV information and SV measurement reports retrieved from APIs
  * such as IGnssSvINfo::getSVInfoList, ILocationListener::onGnssMeasurementsInfo.
  *
  * Client should wait for the command to finish, e.g.: via ResponseCallback received, before
  * issuing a second configureMinElevation command. If this API is called while the GNSS Position
  * Engine is in the middle of a session, ResponseCallback will still be invoked shortly after to
  * indicate the setting has been received by the SPE engine. However the enigne can take some time
  * to apply this setting if it is in middle of a session (as long as 255 seconds in some
  * implementations). It is advised to use this API with caution and only for very limited usage
  * scenario, e.g.: for performance test, certification process and for one-time device
  * configuration.
  *
  * @param [in] minSVElevation - minimum SV elevation to be used by GNSS standard position
  *                              engine (SPE). Valid range is [0, 90] in unit of degree.
  *
  * @param [in] callback - Optional callback to get the response of configure
  *                        minimum SV Elevation angle.
  *
  * @returns Status of configureMinSVElevation i.e. success or suitable status code.
  *
  * @note Eval: This is a new API and is being evaluated. It is subject to change and could
  *             break backwards compatibility.
  *
  */

  virtual telux::common::Status configureMinSVElevation(uint8_t minSVElevation,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
  * This API retrieves the minimum SV Elevation configuration used by the modem GNSS SPE engine.
  * If this API is invoked right after the configureMinSVElevation, the returned setting may not
  * match the one specified in configureMinSVElevation, as the setting received via
  * configureMinSVElevation might not have been applied yet as it takes time to apply the
  * setting if the GNSS SPE engine has an on-going session.
  *
  * @param [in] cb - callback to retrieve the minimum SV elevation.
  *
  * @returns Status of requestMinSVElevation i.e. success or suitable status code.
  *
  * @note Eval: This is a new API and is being evaluated. It is subject to change and could
  *             break backwards compatibility.
  *
  */

  virtual telux::common::Status requestMinSVElevation(GetMinSVElevationCallback cb) = 0;

/**
  * This API deletes specified aiding data from all position engines on the device. For
  * example, removing ephemeris data may trigger GNSS engine to do a warm start.
  *
  * @param [in] aidingDataMask - specify the set of aiding data to be deleted from all position
  *                              engines.
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
 * This API configures various parameters for dead reckoning position engine. Clients should
 * wait for the command to finish e.g.: via ResponseCallback to be received before issuing a
 * second configureDREngineParams command. Behavior is not defined if client issues a second
 * request of configureDREngineParams without waiting for the completion of the previous
 * configureDREngineParams request.
 *
 * @param [in] config - specify dead reckoning engine configuration.
 *
 * @param [in] callback - Optional callback to get the response of configureDREngineParams.
 *
 * @returns Status of configureDREngineParams i.e. success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 *
 */

  virtual telux::common::Status configureDR(const
      DREngineConfiguration& config, telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API is used to configure the NMEA sentence types that clients will receive via
 * @ref ILocationManager::startDetailedReports or
 * @ref ILocationManager::startDetailedEngineReports.
 * Without prior invocation to this API, all NMEA sentences supported in the system will get
 * generated and delivered to all the clients that register to receive NMEA sentences.
 * The NMEA sentence type configuration is common across all clients and updating it will affect
 * all clients.
 * This API call is not incremental and the new NMEA sentence types will completely overwrite the
 * previous call to this API.
 *
 * @param [in] nmeaType - specify the set of NMEA sentences
 *
 * @param [in] callback - Optional callback to get the response of configureNmeaTypes.
 *
 * @returns Status of configureNmeaTypes i.e. success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 *
 */

  virtual telux::common::Status configureNmeaTypes(const NmeaSentenceConfig nmeaType,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * Clients can request Terrestrial Positioning using @ref ILocationManager::getTerrestrialPosition.
 * Terrestrial Positioning requires sending device data to the cloud to get the position.
 * This functionality requires user consent. This API needs to be invoked to provide the user
 * consent.
 *
 * The consent will remain effective across power cycles, until this API is called with a
 * different value.
 *
 * @param [in] userConsent - true indicates user consents to sending device data to cloud,
 *                           false indicates user does not consent.
 *
 * @param [in] callback - Optional callback to get the response of
 *                        provideConsentForTerrestrialPositioning.
 *
 * @returns Status of provideConsentForTerrestrialPositioning i.e. success or suitable
 *          status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 *
 */

  virtual telux::common::Status provideConsentForTerrestrialPositioning(bool userConsent,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API registers an @ref ILocationInjectionListener listener and will receive notifications
 * regarding when to start or stop the location report injection using the @ref injectLocationData
 * API.
 *
 * @param [in] listener - Pointer of @ref ILocationInjectionListener object.
 *
 * @returns Status of registerLocationInjector i.e success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 *
 */
  virtual telux::common::Status
        registerLocationInjector(std::weak_ptr<ILocationInjectionListener> listener) = 0;

/**
 * This API removes a previously registered listener and will also stop receiving notifications
 * related to location data injection for that particular listener.
 *
 * @param [in] listener - Pointer of @ref ILocationInjectionListener object.
 *
 * @returns Status of deregisterLocationInjector i.e success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 *
 */
  virtual telux::common::Status
        deregisterLocationInjector(std::weak_ptr<ILocationInjectionListener> listener) = 0;

/**
 * Inject best location data
 *
 * This API is used to inject the @ref ExternalLocationInfo data into the GNSS engine.
 *
 * This API is intended to be invoked after @ref ILocationInjectionListener::onStartInjection API
 * call and is not expected to be invoked after @ref ILocationInjectionListener::onStopInjection
 * API call.
 * If this API is used without meeting the above mentioned pre-requisites, the GNSS engine
 * might produce low quality GNSS position.
 *
 * @param [in] location  @ref ExternalLocationInfo data to be injected.
 *
 * @return , Status of injectLocationData i.e. success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and
 *             could break backwards compatibility.
 *
 */
  virtual telux::common::Status injectLocationData(const ExternalLocationInfo& location) = 0;


/**
 * This API is used to instruct the specified engine to use the provided integrity risk level for
 * protection level calculation in position report.
 * This API can be called when a position session is in progress.
 * Prior to calling this API for a particular engine, the engine shall not calculate the
 * protection levels and shall not include the protection levels in its position report.
 * The implementation might not support protection levels across all engines. For engines that
 * don't support it, @ref ResponseCallback will get invoked with @ref ErrorCode::NOT_SUPPORTED.
 *
 * @param [in] engineType - the engine that is instructed to use the specified integrity risk
 *                          level for protection level calculation.
 *
 * @param [in] integrityRisk - the integrity risk level used for calculating protection level.
 *                             The integrity risk is defined as a probability per epoch, in unit
 *                             of 2.5e-10. The valid range for actual integrity is
 *                             [2.5e-10, 1-2.5e-10]), this corresponds to range of [1,4e9-1] of
 *                             this parameter.
 *
 * @param [in] callback - Optional callback to get the response of configureEngineIntegrityRisk.
 *
 * @returns Status of configureEngineIntegrityRisk i.e. success or suitable status code.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 *
 */

  virtual telux::common::Status configureEngineIntegrityRisk(const EngineType engineType,
      uint32_t integrityRisk, telux::common::ResponseCallback callback = nullptr ) = 0;

/**
 * This API is used to enable/disable the XTRA (Predicted GNSS Satellite Orbit Data) feature
 * on device. If XTRA feature is to be enabled, this API is also used to configure the various
 * XTRA settings in device.
 *
 * Clients need to note the below-
 *
 * 1. Wait for the ongoing request to finish prior to the next invocation else the behavior is
 *    undefined.
 * 2. The API is non-incremental i.e, the second call will overwrite the first call. Also the
 *    configured XTRA params will be persistent.
 *
 * @param [in] enable - Enable XTRA Feature on the device. False would disable both the XTRA
 *                      Assistance Data and NTP Time Download.
 *
 * @param [in] configParams - Configuration Parameters for XTRA on the device.
 *
 * @param [in] callback - Optional callback stating the response errorcode.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 *
 */

  virtual telux::common::Status configureXtraParams(bool enable, const XtraConfig configParams,
    telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API is used to query xtra feature setting and xtra assistance data status used by the GNSS
 * standard position engine (SPE). If XTRA_DATA_STATUS_UNKNOWN is returned but XTRA feature is
 * enabled, the client shall wait a few seconds before calling this API again.
 *
 * @param [in] callback - Callback to get the Xtra data status information.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 *
 */

  virtual telux::common::Status requestXtraStatus(GetXtraStatusCallback callback) = 0;

/**
 * This API is used to register a configuration listener for getting specific indications/updates.
 *
 * @param [in] indicationList - List of indications client wants to register under
 *                              telux::loc::LocConfigIndicationsType.
 *
 * @param [in] listener - Pointer of ILocationConfigListener object.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 */

  virtual telux::common::Status registerListener(LocConfigIndications indicationList,
    std::weak_ptr<ILocationConfigListener> listener) = 0;

/**
 * This API is used to deregister a configuration listener from specific indications/updates.
 *
 * @param [in] indicationList - List of indications client wants to deregister from under
 *                              telux::loc::LocConfigIndicationsType.
 *
 * @param [in] listener - Pointer of ILocationConfigListener object.
 *
 * @note Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 */

  virtual telux::common::Status deRegisterListener(LocConfigIndications indicationList,
    std::weak_ptr<ILocationConfigListener> listener) = 0;

/**
 * Destructor of ILocationConfigurator
 */
  virtual ~ILocationConfigurator() {};

};
/** @} */ /* end_addtogroup telematics_location */

} // end of namespace loc

} // end of namespace telux

#endif // LOCATION_MANAGER_HPP
