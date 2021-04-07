/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#ifndef LOCATIONDEFINESSTUB_HPP
#define LOCATIONDEFINESSTUB_HPP

#include <cmath>
#include "telux/loc/LocationDefines.hpp"
#include "Logger/Logger.hpp"
namespace telux {
namespace loc {

class LocationInfoBase : public ILocationInfoBase {
    uint32_t locationInfoValidity_ = 0;
    uint32_t locationTechnology_ = 0;
    float speed_ = NAN;
    double latitude_ = NAN;
    double longitude_ = NAN;
    double altitude_ = NAN;
    float heading_ = NAN;
    float horizontalUncertainty_ = NAN;
    float verticalUncertainty_ = NAN;
    uint64_t timeStamp_ = UNKNOWN_TIMESTAMP;
    float speedUncertainty_ = NAN;
    float headingUncertainty_ = NAN;

public:
/**
 * Retrieves the validity of the Location basic Info.
 *
 * returns Location basic validity mask.
 *
 */
    LocationInfoValidity getLocationInfoValidity() override {return locationInfoValidity_;}

/**
 * Retrieves technology used in computing this fix.
 *
 * returns Location technology mask.
 *
 */
    LocationTechnology getTechMask() override {return locationTechnology_;}

/**
 * Retrieves Speed.
 *
 * returns speed in meters per second.
 *
 */
    float getSpeed() override {return speed_;}
/**
 * Retrieves latitude.
 * Positive and negative values indicate northern and southern latitude
 * respectively
 *    - Units: Degrees
 *    - Range: -90.0 to 90.0
 *
 * returns Latitude if available else returns NaN.
 *
 */
    double getLatitude() override {return latitude_;}

/**
 * Retrieves longitude.
 * Positive and negative values indicate eastern and western longitude
 * respectively
 *    - Units: Degrees
 *    - Range: -180.0 to 180.0
 *
 * returns Longitude if available else returns NaN.
 *
 */
    double getLongitude() override { return longitude_;}

/**
 * Retrieves altitude above the WGS 84 reference ellipsoid.
 *    - Units: Meters
 *
 * returns Altitude if available else returns NaN.
 *
 */
    double getAltitude() override { return altitude_;}
/**
 * Retrieves heading/bearing.
 *    - Units: Degrees
 *    - Range: 0 to 359.999
 *
 * returns Heading if available else returns NaN.
 *
 */
    float getHeading() override {return heading_;}

/**
 * Retrieves the horizontal uncertainty.
 *
 * returns Horizontal uncertainty.
 *
 */
    float getHorizontalUncertainty() override{return horizontalUncertainty_;}
/**
 * Retrieves the vertical uncertainty.
 *    - Units: Meters
 *
 * returns Vertical uncertainty if available else returns NaN.
 *
 */
    float getVerticalUncertainty() override {return verticalUncertainty_;}

/**
 * Retrieves UTC timeInfo for the location fix.
 *    - Units: Milliseconds since Jan 1, 1970
 *
 * returns TimeStamp in milliseconds if available else returns UNKNOWN_TIMESTAMP
 * which is zero(as UTC timeStamp has elapsed since January 1, 1970, it cannot be 0)
 *
 */
    uint64_t getTimeStamp() override{return timeStamp_;}

/**
 * Retrieves 3-D speed uncertainty/accuracy.
 *    - Units: Meters per Second
 *
 * returns Speed uncertainty if available else returns NaN.
 *
 */
    float getSpeedUncertainty() override{return speedUncertainty_;}

/**
 * Retrieves heading uncertainty.
 *    - Units: Degrees
 *    - Range: 0 to 359.999
 *
 * returns Heading uncertainty if available else returns NaN.
 *
 */
    float getHeadingUncertainty() override{return headingUncertainty_;}



