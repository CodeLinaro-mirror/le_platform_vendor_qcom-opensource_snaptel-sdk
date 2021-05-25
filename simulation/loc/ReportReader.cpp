/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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

#include "ReportReader.hpp"
#include "StubHelper.hpp"
#include <time.h>
#include <cerrno>
#include "Logger/Logger.hpp"

// Number of lines in the Linux foundation license text.
#define LICENSE_TEXT_NUM_LINES 28

namespace telux {

namespace loc {


ReportReader::ReportReader() {
    std::string str;
    int simulType = false;

    auto &stubReport = StubHelper::getInstance();
    str = stubReport.configValue("REPORT_SIMULATION_TYPE");
    simulType = (str.size())? (std::stoi(str)) : 0;
    //initialize data to default values(0/NaN)
    reportDataInit();
    if(simulType == 1) {
        std::string strval;
        std::string strval1;
        strval = stubReport.configValue("REPORT_FILE_NAME");
        in.open(strval.c_str(),std::ifstream::in);
        if (!in.is_open()) {
            //Resolving Issue of File Not opening  with strval
            //(Maybe special character LF/CR getting embedded)
            strval1 = strval.substr(0, strval.size()-1);
            in.open(strval1.c_str(),std::ifstream::in);
        }
        if(in.is_open()) {
            std::string my_line;
            Debug(__func__, "File Opened");
            // Dummy reading of the license text and the Header lines.
            for (int i = 0; i < (LICENSE_TEXT_NUM_LINES + 2); i++) {
                std::getline(in, my_line);
            }
        } else { // File Not available, simulation defaults to 0 or Not Valid values
            std::cout << "File : " << strval1 << " opening error" << std::endl;
            Error(__func__, "File : " + strval1 + " opening error");
        }
    } else { // either 0 or default
        //intialize report with Canned values
        reportCannedDataInit();
    }
}

ReportReader::~ReportReader() {
    if(in.is_open()) {
        in.close();
    }
}

ReportReader& ReportReader::getInstance() {
    static ReportReader MyReportReader;
    return(MyReportReader);
}

std::shared_ptr<GnssSignalInfo> ReportReader::getGnssSignalInfo() {
    return gnssSignalInfo_;
}

std::shared_ptr<GnssSVInfo> ReportReader::getGnssSVInfo() {
    return gnssSVInfo_;
}
void ReportReader::getLocationInfoBase(std::shared_ptr<LocationInfoBase> & ibase,
        std::shared_ptr<LocationInfoEx> & ibaseEx) {
    if (in.is_open()) {
        readInfoBaseExData();
    }
    ibase = iBase_;
    ibaseEx = iBaseEx_;
}


std::vector<NMEAVals>& ReportReader::getNmeaVal() {
    return(defaultNmeaVals_);
}

struct GnssMeasurements& ReportReader::getGnssMeasurements() {
    return gnssMeasurements_;
}

struct LocationSystemInfo& ReportReader::getSystemInfoReport() {
    return locationSystemInfo_;
}

void ReportReader::readInfoBaseExData() {
    if (!in.eof()) {
        uint32_t infoValidity = 0;
        uint32_t infoExValidity = 0;
        uint32_t bodyFrameValidity = 0;
        struct GnssKinematicsData bodyFrameData;
        std::string my_line, str;
        getline(in,my_line);
        std::stringstream mystream(my_line);

        getline(mystream,str, ',');
        if(!str.empty()) {
            // fetching the current UTC timestamp
            double tm = static_cast<double> (time(NULL));
            if (tm == -1) {
                /* Invalidating the Timestamp in case of error.*/
                infoValidity &= ~(LocationValidityType::HAS_TIMESTAMP_BIT);
                Warn(__func__, "Fetching current timestamp failed with error: ", strerror(errno));
            } else {
                infoValidity |= LocationValidityType::HAS_TIMESTAMP_BIT;
            }
            iBase_->setUtcFixTime(tm);
            iBaseEx_->setUtcFixTime(tm);
        }
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBase_->setLocationTechnology(stoi(str));
            iBaseEx_->setLocationTechnology(stoi(str));
        }
        getline(mystream,str,',');
        if(!str.empty()) {
            iBase_->setLatitude(stod(str));
            iBaseEx_->setLatitude(stod(str));
            infoValidity |= LocationValidityType::HAS_LAT_LONG_BIT;
        }
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBase_->setLongitude(stod(str));
            iBaseEx_->setLongitude(stod(str));
            infoValidity |= LocationValidityType::HAS_LAT_LONG_BIT;
        }
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBase_->setAltitude(stod(str));
            iBaseEx_->setAltitude(stod(str));
            infoValidity |= LocationValidityType::HAS_ALTITUDE_BIT;
        }
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBaseEx_->setNumSvUsed(stoul(str));
            infoExValidity |= LocationInfoExValidityType::HAS_NUM_SV_USED_IN_POSITION;
        }
        getline(mystream,str, ','); //current file has no entries in these columns and hence skipped
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBase_->setHeading(stof(str));
            iBaseEx_->setHeading(stof(str));
            infoValidity |= LocationValidityType::HAS_HEADING_BIT;
        }
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBase_->setSpeed(stof(str));
            iBaseEx_->setSpeed(stof(str));
            infoValidity |= LocationValidityType::HAS_SPEED_BIT;
        }
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        if(!str.empty()) {
            bodyFrameData.latAccel = (stof(str));
            bodyFrameValidity |= KinematicDataValidityType::HAS_LAT_ACCEL;
        }
        getline(mystream,str, ',');
        if(!str.empty()) {
            bodyFrameData.longAccel = (stof(str));
            bodyFrameValidity |= KinematicDataValidityType::HAS_LONG_ACCEL;
        }
        getline(mystream,str, ',');
        if(!str.empty()) {
            bodyFrameData.vertAccel = (stof(str));
            bodyFrameValidity |= KinematicDataValidityType::HAS_VERT_ACCEL;
        }
        getline(mystream,str, ',');
        if(!str.empty()) {
            bodyFrameData.yawRate = (stof(str));
            bodyFrameValidity |= KinematicDataValidityType::HAS_YAW_RATE;
        }
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBase_->setHeadingUncertainty(stof(str));
            iBaseEx_->setHeadingUncertainty(stof(str));
            infoValidity |= LocationValidityType::HAS_HEADING_ACCURACY_BIT;
        }
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBase_->setSpeedUncertainty(stof(str));
            iBaseEx_->setSpeedUncertainty(stof(str));
            infoValidity |= LocationValidityType::HAS_SPEED_ACCURACY_BIT;
        }
        getline(mystream,str, ',');
        getline(mystream,str, ',');
        if(!str.empty()) {
            iBaseEx_->setLeapSeconds(stoi(str));
            infoExValidity |= LocationInfoExValidityType::HAS_LEAP_SECONDS;
        }
        bodyFrameData.bodyFrameDataMask = bodyFrameValidity;
        iBase_->setLocationInfoValidity(infoValidity);
        iBaseEx_->setBodyFrameData(bodyFrameData);
        iBaseEx_->setLocationInfoValidity(infoValidity);
        iBaseEx_->setLocationInfoExValidity(infoExValidity);
    }
}

