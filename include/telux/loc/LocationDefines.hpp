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
 * @file       LocationDefines.hpp
 *
 * @brief      LocationDefines contains types related to location services.
 *
 */

#include <memory>
#include <vector>
#include <bitset>
#include <unordered_map>

#include "telux/common/CommonDefines.hpp"

#ifndef LOCATION_DEFINES_HPP
#define LOCATION_DEFINES_HPP

namespace telux {

namespace loc {
/** @addtogroup telematics_location
* @{ */

const float UNKNOWN_CARRIER_FREQ = -1;
const int UNKNOWN_SIGNAL_MASK = 0;
const uint64_t UNKNOWN_TIMESTAMP = 0;
const float DEFAULT_TUNC_THRESHOLD = 0.0; /**< Default value for threshold of time uncertainty.
                                               Units: milli-seconds. */
const int DEFAULT_TUNC_ENERGY_THRESHOLD = 0; /**< Default value for energy consumed of time
                                                  uncertainty. The default here means that the
                                                  engine is allowed to use infinite power.
                                                  Units: 100 micro watt second. */
/**
 * Specifies which measurements were aided by sensors.
 */
enum MeasurementType {
  UNKNOWN, /**< Unknown measurement type*/
  HEADING, /**<  Bitmask to specify whether a sensor was used to calculate heading */
  SPEED, /**<  Bitmask to specify whether a sensor was used to calculate speed */
  POSITION, /**<  Bitmask to specify whether a sensor was used to calculate
            position */
  VELOCITY, /**<  Bitmask to specify whether a sensor was used to calculate
           velocity */
  MEASUREMENT_COUNT  /**< Bitset */
};

/**
 * 8 bit mask that denotes which of the measurements in MeasurementType enum are
 * aided by sensor data.
 */
using Measurement = std::bitset<MEASUREMENT_COUNT>;

/**
 * Defines RTCM injection data format
 */
enum class DgnssDataFormat{
  /** Source data format is unknown */
  DATA_FORMAT_UNKNOWN                = 0,
  /** Source data format is RTCM_3 */
  DATA_FORMAT_RTCM_3                 = 1,
  /** Source data format is 3GPP RTK Rel-15 */
  DATA_FORMAT_3GPP_RTK_R15           = 2
};

/**
 * Defines status reported by cdfw for RTCM injection.
 */
enum class DgnssStatus{
  /** Dgnss subsystem doesn't support the data source */
  DATA_SOURCE_NOT_SUPPORTED          = 1,
  /** Dgnss subsystem doesn't support the data format */
  DATA_FORMAT_NOT_SUPPORTED          = 2,
  /** After the source injects the data, dgnss subsystem discovers there is
   *  another higher priority source injecting the data at the
   *  same time, and the current injected data is dropped */
  OTHER_SOURCE_IN_USE                = 3,
  /** There is a parsing error such as unrecognized format, CRC
   *  check failure, value range check failure, etc.; the injected
   *  data is dropped */
  MESSAGE_PARSE_ERROR                = 4,
  /** Data source is not usable anymore */
  DATA_SOURCE_NOT_USABLE             = 5,
};

/**
 * Defines recurrence type of the fix.
 * Obsolete
 */
enum class FixRecurrence {
  PERIODIC = 1, /**< Request periodic fixes, minimum interval between final reports
                     will be the periodicity. Client can configure it using
                     LocationService API i.e. setMinIntervalForFinalReports*/
  SINGLE = 2 /**< Request a single fix */
};

/**
 * Defines the horizontal accuracy level of the fix.
 */
enum class HorizontalAccuracyLevel {
  LOW = 1,        /**< Client requires low horizontal accuracy */
  MEDIUM = 2, /**< Client requires medium horizontal accuracy */
  HIGH = 3    /**< Client requires high horizontal accuracy */
};

/**
 * Defines technology used in computing the location fix.
 */
enum PositionTechType {
  SATELLITE,          /**< Satellites used to generate the fix */
  CELLID,             /**< Cell towers used to generate the fix */
  WIFI,               /**< Wi-Fi access points used to generate the fix */
  SENSORS,            /**< Sensors used to generate the fix */
  REFERENCE_LOCATION, /**< Reference location used to generate the fix */
  INJECTED_COARSE_POSITION, /**< Coarse position injected into the location engine
                                 used to generate the fix */
  AFLT, /**< Advanced Forward Link Trilateration(AFLT), the phone
             takes measurements of signals from nearby towers and reports
             the time/distance readings back to the network to generate the fix */
  HYBRID, /**< GNSS and network-provided measurements used to generate the fix*/
  TECH_COUNT /**< Bitset */
};

/**
 * Bitset that denotes which of the technologies defined in PositionTechType
 * enum are used in the location fix.
 */
using PositionTech = std::bitset<TECH_COUNT> ;

/**
 * Specifies the reliability of the position.
 */
enum class LocationReliability {
  UNKNOWN = -1, /**< Unknown location reliability*/
  NOT_SET = 0, /**<  Location reliability is not set */
  VERY_LOW = 1, /**<  Location reliability is very low */
  LOW = 2, /**<  Location reliability is low, little or no cross-checking is possible */
  MEDIUM = 3, /**<  Location reliability is medium, limited cross-check passed */
  HIGH = 4 /**<  Location reliability is high, strong cross-check passed */
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
  SBAS_INTEGRITY, /**< Bit mask to specify whether
                      SBAS integrity information is used */
  SBAS_CORRECTION_DGNSS, /**< Bit mask to specify whether
                              SBAS DGNSS correction is used */
  SBAS_CORRECTION_RTK, /**< Bit mask to specify whether
                            SBAS RTK correction is used */
  SBAS_CORRECTION_PPP, /**< Bit mask to specify whether
                            SBAS PPP correction is used */
  SBAS_COUNT  /**< Bitset */
};

/**
 * 8 bit mask that denotes which of the SBAS corrections in SbasCorrection used
 * to improve the performance of GNSS output.
 */
using SbasCorrection = std::bitset<SBAS_COUNT>;

/**
 * Defines status of the session that is requested by user application.
 */
enum class SessionStatus {
  UNKNOWN = -1, /**< Unknown session status*/
  SUCCESS = 0, /**< Session successful */
  IN_PROGRESS = 1, /**< Session is still in progress, further position reports will be generated
                        until either the fix criteria specified by the client are met or the
                        client response time out occurs */
  GENERAL_FAILURE = 2, /**< Session failed */
  TIMEOUT = 3, /**< Fix request failed because the session timed out */
  USER_END = 4, /**< Fix request failed because the session was ended by the user */
  BAD_PARAMETER = 5, /**< Fix request failed due to bad parameters in the request */
  PHONE_OFFLINE = 6, /**< Fix request failed because the phone is offline */
  ENGINE_LOCKED = 7, /**< Fix request failed because the engine is locked */
};

/**
 * Indicates whether altitude is assumed or calculated.
 */
enum class AltitudeType {
  UNKNOWN = -1, /**< Unknown altitude type*/
  CALCULATED = 0, /**< Altitude is calculated  */
  ASSUMED = 1, /**< Altitude is assumed, there may not be enough
                    satellites to determine the precise altitude */
};

/**
 * Defines constellation type of GNSS.
 */
enum class GnssConstellationType {
  UNKNOWN = -1, /**< Unknown constellation type*/
  GPS = 1, /**< GPS satellite */
  GALILEO = 2, /**< GALILEO satellite */
  SBAS = 3, /**< SBAS satellite */
  COMPASS = 4, /**< COMPASS satellite.
               @deprecated constellation type is not supported.*/
  GLONASS = 5, /**< GLONASS satellite */
  BDS = 6, /**< BDS satellite */
  QZSS = 7, /**< QZSS satellite */
  NAVIC = 8 /**< NAVIC satellite*/
};

/**
 * Health status indicates whether satellite is operational or not.
 * This information comes from the most recent data transmitted in satellite
 * almanacs.
 */
enum class SVHealthStatus {
  UNKNOWN = -1, /**< Unknown sv health status*/
  UNHEALTHY = 0, /**< satellite is not operational and cannot be
                      used in position calculations */
  HEALTHY = 1 /**< satellite is fully operational */
};

/**
 * Satellite vehicle processing status.
 */
enum class SVStatus {
  UNKNOWN = -1, /**< Unknown sv status*/
  IDLE = 0, /**< SV is not being actively processed  */
  SEARCH = 1, /**< The system is searching for this SV */
  TRACK = 2 /**< SV is being tracked */
};

/**
 * Indicates whether Satellite Vehicle info like ephemeris and
 * almanac are present or not
 */
enum class SVInfoAvailability {
  UNKNOWN = -1, /**< Unknown sv info availability*/
  YES = 0, /**< Ephemeris or Almanac exits  */
  NO = 1 /**< Ephemeris or Almanac doesn't exist */
};

/**
 * Defines which sensors were used in calculating the position in the position
 * report
 */
enum class SensorType {
  UNKNOWN = -1, /**< Unknown sensor type*/
  ACCELEROMETER = 1, /**<  Bitmask to specify whether an accelerometer was used */
  GYROSCOPE = 2 /**<  Bitmask to specify whether a gyroscope was used */
};

/**
 * Specifies which position technology was used.
 */
enum GnssPositionTechType {
  GNSS_DEFAULT = 0,
  GNSS_SATELLITE = (1 << 0),
  GNSS_CELLID = (1 << 1),
  GNSS_WIFI = (1 << 2),
  GNSS_SENSORS = (1 << 3),
  GNSS_REFERENCE_LOCATION = (1 << 4),
  GNSS_INJECTED_COARSE_POSITION= (1 << 5),
  GNSS_AFLT = (1 << 6),
  GNSS_HYBRID = (1 << 7),
  GNSS_PPE = (1 << 8)
};

/*Bit mask containing bits from GnssPositionTechType */
using GnssPositionTech = uint32_t;

/**
 * Specifies related kinematics mask
 */
enum KinematicDataValidityType {
  /** Navigation data has Forward Acceleration  */
  HAS_LONG_ACCEL = (1 << 0),
  /** Navigation data has Sideward Acceleration */
  HAS_LAT_ACCEL = (1 << 1),
  /** Navigation data has Vertical Acceleration */
  HAS_VERT_ACCEL = (1 << 2),
  /** Navigation data has Heading Rate */
  HAS_YAW_RATE = (1 << 3),
  /** Navigation data has Body pitch */
  HAS_PITCH = (1 << 4),
  /** Navigation data has Forward Acceleration  */
  HAS_LONG_ACCEL_UNC = (1 << 5),
  /** Navigation data has Sideward Acceleration */
  HAS_LAT_ACCEL_UNC = (1 << 6),
  /** Navigation data has Vertical Acceleration */
  HAS_VERT_ACCEL_UNC = (1 << 7),
  /** Navigation data has Heading Rate */
  HAS_YAW_RATE_UNC = (1 << 8),
  /** Navigation data has Body pitch */
  HAS_PITCH_UNC = (1 << 9)
};

/*Bit mask containing bits from KinematicDataValidityType */
using KinematicDataValidity = uint32_t;

/**
 * Specifies kinematics related information.
 */
struct GnssKinematicsData {
  /** Contains Body frame LocPosDataMask bits. */
  KinematicDataValidity bodyFrameDataMask;
  /** Forward Acceleration in body frame (m/s2)*/
  float longAccel;
  /** Sideward Acceleration in body frame (m/s2)*/
  float latAccel;
  /** Vertical Acceleration in body frame (m/s2)*/
  float vertAccel;
  /** Heading Rate (Radians/second) */
  float yawRate;
  /** Body pitch (Radians) */
  float pitch;
  /** Uncertainty of Forward Acceleration in body frame */
  float longAccelUnc;
  /** Uncertainty of Side-ward Acceleration in body frame */
  float latAccelUnc;
  /** Uncertainty of Vertical Acceleration in body frame */
  float vertAccelUnc;
  /** Uncertainty of Heading Rate */
  float yawRateUnc;
  /** Uncertainty of Body pitch */
  float pitchUnc;
};

/**
 * Specifies type of system.
 */
enum class GnssSystem {
  /** UNKNOWN satellite. */
  GNSS_LOC_SV_SYSTEM_UNKNOWN = -1,
  /** GPS satellite. */
  GNSS_LOC_SV_SYSTEM_GPS = 1,
  /** GALILEO satellite. */
  GNSS_LOC_SV_SYSTEM_GALILEO = 2,
  /** SBAS satellite. */
  GNSS_LOC_SV_SYSTEM_SBAS = 3,
  /** COMPASS satellite.
  @deprecated constellation type
  is not supported.*/
  GNSS_LOC_SV_SYSTEM_COMPASS = 4,
  /** GLONASS satellite. */
  GNSS_LOC_SV_SYSTEM_GLONASS = 5,
  /** BDS satellite. */
  GNSS_LOC_SV_SYSTEM_BDS = 6,
  /** QZSS satellite. */
  GNSS_LOC_SV_SYSTEM_QZSS = 7,
  /** NAVIC satellite. */
  GNSS_LOC_SV_SYSTEM_NAVIC = 8
};

/**
 * Validity field for different system time.
 */
enum GnssTimeValidityType {
  GNSS_SYSTEM_TIME_WEEK_VALID = (1 << 0),
  GNSS_SYSTEM_TIME_WEEK_MS_VALID = (1 << 1),
  GNSS_SYSTEM_CLK_TIME_BIAS_VALID = (1 << 2),
  GNSS_SYSTEM_CLK_TIME_BIAS_UNC_VALID = (1 << 3),
  GNSS_SYSTEM_REF_FCOUNT_VALID = (1 << 4),
  GNSS_SYSTEM_NUM_CLOCK_RESETS_VALID = (1 << 5)
};

/*Bit mask containing bits from GnssTimeValidityType */
using GnssTimeValidity = uint32_t;

struct TimeInfo {
  /** Validity mask for below fields */
  GnssTimeValidity validityMask;
  /** Extended week number at reference tick.
  Unit: Week.
  Set to 65535 if week number is unknown.
  For GPS:
  Calculated from midnight, Jan. 6, 1980.
  OTA decoded 10 bit GPS week is extended to map between:
  [NV6264 to (NV6264 + 1023)].
  NV6264: Minimum GPS week number configuration.
  Default value of NV6264: 1738
  For BDS:
  Calculated from 00:00:00 on January 1, 2006 of Coordinated Universal Time
  (UTC).
  For GAL:
  Calculated from 00:00 UT on Sunday August 22, 1999
  (midnight between August 21 and August 22).*/
  uint16_t systemWeek;
  /** Time in to the current week at reference tick.
  Unit: Millisecond. Range: 0 to 604799999.
  Check for systemClkTimeUncMs before use */
  uint32_t systemMsec;
  /** System clock time bias (sub-millisecond)
  Units: Millisecond
  Note: System time (TOW Millisecond) = systemMsec - systemClkTimeBias.
  Check for systemClkTimeUncMs before use. */
  float systemClkTimeBias;
  /** Single sided maximum time bias uncertainty
  Units: Millisecond */
  float systemClkTimeUncMs;
  /** FCount (free running HW timer) value. Don't use for relative time purpose
  due to possible discontinuities.
  Unit: Millisecond */
  uint32_t refFCount;
  /** Number of clock resets/discontinuities detected,
  affecting the local hardware counter value. */
  uint32_t numClockResets;
};

/**
 * Validity field for GLONASS time.
 */
enum GlonassTimeValidity {
  GNSS_CLO_DAYS_VALID = (1 << 0),
  GNSS_GLOS_MSEC_VALID = (1 << 1),
  GNSS_GLO_CLK_TIME_BIAS_VALID = (1 << 2),
  GNSS_GLO_CLK_TIME_BIAS_UNC_VALID = (1 << 3),
  GNSS_GLO_REF_FCOUNT_VALID = (1 << 4),
  GNSS_GLO_NUM_CLOCK_RESETS_VALID = (1 << 5),
  GNSS_GLO_FOUR_YEAR_VALID = (1 << 6)
};

/*Bit mask containing bits from GlonassTimeValidity */
using TimeValidity = uint32_t;

struct GlonassTimeInfo {
  /** GLONASS day number in four years. Refer to GLONASS ICD.
  Applicable only for GLONASS and shall be ignored for other constellations.
  If unknown shall be set to 65535 */
  uint16_t gloDays;
  /** Validity mask for below fields */
  TimeValidity validityMask;
  /** GLONASS time of day in Millisecond. Refer to GLONASS ICD.
  Units: Millisecond
  Check for gloClkTimeUncMs before use */
  uint32_t gloMsec;
  /** GLONASS clock time bias (sub-millisecond)
  Units: Millisecond
  Note: GLO time (TOD Millisecond) = gloMsec - gloClkTimeBias.
  Check for gloClkTimeUncMs before use. */
  float gloClkTimeBias;
  /** Single sided maximum time bias uncertainty
  Units: Millisecond */
  float gloClkTimeUncMs;
  /** FCount (free running HW timer) value. Don't use for relative time purpose
  due to possible discontinuities.
  Unit: Millisecond */
  uint32_t refFCount;
  /** Number of clock resets/discontinuities detected,
  affecting the local hardware counter value. */
  uint32_t numClockResets;
  /** GLONASS four year number from 1996. Refer to GLONASS ICD.
  Applicable only for GLONASS and shall be ignored for other constellations.
  If unknown shall be set to 255 */
  uint8_t gloFourYear;
};


union SystemTimeInfo {
  TimeInfo gps;
  TimeInfo gal;
  TimeInfo bds;
  TimeInfo qzss;
  GlonassTimeInfo glo;
};

/** @struct
Time applicability of PVT report
*/
struct SystemTime {
  /** Specifies GNSS system time reported. Mandatory field */
  GnssSystem gnssSystemTimeSrc;
  /** Reporting of GPS system time is recommended.
  If GPS time is unknown & other satellite system time is known,
  it should be reported. Mandatory field
  */
  SystemTimeInfo time;
};

/** GNSS Signal Type and RF Band */
enum GnssSignalType {
  /** GPS L1CA Signal */
  GPS_L1CA = (1<<0),
  /** GPS L1C Signal */
  GPS_L1C = (1<<1),
  /** GPS L2 RF Band */
  GPS_L2 = (1<<2),
  /** GPS L5 RF Band */
  GPS_L5 = (1<<3),
  /** GLONASS G1 (L1OF) RF Band */
  GLONASS_G1 = (1<<4),
  /** GLONASS G2 (L2OF) RF Band */
  GLONASS_G2 = (1<<5),
  /** GALILEO E1 RF Band */
  GALILEO_E1 = (1<<6),
  /** GALILEO E5A RF Band */
  GALILEO_E5A = (1<<7),
  /** GALILEO E5B RF Band */
  GALILIEO_E5B = (1<<8),
  /** BEIDOU B1 RF Band */
  BEIDOU_B1 = (1<<9),
  /** BEIDOU B2 RF Band */
  BEIDOU_B2 = (1<<10),
  /** QZSS L1CA RF Band */
  QZSS_L1CA = (1<<11),
  /** QZSS L1S RF Band */
  QZSS_L1S = (1<<12),
  /** QZSS L2 RF Band */
  QZSS_L2 = (1<<13),
  /** QZSS L5 RF Band */
  QZSS_L5 = (1<<14),
  /** SBAS L1 RF Band */
  SBAS_L1 = (1<<15),
  /** BEIDOU B1I RF Band */
  BEIDOU_B1I = (1<<16),
  /** BEIDOU B1C RF Band */
  BEIDOU_B1C = (1<<17),
  /** BEIDOU B2I RF Band */
  BEIDOU_B2I = (1<<18),
  /** BEIDOU B2AI RF Band */
  BEIDOU_B2AI = (1<<19),
  /** NAVIC L5 RF Band */
  NAVIC_L5 = (1<<20),
  /** BEIDOU B2A_Q RF Band */
  BEIDOU_B2AQ = (1<<21)
};

/*Bit mask containing bits from GnssSignalType */
using GnssSignal = uint32_t;

struct GnssMeasurementInfo {
  /** GnssSignalType mask */
  GnssSignal gnssSignalType;
  /** Specifies GNSS Constellation Type */
  GnssSystem gnssConstellation;
  /**  GNSS SV ID.
  For GPS:      1 to 32
  For GLONASS:  65 to 96. When slot-number to SV ID mapping is unknown, set as
  255.
  For SBAS:     120 to 151
  For QZSS-L1CA:193 to 197
  For BDS:      201 to 237
  For GAL:      301 to 336 */
  uint16_t gnssSvId;
};

/** Specify the set of SVs that are used to calculate
 *  GnssLocation.*/
struct SvUsedInPosition {
    /** Specify the set of SVs from GPS constellation that are used
     *  to compute the position. <br/> Bit 0 to Bit 31 corresponds
     *  to GPS SV id 1 to 32.*/
    uint64_t gps;
    /** Specify the set of SVs from GLONASS constellation that are
     *  used to compute the position.
     *  Bit 0 to Bit 31 corresponds to GLO SV id 65 to 96.*/
    uint64_t glo;
    /** Specify the set of SVs from GALILEO constellation that are
     *  used to compute the position.
     *  Bit 0 to Bit 35 corresponds to GAL SV id 301 to 336.*/
    uint64_t gal;
    /** Specify the set of SVs from BEIDOU constellation that are
     *  used to compute the position.
     *  Bit 0 to Bit 36 corresponds to BDS SV id 201 to 237.*/
    uint64_t bds;
    /** Specify the set of SVs from QZSS constellation that are used
     *  to compute the position.
     *  Bit 0 to Bit 4 corresponds to BDS SV id 193 to 197.*/
    uint64_t qzss;
};

enum LocationTechnologyType {
  /** location was calculated using GNSS */
  LOC_GNSS = (1 << 0),
  /** location was calculated using Cell */
  LOC_CELL = (1 << 1),
  /** location was calculated using WiFi */
  LOC_WIFI = (1 << 2),
  /** location was calculated using Sensors */
  LOC_SENSORS = (1 << 3)
};

/*Bit mask containing bits from LocationTechnologyType */
using LocationTechnology = uint32_t;

enum LocationValidityType {
  /** Location has valid latitude and longitude.*/
    HAS_LAT_LONG_BIT          = (1<<0),
    /** Location has valid altitude.*/
    HAS_ALTITUDE_BIT          = (1<<1),
    /** Location has valid speed.*/
    HAS_SPEED_BIT             = (1<<2),
    /** Location has valid heading.*/
    HAS_HEADING_BIT           = (1<<3),
    /* Location has valid horizontal accuracy. */
    HAS_HORIZONTAL_ACCURACY_BIT = (1<<4),
    /** Location has valid vertical accuracy.*/
    HAS_VERTICAL_ACCURACY_BIT = (1<<5),
    /** Location has valid speed accuracy.*/
    HAS_SPEED_ACCURACY_BIT    = (1<<6),
    /** Location has valid heading accuracy.*/
    HAS_HEADING_ACCURACY_BIT  = (1<<7),
    /** Location has valid timestamp.*/
    HAS_TIMESTAMP_BIT         = (1<<8)
};

/*Bit mask containing bits from LocationValidityType */
using LocationInfoValidity = uint32_t;

/** Gnss Location Information mask flags */
enum LocationInfoExValidityType {
  /** valid altitude mean sea level */
  HAS_ALTITUDE_MEAN_SEA_LEVEL = (1 << 0),
  /** valid pdop, hdop, and vdop */
  HAS_DOP = (1 << 1),
  /** valid magnetic deviation */
  HAS_MAGNETIC_DEVIATION = (1 << 2),
  /** valid horizontal reliability */
  HAS_HOR_RELIABILITY = (1 << 3),
  /** valid vertical reliability */
  HAS_VER_RELIABILITY = (1 << 4),
  /** valid elipsode semi major */
  HAS_HOR_ACCURACY_ELIP_SEMI_MAJOR = (1 << 5),
  /** valid elipsode semi minor */
  HAS_HOR_ACCURACY_ELIP_SEMI_MINOR = (1 << 6),
  /** valid accuracy elipsode azimuth */
  HAS_HOR_ACCURACY_ELIP_AZIMUTH = (1 << 7),
  /** valid gnss sv used in pos data */
  HAS_GNSS_SV_USED_DATA = (1 << 8),
  /** valid navSolutionMask */
  HAS_NAV_SOLUTION_MASK = (1 << 9),
  /** valid LocPosTechMask */
  HAS_POS_TECH_MASK = (1 << 10),
  /** valid LocSvInfoSource */
  HAS_SV_SOURCE_INFO = (1 << 11),
  /** valid position dynamics data */
  HAS_POS_DYNAMICS_DATA = (1 << 12),
  /** valid gdop, tdop */
  HAS_EXT_DOP = (1 << 13),
  /**valid North standard deviation */
  HAS_NORTH_STD_DEV = (1 << 14),
  /** valid East standard deviation*/
  HAS_EAST_STD_DEV = (1 << 15),
  /** valid North Velocity */
  HAS_NORTH_VEL = (1 << 16),
  /** valid East Velocity */
  HAS_EAST_VEL = (1 << 17),
  /** valid Up Velocity */
  HAS_UP_VEL = (1 << 18),
  /** valid North Velocity Uncertainty */
  HAS_NORTH_VEL_UNC = (1 << 19),
  /** valid East Velocity Uncertainty */
  HAS_EAST_VEL_UNC = (1 << 20),
  /** valid Up Velocity Uncertainty */
  HAS_UP_VEL_UNC = (1 << 21),
  /** valid leap_seconds */
  HAS_LEAP_SECONDS = (1 << 22),
  /** valid timeUncMs */
  HAS_TIME_UNC = (1 << 23),
  /** valid number of sv used */
  HAS_NUM_SV_USED_IN_POSITION = (1 << 24),
  /** valid sensor calibrationConfidencePercent */
  HAS_CALIBRATION_CONFIDENCE_PERCENT = (1 << 25),
  /** valid sensor calibrationConfidence */
  HAS_CALIBRATION_STATUS = (1 << 26),
  /** valid output engine type */
  HAS_OUTPUT_ENG_TYPE = (1 << 27),
  /** valid output engine mask */
  HAS_OUTPUT_ENG_MASK = (1 << 28),
  /** valid conformity index */
  HAS_CONFORMITY_INDEX_FIX = (1 << 29)
};

/*Bit mask containing bits from LocationInfoExValidityType */
using LocationInfoExValidity = uint32_t;

enum GnssDataSignalTypes {
  GNSS_DATA_SIGNAL_TYPE_GPS_L1CA = 0,       /**<  GPS L1CA Signal  */
  GNSS_DATA_SIGNAL_TYPE_GPS_L1C = 1,        /**<  GPS L1C Signal  */
  GNSS_DATA_SIGNAL_TYPE_GPS_L2C_L = 2,      /**<  GPS L2C_L RF Band  */
  GNSS_DATA_SIGNAL_TYPE_GPS_L5_Q = 3,       /**<  GPS L5_Q RF Band  */
  GNSS_DATA_SIGNAL_TYPE_GLONASS_G1 = 4,     /**<  GLONASS G1 (L1OF) RF Band  */
  GNSS_DATA_SIGNAL_TYPE_GLONASS_G2 = 5,     /**<  GLONASS G2 (L2OF) RF Band  */
  GNSS_DATA_SIGNAL_TYPE_GALILEO_E1_C = 6,   /**<  GALILEO E1_C RF Band  */
  GNSS_DATA_SIGNAL_TYPE_GALILEO_E5A_Q = 7,  /**<  GALILEO E5A_Q RF Band  */
  GNSS_DATA_SIGNAL_TYPE_GALILEO_E5B_Q = 8,  /**<  GALILEO E5B_Q RF Band  */
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B1_I = 9,    /**<  BEIDOU B1_I RF Band  */
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B1C = 10,    /**<  BEIDOU B1C RF Band  */
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2_I = 11,   /**<  BEIDOU B2_I RF Band  */
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2A_I = 12,  /**<  BEIDOU B2A_I RF Band  */
  GNSS_DATA_SIGNAL_TYPE_QZSS_L1CA = 13,     /**<  QZSS L1CA RF Band  */
  GNSS_DATA_SIGNAL_TYPE_QZSS_L1S = 14,      /**<  QZSS L1S RF Band  */
  GNSS_DATA_SIGNAL_TYPE_QZSS_L2C_L = 15,    /**<  QZSS L2C_L RF Band  */
  GNSS_DATA_SIGNAL_TYPE_QZSS_L5_Q = 16,     /**<  QZSS L5_Q RF Band  */
  GNSS_DATA_SIGNAL_TYPE_SBAS_L1_CA = 17,    /**<  SBAS L1_CA RF Band  */
  GNSS_DATA_SIGNAL_TYPE_NAVIC_L5 = 18,      /**<  NAVIC L5 RF Band */
  GNSS_DATA_SIGNAL_TYPE_BEIDOU_B2A_Q = 19,       /**<  BEIDOU B2A_Q RF Band  */
  GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES = 20 /**< Maximum number of signal types */
};

enum GnssDataValidityType {
  /** Jammer Indicator is available */
  HAS_JAMMER = (1ULL << 0),
  /** AGC is available */
  HAS_AGC = (1ULL << 1)
};

/** Specifies GnssDataValidityType mask */
using GnssDataValidity = uint32_t;

struct GnssData {
  /** bitwise OR of GnssDataValidityType */
  GnssDataValidity gnssDataMask[GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES];
  /** Jammer Indication
   * Each index represents the measurement for the signal type in GnssDataSignalTypes
   */
  double jammerInd[GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES];
  /** Automatic gain control
   * Each index represents the measurement for the signal type in GnssDataSignalTypes
   */
  double agc[GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES];
};

enum DrCalibrationStatusType {
  /** Indicate that roll calibration is needed. Need to take more
   turns on level ground */
  DR_ROLL_CALIBRATION_NEEDED  = (1<<0),
  /** Indicate that pitch calibration is needed. Need to take more
   turns on level ground */
  DR_PITCH_CALIBRATION_NEEDED = (1<<1),
  /** Indicate that yaw calibration is needed. Need to accelerate
   in a straight line  */
  DR_YAW_CALIBRATION_NEEDED   = (1<<2),
  /** Indicate that odo calibration is needed. Need to accelerate
   in a straight line  */
  DR_ODO_CALIBRATION_NEEDED   = (1<<3),
  /** Indicate that gyro calibration is needed. Need to take more
   turns on level ground */
  DR_GYRO_CALIBRATION_NEEDED  = (1<<4)
};

/** Specifies DrCalibrationStatusType mask */
using DrCalibrationStatus = uint32_t;

/** Specifies the type of engine requested for fixes*/
enum LocReqEngineType{
    /** Indicate that the fused/default position is needed to be reported back
    for the tracking sessions. The default position is the propagated/aggregated
    reports from all engines running on the system (e.g.: DR/SPE/PPE) according to
    QTI algorithm.
    */
    LOC_REQ_ENGINE_FUSED_BIT = (1<<0),
    /** Indicate that the unmodified SPE position is needed to be reported back for the
    tracking sessions.
    */
    LOC_REQ_ENGINE_SPE_BIT   = (1<<1),
    /** Indicate that the unmodified PPE position is needed to be reported back for the
    tracking sessions.
    */
    LOC_REQ_ENGINE_PPE_BIT   = (1<<2),
};

/** Specifies LocReqEngineType mask*/
using LocReqEngine = uint16_t;

/** Specifies the type of engine for the reported fixes*/
enum LocationAggregationType {
  /** This is the propagated/aggregated reports from all engines
  running on the system (e.g.: DR/SPE/PPE) according to QTI
  algorithm. */
  LOC_OUTPUT_ENGINE_FUSED = 0,
  /** This fix is the unmodified fix from modem GNSS engine */
  LOC_OUTPUT_ENGINE_SPE   = 1,
  /** This is the unmodified fix from PPP/RTK correction engine */
  LOC_OUTPUT_ENGINE_PPE   = 2
};

/** Specifies the type of engine responsible for fixes when the engine type is fused*/
enum PositioningEngineType{
    STANDARD_POSITIONING_ENGINE = (1 << 0),
    DEAD_RECKONING_ENGINE       = (1 << 1),
    PRECISE_POSITIONING_ENGINE  = (1 << 2)
};

/** Specifies PositioningEngineType mask */
using PositioningEngine = uint32_t;


/**
 * Specify parameters related to enable/disable SVs */
struct SvBlackListInfo {
    /** constellation for the sv  */
    GnssConstellationType constellation;
    /** sv id for the constellation:
     * 0 means blacklist for all SVIds of a given constellation type
     * GLONASS SV id range: 65 to 96
     * QZSS SV id range: 193 to 197
     * BDS SV id range: 201 to 237
     * GAL SV id range: 301 to 336
     * SBAS SV id range: 120 to 158 and 183 to 191
     */
    uint32_t              svId;
};

typedef std::vector<SvBlackListInfo> SvBlackList;

/**
 *  Lever ARM type */
enum LeverArmType {
    /** Lever arm parameters regarding the VRP (Vehicle Reference
     *  Point) w.r.t the origin (at the GNSS Antenna) */
    LEVER_ARM_TYPE_GNSS_TO_VRP = 1,
    /** Lever arm regarding GNSS Antenna w.r.t the origin at the
     *  IMU (inertial measurement unit) for DR (dead reckoning
     *  engine) */
    LEVER_ARM_TYPE_DR_IMU_TO_GNSS = 2,
    /** Lever arm regarding GNSS Antenna w.r.t the origin at the
     *  IMU (inertial measurement unit) for VEPP (vision enhanced
     *  precise positioning engine) */
    LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS = 3,
};

/**
 * Specify parameters related to lever arm */
struct LeverArmParams {
    /** Offset along the vehicle forward axis, in unit of meters */
    float forwardOffset;
    /** Offset along the vehicle starboard axis, in unit of
     *  meters */
    float sidewaysOffset;
    /** Offset along the vehicle up axis, in unit of meters  */
    float upOffset;
};

typedef std::unordered_map<LeverArmType, LeverArmParams> LeverArmConfigInfo;

/** Specify valid fields in
 *  GnssMeasurementsData.*/
enum GnssMeasurementsDataValidityType{
    /** Validity of svId.*/
    SV_ID_BIT                        = (1<<0),
    /** Validity of svType.*/
    SV_TYPE_BIT                      = (1<<1),
    /** Validity of stateMask.*/
    STATE_BIT                        = (1<<2),
    /** Validity of receivedSvTimeNs.*/
    RECEIVED_SV_TIME_BIT             = (1<<3),
    /** Validity of receivedSvTimeUncertaintyNs.*/
    RECEIVED_SV_TIME_UNCERTAINTY_BIT = (1<<4),
    /** Validity of carrierToNoiseDbHz.*/
    CARRIER_TO_NOISE_BIT             = (1<<5),
    /** Validity of pseudorangeRateMps.*/
    PSEUDORANGE_RATE_BIT             = (1<<6),
    /** Validity of pseudorangeRateUncertaintyMps.*/
    PSEUDORANGE_RATE_UNCERTAINTY_BIT = (1<<7),
    /** Validity of adrStateMask.*/
    ADR_STATE_BIT                    = (1<<8),
    /** Validity of adrMeters.*/
    ADR_BIT                          = (1<<9),
    /** Validity of adrUncertaintyMeters.*/
    ADR_UNCERTAINTY_BIT              = (1<<10),
    /** Validity of carrierFrequencyHz.*/
    CARRIER_FREQUENCY_BIT            = (1<<11),
    /** Validity of carrierCycles.*/
    CARRIER_CYCLES_BIT               = (1<<12),
    /** Validity of carrierPhase.*/
    CARRIER_PHASE_BIT                = (1<<13),
    /** Validity of carrierPhaseUncertainty.*/
    CARRIER_PHASE_UNCERTAINTY_BIT    = (1<<14),
    /** Validity of multipathIndicator.*/
    MULTIPATH_INDICATOR_BIT          = (1<<15),
    /** Validity of signalToNoiseRatioDb.*/
    SIGNAL_TO_NOISE_RATIO_BIT        = (1<<16),
    /** Validity of agcLevelDb.*/
    AUTOMATIC_GAIN_CONTROL_BIT       = (1<<17)
};

/** Specifies GnssMeasurementsDataValidityType.*/
using GnssMeasurementsDataValidity = uint32_t;

/** Specify GNSS measurement state in
 *  GnssMeasurementsData::stateMask.*/
enum GnssMeasurementsStateValidityType {
    /** State is unknown.*/
    UNKNOWN_BIT                 = 0,
    /** State is "code lock".*/
    CODE_LOCK_BIT               = (1<<0),
    /** State is "bit sync".*/
    BIT_SYNC_BIT                = (1<<1),
    /** State is "subframe sync".*/
    SUBFRAME_SYNC_BIT           = (1<<2),
    /** State is "tow decoded".*/
    TOW_DECODED_BIT             = (1<<3),
    /** State is "msec ambiguous".*/
    MSEC_AMBIGUOUS_BIT          = (1<<4),
    /** State is "symbol sync".*/
    SYMBOL_SYNC_BIT             = (1<<5),
    /** State is "GLONASS string sync".*/
    GLO_STRING_SYNC_BIT         = (1<<6),
    /** State is "GLONASS TOD decoded".*/
    GLO_TOD_DECODED_BIT         = (1<<7),
    /** State is "BDS D2 bit sync".*/
    BDS_D2_BIT_SYNC_BIT         = (1<<8),
    /** State is "BDS D2 subframe sync".*/
    BDS_D2_SUBFRAME_SYNC_BIT    = (1<<9),
    /** State is "Galileo E1BC code lock".*/
    GAL_E1BC_CODE_LOCK_BIT      = (1<<10),
    /** State is "Galileo E1C second code lock".*/
    GAL_E1C_2ND_CODE_LOCK_BIT   = (1<<11),
    /** State is "Galileo E1B page sync".*/
    GAL_E1B_PAGE_SYNC_BIT       = (1<<12),
    /** State is "SBAS sync".*/
    SBAS_SYNC_BIT               = (1<<13)
};

/** Specifies GnssMeasurementsStateValidityType.*/
using GnssMeasurementsStateValidity = uint32_t;

/** Specify accumulated delta range state in
 *  GnssMeasurementsData::adrStateMask.*/
enum GnssMeasurementsAdrStateValidityType {
    /** State is unknown.*/
    UNKNOWN_STATE   = 0,
    /** State is valid.*/
    VALID_BIT       = (1<<0),
    /** State is "reset".*/
    RESET_BIT       = (1<<1),
    /** State is "cycle slip".*/
    CYCLE_SLIP_BIT  = (1<<2)
};

/** Specifies GnssMeasurementsAdrStateValidityType.*/
using GnssMeasurementsAdrStateValidity = uint32_t;

/** Specify the GNSS multipath indicator state in
 *  GnssMeasurementsData::multipathIndicator.*/
enum GnssMeasurementsMultipathIndicator {
    /** Multipath indicator is unknown.*/
    UNKNOWN_INDICATOR     = 0,
    /** Multipath indicator is present.*/
    PRESENT               = 1,
    /** Multipath indicator is not present.*/
    NOT_PRESENT           = 2
};

/** Specify the valid fields in
 *  GnssMeasurementsClock.
 */
enum GnssMeasurementsClockValidityType {
    /** Validity of leapSecond.*/
    LEAP_SECOND_BIT                   = (1<<0),
    /** Validity of timeNs.*/
    TIME_BIT                          = (1<<1),
    /** Validity of timeUncertaintyNs.*/
    TIME_UNCERTAINTY_BIT              = (1<<2),
    /** Validity of fullBiasNs.*/
    FULL_BIAS_BIT                     = (1<<3),
    /** Validity of biasNs.*/
    BIAS_BIT                          = (1<<4),
    /** Validity of biasUncertaintyNs.*/
    BIAS_UNCERTAINTY_BIT              = (1<<5),
    /** Validity of driftNsps.*/
    DRIFT_BIT                         = (1<<6),
    /** Validity of driftUncertaintyNsps.*/
    DRIFT_UNCERTAINTY_BIT             = (1<<7),
    /** Validity of hwClockDiscontinuityCount.*/
    HW_CLOCK_DISCONTINUITY_COUNT_BIT  = (1<<8)
};

/** Specifies GnssMeasurementsClockValidityType.*/
using GnssMeasurementsClockValidity = uint32_t;

/** Specify the signal measurement information such as satellite vehicle pseudo range,
 *  satellite vehicle time, carrier phase measurement etc. from GNSS positioning engine.
 */
struct GnssMeasurementsData {
    /** Bitwise OR of GnssMeasurementsDataValidityType to specify the
     *  valid fields in GnssMeasurementsData. */
    GnssMeasurementsDataValidity valid;
    /** Specify satellite vehicle ID number.*/
    int16_t svId;
    /** SV constellation type.*/
    GnssConstellationType svType;
    /** Time offset when the measurement was taken,
     *  in unit of nanoseconds.*/
    double timeOffsetNs;
    /** Bitwise OR of GnssMeasurementsStateValidityType to specify the
     *  GNSS measurement state.*/
    GnssMeasurementsStateValidity stateMask;
    /** Received GNSS time of the week in nanoseconds when the
     *  measurement was taken.*/
    int64_t receivedSvTimeNs;
    /** Satellite time.
     *  All SV times in the current measurement block are already
     *  propagated to a common reference time epoch, in unit of
     *  nano seconds.*/
    int64_t receivedSvTimeUncertaintyNs;
    /** Signal strength, carrier to noise ratio, in unit of dB-Hz.*/
    double carrierToNoiseDbHz;
    /** Uncorrected pseudorange rate, in unit of metres/second.*/
    double pseudorangeRateMps;
    /** Uncorrected pseudorange rate uncertainty, in unit of
     *  meters/second.*/
    double pseudorangeRateUncertaintyMps;
    /** Bitwise OR of GnssMeasurementsAdrStateValidityType.*/
    GnssMeasurementsAdrStateValidity adrStateMask;
    /** Accumulated delta range, in unit of meters.*/
    double adrMeters;
    /** Accumulated delta range uncertainty, in unit of meters.*/
    double adrUncertaintyMeters;
    /** Carrier frequency of the tracked signal, in unit of Hertz.*/
    float carrierFrequencyHz;
    /** The number of full carrier cycles between the receiver and
     *  the satellite.*/
    int64_t carrierCycles;
    /** The RF carrier phase that the receiver has detected.*/
    double carrierPhase;
    /** The RF carrier phase uncertainty.*/
    double carrierPhaseUncertainty;
    /** Multipath indicator, could be unknown, present or not
     *  present.*/
    GnssMeasurementsMultipathIndicator multipathIndicator;
    /** Signal to noise ratio, in unit of dB.*/
    double signalToNoiseRatioDb;
    /** Automatic gain control level, in unit of dB.*/
    double agcLevelDb;
};

/** Specify GNSS measurements clock.
 *  The main equation describing the relationship between
 *  various components is:
 *  utcTimeNs = timeNs - (fullBiasNs + biasNs) - leapSecond *
 *  1,000,000,000*/
struct GnssMeasurementsClock {
    /** Bitwise OR of GnssMeasurementsClockValidityType.*/
    GnssMeasurementsClockValidity valid;
    /** Leap second, in unit of seconds.*/
    int16_t leapSecond;
    /** Time, monotonically increasing as long as the power is on,
     *  in unit of nanoseconds.*/
    int64_t timeNs;
    /** Time uncertainty (one sigma), in unit of nanoseconds.*/
    double timeUncertaintyNs;
    /** Full bias, in uint of nanoseconds.*/
    int64_t fullBiasNs;
    /** Sub-nanoseconds bias, in unit of nonoseconds.*/
    double biasNs;
    /** Bias uncertainty (one sigma), in unit of nanoseconds.*/
    double biasUncertaintyNs;
    /** Clock drift, in unit of nanoseconds/second.*/
    double driftNsps;
    /** Clock drift uncertainty (one sigma), in unit of
     *  nanoseconds/second.*/
    double driftUncertaintyNsps;
    /** HW clock discontinuity count - incremented
     *  for each discontinuity in HW clock.*/
    uint32_t hwClockDiscontinuityCount;
};

/** Specify GNSS measurements clock and data.
 *  GnssMeasurementInfo is used to convey the satellite vehicle info whose measurements are
 *  actually used to generate the current position report. While GnssMeasurements contains the
 *  satellite measurements that device observed during tracking session, regardless the measurement
 *  is used or not used to compute the fix. Furthermore GnssMeasurements contains much richer set
 *  of information which can enable other third party engines to utilize the measurements and
 *  compute the position by itself.
 */
struct GnssMeasurements {
    /** GNSS measurements clock info.*/
    GnssMeasurementsClock clock;
    /** GNSS measurements data.*/
    std::vector<GnssMeasurementsData> measurements;
};

struct LeapSecondChangeInfo {
    /** GPS timestamp that corrresponds to the last known leap
        second change event.
        The info can be available on two scenario:
        1: This leap second change event has been scheduled and yet
           to happen
        2: This leap second change event has already happened and
           next leap second change event has not yet been
           scheduled. */
    TimeInfo timeInfo;
    /** Number of leap seconds prior to the leap second change event
      that corresponds to the timestamp at timeInfo. */
    uint8_t leapSecondsBeforeChange;
    /** Number of leap seconds after the leap second change event
      that corresponds to the timestamp at timeInfo. */
    uint8_t leapSecondsAfterChange;
};

/** Specify the valid fields in LeapSecondInfo*/
enum LeapSecondInfoValidityType{
    /** Current leap second info is available. This info will only
      be available if the leap second change info is not available
      If leap second change info is avaiable, to figure out the
      current leap second info, compare current gps time with the
      gps timestamp of leap second change to know whether to choose
      leapSecondBefore or leapSecondAfter as current leap second. */
    LEAP_SECOND_SYS_INFO_CURRENT_LEAP_SECONDS_BIT = (1ULL << 0),
    /** The last known leap change event is available.
        The info can be available on two scenario:
        1: This leap second change event has been scheduled and yet
           to happen
        2: This leap second change event has already happened and
           next leap second change event has not yet been scheduled.
    */
    LEAP_SECOND_SYS_INFO_LEAP_SECOND_CHANGE_BIT = (1ULL << 1)
};

/** Specifies LeapSecondInfoValidityType mask */
using LeapSecondInfoValidity = uint32_t;

struct LeapSecondInfo {
    LeapSecondInfoValidity valid;

/** Current leap seconds, in unit of seconds.
 *  This info will only be available only if the leap second change info
 *  is not available.
 */
    uint8_t               current;
    LeapSecondChangeInfo  info;
};

/** Specify the set of valid fields in LocationSystemInfo*/
enum LocationSystemInfoValidityType{
    /** contains current leap second or leap second change info */
    LOCATION_SYS_INFO_LEAP_SECOND = (1ULL << 0),
};

/** Specifies LocationSystemInfoValidityType mask */
using LocationSystemInfoValidity = uint32_t;

struct LocationSystemInfo {
    LocationSystemInfoValidity valid;
    LeapSecondInfo   info;
};

/**
 *  Specify the valid fields in GnssEnergyConsumedInfo. */
enum GnssEnergyConsumedInfoValidityType {
    /** validity of GnssEnergyConsumedInfo*/
    ENERGY_CONSUMED_SINCE_FIRST_BOOT_BIT = (1<<0)
};

/** Specifies GnssEnergyConsumedInfoValidityType */
using GnssEnergyConsumedInfoValidity = uint16_t;

/** Specify the info regarding energy consumed by GNSS
 *  engine.*/
struct GnssEnergyConsumedInfo {
    /** Bitwise OR of GnssEnergyConsumedInfoValidityType to
     *  specify the valid fields in GnssEnergyConsumedInfo.*/
    GnssEnergyConsumedInfoValidity valid;