    void setLocationInfoValidity(uint32_t value) {locationInfoValidity_ = value;}
    void setLocationTechnology(uint32_t value) {locationTechnology_ = value;}
    void setSpeed(float val) { speed_ = val;}
    void setLatitude(double val) { latitude_ = val;}
    void setLongitude(double val) { longitude_ = val;}
    void setAltitude(double val) { altitude_ = val;}
    void setHeading(float val) { heading_ = val;}
    void setHorizontalUncertainty(float val) { horizontalUncertainty_ = val;}
    void setVerticalUncertainty(float val) { verticalUncertainty_ = val;}
    void setUtcFixTime(uint64_t value) {timeStamp_ = value;}
    void setSpeedUncertainty(float val) { speedUncertainty_ = val;}
    void setHeadingUncertainty(float val) { headingUncertainty_ = val;}

};

class LocationInfoEx : public ILocationInfoEx {
    uint32_t locationInfoValidity_ = 0;
    uint32_t locationTechnology_ = 0;
    float speed_ = NAN;
    double latitude_ = NAN;
    double longitude_ = NAN;
    double altitude_ = NAN;
    float heading_ = NAN;
    float horizontalUncertainty_ = NAN;
    float verticalUncertainty_ = NAN;
    uint64_t timeStamp_ = UNKNOWN_TIMESTAMP;
    float speedUncertainty_ = NAN;
    float headingUncertainty_ = NAN;