void ReportReader::reportDataInit() {
    //GnssSignalInfo Init
    gnssSignalInfo_ = std::make_shared<GnssSignalInfo>();
    struct GnssData gnssdata;
    auto maxGnssSignalTypes =
        telux::loc::GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES;
    for (auto i = 0; i < maxGnssSignalTypes; i++) {
        gnssdata.gnssDataMask[i]= 0;
        gnssdata.jammerInd[i] = 0;
        gnssdata.agc[i] = 0;
    }
    gnssSignalInfo_->setGnssData(gnssdata);
    //GnssSVInfo Init
    gnssSVInfo_ = std::make_shared<GnssSVInfo>();
    //GnssNmeaInfo Init
    struct NMEAVals nmeaVals;
    nmeaVals.nmeaTimestamp = 0;
    nmeaVals.nmeaString = "";
    defaultNmeaVals_.push_back(nmeaVals);
    iBase_ = std::make_shared<LocationInfoBase>();
    iBaseEx_ = std::make_shared<LocationInfoEx>();
}

void reportCannedIBaseInit(std::shared_ptr<LocationInfoBase> iBase_) {
    iBase_->setLocationTechnology(FIX_MODE_VAL);
    iBase_->setSpeed(VELOCITY_VAL);
    iBase_->setLatitude(LATITUDE_VAL);
    iBase_->setLongitude(LONGITUDE_VAL);
    iBase_->setAltitude(ALTITUDE_VAL);
    iBase_->setHeading(HEADING_VAL);
    iBase_->setHorizontalUncertainty(HORIZONTAL_CONFIDENCE_VAL);
    iBase_->setVerticalUncertainty(VERTICAL_CONFIDENCE_VAL);
    iBase_->setUtcFixTime(UTC_FIX_TIME_VAL);
    iBase_->setSpeedUncertainty(VELOCITY_CONFIDENCE_VAL);
    iBase_->setHeadingUncertainty(HEADING_CONFIDENCE_VAL);
    iBase_->setLocationInfoValidity(0xff);
}

