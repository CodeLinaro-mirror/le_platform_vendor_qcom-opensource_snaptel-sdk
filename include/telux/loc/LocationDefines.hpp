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
* @file       LocationDefines.hpp
*
* @brief      LocationDefines contains types related to location services.
*
* @note       Eval: This is a new API and is being evaluated. It is subject to
*             change and could break backwards compatibility.
*/

#include <memory>
#include <vector>
#include <bitset>

#include "telux/common/CommonDefines.hpp"

#ifndef LOCATION_DEFINES_HPP
#define LOCATION_DEFINES_HPP

namespace telux {

namespace loc {
/** @addtogroup telematics_location
 * @{ */

/**
 * Defines recurrence type of the fix.
 */
enum class FixRecurrence {
   PERIODIC = 1, /**< Request periodic fixes, minimum interval between final reports
                      will be the periodicity. Client can configure it using
                      LocationService API i.e. setMinIntervalForFinalReports*/
   SINGLE = 2    /**< Request a single fix */
};

/**
 * Defines the horizontal accuracy level of the fix.
 */
enum class HorizontalAccuracyLevel {
   LOW = 1,    /**< Client requires low horizontal accuracy */
   MEDIUM = 2, /**< Client requires medium horizontal accuracy */
   HIGH = 3    /**< Client requires high horizontal accuracy */
};

/**
 * Defines technology used in computing the location fix.
 */
enum PositionTechType {
   SATELLITE,                /**< Satellites used to generate the fix */
   CELLID,                   /**< Cell towers used to generate the fix */
   WIFI,                     /**< Wi-Fi access points used to generate the fix */
   SENSORS,                  /**< Sensors used to generate the fix */
   REFERENCE_LOCATION,       /**< Reference location used to generate the fix */
   INJECTED_COARSE_POSITION, /**< Coarse position injected into the location engine
                                  used to generate the fix */
   AFLT,                     /**< Advanced Forward Link Trilateration(AFLT), the phone
                                  takes measurements of signals from nearby towers and reports
                                  the time/distance readings back to the network
                                  to generate the fix */
   HYBRID                    /**< GNSS and network-provided measurements used
                                  to generate the fix*/
};

/**
 * 16 bit mask that denotes which of the technologies defined in PositionTech enum are used
 * in the location fix.
 */
using PositionTech = std::bitset<16>;

/**
 * Specifies the reliability of the position.
 */
enum class LocationReliability {
   UNKNOWN = -1,
   NOT_SET = 0,  /**<  Location reliability is not set */
   VERY_LOW = 1, /**<  Location reliability is very low */
   LOW = 2,      /**<  Location reliability is low, little or no cross-checking is possible */
   MEDIUM = 3,   /**<  Location reliability is medium, limited cross-check passed */
   HIGH = 4      /**<  Location reliability is high, strong cross-check passed */
};

/**
 * Defines Satellite Based Augmentation System(SBAS) corrections.
 * SBAS contributes to improve the performance of GNSS system.
 */
enum SbasCorrectionType {
   SBAS_CORRECTION_IONO, /**< Bit mask to specify whether
                              SBAS ionospheric correction is used */
   SBAS_CORRECTION_FAST, /**< Bit mask to specify whether
                              SBAS fast correction is used */
   SBAS_CORRECTION_LONG, /**< Bit mask to specify whether
                              SBAS long correction is used */
   SBAS_INTEGRITY        /**< Bit mask to specify whether
                              SBAS integrity information is used */
};

/**
 * 8 bit mask that denotes which of the SBAS corrections in SbasCorrection used
 * to improve the performance of GNSS output.
 */
using SbasCorrection = std::bitset<8>;

/**
 * Defines status of the session that is requested by user application.
 */
enum class SessionStatus {
   UNKNOWN = -1,
   SUCCESS = 0,     /**< Session successful */
   IN_PROGRESS = 1, /**< Session is still in progress, further position reports will be generated
                     until either the fix criteria specified by the client are met or the
                     client response time out occurs */
   GENERAL_FAILURE = 2, /**< Session failed */
   TIMEOUT = 3,         /**< Fix request failed because the session timed out */
   USER_END = 4,        /**< Fix request failed because the session was ended by the user */
   BAD_PARAMETER = 5,   /**< Fix request failed due to bad parameters in the request */
   PHONE_OFFLINE = 6,   /**< Fix request failed because the phone is offline */
   ENGINE_LOCKED = 7,   /**< Fix request failed because the engine is locked */
};

/**
 * Indicates whether altitude is assumed or calculated.
 */
enum class AltitudeType {
   UNKNOWN = -1,
   CALCULATED = 0, /**< Altitude is calculated  */
   ASSUMED = 1,    /**< Altitude is assumed, there may not be enough
                        satellites to determine the precise altitude */
};

/**
 * Defines constellation type of GNSS.
 */
enum class GnssConstellationType {
   UNKNOWN = -1,
   GPS = 1,     /**< GPS satellite */
   GALILEO = 2, /**< GALILEO satellite */
   SBAS = 3,    /**< SBAS satellite */
   COMPASS = 4, /**< COMPASS satellite */
   GLONASS = 5, /**< GLONASS satellite */
   BDS = 6,     /**< BDS satellite */
   QZSS = 7,    /**< QZSS satellite */
};

/**
 * Health status indicates whether satellite is operational or not.
 * This information comes from the most recent data transmitted in satellite almanacs.
 */
enum class SVHealthStatus {
   UNKNOWN = -1,
   UNHEALTHY = 0, /**< satellite is not operational and cannot be used in position calculations */
   HEALTHY = 1    /**< satellite is fully operational */
};

/**
 * Satellite vehicle processing status.
 */
enum class SVStatus {
   UNKNOWN = -1,
   IDLE = 0,   /**< SV is not being actively processed  */
   SEARCH = 1, /**< The system is searching for this SV */
   TRACK = 2   /**< SV is being tracked */
};

/**
 * Indicates whether Satellite Vehicle info like ephemeris and
 * almanac are present or not
 */
enum class SVInfoAvailability {
   UNKNOWN = -1,
   YES = 0, /**< Ephemeris or Almanac exits  */
   NO = 1   /**< Ephemeris or Almanac doesn't exist */
};

/**
 * Defines which sensors were used in calculating the position in the position report
 */
enum class SensorType {
   UNKNOWN = -1,
   ACCELEROMETER = 1, /**<  Bitmask to specify whether an accelerometer was used */
   GYROSCOPE = 2      /**<  Bitmask to specify whether a gyroscope was used */
};

/**
 * Specifies which measurements were aided by sensors.
 */
enum MeasurementType {
   UNKNOWN = -1,
   HEADING,  /**<  Bitmask to specify whether a sensor was used to calculate heading */
   SPEED,    /**<  Bitmask to specify whether a sensor was used to calculate speed */
   POSITION, /**<  Bitmask to specify whether a sensor was used to calculate position */
   VELOCITY  /**<  Bitmask to specify whether a sensor was used to calculate velocity */
};

/**
 * 8 bit mask that denotes which of the measurements in MeasurementType enum are aided
 * by sensor data.
 */
using Measurement = std::bitset<8>;

/**
 * @brief IGpsTime provides interface to get current GPS week and elapsed
 *        time in current GPS week
 *
 * @note    Eval: This is a new API and is being evaluated.It is subject to change and could break
 * backwards compatibility.
 */
class IGpsTime {
public:
   /**
    * Retrieves current GPS week as calculated from midnight, Jan 6, 1980.
    *
    * @returns Unsigned 32-bit integer containing week number.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual uint32_t getWeek() = 0;

   /**
    * Retrieves elapsed time in the current GPS week starting from 12:00 am on Sunday.
    *
    * @returns Unsigned 32-bit integer containing time in milliseconds.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual uint32_t getTimeOfWeekMsec() = 0;
};

/**
 * @brief Specifies the sensors used for calculating the fixes
 *        and the type of measurements which were aided by sensor data.
 *
 * @note    Eval: This is a new API and is being evaluated.It is subject to change and could break
 * backwards compatibility.
 */
class ISensorDataUsage {
public:
   /**
    * Retrieves which sensors were used in calculating the position in the position report.
    *
    * @returns @ref SensorType if available.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual SensorType getSensorType() = 0;

   /**
    * Retrieves which measurements were aided by sensor data.
    *
    * @returns Measurement types if available.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual Measurement getMeasurement() = 0;
};

/**
 * @brief ILocationInfo provides interface to get postion related information like
 *        latitude, longitude, altitude and other information like timestamp, session status etc
 *
 * @note    Eval: This is a new API and is being evaluated.It is subject to change and could break
 * backwards compatibility.
 */
class ILocationInfo {
public:
   /**
    * Retrieves technology used in computing this fix.
    *
    * @returns Position technology.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual PositionTech getPositionTechnology() = 0;

   /**
    * Retrieves latitude.
    * Positive and negative values indicate northern and southern latitude respectively
    *    - Units: Degrees
    *    - Range: -90.0 to 90.0
    *
    * @returns Latitude if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual double getLatitude() = 0;

   /**
    * Retrieves longitude.
    * Positive and negative values indicate eastern and western longitude respectively
    *    - Units: Degrees
    *    - Range: -180.0 to 180.0
    *
    * @returns Longitude if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual double getLongitude() = 0;

   /**
    * Retrieves altitude above the WGS 84 reference ellipsoid.
    *    - Units: Meters
    *
    * @returns Altitude if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual double getAltitude() = 0;

   /**
    * Retrieves heading.
    *    - Units: Degrees
    *    - Range: 0 to 359.999
    *
    * @returns Heading if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getHeading() = 0;

   /**
    * Retrieves the vertical uncertainty.
    *    - Units: Meters
    *
    * @returns Vertical uncertainty if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getVerticalUncertainty() = 0;

   /**
    * Retrieves UTC timeStamp for the location fix.
    *    - Units: Milliseconds since Jan 1, 1970
    *
    * @returns TimeStamp in seconds if available else returns 0
    * (as UTC timeStamp has elapsed since January 1, 1970, it cannot be 0)
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual uint64_t getTimeStamp() = 0;

   /**
    * Retrieves the altitude with respect to mean sea level.
    *    - Units: Meters
    *
    * @returns Altitude with respect to mean sea level if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getAltitudeMeanSeaLevel() = 0;

   /**
    * Retrieves position dilution of precision.
    *
    * @returns Position dilution of precision if available else returns NaN.
    * Range: 1 (highest accuracy) to 50 (lowest accuracy)
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getPositionDop() = 0;

   /**
    * Retrieves horizontal dilution of precision.
    *
    * @returns Horizontal dilution of precision if available else returns NaN.
    * Range: 1 (highest accuracy) to 50 (lowest accuracy)
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getHorizontalDop() = 0;

   /**
    * Retrieves vertical dilution of precision.
    *
    * @returns Vertical dilution of precision if available else returns NaN
    * Range: 1 (highest accuracy) to 50 (lowest accuracy)
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getVerticalDop() = 0;

   /**
    * Retrieves the difference between the bearing to true north and the bearing
    * shown on magnetic compass. The deviation is positive when the magnetic
    * north is east of true north.
    *    - Units: Degrees
    *
    * @returns Magnetic Deviation if available else returns NaN
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getMagneticDeviation() = 0;

   /**
    * Retrieves 3-D speed uncertainty.
    *    - Units: Meters per Second
    *
    * @returns Speed uncertainty if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getSpeedUncertainty() = 0;

   /**
    * Retrieves heading uncertainty.
    *    - Units: Degrees
    *    - Range: 0 to 359.999
    *
    * @returns Heading uncertainty if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getHeadingUncertainty() = 0;

   /**
    * Specifies the reliability of the horizontal position.
    *
    * @returns @ref LocationReliability of the horizontal position if available else returns
    * UNKNOWN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual LocationReliability getHorizontalReliability() = 0;

   /**
    * Specifies the reliability of the vertical position.
    *
    * @returns @ref LocationReliability of the vertical position if available else returns UNKNOWN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual LocationReliability getVerticalReliability() = 0;

   /**
    * Retrieves semi-major axis of horizontal elliptical uncertainty.
    *    - Units: Meters
    *
    * @returns Semi-major horizontal elliptical uncertainty if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getHorizontalUncertaintySemiMajor() = 0;

   /**
    * Retrieves semi-minor axis of horizontal elliptical uncertainty.
    *    - Units: Meters
    *
    * @returns Semi-minor horizontal elliptical uncertainty
    * if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getHorizontalUncertaintySemiMinor() = 0;

   /**
    * Retrieves elliptical horizontal uncertainty azimuth of orientation.
    *    - Units: Decimal degrees
    *    - Range: 0 to 180
    *
    * @returns Elliptical horizontal uncertainty azimuth of orientation
    * if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getHorizontalUncertaintyAzimuth() = 0;

   /**
    * Retrieves GNSS Satellite Vehicles used in position data.
    *
    * @param [out] idsOfUsedSVs Vector of Satellite Vehicle identifiers.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual void getSVIds(std::vector<uint16_t> &idsOfUsedSVs) = 0;

   /**
    * Retrieves navigation solution mask used to indicate SBAS corrections.
    *
    * @return - SBAS (Satellite Based Augmentation System) Correction mask used.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual SbasCorrection getSbasCorrection() = 0;

   /**
    * Retrieves status of the session that is requested by user application.
    *
    * @returns @ref SessionStatus
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   // TODO: Need to add interface to handle multiple sessions
   //       Need to support multiple user sessions
   virtual SessionStatus getSessionStatus() = 0;

   /**
    * Retrieves leap seconds if available.
    *
    * @param [out] leapSeconds - leap seconds
    *       - Units: Seconds
    *
    * @returns Status of leap seconds.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual telux::common::Status getLeapSeconds(uint8_t &leapSeconds) = 0;

   /**
    * Retrieves GPS time structure.
    *
    * @returns Pointer of IGpsTime object if available else returns null pointer.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual std::shared_ptr<IGpsTime> getGpsTime() = 0;

   /**
    * Retrieves horizontal position uncertainty (circular) if available.
    *
    * @param [out] circularHorizontalUncertainty - circular horizontal uncertainty
    *       - Units: Meters
    *
    * @returns Status of circular horizontal uncertainty.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual telux::common::Status
      getCircularHorizontalUncertainty(float &circularHorizontalUncertainty)
      = 0;

   /**
    * Retrieves horizontal uncertainty confidence if available.
    *
    * @param [out] horizontalConfidence - horizontal uncertainty confidence
    *       - Units: Percent
    *       - Range: 0 to 99
    *
    * @returns Status of horizontal uncertainty confidence.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual telux::common::Status getHorizontalConfidence(uint8_t &horizontalConfidence) = 0;

   /**
    * Retrieves horizontal speed.
    *       - Units: Meters/second
    *
    * @returns horizontal speed if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getHorizontalSpeed() = 0;

   /**
    * Retrieves vertical uncertainty confidence if available.
    *
    * @param [out] verticalConfidence - vertical uncertainty confidence
    *       - Units: Percent
    *       - Range: 0 to 99
    *
    * @returns Status of vertical uncertainty confidence.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual telux::common::Status getVerticalConfidence(uint8_t &verticalConfidence) = 0;

   /**
    * Retrieves vertical speed.
    *       - Units: Meters/second
    *
    * @returns Float containing vertical speed if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getVerticalSpeed() = 0;

   /**
    * Retrieves sensor data was used in computing the position if available.
    *
    * @param [out] sensorDataUsage - which sensors were used in calculating the position
    *
    * @returns Status of availability of sensorDataUsage.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual telux::common::Status
      getSensorDataUsage(std::shared_ptr<ISensorDataUsage> &sensorDataUsage)
      = 0;

   /**
    * Retrieves fix count if available. Fix count of a session starts with 0 and
    * increments by one for each successive position report for a particular session.
    *
    * @param [out] fixId - identifier of fix for session
    *
    * @returns Status of availability of fix identifier.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual telux::common::Status getFixId(uint32_t &fixId) = 0;

   /**
    * Retrieves east, North, Up velocity if available.
    *
    * @param [out] velocityEastNorthUp - east, North, Up velocity
    *       - Units: Meters/second
    *
    * @returns Status of availability of east, North, Up velocity.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual telux::common::Status getVelocityEastNorthUp(std::vector<float> &velocityEastNorthUp)
      = 0;

   /**
    * Retrieves east, North, Up velocity uncertainty if available.
    *
    * @param [out] velocityUncertaintyEastNorthUp - east, North, Up velocity uncertainty
    *       - Units: Meters/second
    *
    * @returns Status of availability of east, North, Up velocity uncertainty.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual telux::common::Status
      getVelocityUncertaintyEastNorthUp(std::vector<float> &velocityUncertaintyEastNorthUp)
      = 0;
};

/**
 * @brief ISVInfo provides interface to retreive inforation
 *        about Satellite Vehicles, their position and health status
 */
class ISVInfo {
public:
   /**
    * Indicates to which constellation this satellite vehicle belongs.
    *
    * @returns  @ref GnssConstellationType if available else returns UNKNOWN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual GnssConstellationType getConstellation() = 0;

   /**
    * GNSS satellite vehicle ID.
    *
    * @returns Identifier of the satellite vehicle otherwise 0(as 0 is not an ID for any of the SVs)
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual uint16_t getId() = 0;

   /**
    * Health status of satellite vehicle.
    *
    * @returns  HealthStatus of Satellite Vehicle if available else returns UNKNOWN.
    *          - @ref SVHealthStatus
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual SVHealthStatus getSVHealthStatus() = 0;

   /**
    * Status of satellite vehicle.
    *
    * @note    This API is work-in-progress and is subject to change.
    * @returns Satellite Vehicle Status if available else returns UNKNOWN.
    *          - @ref SVStatus
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual SVStatus getStatus() = 0;

   /**
    * Indicates whether ephemeris information(which allows the receiver
    * to calculate the satellite's position) is available.
    *
    * @returns @ref SVInfoAvailability if Ephemeris exists or not else returns UNKNOWN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual SVInfoAvailability getHasEphemeris() = 0;

   /**
    * Indicates whether almanac information(which allows receivers to know
    * which satellites are available for tracking) is available.
    *
    * @returns @ref SVInfoAvailability if almanac exists or not else returns UNKNOWN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual SVInfoAvailability getHasAlmanac() = 0;

   /**
    * Retrieves satellite vehicle elevation angle.
    *    - Units: Degrees
    *    - Range: 0 to 90
    *
    * @returns Elevation if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getElevation() = 0;

   /**
    * Retrieves satellite vehicle azimuth angle.
    *    - Units: Degrees
    *    - Range: 0 to 360
    *
    * @returns Azimuth if available else returns NaN.
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getAzimuth() = 0;

   /**
    * Retrieves satellite vehicle signal-to-noise ratio.
    *    - Units: dB-Hz
    *
    * @returns SNR if available else returns NaN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual float getSnr() = 0;
};

/**
 * @brief IGnssSVInfo provides interface to retrieve the list of SV info available
 *        and whether altitude is assumed or calculated.
 */
class IGnssSVInfo {
public:
   /**
    * Indicates whether altitude is assumed or calculated.
    *
    * @returns @ref AltitudeType if available else returns UNKNOWN.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    * break backwards compatibility.
    */
   virtual AltitudeType getAltitudeType() = 0;

   /**
    * Pointer to satellite vehicles information for all GNSS
    * constellations except GPS.
    *
    * @returns Vector of pointer of ISVInfo object if available else returns empty vector.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual std::vector<std::shared_ptr<ISVInfo>> getSVInfoList() = 0;
};

/** @} */ /* end_addtogroup telematics_location */

}  // end of namespace loc
}  // end of namespace telux

#endif  // LOCATION_DEFINES_HPP
