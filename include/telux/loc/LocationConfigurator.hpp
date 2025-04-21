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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef TELUX_LOC_LOCATIONCONFIGURATOR_HPP
#define TELUX_LOC_LOCATIONCONFIGURATOR_HPP

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
 * This function is called with the response to requestSecondaryBandConfig API.
 *
 * @param[in] set - disabled secondary band constellation configuration used by the GNSS
 *                  standard position engine (SPE).
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
 *
 */
 using GetSecondaryBandCallback = std::function<void(const telux::loc::ConstellationSet set,
     telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestMinGpsWeek API.
 *
 * @param[in] minGpsWeek - minimum gps week.
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 *
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
 */
 using GetMinSVElevationCallback = std::function<void(uint8_t minSVElevation,
     telux::common::ErrorCode error)>;

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
 * @deprecated use getServiceStatus()
 *
 */
  virtual bool isSubsystemReady() = 0;

/**
 * This status indicates whether the object is in a usable state.
 *
 * @returns SERVICE_AVAILABLE    -  If location configurator is ready for service.
 *          SERVICE_UNAVAILABLE  -  If location configurator is temporarily unavailable.
 *          SERVICE_FAILED       -  If location configurator encountered an irrecoverable failure.
 *
 */
  virtual telux::common::ServiceStatus getServiceStatus() = 0;

/**
 * Wait for location configuration subsystem to be ready.
 *
 * @returns  A future that caller can wait on to be notified when location
 *           configuration subsystem is ready.
 *
 * @deprecated The callback mechanism introduced in the
 * @ref LocationFactory::getLocationConfigurator() API will provide the similar notification
 * mechanism as onSubsystemReady(). This API will soon be removed from further releases.
 *
 */
  virtual std::future<bool> onSubsystemReady() = 0;

/**
 * This API enables or disables position assisted clock estimator feature. For multiple
 * invocations of this API, client should wait for the command to finish, e.g.: via
 * ResponseCallback received before issuing a second configurePACE command. Behavior is
 * not defined if client issues a second request of configurePACE without waiting for
 * the finish of the previous configurePACE request.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
 *
 * @param [in] enable - to enable/disable position assisted clock estimator feature.
 *
 * @param [in] callback - Optional callback to get the response of enablement/disablement of
 *                        PACE.
 */

  virtual telux::common::Status configurePACE(bool enable, telux::common::ResponseCallback callback
        = nullptr) = 0;

/**
 * This API deletes all forms of aiding data from all position engines. This API deletes all
 * assistance data used by GPS engine and force engine to do a cold start for next session.
 * Invoking this API will trigger cold start of all position engines on the device and will
 * cause significant delay for the position engines to produce next fix and may have other
 * performance impact. So, this API should only be exercised with caution and only for very
 * limited usage scenario, e.g.: for performance test and certification process.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
 *
 * @param [in] callback - Optional callback to get the response of delete aiding data.
 *
 */

  virtual telux::common::Status deleteAllAidingData(telux::common::ResponseCallback callback
        = nullptr) = 0;

/**
 * This API blacklists some constellations or subset of SVs from the constellation from being used
 * by the GNSS standard position engine (SPE).
 * Supported constellations for this API are GPS, GLONASS, QZSS, BEIDOU, GALILEO, SBAS and NAVIC.
 * For other constellations NOTSUPPORTED status will be returned.
 * For SBAS, SVs are not used in positioning by the GNSS standard position engine (SPE) by
 * default. Blacklisting SBAS SV only blocks SBAS data demodulation and will not disable SBAS
 * cross-correlation detection algorithms as they are necessary for optimal GNSS standard
 * position engine (SPE) performance.
 * When resetToDefault is false then the list is expected to contain the constellations or SVs
 * that should be blacklisted. An empty list could be specified to allow all constellations/SVs
 * (i.e. none will be blacklisted) in determining the fix.
 * When resetToDefault is set to true, the device will revert to the default list of SV/
 * constellations to be blacklisted.
 * For multiple invocations of this API, client should wait for the command to finish, e.g.: via
 * ResponseCallback received before issuing a second configureConstellations command. Behavior is
 * not defined if client issues a second request of configureConstellations without waiting for
 * the finish of the previous configureConstellations request. This API call is not incremental
 * and the new settings will completely overwrite the previous call.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
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
 * This API configures the secondary band constellations used by the GNSS standard position
 * engine. This API call is not incremental and the new settings will completely overwrite the
 * previous call.
 * The set specifies the supported constellations whose secondary band information should be
 * disabled. The absence of a constellation in the set will result in the secondary band being
 * enabled for that constellation. The modem has its own configuration in NV (persistent memory)
 * about which constellation's secondary bands are allowed to be enabled. When a constellation is
 * omitted when this API is invoked the secondary band for that constellation will only be enabled
 * if the modem configuration allows it. If not allowed then this API would be a no-op for that
 * constellation.
 * Passing an empty set to this API will result in all constellations as allowed by the modem
 * configuration to be enabled.
 * For multiple invocations of this API, client should wait for the command to finish, e.g.:
 * via ResponseCallback recieved, before issuing a second configureSecondaryBand command.
 * Behavior is not defined if client issues a second request of configureSecondaryBand without
 * waiting for the finish of the previous configureSecondaryBand request.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
 *
 * @param [in] set - specifies the set of constellations whose secondary bands need to be
 *                   disabled.
 *
 * @param [in] callback - Optional callback to get the response of configureSecondaryBand.
 *
 *
 */

  virtual telux::common::Status configureSecondaryBand(const ConstellationSet& set,
        telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API retrieves the secondary band configurations for constellation used by the standard
 * GNSS engine (SPE).
 *
 * @param [in] cb - callback to retrieve secondary band information about constellations.
 *
 * @returns Status of requestSecondaryBandConfig i.e. success or suitable status code.
 *
 *
 */

  virtual telux::common::Status requestSecondaryBandConfig(GetSecondaryBandCallback cb) = 0;

/**
 * This API configures the minimum GPS week used by the modem GNSS standard position engine (SPE)
 * and shall not be called while GNSS SPE is in the middle of a session.
 * Client needs to assure that there is no active GNSS SPE session prior to issuing this command.
 * Client should wait for the command to finish, e.g.: via ResponseCallback received before
 * issuing a second configureMinGpsWeek command. Behavior is not defined if client issues a second
 * request of configureMinGpsWeek without waiting for the previous configureMinGpsWeek to finish.
 * Additionally minimum GPS week number shall NEVER be in the future of the current GPS Week.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
 *
 * @param [in] minGpsWeek - minimum GPS week to be used by modem GNSS engine.
 *
 * @param [in] callback - Optional callback to get the response of configure
 *                        minimum GPS week.
 *
 * @returns Status of configureMinGpsWeek i.e. success or suitable status code.
 *
 *
 */
  virtual telux::common::Status configureMinGpsWeek(uint16_t minGpsWeek,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API retrieves the minimum GPS week configuration used by the modem GNSS standard position
 * engine (SPE). If this API is called right after configureMinGpsWeek, the returned setting may
 * not match the one specified in configureMinGpsWeek, as the setting configured via
 * configureMinGpsWeek can not be applied to the GNSS standard position engine(SPE) when the
 * engine is in middle of a session. In poor GPS signal condition, the session may take up to 255
 * seconds to finish. If after 255 seconds of invoking configureMinGpsWeek, the returned value
 * still does not match, then the caller need to reapply the setting by invoking
 * configureMinGpsWeek again.
 *
 * @param [in] cb - callback to retrieve the minimum gps week.
 *
 * @returns Status of requestMinGpsWeek i.e. success or suitable status code.
 *
 *
 */
  virtual telux::common::Status requestMinGpsWeek(GetMinGpsWeekCallback cb) = 0;

/**
 * This API configures the minimum SV elevation angle setting used by the GNSS standard position
 * engine. Configuring minimum SV elevation setting will not cause SPE to stop tracking low
 * elevation SVs. It only controls the list of SVs that are used in the filtered position
 * solution, so SVs with elevation below the setting will be excluded from use in the filtered
 * position solution. Configuring this setting to large angle will cause more SVs to get filtered
 * out in the filtered position solution and will have negative performance impact.
 *
 * This setting does not impact the SV information and SV measurement reports retrieved from APIs
 * such as IGnssSvINfo::getSVInfoList, ILocationListener::onGnssMeasurementsInfo.
 *
 * To apply the setting, the GNSS standard position engine(SPE) will require GNSS measurement
 * engine and position engine to be turned off briefly. This may cause glitch for on-going
 * tracking session and may have other performance impact. So, it is advised to use this API with
 * caution and only for very limited usage scenario, e.g.: for performance test and certification
 * process and for one-time device configuration.
 *
 * Client should wait for the command to finish, e.g.: via ResponseCallback received, before
 * issuing a second configureMinElevation command. If this API is called while the GNSS Position
 * Engine is in the middle of a session, ResponseCallback will still be invoked shortly to
 * indicate the setting has been received by the SPE engine. However the actual setting can not
 * be applied until the current session ends, and this may take up to 255 seconds in poor GPS
 * signal condition.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
 *
 * @param [in] minSVElevation - minimum SV elevation to be used by GNSS standard position
 *                              engine (SPE). Valid range is [0, 90] in unit of degree.
 *
 * @param [in] callback - Optional callback to get the response of configure
 *                        minimum SV Elevation angle.
 *
 * @returns Status of configureMinSVElevation i.e. success or suitable status code.
 *
 *
 */

  virtual telux::common::Status configureMinSVElevation(uint8_t minSVElevation,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API retrieves the minimum SV Elevation configuration used by the modem GNSS SPE engine.
 * If this API is invoked right after the configureMinSVElevation, the returned setting may not
 * match the one specified in configureMinSVElevation, as the setting received via
 * configureMinSVElevation might not have been applied yet as it takes time to apply the
 * setting if the GNSS SPE engine has an on-going session. In poor GPS signal condition, the
 * session may take up to 255 seconds to finish. If after 255 seconds of invoking
 * configureMinSVElevation, the returned value still does not match, then the caller need to
 * reapply the setting by invoking configureMinSVElevation again.
 *
 * @param [in] cb - callback to retrieve the minimum SV elevation.
 *
 * @returns Status of requestMinSVElevation i.e. success or suitable status code.
 *
 *
 */

  virtual telux::common::Status requestMinSVElevation(GetMinSVElevationCallback cb) = 0;

/**
 * This API deletes specified aiding data from all position engines on the device. For
 * example, removing ephemeris data may trigger GNSS engine to do a warm start. Invoking this API
 * may cause noticeable delay for the position engine to produce first fix and may have other
 * performance impact. So, this API should only be exercised with caution and only for very
 * limited usage scenario, e.g.: for performance test and certification process.
 *
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
 *
 * @param [in] aidingDataMask - specify the set of aiding data to be deleted from all position
 *                              engines. Currently, only ephemeris deletion is supported.
 *
 * @param [in] callback - Optional callback to get the response of delete aiding data.
 *
 * @returns Status of deleteAidingData i.e. success or suitable status code.
 *
 *
 */

  virtual telux::common::Status deleteAidingData(AidingData aidingDataMask,
      telux::common::ResponseCallback callback = nullptr) = 0;

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
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
 *
 * @param [in] nmeaType - specify the set of NMEA sentences
 *
 * @param [in] callback - Optional callback to get the response of configureNmeaTypes.
 *
 * @returns Status of configureNmeaTypes i.e. success or suitable status code.
 *
 */

  virtual telux::common::Status configureNmeaTypes(const NmeaSentenceConfig nmeaType,
      telux::common::ResponseCallback callback = nullptr) = 0;

/**
 * This API is used to configure the NMEA sentences that the clients
 * will receive via @ref ILocationListener class APIs.
 * NMEA updates can be received by either:
 * a) Setting the
 * @ref telux::loc::GnssReportType::NMEA bit in the reportMask passed as a paramter to
 * @ref ILocationManager::startDetailedReports OR @ref ILocationManager::startDetailedEngineReports
 * and receive the sentences via @ref ILocationListener::onGnssNmeaInfo.
 * b) Setting the
 * @ref telux::loc::GnssReportType::ENGINE_NMEA bit in the reportMask passed as a paramter to
 * @ref ILocationManager::startDetailedEngineReports
 * and receive the sentences via @ref ILocationListener::onEngineNmeaInfo.
 *
 * Further, the engines from which NMEA sentences will be received depends
 * on the configuration made through this API AND the engine types chosen when starting the
 * position reports via @ref ILocationManager::startDetailedEngineReports.
 * Fused engine is always considered as set even if the client does not explicitly specify it.
 *
 * Without prior invocation to this API, all NMEA sentences supported in the system will get
 * generated and delivered to all the clients that register to receive NMEA sentences.
 * The NMEA sentence type configuration is common across all clients and updating it will affect
 * all clients.
 *
 * Please note that for the NMEA datum type request to be successful,
 * the nmea provider configuration in the GPS configuration file
 * should be set to application processor.
 *
 * This API call is not incremental and the new NMEA configuration will completely overwrite the
 * previous call to this API.
 *
 * Also refer to @ref ILocationManager::startDetailedEngineReports
 * to understand the types of NMEA updates to be received.
 *
 * @param [in] configParams - Configuration Parameters for Nmea on the device.
 *
 * @param [in] callback - Optional callback to get the response of configureNmea.
 *
 * @returns Status of configureNmea i.e. success or suitable status code.
 *
 *
 */

  virtual telux::common::Status configureNmea(const NmeaConfig configParams,
    telux::common::ResponseCallback callback = nullptr) = 0;

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
 * On platforms with Access control enabled, caller needs to have TELUX_LOC_CONFIG permission to
 * invoke this API successfully.
 *
 * @param [in] enable - Enable XTRA Feature on the device. False would disable both the XTRA
 *                      Assistance Data and NTP Time Download.
 *
 * @param [in] configParams - Configuration Parameters for XTRA on the device.
 *
 * @param [in] callback - Optional callback stating the response errorcode.
 *
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

#endif // TELUX_LOC_LOCATIONCONFIGURATOR_HPP