void reportCannedIBaseExInit(std::shared_ptr<LocationInfoEx> iBaseEx_) {
    iBaseEx_->setLocationTechnology(FIX_MODE_VAL);
    iBaseEx_->setSpeed(VELOCITY_VAL);
    iBaseEx_->setLatitude(LATITUDE_VAL);
    iBaseEx_->setLongitude(LONGITUDE_VAL);
    iBaseEx_->setAltitude(ALTITUDE_VAL);
    iBaseEx_->setHeading(HEADING_VAL);
    iBaseEx_->setHorizontalUncertainty(HORIZONTAL_CONFIDENCE_VAL);
    iBaseEx_->setVerticalUncertainty(VERTICAL_CONFIDENCE_VAL);
    iBaseEx_->setUtcFixTime(UTC_FIX_TIME_VAL);
    iBaseEx_->setSpeedUncertainty(VELOCITY_CONFIDENCE_VAL);
    iBaseEx_->setHeadingUncertainty(HEADING_CONFIDENCE_VAL);
    iBaseEx_->setLocationInfoValidity(0xff);

    iBaseEx_->setAltitudeMeanSeaLevel(ALTITUDE_MEAN_SEA_VAL);
    iBaseEx_->setPositionDop(POSITION_DOP);
    iBaseEx_->setHorizontalDop(HORIZON_DOP);
    iBaseEx_->setVerticalDop(VERTICAL_DOP);
    iBaseEx_->setGeometricDop(GEOMETRIC_DOP);
    iBaseEx_->setTimeDop(TIME_DOP);
    iBaseEx_->setMagneticDeviation(MAGNETIC_DEVIATION);
    iBaseEx_->setHorizontalReliability(HORIZONTAL_RELIABILITY);
    iBaseEx_->setVerticalReliability(VERTICAL_RELIABILITY);
    iBaseEx_->setHorizontalUncertaintySemiMajor(HORIZONAL_UNCERTAINITY_SEMI_MAJOR);
    iBaseEx_->setHorizontalUncertaintySemiMinor(HORIZONAL_UNCERTAINITY_SEMI_MINOR);
    iBaseEx_->setHorizontalUncertaintyAzimuth(HORIZONAL_UNCERTAINITY_AZIMUTH);
    iBaseEx_->setEastStandardDeviation(EAST_STANDARD_DEVIATION);
    iBaseEx_->setNorthStandardDeviation(NORTH_STANDARD_DEVIATION);
    iBaseEx_->setNumSvUsed(SV_USED);
    struct SvUsedInPosition svUser;
    svUser.gps = 2156529280;
    svUser.glo = 426040;
    svUser.gal = 604000450;
    svUser.bds = 60166542068;
    svUser.qzss = 3;
    iBaseEx_->setSvUsedInPosition(svUser);
    std::vector<uint16_t> usedSvs;
    usedSvs.push_back(8);
    usedSvs.push_back(10);
    usedSvs.push_back(11);
    usedSvs.push_back(18);
    usedSvs.push_back(20);
    usedSvs.push_back(24);
    usedSvs.push_back(32);
    usedSvs.push_back(70);
    usedSvs.push_back(82);
    usedSvs.push_back(80);
    iBaseEx_->setUsedSVsIds(usedSvs);
    std::bitset<SBAS_COUNT> sbas = 0x7;
    iBaseEx_->setSbasCorrection(sbas);
    iBaseEx_->setPositionTechnology(POSITION_TECHNOLOGY);
    GnssKinematicsData bodyFrameData;
    bodyFrameData.latAccel = 0.012653895654;
    bodyFrameData.longAccel = -0.000767216377;
    bodyFrameData.vertAccel = -0.001908625592;
    bodyFrameData.yawRate = 0.016897117609;
    bodyFrameData.pitch = 0.01236311;
    bodyFrameData.latAccelUnc = 0.02345111;
    bodyFrameData.longAccelUnc = 0.233222342;
    bodyFrameData.vertAccelUnc = 0.00345121;
    bodyFrameData.yawRateUnc = 0.009981211;
    bodyFrameData.pitchUnc = 0.0099911;
    bodyFrameData.bodyFrameDataMask = 0xff;
    iBaseEx_->setBodyFrameData(bodyFrameData);
    std::vector<GnssMeasurementInfo> gnssMeasInfoVec;
    GnssMeasurementInfo gnssMeasInfo;
    gnssMeasInfo.gnssSignalType = 1; //GPS_L1CA
    gnssMeasInfo.gnssConstellation = GnssSystem::GNSS_LOC_SV_SYSTEM_GPS;
    gnssMeasInfo.gnssSvId = 8;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSvId = 10;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSvId = 11;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSvId = 18;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSvId = 20;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSvId = 24;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSvId = 32;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSignalType = GLONASS_G1;
    gnssMeasInfo.gnssConstellation = GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS;
    gnssMeasInfo.gnssSvId = 70;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSvId = 82;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    gnssMeasInfo.gnssSvId = 80;
    gnssMeasInfoVec.push_back(gnssMeasInfo);
    iBaseEx_->setMeasUsageInfo(gnssMeasInfoVec);

    SystemTime systemTime;
    systemTime.gnssSystemTimeSrc = GnssSystem::GNSS_LOC_SV_SYSTEM_BDS;
    SystemTimeInfo timeInfo;
    timeInfo.bds.validityMask = 0xff;
    timeInfo.bds.numClockResets = 2;
    timeInfo.bds.refFCount = 54899632;
    timeInfo.bds.systemClkTimeUncMs = 0.04216802;
    timeInfo.bds.systemClkTimeBias =  0.00743252;
    timeInfo.bds.systemMsec = 23940138;
    timeInfo.bds.systemWeek = 52;
    systemTime.time = timeInfo;
    iBaseEx_->setGnssSystemTime(systemTime);

    iBaseEx_->setTimeUncMs(TIME_UNC);
    iBaseEx_->setLeapSeconds(LEAP_SECONDS_VAL);
    iBaseEx_->setCalibrationConfidencePercent(CALIBRATION_CONFIDENCE_PERCENT);
    iBaseEx_->setCalibrationStatus(CALIBRATION_STATUS);
    iBaseEx_->setLocOutputEngType(LOC_OUTPUT_ENGINE_FUSED);
    iBaseEx_->setLocOutputEngMask(STANDARD_POSITIONING_ENGINE);
    iBaseEx_->setConformityIndex(CONFORMITY_INDEX);

    std::vector<float> velocityEastNorthUp;
    velocityEastNorthUp.push_back(0.56757);
    velocityEastNorthUp.push_back(0.6757112);
    velocityEastNorthUp.push_back(0.0654675);
    velocityEastNorthUp.push_back(0.984823);
    iBaseEx_->setVelocityEastNorthUp(velocityEastNorthUp);

    std::vector<float> velocityUncertaintyEastNorthUp;
    velocityUncertaintyEastNorthUp.push_back(0.546543);
    velocityUncertaintyEastNorthUp.push_back(0.3323001);
    velocityUncertaintyEastNorthUp.push_back(0.005461);
    velocityUncertaintyEastNorthUp.push_back(0.67265431);
    velocityUncertaintyEastNorthUp.push_back(0.9095441);
    iBaseEx_->setVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp);
    iBaseEx_->setLocationInfoExValidity(0xffffffff);
}