    /** Energy consumed by the modem GNSS engine since device first
     *  ever bootup, in unit of 0.1 milli watt seconds.*/
    uint64_t energySinceFirstBoot;
};

/**
 *  Specifies the set of aiding data. This is referenced in the
 *  deleteAidingData for deleting any aiding data. */
enum AidingDataType {
    /** Mask to delete ephemeris aiding data */
    AIDING_DATA_EPHEMERIS  = (1 << 0)
};

/** Specifies AidingDataType mask */
using AidingData = uint32_t;

/**
 * @brief IGpsTime provides interface to get current GPS week and elapsed
 *        time in current GPS week
 */
class IGpsTime {
public:
/**
 * Retrieves current GPS week as calculated from midnight, Jan 6, 1980.
 *
 * @returns Unsigned 32-bit integer containing week number.
 *
 */
  virtual uint32_t getWeek() = 0;

/**
 * Retrieves elapsed time in the current GPS week starting from 12:00 am on
 * Sunday.
 *
 * @returns Unsigned 32-bit integer containing time in milliseconds.
 *
 */
  virtual uint32_t getTimeOfWeekMsec() = 0;
};

/**
 * @brief Specifies the sensors used for calculating the fixes
 * and the type of measurements which were aided by sensor data.
 *
 */
class ISensorDataUsage {
public:
/**
 * Retrieves which sensors were used in calculating the position in the
 * position report.
 *
 * @returns @ref SensorType if available.
 *
 */
  virtual SensorType getSensorType() = 0;

/**
 * Retrieves which measurements were aided by sensor data.
 *
 * @returns Measurement types if available.
 *
 */
  virtual Measurement getMeasurement() = 0;
};

/**
 * @brief ILocationInfo provides interface to get basic position related
 * information like latitude, longitude, altitude, timestamp and other information
 * like time stamp, session status,
 *
 */
class ILocationInfo {
public:
/**
 * Retrieves technology used in computing this fix.
 *
 * @returns Position technology.
 *
 */
  virtual PositionTech getPositionTechnology() = 0;

/**
 * Retrieves latitude.
 * Positive and negative values indicate northern and southern latitude
 * respectively
 *    - Units: Degrees
 *    - Range: -90.0 to 90.0
 *
 * @returns Latitude if available else returns NaN.
 *
 */
  virtual double getLatitude() = 0;

/**
 * Retrieves longitude.
 * Positive and negative values indicate eastern and western longitude
 * respectively
 *    - Units: Degrees
 *    - Range: -180.0 to 180.0
 *
 * @returns Longitude if available else returns NaN.
 *
 */
  virtual double getLongitude() = 0;

/**
 * Retrieves altitude above the WGS 84 reference ellipsoid.
 *    - Units: Meters
 *
 * @returns Altitude if available else returns NaN.
 *
 */
  virtual double getAltitude() = 0;

/**
 * Retrieves heading.
 *    - Units: Degrees
 *    - Range: 0 to 359.999
 *
 * @returns Heading if available else returns NaN.
 *
 */
  virtual float getHeading() = 0;

/**
 * Retrieves the vertical uncertainty.
 *    - Units: Meters
 *
 * @returns Vertical uncertainty if available else returns NaN.
 *
 */
  virtual float getVerticalUncertainty() = 0;

/**
 * Retrieves UTC timeInfo for the location fix.
 *    - Units: Milliseconds since Jan 1, 1970
 *
 * @returns TimeStamp in milliseconds if available else returns UNKNOWN_TIMESTAMP,
 * which is zero(as UTC timeStamp has elapsed since January 1, 1970, it cannot be 0)
 *
 */
  virtual uint64_t getTimeStamp() = 0;

/**
 * Retrieves 3-D speed uncertainty.
 *    - Units: Meters per Second
 *
 * @returns Speed uncertainty if available else returns NaN.
 *
 */
  virtual float getSpeedUncertainty() = 0;

/**
 * Retrieves heading uncertainty.
 *    - Units: Degrees
 *    - Range: 0 to 359.999
 *
 * @returns Heading uncertainty if available else returns NaN.
 *
 */
  virtual float getHeadingUncertainty() = 0;

/**
 * Retrieves the altitude with respect to mean sea level.
 *    - Units: Meters
 *
 * @returns Altitude with respect to mean sea level if available else returns
 * NaN.
 *
 */
  virtual float getAltitudeMeanSeaLevel() = 0;

/**
 * Retrieves position dilution of precision.
 *
 * @returns Position dilution of precision if available else returns NaN.
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getPositionDop() = 0;

/**
 * Retrieves horizontal dilution of precision.
 *
 * @returns Horizontal dilution of precision if available else returns NaN.
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getHorizontalDop() = 0;

/**
 * Retrieves vertical dilution of precision.
 *
 * @returns Vertical dilution of precision if available else returns NaN
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
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
 */
  virtual float getMagneticDeviation() = 0;

/**
 * Specifies the reliability of the horizontal position.
 *
 * @returns @ref LocationReliability of the horizontal position if available
 * else returns UNKNOWN.
 *
 */
  virtual LocationReliability getHorizontalReliability() = 0;

/**
 * Specifies the reliability of the vertical position.
 *
 * @returns @ref LocationReliability of the vertical position if available
 * else returns UNKNOWN.
 *
 */
  virtual LocationReliability getVerticalReliability() = 0;

/**
 * Retrieves semi-major axis of horizontal elliptical uncertainty.
 *    - Units: Meters
 *
 * @returns Semi-major horizontal elliptical uncertainty if available else
 * returns NaN.
 *
 */
  virtual float getHorizontalUncertaintySemiMajor() = 0;

/**
 * Retrieves semi-minor axis of horizontal elliptical uncertainty.
 *    - Units: Meters
 *
 * @returns Semi-minor horizontal elliptical uncertainty
 * if available else returns NaN.
 *
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
 */
  virtual float getHorizontalUncertaintyAzimuth() = 0;

/**
 * Retrieves GNSS Satellite Vehicles used in position data.
 *
 * @param [out] idsOfUsedSVs Vector of Satellite Vehicle identifiers.
 *
 */
  virtual void getSVIds(std::vector<uint16_t> &idsOfUsedSVs) = 0;

/**
 * Retrieves navigation solution mask used to indicate SBAS corrections.
 *
 * @return - SBAS (Satellite Based Augmentation System) Correction mask used.
 *
 */
  virtual SbasCorrection getSbasCorrection() = 0;

/**
 * Retrieves status of the session that is requested by user application.
 *
 * @returns @ref SessionStatus
 *
 */
  virtual SessionStatus getSessionStatus() = 0;

/**
 * Retrieves leap seconds if available.
 *
 * @param [out] leapSeconds - leap seconds
 *       - Units: Seconds
 *
 * @returns Status of leap seconds.
 *
 */
  virtual telux::common::Status getLeapSeconds(uint8_t &leapSeconds) = 0;

/**
 * Retrieves GPS time structure.
 *
 * @returns Pointer of IGpsTime object if available else returns null pointer.
 *
 */
  virtual std::shared_ptr<IGpsTime> getGpsTime() = 0;

/**
 * Retrieves horizontal position uncertainty (circular) if available.
 *
 * @param [out] circularHorizontalUncertainty - circular horizontal
 * uncertainty
 *       - Units: Meters
 *
 * @returns Status of circular horizontal uncertainty.
 *
 */
  virtual telux::common::Status getCircularHorizontalUncertainty(
      float &circularHorizontalUncertainty) = 0;

/**
 * Retrieves horizontal uncertainty confidence if available.
 *
 * @param [out] horizontalConfidence - horizontal uncertainty confidence
 *       - Units: Percent
 *       - Range: 0 to 99
 *
 * @returns Status of horizontal uncertainty confidence.
 *
 */
  virtual telux::common::Status
      getHorizontalConfidence(uint8_t &horizontalConfidence) = 0;

/**
 * Retrieves horizontal speed.
 *       - Units: Meters/second
 *
 * @returns horizontal speed if available else returns NaN.
 *
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
 */
  virtual telux::common::Status
      getVerticalConfidence(uint8_t &verticalConfidence) = 0;

/**
 * Retrieves vertical speed.
 *       - Units: Meters/second
 *
 * @returns Float containing vertical speed if available else returns NaN.
 *
 */
  virtual float getVerticalSpeed() = 0;

/**
 * Retrieves sensor data was used in computing the position if available.
 *
 * @param [out] sensorDataUsage - which sensors were used in calculating the
 * position
 *
 * @returns Status of availability of sensorDataUsage.
 *
 */
  virtual telux::common::Status getSensorDataUsage(
      std::shared_ptr<ISensorDataUsage> &sensorDataUsage) = 0;

/**
 * Retrieves fix count if available. Fix count of a session starts with 0 and
 * increments by one for each successive position report for a particular
 * session.
 *
 * @param [out] fixId - identifier of fix for session
 *
 * @returns Status of availability of fix identifier.
 *
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
 */
  virtual telux::common::Status
      getVelocityEastNorthUp(std::vector<float> &velocityEastNorthUp) = 0;

/**
 * Retrieves east, North, Up velocity uncertainty if available.
 *
 * @param [out] velocityUncertaintyEastNorthUp - east, North, Up velocity
 * uncertainty
 *         Units: Meters/second
 *
 * @returns Status of availability of east, North, Up velocity uncertainty.
 *
 */
  virtual telux::common::Status getVelocityUncertaintyEastNorthUp(
      std::vector<float> &velocityUncertaintyEastNorthUp) = 0;
};

/**
 * @brief ILocationInfoBase provides interface to get basic position related
 * information like latitude, longitude, altitude, timestamp.
 *
 */
class ILocationInfoBase {
public:
/**
 * Retrieves the validity of the Location basic Info.
 *
 * @returns Location basic validity mask.
 *
 */
  virtual LocationInfoValidity getLocationInfoValidity() = 0;

/**
 * Retrieves technology used in computing this fix.
 *
 * @returns Location technology mask.
 *
 */
  virtual LocationTechnology getTechMask() = 0;

/**
 * Retrieves Speed.
 *
 * @returns speed in meters per second.
 *
 */
  virtual float getSpeed() = 0;
/**
 * Retrieves latitude.
 * Positive and negative values indicate northern and southern latitude
 * respectively
 *    - Units: Degrees
 *    - Range: -90.0 to 90.0
 *
 * @returns Latitude if available else returns NaN.
 *
 */
  virtual double getLatitude() = 0;

/**
 * Retrieves longitude.
 * Positive and negative values indicate eastern and western longitude
 * respectively
 *    - Units: Degrees
 *    - Range: -180.0 to 180.0
 *
 * @returns Longitude if available else returns NaN.
 *
 */
  virtual double getLongitude() = 0;

/**
 * Retrieves altitude above the WGS 84 reference ellipsoid.
 *    - Units: Meters
 *
 * @returns Altitude if available else returns NaN.
 *
 */
  virtual double getAltitude() = 0;

/**
 * Retrieves heading/bearing.
 *    - Units: Degrees
 *    - Range: 0 to 359.999
 *
 * @returns Heading if available else returns NaN.
 *
 */
  virtual float getHeading() = 0;

/**
 * Retrieves the horizontal uncertainty.
 *
 * @returns Horizontal uncertainty.
 *
 */
  virtual float getHorizontalUncertainty() = 0;
/**
 * Retrieves the vertical uncertainty.
 *    - Units: Meters
 *
 * @returns Vertical uncertainty if available else returns NaN.
 *
 */
  virtual float getVerticalUncertainty() = 0;

/**
 * Retrieves UTC timeInfo for the location fix.
 *    - Units: Milliseconds since Jan 1, 1970
 *
 * @returns TimeStamp in milliseconds if available else returns UNKNOWN_TIMESTAMP
 * which is zero(as UTC timeStamp has elapsed since January 1, 1970, it cannot be 0)
 *
 */
  virtual uint64_t getTimeStamp() = 0;

/**
 * Retrieves 3-D speed uncertainty/accuracy.
 *    - Units: Meters per Second
 *
 * @returns Speed uncertainty if available else returns NaN.
 *
 */
  virtual float getSpeedUncertainty() = 0;

/**
 * Retrieves heading uncertainty.
 *    - Units: Degrees
 *    - Range: 0 to 359.999
 *
 * @returns Heading uncertainty if available else returns NaN.
 *
 */
  virtual float getHeadingUncertainty() = 0;

};

/**
 * @brief ILocationInfoEx provides interface to get richer position related
 * information like latitude, longitude, altitude and other information like time stamp,
 * session status, dop, reliabilities, uncertainities etc.
 *
 */
class ILocationInfoEx : public ILocationInfoBase {
public:

/**
 * Retrives the validity of the location info ex. It provides the validity of various information
 * like dop, reliabilities, uncertainities etc.
 *
 * @returns Location ex validity mask
 */
  virtual LocationInfoExValidity getLocationInfoExValidity() = 0;

/**
 * Retrieves the altitude with respect to mean sea level.
 *    - Units: Meters
 *
 * @returns Altitude with respect to mean sea level if available else returns
 * NaN.
 *
 */
  virtual float getAltitudeMeanSeaLevel() = 0;

/**
 * Retrieves position dilution of precision.
 *
 * @returns Position dilution of precision if available else returns NaN.
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getPositionDop() = 0;

/**
 * Retrieves horizontal dilution of precision.
 *
 * @returns Horizontal dilution of precision if available else returns NaN.
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getHorizontalDop() = 0;

/**
 * Retrieves vertical dilution of precision.
 *
 * @returns Vertical dilution of precision if available else returns NaN
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
 */
  virtual float getVerticalDop() = 0;
/**
 * Retrieves geometric dilution of precision.
 *
 * @returns geometric dilution of precision.
 *
 */
  virtual float getGeometricDop() = 0;
/**
 * Retrieves time dilution of precision.
 *
 * @returns Time dilution of precision.
 *
 */
  virtual float getTimeDop() = 0;

/**
 * Retrieves the difference between the bearing to true north and the bearing
 * shown on magnetic compass. The deviation is positive when the magnetic
 * north is east of true north.
 *    - Units: Degrees
 *
 * @returns Magnetic Deviation if available else returns NaN
 *
 */
  virtual float getMagneticDeviation() = 0;

/**
 * Specifies the reliability of the horizontal position.
 *
 * @returns @ref LocationReliability of the horizontal position if available
 * else returns
 * UNKNOWN.
 *
 */
  virtual LocationReliability getHorizontalReliability() = 0;

/**
 * Specifies the reliability of the vertical position.
 *
 * @returns @ref LocationReliability of the vertical position if available
 * else returns UNKNOWN.
 *
 */
  virtual LocationReliability getVerticalReliability() = 0;

/**
 * Retrieves semi-major axis of horizontal elliptical uncertainty.
 *    - Units: Meters
 *
 * @returns Semi-major horizontal elliptical uncertainty if available else
 * returns NaN.
 *
 */
  virtual float getHorizontalUncertaintySemiMajor() = 0;

/**
 * Retrieves semi-minor axis of horizontal elliptical uncertainty.
 *    - Units: Meters
 *
 * @returns Semi-minor horizontal elliptical uncertainty
 * if available else returns NaN.
 *
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
 */
  virtual float getHorizontalUncertaintyAzimuth() = 0;
/**
 * Retrieves east standard deviation.
 *    - Units: Meters
 *
 * @returns East Standard Deviation.
 *
 */
  virtual float getEastStandardDeviation() = 0;

/**
 * Retrieves north standard deviation.
 *    - Units: Meters
 *
 * @returns North Standard Deviation.
 *
 */
  virtual float getNorthStandardDeviation() = 0;

/**
 * Retrieves number of satellite vehicles used in position report.
 *
 * @returns number of Sv used.
 *
 */
  virtual uint16_t getNumSvUsed() = 0;

/**
 * Retrives the set of satellite vehicles that are used to calculate position.
 *
 * @returns set of satellite vehicles for different constellations.
 */
  virtual SvUsedInPosition getSvUsedInPosition() = 0;

/**
 * Retrieves GNSS Satellite Vehicles used in position data.
 *
 * @param [out] idsOfUsedSVs Vector of Satellite Vehicle identifiers.
 *
 */
  virtual void getSVIds(std::vector<uint16_t> &idsOfUsedSVs) = 0;

/**
 * Retrieves navigation solution mask used to indicate SBAS corrections.
 *
 * @return - SBAS (Satellite Based Augmentation System) Correction mask used.
 *
 */
  virtual SbasCorrection getSbasCorrection() = 0;

/**
 * Retrieves position technology mask used to indicate which technology is used.
 *
 * @return - Position technology used in computing this fix.
 *
 */
  virtual GnssPositionTech getPositionTechnology() = 0;

/**
 * Retrieves position related information.
 *
 */
  virtual GnssKinematicsData getBodyFrameData() = 0;

/**
 * Retrieves gnss measurement usage info.
 *
 */
  virtual std::vector<GnssMeasurementInfo> getmeasUsageInfo() = 0;

/**
 * Retrieves type of gnss system.
 *
 * @return - Type of Gnss System.
 *
 */
  virtual SystemTime getGnssSystemTime() = 0;

/**
 * Retrieves time uncertainity.
 *
 * @return - Time uncertainty in milliseconds.
 *
 */
  virtual float getTimeUncMs() = 0;

/**
 * Retrieves leap seconds if available.
 *
 * @param [out] leapSeconds - leap seconds
 *       - Units: Seconds
 *
 * @returns Status of leap seconds.
 *
 */
  virtual telux::common::Status getLeapSeconds(uint8_t &leapSeconds) = 0;

/**
 * Retrieves east, North, Up velocity if available.
 *
 * @param [out] velocityEastNorthUp - east, North, Up velocity
 *       - Units: Meters/second
 *
 * @returns Status of availability of east, North, Up velocity.
 *
 */
  virtual telux::common::Status
      getVelocityEastNorthUp(std::vector<float> &velocityEastNorthUp) = 0;

/**
 * Retrieves east, North, Up velocity uncertainty if available.
 *
 * @param [out] velocityUncertaintyEastNorthUp - east, North, Up velocity
 * uncertainty
 *       - Units: Meters/second
 *
 * @returns Status of availability of east, North, Up velocity uncertainty.
 *
 */
  virtual telux::common::Status getVelocityUncertaintyEastNorthUp(
      std::vector<float> &velocityUncertaintyEastNorthUp) = 0;

/**
 * Sensor calibration confidence percent, range [0, 100].
 *
 * @returns the percentage of calibration taking all the parameters into account.
 *
 */
  virtual uint8_t getCalibrationConfidencePercent() = 0;

/**
 * Sensor calibration status.
 *
 * @returns mask indicating the calibration status with respect to different parameters.
 *
 */
  virtual DrCalibrationStatus getCalibrationStatus() = 0;

/**
 * Location engine type. When the type is set to LOC_ENGINE_SRC_FUSED, the fix is
 * the propagated/aggregated reports from all engines running on the system (e.g.:
 * DR/SPE/PPE) based QTI algorithm. To check which location engine contributes
 * to the fused output, check for locOutputEngMask.
 *
 * @returns the type of engine that was used for calculating the position fix.
 *
 */
  virtual LocationAggregationType getLocOutputEngType() = 0;

/**
 * When loc output eng type is set to fused, this field indicates the set of engines
 * contribute to the fix.
 *
 * @returns the combination of position engines used in calculating the position report
 * when the loc output end type is set to fused.
 *
 */
  virtual PositioningEngine getLocOutputEngMask() = 0;

/**
 * When robust location is enabled, this field will indicate how well the various input
 * data considered for navigation solution conforms to expectations.
 *
 * @returns values in the range [0.0, 1.0], with 0.0 for least conforming and 1.0 for
 * most conforming.
 *
 */
  virtual float getConformityIndex() = 0;
};

/**
 * @brief ISVInfo provides interface to retrieve information
 *        about Satellite Vehicles, their position and health status
 */
class ISVInfo {
public:
/**
 * Indicates to which constellation this satellite vehicle belongs.
 *
 * @returns  @ref GnssConstellationType if available else returns UNKNOWN.
 *
 */
  virtual GnssConstellationType getConstellation() = 0;

/**
 * GNSS satellite vehicle ID.
 *
 * @returns Identifier of the satellite vehicle otherwise 0(as 0 is not an ID
 * for any of the SVs)
 *
 */
  virtual uint16_t getId() = 0;

/**
 * Health status of satellite vehicle.
 *
 * @returns  HealthStatus of Satellite Vehicle if available else returns
 * UNKNOWN.
 *          - @ref SVHealthStatus
 *
 */
  virtual SVHealthStatus getSVHealthStatus() = 0;

/**
 * Status of satellite vehicle.
 *
 * @note    This API is work-in-progress and is subject to change.
 * @returns Satellite Vehicle Status if available else returns UNKNOWN.
 *          - @ref SVStatus
 *
 */
  virtual SVStatus getStatus() = 0;

/**
 * Indicates whether ephemeris information(which allows the receiver
 * to calculate the satellite's position) is available.
 *
 * @returns @ref SVInfoAvailability if Ephemeris exists or not else returns
 * UNKNOWN.
 *
 */
  virtual SVInfoAvailability getHasEphemeris() = 0;

/**
 * Indicates whether almanac information(which allows receivers to know
 * which satellites are available for tracking) is available.
 *
 * @returns @ref SVInfoAvailability if almanac exists or not else returns
 * UNKNOWN.
 *
 */
  virtual SVInfoAvailability getHasAlmanac() = 0;

/**
 * Indicates whether the satellite is used in computing the fix.
 *
 * @returns @ref SVInfoAvailability, if satellite used or not else returns
 * UNKNOWN.
 *
 */
  virtual SVInfoAvailability getHasFix() = 0;

/**
 * Retrieves satellite vehicle elevation angle.
 *    - Units: Degrees
 *    - Range: 0 to 90
 *
 * @returns Elevation if available else returns NaN.
 *
 */
  virtual float getElevation() = 0;

/**
 * Retrieves satellite vehicle azimuth angle.
 *    - Units: Degrees
 *    - Range: 0 to 360
 *
 * @returns Azimuth if available else returns NaN.
 */
  virtual float getAzimuth() = 0;

/**
 * Retrieves satellite vehicle signal-to-noise ratio.
 *    - Units: dB-Hz
 *
 * @returns SNR if available else returns NaN.
 *
 */
  virtual float getSnr() = 0;

/**
 * Indicates the carrier frequency of the signal tracked.
 *
 * @returns carrier frequency in Hz else returns UNKNOWN_CARRIER_FREQ frequency
 * when not supported.
 */
  virtual float getCarrierFrequency() = 0;

/**
 * Indicates the validity for different types of signal
 * for gps, galileo, beidou etc.
 *
 * @returns signalType mask else return UNKNOWN_SIGNAL_MASK when not supported.
 */
  virtual GnssSignal getSignalType() = 0;
};

/**
 * @brief IGnssSVInfo provides interface to retrieve the list of SV info
 * available and whether altitude is assumed or calculated.
 */
class IGnssSVInfo {
public:
/**
 * Indicates whether altitude is assumed or calculated.
 *
 * @returns @ref AltitudeType if available else returns UNKNOWN.
 *
 */
  virtual AltitudeType getAltitudeType() = 0;

/**
 * Pointer to satellite vehicles information for all GNSS
 * constellations except GPS.
 *
 * @returns Vector of pointer of ISVInfo object if available else returns
 * empty vector.
 *
 */
  virtual std::vector<std::shared_ptr<ISVInfo> > getSVInfoList() = 0;
};

/**
 * @brief IGnssSignalInfo provides interface to retrieve GNSS data information
 * like jammer metrics and automatic gain control for satellite signal type.
 */
class IGnssSignalInfo {
public:
/**
 * Retrieves jammer metric and Automatic Gain Control(AGC) corresponding to signal types.Jammer metric is
 * linearly proportional to the sum of jammer and noise power at the GNSS
 * antenna port.
 *
 * @returns List of jammer metric and a list of automatic gain control for signal type.
 *
 */

  virtual GnssData getGnssData() = 0;
};

/** @} */ /* end_addtogroup telematics_location */

} // end of namespace loc
} // end of namespace telux

#endif // LOCATION_DEFINES_HPP