    uint32_t locationInfoExValidity_ = 0;
    float altitudeMeanSeaLevel_ = NAN;
    float positionDop_ = NAN;
    float horizontalDop_ = NAN;
    float verticalDop_ = NAN;
    float geometricDop_ = 0;
    float timeDop_ = 0;
    float magneticDeviation_ = NAN;
    LocationReliability horizontalReliability_ = LocationReliability::UNKNOWN;
    LocationReliability verticalReliability_ = LocationReliability::UNKNOWN;
    float horizontalUncertaintySemiMajor_ = NAN;
    float horizontalUncertaintySemiMinor_ = NAN;
    float horizontalUncertaintyAzimuth_ = NAN;
    float eastStandardDeviation_ = 0;
    float northStandardDeviation_ = 0;
    uint16_t numSvUsed_ = 0;
    SvUsedInPosition svUsedInPosition_;
    std::vector<uint16_t> usedSVsIds_;
    SbasCorrection sbasCorrection_;
    uint32_t positionTechnology_ = 0;
    GnssKinematicsData bodyFrameData_;
    std::vector<GnssMeasurementInfo> measUsageInfo_;
    SystemTime gnssSystemTime_;
    float timeUncMs_ = 0;
    uint8_t leapSeconds_ = 0;
    std::vector<float> velocityEastNorthUp_;
    std::vector<float> velocityUncertaintyEastNorthUp_;
    uint8_t calibrationConfidencePercent_ = 0;
    uint32_t calibrationStatus_ = 0;
    LocationAggregationType locOutputEngType_ = LocationAggregationType::LOC_OUTPUT_ENGINE_FUSED;
    uint32_t locOutputEngMask_ = 0;
    float conformityIndex_ = 0;
    AltitudeType altitudeType_ = AltitudeType::UNKNOWN;
    ReportStatus reportStatus_ = ReportStatus::UNKNOWN;

public:
/**
 * Retrieves the validity of the Location basic Info.
 *
 * returns Location basic validity mask.
 *
 */
    LocationInfoValidity getLocationInfoValidity() override {return locationInfoValidity_;}

/**
 * Retrieves technology used in computing this fix.
 *
 * returns Location technology mask.
 *
 */
    LocationTechnology getTechMask() override {return locationTechnology_;}

/**
 * Retrieves Speed.
 *
 * returns speed in meters per second.
 *
 */
    float getSpeed() override {return speed_;}
/**
 * Retrieves latitude.
 * Positive and negative values indicate northern and southern latitude
 * respectively
 *    - Units: Degrees
 *    - Range: -90.0 to 90.0
 *
 * returns Latitude if available else returns NaN.
 *
 */
    double getLatitude() override {return latitude_;}

/**
 * Retrieves longitude.
 * Positive and negative values indicate eastern and western longitude
 * respectively
 *    - Units: Degrees
 *    - Range: -180.0 to 180.0
 *
 * returns Longitude if available else returns NaN.
 *
 */
    double getLongitude() override { return longitude_;}

/**
 * Retrieves altitude above the WGS 84 reference ellipsoid.
 *    - Units: Meters
 *
 * returns Altitude if available else returns NaN.
 *
 */
    double getAltitude() override { return altitude_;}
/**
 * Retrieves heading/bearing.
 *    - Units: Degrees
 *    - Range: 0 to 359.999
 *
 * returns Heading if available else returns NaN.
 *
 */
    float getHeading() override {return heading_;}

/**
 * Retrieves the horizontal uncertainty.
 *
 * returns Horizontal uncertainty.
 *
 */
    float getHorizontalUncertainty() override{return horizontalUncertainty_;}
/**
 * Retrieves the vertical uncertainty.
 *    - Units: Meters
 *
 * returns Vertical uncertainty if available else returns NaN.
 *
 */
    float getVerticalUncertainty() override {return verticalUncertainty_;}

/**
 * Retrieves UTC timeInfo for the location fix.
 *    - Units: Milliseconds since Jan 1, 1970
 *
 * returns TimeStamp in milliseconds if available else returns UNKNOWN_TIMESTAMP
 * which is zero(as UTC timeStamp has elapsed since January 1, 1970, it cannot be 0)
 *
 */
    uint64_t getTimeStamp() override{return timeStamp_;}

/**
 * Retrieves 3-D speed uncertainty/accuracy.
 *    - Units: Meters per Second
 *
 * returns Speed uncertainty if available else returns NaN.
 *
 */
    float getSpeedUncertainty() override{return speedUncertainty_;}

/**
 * Retrieves heading uncertainty.
 *    - Units: Degrees
 *    - Range: 0 to 359.999
 *
 * returns Heading uncertainty if available else returns NaN.
 *
 */
    float getHeadingUncertainty() override{return headingUncertainty_;}


/**
 * Retrives the validity of the location info ex. It provides the validity of various information
 * like dop, reliabilities, uncertainities etc.
 *
 * returns Location ex validity mask
 */
    LocationInfoExValidity getLocationInfoExValidity() override { return locationInfoExValidity_;}

/**
 * Retrieves the altitude with respect to mean sea level.
 *    - Units: Meters
 *
 * returns Altitude with respect to mean sea level if available else returns
 * NaN.
 *
 */
    float getAltitudeMeanSeaLevel() override { return altitudeMeanSeaLevel_;}

/**
 * Retrieves position dilution of precision.
 *
 * returns Position dilution of precision if available else returns NaN.
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
 */
    float getPositionDop() override { return positionDop_;}

/**
 * Retrieves horizontal dilution of precision.
 *
 * returns Horizontal dilution of precision if available else returns NaN.
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
 */
    float getHorizontalDop() override { return horizontalDop_;}

/**
 * Retrieves vertical dilution of precision.
 *
 * returns Vertical dilution of precision if available else returns NaN
 * Range: 1 (highest accuracy) to 50 (lowest accuracy)
 *
 */
    float getVerticalDop() override { return verticalDop_;}

/**
 * Retrieves geometric dilution of precision.
 *
 * returns geometric dilution of precision.
 *
 */
    float getGeometricDop() override { return geometricDop_;}
/**
 * Retrieves time dilution of precision.
 *
 * returns Time dilution of precision.
 *
 */
    float getTimeDop() override { return timeDop_;}

/**
 * Retrieves the difference between the bearing to true north and the bearing
 * shown on magnetic compass. The deviation is positive when the magnetic
 * north is east of true north.
 *    - Units: Degrees
 *
 * returns Magnetic Deviation if available else returns NaN
 *
 */
    float getMagneticDeviation() override { return magneticDeviation_;}

/**
 * Specifies the reliability of the horizontal position.
 *
 * returns ref LocationReliability of the horizontal position if available
 * else returns
 * UNKNOWN.
 *
 */
    LocationReliability getHorizontalReliability() override { return horizontalReliability_;}

/**
 * Specifies the reliability of the vertical position.
 *
 * returns ref LocationReliability of the vertical position if available
 * else returns UNKNOWN.
 *
 */
    LocationReliability getVerticalReliability() override { return verticalReliability_;}

/**
 * Retrieves semi-major axis of horizontal elliptical uncertainty.
 *    - Units: Meters
 *
 * returns Semi-major horizontal elliptical uncertainty if available else
 * returns NaN.
 *
 */
    float getHorizontalUncertaintySemiMajor() override { return horizontalUncertaintySemiMajor_;}

/**
 * Retrieves semi-minor axis of horizontal elliptical uncertainty.
 *    - Units: Meters
 *
 * returns Semi-minor horizontal elliptical uncertainty
 * if available else returns NaN.
 *
 */
    float getHorizontalUncertaintySemiMinor() override { return horizontalUncertaintySemiMinor_;}

/**
 * Retrieves elliptical horizontal uncertainty azimuth of orientation.
 *    - Units: Decimal degrees
 *    - Range: 0 to 180
 *
 * returns Elliptical horizontal uncertainty azimuth of orientation
 * if available else returns NaN.
 *
 */
    float getHorizontalUncertaintyAzimuth() override { return horizontalUncertaintyAzimuth_;}
/**
 * Retrieves east standard deviation.
 *    - Units: Meters
 *
 * returns East Standard Deviation.
 *
 */
    float getEastStandardDeviation() override { return eastStandardDeviation_;}

/**
 * Retrieves north standard deviation.
 *    - Units: Meters
 *
 * returns North Standard Deviation.
 *
 */
    float getNorthStandardDeviation() override { return northStandardDeviation_;}


/**
 * Retrieves number of satellite vehicles used in position report.
 *
 * returns number of Sv used.
 *
 */
    uint16_t getNumSvUsed() override { return numSvUsed_;}

/**
 * Retrives the set of satellite vehicles that are used to calculate position.
 *
 * returns set of satellite vehicles for different constellations.
 */
    SvUsedInPosition getSvUsedInPosition() override { return svUsedInPosition_;}

/**
 * Retrieves GNSS Satellite Vehicles used in position data.
 *
 * param [out] idsOfUsedSVs Vector of Satellite Vehicle identifiers.
 *
 */
    void getSVIds(std::vector<uint16_t> &idsOfUsedSVs) override {
         for (uint16_t num : usedSVsIds_) idsOfUsedSVs.push_back(num);

    }

/**
 * Retrieves navigation solution mask used to indicate SBAS corrections.
 *
 * return - SBAS (Satellite Based Augmentation System) Correction mask used.
 *
 */
    SbasCorrection getSbasCorrection() override { return sbasCorrection_;}

/**
 * Retrieves position technology mask used to indicate which technology is used.
 *
 * return - Position technology used in computing this fix.
 *
 */
    GnssPositionTech getPositionTechnology() override { return positionTechnology_;}

/**
 * Retrieves position related information.
 *
 */
    GnssKinematicsData getBodyFrameData() override { return bodyFrameData_;}

/**
 * Retrieves gnss measurement usage info.
 *
 */
    std::vector<GnssMeasurementInfo> getmeasUsageInfo() override { return measUsageInfo_;}

/**
 * Retrieves type of gnss system.
 *
 * return - Type of Gnss System.
 *
 */
    SystemTime getGnssSystemTime() override { return gnssSystemTime_;}

/**
 * Retrieves time uncertainity.
 *
 * return - Time uncertainty in milliseconds.
 *
 */
    float getTimeUncMs() override { return timeUncMs_;}

/**
 * Retrieves leap seconds if available.
 *
 * param [out] leapSeconds - leap seconds
 *       - Units: Seconds
 *
 * returns Status of leap seconds.
 *
 */
    telux::common::Status getLeapSeconds(uint8_t &leapSeconds) override {
      leapSeconds = leapSeconds_;
      return telux::common::Status::SUCCESS;
  }

/**
 * Retrieves east, North, Up velocity if available.
 *
 * param [out] velocityEastNorthUp - east, North, Up velocity
 *       - Units: Meters/second
 *
 * returns Status of availability of east, North, Up velocity.
 *
 */
    telux::common::Status
        getVelocityEastNorthUp(std::vector<float> &velocityEastNorthUp) override {
        if(velocityEastNorthUp_.size() > 0) {
            for(float val : velocityEastNorthUp_)
                velocityEastNorthUp.push_back(val);
            return telux::common::Status::SUCCESS;
        }
        return telux::common::Status::FAILED;
    }

/**
 * Retrieves east, North, Up velocity uncertainty if available.
 *
 * param [out] velocityUncertaintyEastNorthUp - east, North, Up velocity
 * uncertainty
 *       - Units: Meters/second
 *
 * returns Status of availability of east, North, Up velocity uncertainty.
 *
 */
    telux::common::Status getVelocityUncertaintyEastNorthUp(
        std::vector<float> &velocityUncertaintyEastNorthUp) override {
        if(velocityUncertaintyEastNorthUp_.size() > 0) {
            for(float val : velocityUncertaintyEastNorthUp_)
                velocityUncertaintyEastNorthUp.push_back(val);
            return telux::common::Status::SUCCESS;
        }
        return telux::common::Status::FAILED;
    }

/**
 * Sensor calibration confidence percent, range [0, 100].
 *
 * returns the percentage of calibration taking all the parameters into account.
 *
 */
    uint8_t getCalibrationConfidencePercent() override { return calibrationConfidencePercent_;}

/**
 * Sensor calibration status.
 *
 * returns mask indicating the calibration status with respect to different parameters.
 *
 */
    DrCalibrationStatus getCalibrationStatus() override { return calibrationStatus_;}

/**
 * Location engine type. When the type is set to LOC_ENGINE_SRC_FUSED, the fix is
 * the propagated/aggregated reports from all engines running on the system (e.g.:
 * DR/SPE/PPE) based QTI algorithm. To check which location engine contributes
 * to the fused output, check for locOutputEngMask.
 *
 * returns the type of engine that was used for calculating the position fix.
 *
 */
    LocationAggregationType getLocOutputEngType() override { return locOutputEngType_;}

/**
 * When loc output eng type is set to fused, this field indicates the set of engines
 * contribute to the fix.
 *
 * returns the combination of position engines used in calculating the position report
 * when the loc output end type is set to fused.
 *
 */
    PositioningEngine getLocOutputEngMask() override { return locOutputEngMask_;}

/**
 * When robust location is enabled, this field will indicate how well the various input
 * data considered for navigation solution conforms to expectations.
 *
 * returns values in the range [0.0, 1.0], with 0.0 for least conforming and 1.0 for
 * most conforming.
 *
 */
    float getConformityIndex() override { return conformityIndex_;}

/**
 * Vehicle Reference Point(VRP) based latitude, longitude and altitude information.
 *
 */
    LLAInfo getVRPBasedLLA() {
        LLAInfo llaInfo;
        llaInfo.latitude = NAN;
        llaInfo.longitude = NAN;
        llaInfo.altitude = NAN;
        return llaInfo;
    }

/**
 * VRP-based east, north and up velocity information.
 * returns - vector of directional velocities in this order {east velocity, north velocity,
 *            up velocity}
 */
    virtual std::vector<float> getVRPBasedENUVelocity() {
        std::vector<float> vel;
        vel.push_back(NAN);
        vel.push_back(NAN);
        vel.push_back(NAN);
        return vel;
    }

/**
 * Determination of altitude is assumed or calculated. ASSUMED means there may not be
 * enough satellites to determine the precise altitude.
 * returns altitude type ASSUMED/CALCULATED or if not avalilable then UNKNOWN.
 */
  virtual AltitudeType getAltitudeType() { return altitudeType_; }

/**
 * Indicates the status of this report in terms of how optimally the report was calculated
 * by the engine.
 *
 * returns Status of the report. Returns ReportStatus::UNKNOWN if status is unavailable.
 */
  virtual ReportStatus getReportStatus() { return reportStatus_; }