void reportCannedGnssSignalInfoInit(std::shared_ptr<GnssSignalInfo> gnssSignalInfo_) {
    auto gnssdata = gnssSignalInfo_->getGnssData();
    gnssdata.gnssDataMask[7]= 3;
    gnssdata.jammerInd[7] = 32;
    gnssdata.agc[7] = 2.24;
    gnssdata.gnssDataMask[9]= 3;
    gnssdata.jammerInd[9] = 56;
    gnssdata.agc[9] = 1.44;
    gnssdata.gnssDataMask[15]= 3;
    gnssdata.jammerInd[15] = 12;
    gnssdata.agc[15] = -0.12;
    gnssSignalInfo_->setGnssData(gnssdata);
}

void reportCannedGnssSVInfoInit(std::shared_ptr<GnssSVInfo> gnssSVInfo_) {
    std::shared_ptr<SVInfo> svInfo = std::make_shared<SVInfo>();
    svInfo->setConstellation(GnssConstellationType::GPS);
    svInfo->setId(8);
    svInfo->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo->setHasEphemeris(SVInfoAvailability::YES);
    svInfo->setHasAlmanac(SVInfoAvailability::YES);
    svInfo->setHasFix(SVInfoAvailability::YES);
    svInfo->setElevation(1422.83);
    svInfo->setAzimuth(67.5);
    svInfo->setSnr(37.2);
    svInfo->setCarrierFrequency(1575420032.0);
    svInfo->setSignalType(GnssSignalType::GPS_L1CA);

    std::shared_ptr<SVInfo> svInfo2 = std::make_shared<SVInfo>();
    svInfo2->setConstellation(GnssConstellationType::GPS);
    svInfo2->setId(10);
    svInfo2->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo2->setHasEphemeris(SVInfoAvailability::NO);
    svInfo2->setHasAlmanac(SVInfoAvailability::YES);
    svInfo2->setHasFix(SVInfoAvailability::YES);
    svInfo2->setElevation(40);
    svInfo2->setAzimuth(19);
    svInfo2->setSnr(48);
    svInfo2->setCarrierFrequency(1575420032.00000);
    svInfo2->setSignalType(GnssSignalType::GPS_L1CA);

    std::shared_ptr<SVInfo> svInfo3 = std::make_shared<SVInfo>();
    svInfo3->setConstellation(GnssConstellationType::BDS);
    svInfo3->setId(30);
    svInfo3->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo3->setHasEphemeris(SVInfoAvailability::NO);
    svInfo3->setHasAlmanac(SVInfoAvailability::YES);
    svInfo3->setHasFix(SVInfoAvailability::YES);
    svInfo3->setElevation(35);
    svInfo3->setAzimuth(25);
    svInfo3->setSnr(34.4000015);
    svInfo3->setCarrierFrequency(1561097984.00);
    svInfo3->setSignalType(GnssSignalType::BEIDOU_B1);

    std::shared_ptr<SVInfo> svInfo4 = std::make_shared<SVInfo>();
    svInfo4->setConstellation(GnssConstellationType::GPS);
    svInfo4->setId(11);
    svInfo4->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo4->setHasEphemeris(SVInfoAvailability::NO);
    svInfo4->setHasAlmanac(SVInfoAvailability::YES);
    svInfo4->setHasFix(SVInfoAvailability::YES);
    svInfo4->setElevation(3.1400000000000000);
    svInfo4->setAzimuth(317.000000000000);
    svInfo4->setSnr(33.5999984741211);
    svInfo4->setCarrierFrequency(1575420032.00000);
    svInfo4->setSignalType(GnssSignalType::GPS_L1CA);

    std::shared_ptr<SVInfo> svInfo5 = std::make_shared<SVInfo>();
    svInfo5->setConstellation(GnssConstellationType::GPS);
    svInfo5->setId(18);
    svInfo5->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo5->setHasEphemeris(SVInfoAvailability::NO);
    svInfo5->setHasAlmanac(SVInfoAvailability::YES);
    svInfo5->setHasFix(SVInfoAvailability::YES);
    svInfo5->setElevation(3.1400000000000000);
    svInfo5->setAzimuth(140.000000000000);
    svInfo5->setSnr(46.0999984741211);
    svInfo5->setCarrierFrequency(1575420032.00000);
    svInfo5->setSignalType(GnssSignalType::GPS_L1CA);

    std::shared_ptr<SVInfo> svInfo6 = std::make_shared<SVInfo>();
    svInfo6->setConstellation(GnssConstellationType::BDS);
    svInfo6->setId(206);
    svInfo6->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo6->setHasEphemeris(SVInfoAvailability::YES);
    svInfo6->setHasAlmanac(SVInfoAvailability::YES);
    svInfo6->setHasFix(SVInfoAvailability::NO);
    svInfo6->setElevation(35.0000000000000000);
    svInfo6->setAzimuth(25.000000000000);
    svInfo6->setSnr(32.0000000000000);
    svInfo6->setCarrierFrequency(1561097984.0000);
    svInfo6->setSignalType(GnssSignalType::BEIDOU_B1I);

    std::shared_ptr<SVInfo> svInfo7 = std::make_shared<SVInfo>();
    svInfo7->setConstellation(GnssConstellationType::GALILEO);
    svInfo7->setId(307);
    svInfo7->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo7->setHasEphemeris(SVInfoAvailability::YES);
    svInfo7->setHasAlmanac(SVInfoAvailability::YES);
    svInfo7->setHasFix(SVInfoAvailability::YES);
    svInfo7->setElevation(44.0000000000000000);
    svInfo7->setAzimuth(350.000000000000);
    svInfo7->setSnr(43.5999984741211);
    svInfo7->setCarrierFrequency(1575420032.00000);
    svInfo7->setSignalType(GnssSignalType::GALILEO_E1);

    std::shared_ptr<SVInfo> svInfo8 = std::make_shared<SVInfo>();
    svInfo8->setConstellation(GnssConstellationType::GLONASS);
    svInfo8->setId(23);
    svInfo8->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo8->setHasEphemeris(SVInfoAvailability::YES);
    svInfo8->setHasAlmanac(SVInfoAvailability::YES);
    svInfo8->setHasFix(SVInfoAvailability::YES);
    svInfo8->setElevation(14.0000000000000000);
    svInfo8->setAzimuth(170.000000000000);
    svInfo8->setSnr(35.4000015258789);
    svInfo8->setCarrierFrequency(1600312448.00000);
    svInfo8->setSignalType(GnssSignalType::GLONASS_G1);

    std::shared_ptr<SVInfo> svInfo9 = std::make_shared<SVInfo>();
    svInfo9->setConstellation(GnssConstellationType::QZSS);
    svInfo9->setId(194);
    svInfo9->setSVHealthStatus(SVHealthStatus::HEALTHY);
    svInfo9->setHasEphemeris(SVInfoAvailability::NO);
    svInfo9->setHasAlmanac(SVInfoAvailability::YES);
    svInfo9->setHasFix(SVInfoAvailability::YES);
    svInfo9->setElevation(17.0000000000000000);
    svInfo9->setAzimuth(79.000000000000);
    svInfo9->setSnr(38.0999984741211);
    svInfo9->setCarrierFrequency(1575420032.00000);
    svInfo9->setSignalType(GnssSignalType::QZSS_L1CA);

    std::vector<std::shared_ptr<ISVInfo>> svInfoList;
    svInfoList.push_back(svInfo);
    svInfoList.push_back(svInfo2);
    svInfoList.push_back(svInfo3);
    svInfoList.push_back(svInfo4);
    svInfoList.push_back(svInfo5);
    svInfoList.push_back(svInfo6);
    svInfoList.push_back(svInfo7);
    svInfoList.push_back(svInfo8);
    svInfoList.push_back(svInfo9);
    gnssSVInfo_->setSVInfoList(svInfoList);
    gnssSVInfo_->setAltitudeType(AltitudeType::ASSUMED);
}

void reportCannedGnssMeasurementsInit(struct GnssMeasurements &gnssMeasurements_) {
    GnssMeasurementsData measurementsData;
    GnssMeasurementsDataValidity validMask= 0;
    measurementsData.svId = 24;
    validMask |= SV_ID_BIT;
    measurementsData.svType = GnssConstellationType::GPS;
    validMask |= SV_TYPE_BIT;
    measurementsData.timeOffsetNs = 0;
    GnssMeasurementsStateValidity validSMask= 0;
    validSMask |= CODE_LOCK_BIT;
    validSMask |= BIT_SYNC_BIT;
    validSMask |= SUBFRAME_SYNC_BIT;
    validSMask |= TOW_DECODED_BIT;
    measurementsData.stateMask = validSMask;
    measurementsData.receivedSvTimeNs = 388401344958305;
    validMask |= RECEIVED_SV_TIME_BIT;
    measurementsData.receivedSvTimeUncertaintyNs= 31;
    validMask |= RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    measurementsData.carrierToNoiseDbHz = 26.8;
    validMask |= CARRIER_TO_NOISE_BIT;
    measurementsData.pseudorangeRateMps = 665.942;
    validMask |= PSEUDORANGE_RATE_BIT;
    measurementsData.pseudorangeRateUncertaintyMps = 0.7475;
    validMask |= PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    measurementsData.adrMeters = 0;
    validMask |= ADR_BIT;
    measurementsData.adrUncertaintyMeters = 0;
    validMask |= ADR_UNCERTAINTY_BIT;
    measurementsData.carrierFrequencyHz = 1.57542e+09;
    validMask |= CARRIER_FREQUENCY_BIT;
    measurementsData.carrierCycles = 0;
    validMask |= CARRIER_CYCLES_BIT;
    measurementsData.carrierPhase = 0;
    validMask |= CARRIER_PHASE_BIT;
    measurementsData.carrierPhaseUncertainty = 0;
    validMask |= CARRIER_PHASE_UNCERTAINTY_BIT;
    measurementsData.multipathIndicator = UNKNOWN_INDICATOR;
    validMask |= MULTIPATH_INDICATOR_BIT;
    measurementsData.signalToNoiseRatioDb = 0;
    validMask |= SIGNAL_TO_NOISE_RATIO_BIT;
    measurementsData.agcLevelDb = 2.23;
    validMask |= AUTOMATIC_GAIN_CONTROL_BIT;
    measurementsData.valid=validMask;
    gnssMeasurements_.measurements.push_back(measurementsData);

    GnssMeasurementsData measurementsData1;
    validMask = 0;
    measurementsData1.svId = 207;
    validMask |= SV_ID_BIT;
    measurementsData1.svType = GnssConstellationType::BDS;
    validMask |= SV_TYPE_BIT;
    measurementsData1.timeOffsetNs = 0;
    validSMask = 0;
    validSMask |= CODE_LOCK_BIT;
    validSMask |= BIT_SYNC_BIT;
    validSMask |= SUBFRAME_SYNC_BIT;
    validSMask |= TOW_DECODED_BIT;
    measurementsData1.stateMask = validSMask;
    measurementsData1.receivedSvTimeNs = 388386297448103;
    validMask |= RECEIVED_SV_TIME_BIT;
    measurementsData1.receivedSvTimeUncertaintyNs = 27;
    validMask |= RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    measurementsData1.carrierToNoiseDbHz = 29.1;
    validMask |= CARRIER_TO_NOISE_BIT;
    measurementsData1.pseudorangeRateMps = -228.853;
    validMask |= PSEUDORANGE_RATE_BIT;
    measurementsData1.pseudorangeRateUncertaintyMps = 0.578;
    validMask |= PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    measurementsData1.adrMeters = 0;
    validMask |= ADR_BIT;
    measurementsData1.adrUncertaintyMeters = 0;
    validMask |= ADR_UNCERTAINTY_BIT;
    measurementsData1.carrierFrequencyHz = 1.5611e+09;
    validMask |= CARRIER_FREQUENCY_BIT;
    measurementsData1.carrierCycles = 0;
    validMask |= CARRIER_CYCLES_BIT;
    measurementsData1.carrierPhase = 0;
    validMask |= CARRIER_PHASE_BIT;
    measurementsData1.carrierPhaseUncertainty = 0;
    validMask |= CARRIER_PHASE_UNCERTAINTY_BIT;
    measurementsData1.multipathIndicator = UNKNOWN_INDICATOR;
    validMask |= MULTIPATH_INDICATOR_BIT;
    measurementsData1.signalToNoiseRatioDb = 0;
    validMask |= SIGNAL_TO_NOISE_RATIO_BIT;
    measurementsData1.agcLevelDb = 0.07;
    validMask |= AUTOMATIC_GAIN_CONTROL_BIT;
    measurementsData1.valid = validMask;
    gnssMeasurements_.measurements.push_back(measurementsData1);

    GnssMeasurementsData measurementsData2;
    validMask = 0;
    measurementsData2.svId = 307;
    validMask |= SV_ID_BIT;
    measurementsData2.svType = GnssConstellationType::GALILEO;
    validMask |= SV_TYPE_BIT;
    measurementsData2.timeOffsetNs = 0;
    validSMask = 0;
    validSMask |= CODE_LOCK_BIT;
    validSMask |= BIT_SYNC_BIT;
    validSMask |= SUBFRAME_SYNC_BIT;
    validSMask |= TOW_DECODED_BIT;
    validSMask |= GAL_E1BC_CODE_LOCK_BIT;
    validSMask |= GAL_E1C_2ND_CODE_LOCK_BIT;
    validSMask |= GAL_E1B_PAGE_SYNC_BIT;
    measurementsData2.stateMask = validSMask;
    measurementsData2.receivedSvTimeNs = 388400346588677;
    validMask |= RECEIVED_SV_TIME_BIT;
    measurementsData2.receivedSvTimeUncertaintyNs = 6;
    validMask |= RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    measurementsData2.carrierToNoiseDbHz = 44.2;
    validMask |= CARRIER_TO_NOISE_BIT;
    measurementsData2.pseudorangeRateMps = 186.973;
    validMask |= PSEUDORANGE_RATE_BIT;
    measurementsData2.pseudorangeRateUncertaintyMps = 0.0197921;
    validMask |= PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    measurementsData2.adrMeters = 0;
    validMask |= ADR_BIT;
    measurementsData2.adrUncertaintyMeters = 0;
    validMask |= ADR_UNCERTAINTY_BIT;
    measurementsData2.carrierFrequencyHz = 1.57542e+09;
    validMask |= CARRIER_FREQUENCY_BIT;
    measurementsData2.carrierCycles = 0;
    validMask |= CARRIER_CYCLES_BIT;
    measurementsData2.carrierPhase = 0;
    validMask |= CARRIER_PHASE_BIT;
    measurementsData2.carrierPhaseUncertainty = 0;
    validMask |= CARRIER_PHASE_UNCERTAINTY_BIT;
    measurementsData2.multipathIndicator = UNKNOWN_INDICATOR;
    validMask |= MULTIPATH_INDICATOR_BIT;
    measurementsData2.signalToNoiseRatioDb = 0;
    validMask |= SIGNAL_TO_NOISE_RATIO_BIT;
    measurementsData2.agcLevelDb = 2.09;
    gnssMeasurements_.measurements.push_back(measurementsData2);

    GnssMeasurementsData measurementsData3;
    validMask = 0;
    measurementsData3.svId = 193;
    validMask |= SV_ID_BIT;
    measurementsData3.svType = GnssConstellationType::QZSS;
    validMask |= SV_TYPE_BIT;
    measurementsData3.timeOffsetNs = 0;
    validSMask = 0;
    validSMask |= CODE_LOCK_BIT;
    validSMask |= BIT_SYNC_BIT;
    validSMask |= SUBFRAME_SYNC_BIT;
    validSMask |= TOW_DECODED_BIT;
    measurementsData3.stateMask = validSMask;
    measurementsData3.receivedSvTimeNs = 388386297448103;
    validMask |= RECEIVED_SV_TIME_BIT;
    measurementsData3.receivedSvTimeUncertaintyNs = 9;
    validMask |= RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    measurementsData3.carrierToNoiseDbHz = 39.4;
    validMask |= CARRIER_TO_NOISE_BIT;
    measurementsData3.pseudorangeRateMps = -223.677;
    validMask |= PSEUDORANGE_RATE_BIT;
    measurementsData3.pseudorangeRateUncertaintyMps = 0.0323092;
    validMask |= PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    measurementsData3.adrMeters = 0;
    validMask |= ADR_BIT;
    measurementsData3.adrUncertaintyMeters = 0;
    validMask |= ADR_UNCERTAINTY_BIT;
    measurementsData3.carrierFrequencyHz = 1.57542e+09;
    validMask |= CARRIER_FREQUENCY_BIT;
    measurementsData3.carrierCycles = 0;
    validMask |= CARRIER_CYCLES_BIT;
    measurementsData3.carrierPhase = 0;
    validMask |= CARRIER_PHASE_BIT;
    measurementsData3.carrierPhaseUncertainty = 0;
    validMask |= CARRIER_PHASE_UNCERTAINTY_BIT;
    measurementsData3.multipathIndicator = UNKNOWN_INDICATOR;
    validMask |= MULTIPATH_INDICATOR_BIT;
    measurementsData3.signalToNoiseRatioDb = 0;
    validMask |= SIGNAL_TO_NOISE_RATIO_BIT;
    measurementsData3.agcLevelDb = 2.23;
    validMask |= AUTOMATIC_GAIN_CONTROL_BIT;
    measurementsData3.valid = validMask;
    gnssMeasurements_.measurements.push_back(measurementsData3);

    GnssMeasurementsData measurementsData4;
    validMask = 0;
    measurementsData4.svId = 70;
    validMask |= SV_ID_BIT;
    measurementsData4.svType = GnssConstellationType::GLONASS;
    validMask |= SV_TYPE_BIT;
    measurementsData4.timeOffsetNs = 0;
    validSMask = 0;
    validSMask |= CODE_LOCK_BIT;
    validSMask |= BIT_SYNC_BIT;
    validSMask |= SUBFRAME_SYNC_BIT;
    validSMask |= TOW_DECODED_BIT;
    validSMask |= GLO_STRING_SYNC_BIT;
    validSMask |= GLO_TOD_DECODED_BIT;
    measurementsData4.stateMask = validSMask;
    measurementsData4.receivedSvTimeNs = 53583350638177;
    validMask |= RECEIVED_SV_TIME_BIT;
    measurementsData4.receivedSvTimeUncertaintyNs = 27;
    validMask |= RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    measurementsData4.carrierToNoiseDbHz = 28.9;
    validMask |= CARRIER_TO_NOISE_BIT;
    measurementsData4.pseudorangeRateMps = -115.822;
    validMask |= PSEUDORANGE_RATE_BIT;
    measurementsData4.pseudorangeRateUncertaintyMps = 0.6285;
    validMask |= PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    measurementsData4.adrMeters = 0;
    validMask |= ADR_BIT;
    measurementsData4.adrUncertaintyMeters = 0;
    validMask |= ADR_UNCERTAINTY_BIT;
    measurementsData4.carrierFrequencyHz = 1.602e+09;
    validMask |= CARRIER_FREQUENCY_BIT;
    measurementsData4.carrierCycles = 0;
    validMask |= CARRIER_CYCLES_BIT;
    measurementsData4.carrierPhase = 0;
    validMask |= CARRIER_PHASE_BIT;
    measurementsData4.carrierPhaseUncertainty = 0;
    validMask |= CARRIER_PHASE_UNCERTAINTY_BIT;
    measurementsData4.multipathIndicator = UNKNOWN_INDICATOR;
    validMask |= MULTIPATH_INDICATOR_BIT;
    measurementsData4.signalToNoiseRatioDb = 0;
    validMask |= SIGNAL_TO_NOISE_RATIO_BIT;
    measurementsData4.agcLevelDb = -0.13;
    validMask |= AUTOMATIC_GAIN_CONTROL_BIT;
    measurementsData4.valid = validMask;
    gnssMeasurements_.measurements.push_back(measurementsData4);

    GnssMeasurementsData measurementData5;
    validMask = 0;
    measurementData5.svId = 201;
    validMask |= SV_ID_BIT;
    measurementData5.svType = GnssConstellationType::GLONASS;
    validMask |= SV_TYPE_BIT;
    measurementData5.timeOffsetNs = 0;
    validSMask = 0;
    validSMask |= CODE_LOCK_BIT;
    validSMask |= BIT_SYNC_BIT;
    validSMask |= SUBFRAME_SYNC_BIT;
    validSMask |= TOW_DECODED_BIT;
    measurementData5.stateMask = validSMask;
    measurementData5.receivedSvTimeNs = 786875476413;
    validMask |= RECEIVED_SV_TIME_BIT;
    measurementData5.receivedSvTimeUncertaintyNs = 4;
    validMask |= RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    measurementData5.carrierToNoiseDbHz = 78.2;
    validMask |= CARRIER_TO_NOISE_BIT;
    measurementData5.pseudorangeRateMps = -67.321;
    validMask |= PSEUDORANGE_RATE_BIT;
    measurementData5.pseudorangeRateUncertaintyMps = 0.06751;
    validMask |= PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    measurementData5.adrMeters = 2;
    validMask |= ADR_BIT;
    measurementData5.adrUncertaintyMeters = 0;
    validMask |= ADR_UNCERTAINTY_BIT;
    measurementData5.carrierFrequencyHz = 1.8942e+09;
    validMask |= CARRIER_FREQUENCY_BIT;
    measurementData5.carrierCycles = 0;
    validMask |= CARRIER_CYCLES_BIT;
    measurementData5.carrierPhase = 0;
    validMask |= CARRIER_PHASE_BIT;
    measurementData5.carrierPhaseUncertainty = 0;
    validMask |= CARRIER_PHASE_UNCERTAINTY_BIT;
    measurementData5.multipathIndicator = UNKNOWN_INDICATOR;
    validMask |= MULTIPATH_INDICATOR_BIT;
    measurementData5.signalToNoiseRatioDb = 0;
    validMask |= SIGNAL_TO_NOISE_RATIO_BIT;
    measurementData5.agcLevelDb = 2.73;
    validMask |= AUTOMATIC_GAIN_CONTROL_BIT;
    measurementData5.valid = validMask;
    gnssMeasurements_.measurements.push_back(measurementData5);


    GnssMeasurementsData measurementData6;
    validMask = 0;
    measurementData6.svId = 212;
    validMask |= SV_ID_BIT;
    measurementData6.svType = GnssConstellationType::QZSS;
    validMask |= SV_TYPE_BIT;
    measurementData6.timeOffsetNs = 0;
    validSMask = 0;
    validSMask |= CODE_LOCK_BIT;
    validSMask |= BIT_SYNC_BIT;
    validSMask |= SUBFRAME_SYNC_BIT;
    validSMask |= TOW_DECODED_BIT;
    measurementData6.stateMask = validSMask;
    measurementData6.receivedSvTimeNs = 234134123;
    validMask |= RECEIVED_SV_TIME_BIT;
    measurementData6.receivedSvTimeUncertaintyNs = 4;
    validMask |= RECEIVED_SV_TIME_UNCERTAINTY_BIT;
    measurementData6.carrierToNoiseDbHz = 0.32;
    validMask |= CARRIER_TO_NOISE_BIT;
    measurementData6.pseudorangeRateMps = 4.67;
    validMask |= PSEUDORANGE_RATE_BIT;
    measurementData6.pseudorangeRateUncertaintyMps = 0.042492;
    validMask |= PSEUDORANGE_RATE_UNCERTAINTY_BIT;
    measurementData6.adrMeters = 3;
    validMask |= ADR_BIT;
    measurementData6.adrUncertaintyMeters = 0;
    validMask |= ADR_UNCERTAINTY_BIT;
    measurementData6.carrierFrequencyHz = 1.57542e+09;
    validMask |= CARRIER_FREQUENCY_BIT;
    measurementData6.carrierCycles = 0;
    validMask |= CARRIER_CYCLES_BIT;
    measurementData6.carrierPhase = 0;
    validMask |= CARRIER_PHASE_BIT;
    measurementData6.carrierPhaseUncertainty = 0;
    validMask |= CARRIER_PHASE_UNCERTAINTY_BIT;
    measurementData6.multipathIndicator = UNKNOWN_INDICATOR;
    validMask |= MULTIPATH_INDICATOR_BIT;
    measurementData6.signalToNoiseRatioDb = 0;
    validMask |= SIGNAL_TO_NOISE_RATIO_BIT;
    measurementData6.agcLevelDb = 34.75;
    validMask |= AUTOMATIC_GAIN_CONTROL_BIT;
    measurementData6.valid = validMask;
    gnssMeasurements_.measurements.push_back(measurementData6);
}