    void setLocationInfoValidity(uint32_t value) {locationInfoValidity_ = value;}
    void setLocationTechnology(uint32_t value) {locationTechnology_ = value;}
    void setSpeed(float val) { speed_ = val;}
    void setLatitude(double val) { latitude_ = val;}
    void setLongitude(double val) { longitude_ = val;}
    void setAltitude(double val) { altitude_ = val;}
    void setHeading(float val) { heading_ = val;}
    void setHorizontalUncertainty(float val) { horizontalUncertainty_ = val;}
    void setVerticalUncertainty(float val) { verticalUncertainty_ = val;}
    void setUtcFixTime(uint64_t value) {timeStamp_ = value;}
    void setSpeedUncertainty(float val) { speedUncertainty_ = val;}
    void setHeadingUncertainty(float val) { headingUncertainty_ = val;}

    void setLocationInfoExValidity(uint32_t val) { locationInfoExValidity_ = val;}
    void setAltitudeMeanSeaLevel(float val) { altitudeMeanSeaLevel_ = val;}
    void setPositionDop(float val) { positionDop_ = val;}
    void setHorizontalDop(float val) { horizontalDop_ = val;}
    void setVerticalDop(float val) { verticalDop_ = val;}
    void setGeometricDop(float val) { geometricDop_ = val;}
    void setTimeDop(float val) { timeDop_ = val;}
    void setMagneticDeviation(float val) { magneticDeviation_ = val;}
    void setHorizontalReliability(LocationReliability val) { horizontalReliability_ = val;}
    void setVerticalReliability(LocationReliability val) { verticalReliability_ = val;}
    void setHorizontalUncertaintySemiMajor(float val) { horizontalUncertaintySemiMajor_ = val;}
    void setHorizontalUncertaintySemiMinor(float val) { horizontalUncertaintySemiMinor_ = val;}
    void setHorizontalUncertaintyAzimuth(float val) { horizontalUncertaintyAzimuth_ = val;}
    void setEastStandardDeviation(float val) { eastStandardDeviation_ = val;}
    void setNorthStandardDeviation(float val) { northStandardDeviation_ = val;}
    void setNumSvUsed(uint16_t val) { numSvUsed_ = val;}
    void setSvUsedInPosition(SvUsedInPosition &val) { svUsedInPosition_ = val;}
    void setUsedSVsIds(std::vector<uint16_t> &val) {
        for (uint16_t num : val) {
            usedSVsIds_.push_back(num);
        }
    }
    void setSbasCorrection(SbasCorrection &val) { sbasCorrection_ = val;}
    void setPositionTechnology(uint32_t val) { positionTechnology_ = val;}
    void setBodyFrameData(GnssKinematicsData &val) { bodyFrameData_ = val;}
    void setMeasUsageInfo(std::vector<GnssMeasurementInfo> &val) {
        for(GnssMeasurementInfo info : val) {
            measUsageInfo_.push_back(info);
        }
    }
    void setGnssSystemTime(SystemTime &val){ gnssSystemTime_ = val;}
    void setTimeUncMs(float val){ timeUncMs_ = val;}
    void setLeapSeconds(uint8_t val){ leapSeconds_ = val;}
    void setVelocityEastNorthUp(std::vector<float> &val){
        for(float num : val) {
            velocityEastNorthUp_.push_back(num);
        }
    }
    void setVelocityUncertaintyEastNorthUp(std::vector<float> &val){
        for(float num : val) {
            velocityUncertaintyEastNorthUp_.push_back(num);
        }
    }
    void setCalibrationConfidencePercent(uint8_t val) { calibrationConfidencePercent_ = val;}
    void setCalibrationStatus(uint32_t val) { calibrationStatus_ = val;}
    void setLocOutputEngType(LocationAggregationType val) { locOutputEngType_ = val;}
    void setLocOutputEngMask(uint32_t val) { locOutputEngMask_ = val;}
    void setConformityIndex(float val) { conformityIndex_ = val;}
};


class SVInfo : public ISVInfo {
    GnssConstellationType constellation_ = GnssConstellationType::UNKNOWN;
    uint16_t id_ = 0;
    SVHealthStatus healthStatus_ = SVHealthStatus::UNKNOWN;
    SVStatus status_ = SVStatus::UNKNOWN;
    SVInfoAvailability isEphemerisAvailable_ = SVInfoAvailability::UNKNOWN;
    SVInfoAvailability isAlmanacAvailable_ = SVInfoAvailability::UNKNOWN;
    SVInfoAvailability isFixUsed_ = SVInfoAvailability::UNKNOWN;
    float elevation_ = NAN;
    float azimuth_ = NAN;
    float snr_ = NAN;
    float carrierFrequencyHz_ = UNKNOWN_CARRIER_FREQ;
    GnssSignal signalType_ = UNKNOWN_SIGNAL_MASK;
    uint16_t glonassFcn_ = 0;

public:
/**
 * Indicates to which constellation this satellite vehicle belongs.
 *
 * returns  ref GnssConstellationType if available else returns UNKNOWN.
 *
 */
    GnssConstellationType getConstellation(){return constellation_;}

/**
 * GNSS satellite vehicle ID.
 *
 * returns Identifier of the satellite vehicle otherwise 0(as 0 is not an ID
 * for any of the SVs)
 *
 */
    uint16_t getId() {return id_;}

/**
 * Health status of satellite vehicle.
 *
 * returns  HealthStatus of Satellite Vehicle if available else returns
 * UNKNOWN.
 *          - ref SVHealthStatus
 *
 */
    SVHealthStatus getSVHealthStatus() {return healthStatus_;}

/**
 * Status of satellite vehicle.
 *
 * note    This API is work-in-progress and is subject to change.
 * returns Satellite Vehicle Status if available else returns UNKNOWN.
 *          - ref SVStatus
 *
 */
    SVStatus getStatus() {return status_;}

/**
 * Indicates whether ephemeris information(which allows the receiver
 * to calculate the satellite's position) is available.
 *
 * returns ref SVInfoAvailability if Ephemeris exists or not else returns
 * UNKNOWN.
 *
 */
    SVInfoAvailability getHasEphemeris() { return isEphemerisAvailable_;}

/**
 * Indicates whether almanac information(which allows receivers to know
 * which satellites are available for tracking) is available.
 *
 * returns ref SVInfoAvailability if almanac exists or not else returns
 * UNKNOWN.
 *
 */
    SVInfoAvailability getHasAlmanac() {return isAlmanacAvailable_;}

/**
 * Indicates whether the satellite is used in computing the fix.
 *
 * returns ref SVInfoAvailability, if satellite used or not else returns
 * UNKNOWN.
 *
 */
    SVInfoAvailability getHasFix() {return isFixUsed_;}

/**
 * Retrieves satellite vehicle elevation angle.
 *    - Units: Degrees
 *    - Range: 0 to 90
 *
 * returns Elevation if available else returns NaN.
 *
 */
    float getElevation() {return elevation_;}

/**
 * Retrieves satellite vehicle azimuth angle.
 *    - Units: Degrees
 *    - Range: 0 to 360
 *
 * returns Azimuth if available else returns NaN.
 */
    float getAzimuth() {return azimuth_;}

/**
 * Retrieves satellite vehicle signal-to-noise ratio.
 *    - Units: dB-Hz
 *
 * returns SNR if available else returns NaN.
 *
 */
    float getSnr() {return snr_;}

/**
 * Indicates the carrier frequency of the signal tracked.
 *
 * returns carrier frequency in Hz else returns UNKNOWN_CARRIER_FREQ frequency
 * when not supported.
 */
    float getCarrierFrequency() {return carrierFrequencyHz_;}

/**
 * Indicates the validity for different types of signal
 * for gps, galileo, beidou etc.
 *
 * returns signalType mask else return UNKNOWN_SIGNAL_MASK when not supported.
 */
    GnssSignal getSignalType() {return signalType_;}

 /**
  * Retrieves GLONASS frequency channel number in the range [1, 14].
  *
  * @returns GLONASS frequency channel number.
  */
    uint16_t getGlonassFcn() {return glonassFcn_;}


    void setConstellation(GnssConstellationType val) {constellation_ = val;}
    void setId(uint16_t val) {id_ = val;}
    void setSVHealthStatus(SVHealthStatus val) {healthStatus_ = val;}
    void setStatus(SVStatus &val) {status_ = val;}
    void setHasEphemeris(SVInfoAvailability val) {isEphemerisAvailable_ = val;}
    void setHasAlmanac(SVInfoAvailability val) {isAlmanacAvailable_ = val;}
    void setHasFix(SVInfoAvailability val) {isFixUsed_ = val;}
    void setElevation(float val) {elevation_ = val;}
    void setAzimuth(float val) {azimuth_ = val;}
    void setSnr(float val) {snr_ = val;}
    void setCarrierFrequency(float val) {carrierFrequencyHz_ = val;}
    void setSignalType(GnssSignalType val) {signalType_ |= val;}
    void setGlonassFcn(uint16_t val) {glonassFcn_ = val;}
};

class GnssSVInfo : public IGnssSVInfo{
    AltitudeType altitudeType = AltitudeType::UNKNOWN;
    std::vector<std::shared_ptr<ISVInfo>> svInfoList;

public:
/**
 * Indicates whether altitude is assumed or calculated.
 *
 * returns ref AltitudeType if available else returns UNKNOWN.
 *
 */
    AltitudeType getAltitudeType() {return altitudeType;}

/**
 * Pointer to satellite vehicles information for all GNSS
 * constellations except GPS.
 *
 * returns Vector of pointer of ISVInfo object if available else returns
 * empty vector.
 *
 */
    std::vector<std::shared_ptr<ISVInfo>> getSVInfoList() {return svInfoList;}

    void setAltitudeType(AltitudeType val) {altitudeType = val;}
    void setSVInfoList(std::vector<std::shared_ptr<ISVInfo>> &val) {
        for(std::shared_ptr<ISVInfo> svInfo : val)
            svInfoList.push_back(svInfo);
    }
};

class GnssSignalInfo : public IGnssSignalInfo {
    GnssData gnssData;

public:
/**
 * Retrieves jammer metric and Automatic Gain Control(AGC) corresponding to signal types.Jammer metric is
 * linearly proportional to the sum of jammer and noise power at the GNSS
 * antenna port.
 *
 * returns List of jammer metric and a list of automatic gain control for signal type.
 *
 */
    GnssData getGnssData() {return gnssData;};

    void setGnssData(GnssData &val) {
        auto maxGnssSignalTypes =
            telux::loc::GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES;
        for (auto i = 0; i< maxGnssSignalTypes; i++) {
            gnssData.gnssDataMask[i]= val.gnssDataMask[i];
            gnssData.jammerInd[i] = val.jammerInd[i];
            gnssData.agc[i] = val.agc[i];
        }
    }

};

}
}
#endif