void ReportReader::reportCannedDataInit() {
    reportCannedIBaseInit(iBase_);
    reportCannedIBaseExInit(iBaseEx_);
    reportCannedGnssSignalInfoInit(gnssSignalInfo_);
    reportCannedGnssSVInfoInit(gnssSVInfo_);
    reportCannedGnssMeasurementsInit(gnssMeasurements_);

    locationSystemInfo_.valid = 1;
    locationSystemInfo_.info.valid = 3;
    locationSystemInfo_.info.current = (uint8_t) 18;

    struct NMEAVals nmeaVals;
    defaultNmeaVals_.clear();
    nmeaVals.nmeaString = "$GPGSV,2,1,08,08,12,288,37,10,40,021,47,11,05,316,36,18,35,141,45,1*69";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GPGSV,2,2,08,20,36,068,45,23,00,000,47,24,02,038,33,32,52,329,49,1*6A";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GLGSV,1,1,03,82,11,169,35,69,26,340,35,68,14,032,36,1*4C";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GAGSV,2,1,06,07,43,352,46,08,15,042,36,13,62,178,47,15,13,153,42,7*73";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GAGSV,2,2,06,27,21,160,43,30,23,104,43,7*73";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GQGSV,1,1,02,01,23,065,38,02,18,079,38,1*61";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GBGSV,4,1,13,45,48,135,42,44,48,245,42,36,29,067,40,35,28,179,36,1*79";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GBGSV,4,2,13,34,19,303,34,26,18,192,38,21,29,321,37,16,40,019,37,1*75";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GBGSV,4,3,15,21,29,321,36,16,40,019,38,13,65,130,40,10,21,151,35,1*7E";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
    nmeaVals.nmeaString = "$GBGSV,4,4,15,08,39,138,36,07,18,126,30,06,35,025,35,1*46";
    nmeaVals.nmeaTimestamp = 1597924258130;
    defaultNmeaVals_.push_back(nmeaVals);
}


}
}
