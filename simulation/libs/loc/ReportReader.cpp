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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ReportReader.hpp"
#include <time.h>
#include <cerrno>
#include "../common/Logger.hpp"
#include <cstring>
#include <iostream>
#include <thread>

namespace telux {

namespace loc {

ReportReader::ReportReader() {
    LOG(DEBUG, __FUNCTION__);
    std::string str;
    simulType_ = false;
    std::shared_ptr<SimulationConfigParser> configParser =
        std::make_shared<SimulationConfigParser>();
    str = configParser->getValue("REPORT_SIMULATION_TYPE");
    simulType_ = (str.size())? (std::stoi(str)) : 0;
    //initialize data to default values(0/NaN)
    reportDataInit();
    if(simulType_ == 1) {
        readCsvData("LOCATION_REPORT_FILE_NAME", csvData_, configParser);
    } else { // either 0 or default
        //intialize report with Canned values
        reportCannedDataInit();
    }
}

void ReportReader::readCsvData(std::string configVal, csvData &csvReportData,
    std::shared_ptr<SimulationConfigParser> configParser) {
    std::string fileName = configParser->getValue(configVal);
    std::shared_ptr<CsvHandler> csvHandler = std::make_shared<CsvHandler>(fileName);
    Status status = csvHandler->readCsv(csvReportData);
    if(status != Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " File opening error");
    }
}

ReportReader::~ReportReader() {
    LOG(DEBUG, __FUNCTION__);
}

ReportReader& ReportReader::getInstance() {
    static ReportReader MyReportReader;
    return (MyReportReader);
}

std::shared_ptr<GnssSignalInfo> ReportReader::getGnssSignalInfo() {
    return gnssSignalInfo_;
}

std::shared_ptr<GnssSVInfo> ReportReader::getGnssSVInfo() {
    return gnssSVInfo_;
}

void ReportReader::getLocationInfoBase(std::shared_ptr<LocationInfoBase> &ibase) {
    if(simulType_ == 1) {
        reportCsvIBase();
    }
    ibase = iBase_;
}

void ReportReader::getLocationInfoEx(std::shared_ptr<LocationInfoEx> & ibaseEx) {
    if(simulType_ == 1) {
        reportCsvIBaseEx();
    }
    ibaseEx = iBaseEx_;
}

std::vector<NMEAVals> ReportReader::getNmeaVal() {
    return defaultNmeaVals_;
}

struct GnssMeasurements &ReportReader::getGnssMeasurements() {
    return gnssMeasurements_;
}

struct LocationSystemInfo &ReportReader::getSystemInfoReport() {
    return locationSystemInfo_;
}

void ReportReader::reportDataInit() {
    // GnssSignalInfo Init
    gnssSignalInfo_ = std::make_shared<GnssSignalInfo>();
    struct GnssData gnssdata;
    auto maxGnssSignalTypes = telux::loc::GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES;
    for (auto i = 0; i < maxGnssSignalTypes; i++) {
        gnssdata.gnssDataMask[i] = 0;
        gnssdata.jammerInd[i] = 0;
        gnssdata.agc[i] = 0;
    }
    gnssSignalInfo_->setGnssData(gnssdata);
    // GnssSVInfo Init
    gnssSVInfo_ = std::make_shared<GnssSVInfo>();
    // GnssNmeaInfo Init
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

void ReportReader::reportCsvIBase() {
    if(!csvData_["location_basic_validity"].empty()) {
        iBase_->setLocationInfoValidity(stoul(csvData_["location_basic_validity"][0]));
        csvData_["location_basic_validity"].push_back(csvData_["location_basic_validity"][0]);
        csvData_["location_basic_validity"].erase(
            csvData_["location_basic_validity"].begin());
    }

    if(!csvData_["fix_mode"].empty()) {
        iBase_->setLocationTechnology(stoul(csvData_["fix_mode"][0]));
        csvData_["fix_mode"].push_back(csvData_["fix_mode"][0]);
        csvData_["fix_mode"].erase(csvData_["fix_mode"].begin());
    }

    if(!csvData_["horizontal_confidence"].empty()) {
        iBase_->setHorizontalUncertainty(stof(csvData_["horizontal_confidence"][0]));
        csvData_["horizontal_confidence"].push_back(csvData_["horizontal_confidence"][0]);
        csvData_["horizontal_confidence"].erase(
            csvData_["horizontal_confidence"].begin());
    }

    if(!csvData_["vertical_confidence"].empty()) {
        iBase_->setVerticalUncertainty(stof(csvData_["vertical_confidence"][0]));
        csvData_["vertical_confidence"].push_back(csvData_["vertical_confidence"][0]);
        csvData_["vertical_confidence"].erase(csvData_["vertical_confidence"].begin());
    }

    if(!csvData_["velocity"].empty()) {
        iBase_->setSpeed(stof(csvData_["velocity"][0]));
        csvData_["velocity"].push_back(csvData_["velocity"][0]);
        csvData_["velocity"].erase(csvData_["velocity"].begin());
    }

    if(!csvData_["latitude"].empty()) {
        iBase_->setLatitude(stod(csvData_["latitude"].front()));
        csvData_["latitude"].push_back(csvData_["latitude"][0]);
        csvData_["latitude"].erase(csvData_["latitude"].begin());
    }

    if(!csvData_["longitude"].empty()) {
        iBase_->setLongitude(stod(csvData_["longitude"][0]));
        csvData_["longitude"].push_back(csvData_["longitude"][0]);
        csvData_["longitude"].erase(csvData_["longitude"].begin());
    }

    if(!csvData_["altitude"].empty()){
        iBase_->setAltitude(stod(csvData_["altitude"][0]));
        csvData_["altitude"].push_back(csvData_["altitude"][0]);
        csvData_["altitude"].erase(csvData_["altitude"].begin());
    }

    if(!csvData_["heading"].empty()) {
        iBase_->setHeading(stof(csvData_["heading"][0]));
        csvData_["heading"].push_back(csvData_["heading"][0]);
        csvData_["heading"].erase(csvData_["heading"].begin());
    }

    if(!csvData_["utc_fix_time"].empty()) {
        iBase_->setUtcFixTime(stoull(csvData_["utc_fix_time"][0]));
        csvData_["utc_fix_time"].push_back(csvData_["utc_fix_time"][0]);
        csvData_["utc_fix_time"].erase(csvData_["utc_fix_time"].begin());
    }

    if(!csvData_["velocity_confidence"].empty()) {
        iBase_->setSpeedUncertainty(stof(csvData_["velocity_confidence"][0]));
        csvData_["velocity_confidence"].push_back(csvData_["velocity_confidence"][0]);
        csvData_["velocity_confidence"].erase(csvData_["velocity_confidence"].begin());
    }

    if(!csvData_["heading_confidence"].empty()) {
        iBase_->setHeadingUncertainty(stof(csvData_["heading_confidence"][0]));
        csvData_["heading_confidence"].push_back(csvData_["heading_confidence"][0]);
        csvData_["heading_confidence"].erase(csvData_["heading_confidence"].begin());
    }

    if(!csvData_["elapsed_realtime"].empty()) {
        iBase_->setElapsedRealTime(stoull(csvData_["elapsed_realtime"][0]));
        csvData_["elapsed_realtime"].push_back(csvData_["elapsed_realtime"][0]);
        csvData_["elapsed_realtime"].erase(csvData_["elapsed_realtime"].begin());
    }

    if(!csvData_["elapsed_realtime_unc"].empty()) {
        iBase_->setElapsedRealTimeUncertainty(stoull(csvData_["elapsed_realtime_unc"][0]));
        csvData_["elapsed_realtime_unc"].push_back(csvData_["elapsed_realtime_unc"][0]);
        csvData_["elapsed_realtime_unc"].erase(csvData_["elapsed_realtime_unc"].begin());
    }
}

void ReportReader::reportCsvIBaseEx() {
    if(!csvData_["location_basic_validity"].empty()) {
        iBaseEx_->setLocationInfoValidity(stoul(csvData_["location_basic_validity"][0]));
        csvData_["location_basic_validity"].push_back(csvData_["location_basic_validity"][0]);
        csvData_["location_basic_validity"].erase(
            csvData_["location_basic_validity"].begin());
    }

    if(!csvData_["fix_mode"].empty()) {
        iBaseEx_->setLocationTechnology(stoul(csvData_["fix_mode"][0]));
        csvData_["fix_mode"].push_back(csvData_["fix_mode"][0]);
        csvData_["fix_mode"].erase(csvData_["fix_mode"].begin());
    }

    if(!csvData_["horizontal_confidence"].empty()) {
        iBaseEx_->setHorizontalUncertainty(stof(csvData_["horizontal_confidence"][0]));
        csvData_["horizontal_confidence"].push_back(csvData_["horizontal_confidence"][0]);
        csvData_["horizontal_confidence"].erase(
            csvData_["horizontal_confidence"].begin());
    }

    if(!csvData_["vertical_confidence"].empty()) {
        iBaseEx_->setVerticalUncertainty(stof(csvData_["vertical_confidence"][0]));
        csvData_["vertical_confidence"].push_back(csvData_["vertical_confidence"][0]);
        csvData_["vertical_confidence"].erase(csvData_["vertical_confidence"].begin());
    }

    if(!csvData_["velocity"].empty()) {
        iBaseEx_->setSpeed(stof(csvData_["velocity"][0]));
        csvData_["velocity"].push_back(csvData_["velocity"][0]);
        csvData_["velocity"].erase(csvData_["velocity"].begin());
    }

    if(!csvData_["latitude"].empty()) {
        iBaseEx_->setLatitude(stod(csvData_["latitude"].front()));
        csvData_["latitude"].push_back(csvData_["latitude"][0]);
        csvData_["latitude"].erase(csvData_["latitude"].begin());
    }

    if(!csvData_["longitude"].empty()) {
        iBaseEx_->setLongitude(stod(csvData_["longitude"][0]));
        csvData_["longitude"].push_back(csvData_["longitude"][0]);
        csvData_["longitude"].erase(csvData_["longitude"].begin());
    }

    if(!csvData_["altitude"].empty()){
        iBaseEx_->setAltitude(stod(csvData_["altitude"][0]));
        csvData_["altitude"].push_back(csvData_["altitude"][0]);
        csvData_["altitude"].erase(csvData_["altitude"].begin());
    }

    if(!csvData_["heading"].empty()) {
        iBaseEx_->setHeading(stof(csvData_["heading"][0]));
        csvData_["heading"].push_back(csvData_["heading"][0]);
        csvData_["heading"].erase(csvData_["heading"].begin());
    }

    if(!csvData_["utc_fix_time"].empty()) {
        iBaseEx_->setUtcFixTime(stoull(csvData_["utc_fix_time"][0]));
        csvData_["utc_fix_time"].push_back(csvData_["utc_fix_time"][0]);
        csvData_["utc_fix_time"].erase(csvData_["utc_fix_time"].begin());
    }

    if(!csvData_["velocity_confidence"].empty()) {
        iBaseEx_->setSpeedUncertainty(stof(csvData_["velocity_confidence"][0]));
        csvData_["velocity_confidence"].push_back(csvData_["velocity_confidence"][0]);
        csvData_["velocity_confidence"].erase(csvData_["velocity_confidence"].begin());
    }

    if(!csvData_["heading_confidence"].empty()) {
        iBaseEx_->setHeadingUncertainty(stof(csvData_["heading_confidence"][0]));
        csvData_["heading_confidence"].push_back(csvData_["heading_confidence"][0]);
        csvData_["heading_confidence"].erase(csvData_["heading_confidence"].begin());
    }

    if(!csvData_["elapsed_realtime"].empty()) {
        iBaseEx_->setElapsedRealTime(stoull(csvData_["elapsed_realtime"][0]));
        csvData_["elapsed_realtime"].push_back(csvData_["elapsed_realtime"][0]);
        csvData_["elapsed_realtime"].erase(csvData_["elapsed_realtime"].begin());
    }

    if(!csvData_["elapsed_realtime_unc"].empty()) {
        iBaseEx_->setElapsedRealTimeUncertainty(stoull(csvData_["elapsed_realtime_unc"][0]));
        csvData_["elapsed_realtime_unc"].push_back(csvData_["elapsed_realtime_unc"][0]);
        csvData_["elapsed_realtime_unc"].erase(csvData_["elapsed_realtime_unc"].begin());
    }

    if(!csvData_["location_ex_validity"].empty()) {
        iBaseEx_->setLocationInfoExValidity(stoull(csvData_["location_ex_validity"][0]));
        csvData_["location_ex_validity"].push_back(csvData_["location_ex_validity"][0]);
        csvData_["location_ex_validity"].erase(
            csvData_["location_ex_validity"].begin());
    }

    if(!csvData_["altitude_mean_sealevel"].empty()) {
        iBaseEx_->setAltitudeMeanSeaLevel(stof(csvData_["altitude_mean_sealevel"][0]));
        csvData_["altitude_mean_sealevel"].push_back(csvData_["altitude_mean_sealevel"][0]);
        csvData_["altitude_mean_sealevel"].erase(
            csvData_["altitude_mean_sealevel"].begin());
    }

    if(!csvData_["position_dop"].empty()) {
        iBaseEx_->setPositionDop(stof(csvData_["position_dop"][0]));
        csvData_["position_dop"].push_back(csvData_["position_dop"][0]);
        csvData_["position_dop"].erase(csvData_["position_dop"].begin());
    }

    if(!csvData_["horizontal_dop"].empty()) {
        iBaseEx_->setHorizontalDop(stof(csvData_["horizontal_dop"][0]));
        csvData_["horizontal_dop"].push_back(csvData_["horizontal_dop"][0]);
        csvData_["horizontal_dop"].erase(csvData_["horizontal_dop"].begin());
    }

    if(!csvData_["vertical_dop"].empty()) {
        iBaseEx_->setVerticalDop(stof(csvData_["vertical_dop"][0]));
        csvData_["vertical_dop"].push_back(csvData_["vertical_dop"][0]);
        csvData_["vertical_dop"].erase(csvData_["vertical_dop"].begin());
    }

    if(!csvData_["geometric_dop"].empty()) {
        iBaseEx_->setGeometricDop(stof(csvData_["geometric_dop"][0]));
        csvData_["geometric_dop"].push_back(csvData_["geometric_dop"][0]);
        csvData_["geometric_dop"].erase(csvData_["geometric_dop"].begin());
    }

    if(!csvData_["time_dop"].empty()) {
        iBaseEx_->setTimeDop(stof(csvData_["time_dop"][0]));
        csvData_["time_dop"].push_back(csvData_["time_dop"][0]);
        csvData_["time_dop"].erase(csvData_["time_dop"].begin());
    }

    if(!csvData_["magnetic_deviation"].empty()) {
        iBaseEx_->setMagneticDeviation(stof(csvData_["magnetic_deviation"][0]));
        csvData_["magnetic_deviation"].push_back(csvData_["magnetic_deviation"][0]);
        csvData_["magnetic_deviation"].erase(
            csvData_["magnetic_deviation"].begin());
    }

    if(!csvData_["Horizontal_reliability"].empty()) {
        iBaseEx_->setHorizontalReliability(convertLocationReliability(
            stoi(csvData_["Horizontal_reliability"][0])));
        csvData_["Horizontal_reliability"].push_back(csvData_["Horizontal_reliability"][0]);
        csvData_["Horizontal_reliability"].erase(
            csvData_["Horizontal_reliability"].begin());
    }

    if(!csvData_["Vertical_reliability"].empty()) {
        iBaseEx_->setVerticalReliability(convertLocationReliability(
            stoi(csvData_["Vertical_reliability"][0])));
        csvData_["Vertical_reliability"].push_back(csvData_["Vertical_reliability"][0]);
        csvData_["Vertical_reliability"].erase(
            csvData_["Vertical_reliability"].begin());
    }

    if(!csvData_["HorizontalUncertainty_Semimajor"].empty()) {
        iBaseEx_->setHorizontalUncertaintySemiMajor(stof(
            csvData_["HorizontalUncertainty_Semimajor"][0]));
        csvData_["HorizontalUncertainty_Semimajor"].push_back(csvData_["HorizontalUncertainty_Semimajor"][0]);
        csvData_["HorizontalUncertainty_Semimajor"].erase(
            csvData_["HorizontalUncertainty_Semimajor"].begin());
    }

    if(!csvData_["HorizontalUncertainty_Semiminor"].empty()) {
        iBaseEx_->setHorizontalUncertaintySemiMinor(stof(
            csvData_["HorizontalUncertainty_Semiminor"][0]));
        csvData_["HorizontalUncertainty_Semiminor"].push_back(csvData_["HorizontalUncertainty_Semiminor"][0]);
        csvData_["HorizontalUncertainty_Semiminor"].erase(
            csvData_["HorizontalUncertainty_Semiminor"].begin());
    }

    if(!csvData_["HorizontalUncertainty_Azimuth"].empty()) {
        iBaseEx_->setHorizontalUncertaintyAzimuth(stof(
            csvData_["HorizontalUncertainty_Azimuth"][0]));
        csvData_["HorizontalUncertainty_Azimuth"].push_back(csvData_["HorizontalUncertainty_Azimuth"][0]);
        csvData_["HorizontalUncertainty_Azimuth"].erase(
            csvData_["HorizontalUncertainty_Azimuth"].begin());
    }

    if(!csvData_["HorizontalUncertainty_EastStandardDeviation"].empty()) {
        iBaseEx_->setEastStandardDeviation(stof(
            csvData_["HorizontalUncertainty_EastStandardDeviation"][0]));
        csvData_["HorizontalUncertainty_EastStandardDeviation"].push_back(csvData_["HorizontalUncertainty_EastStandardDeviation"][0]);
        csvData_["HorizontalUncertainty_EastStandardDeviation"].erase(
            csvData_["HorizontalUncertainty_EastStandardDeviation"].begin());
    }

    if(!csvData_["HorizontalUncertainty_NorthStandardDeviation"].empty()) {
        iBaseEx_->setNorthStandardDeviation(stof(
            csvData_["HorizontalUncertainty_NorthStandardDeviation"][0]));
        csvData_["HorizontalUncertainty_NorthStandardDeviation"].push_back(csvData_["HorizontalUncertainty_NorthStandardDeviation"][0]);
        csvData_["HorizontalUncertainty_NorthStandardDeviation"].erase(
            csvData_["HorizontalUncertainty_NorthStandardDeviation"].begin());
    }

    if(!csvData_["HorizontalUncertainty_NumSvUsed"].empty()) {
        iBaseEx_->setNumSvUsed(stoul(
            csvData_["HorizontalUncertainty_NumSvUsed"][0]));
        csvData_["HorizontalUncertainty_NumSvUsed"].push_back(csvData_["HorizontalUncertainty_NumSvUsed"][0]);
        csvData_["HorizontalUncertainty_NumSvUsed"].erase(
            csvData_["HorizontalUncertainty_NumSvUsed"].begin());
    }

    telux::loc::SvUsedInPosition svUsedInPosition;
    if(!csvData_["SVs_GPS"].empty()) {
        svUsedInPosition.gps = stoull(csvData_["SVs_GPS"][0]);
        csvData_["SVs_GPS"].push_back(csvData_["SVs_GPS"][0]);
        csvData_["SVs_GPS"].erase(csvData_["SVs_GPS"].begin());
    }
    if(!csvData_["SVs_GLONASS"].empty()) {
        svUsedInPosition.glo = stoull(csvData_["SVs_GLONASS"][0]);
        csvData_["SVs_GLONASS"].push_back(csvData_["SVs_GLONASS"][0]);
        csvData_["SVs_GLONASS"].erase(csvData_["SVs_GLONASS"].begin());
    }
    if(!csvData_["SVs_GALILEO"].empty()) {
        svUsedInPosition.gal = stoull(csvData_["SVs_GALILEO"][0]);
        csvData_["SVs_GALILEO"].push_back(csvData_["SVs_GALILEO"][0]);
        csvData_["SVs_GALILEO"].erase(csvData_["SVs_GALILEO"].begin());
    }
    if(!csvData_["SVs_BEIDOU"].empty()) {
        svUsedInPosition.bds = stoull(csvData_["SVs_BEIDOU"][0]);
        csvData_["SVs_BEIDOU"].push_back(csvData_["SVs_BEIDOU"][0]);
        csvData_["SVs_BEIDOU"].erase(csvData_["SVs_BEIDOU"].begin());
    }
    if(!csvData_["SVs_QZSS"].empty()) {
        svUsedInPosition.qzss = stoull(csvData_["SVs_QZSS"][0]);
        csvData_["SVs_QZSS"].push_back(csvData_["SVs_QZSS"][0]);
        csvData_["SVs_QZSS"].erase(csvData_["SVs_QZSS"].begin());
    }
    if(!csvData_["SVs_NAVIC"].empty()) {
        svUsedInPosition.navic = stoull(csvData_["SVs_NAVIC"][0]);
        csvData_["SVs_NAVIC"].push_back(csvData_["SVs_NAVIC"][0]);
        csvData_["SVs_NAVIC"].erase(csvData_["SVs_NAVIC"].begin());
    }
    iBaseEx_->setSvUsedInPosition(svUsedInPosition);

    std::vector<uint16_t> usedSvs = {10, 18, 23, 24, 27, 28, 29, 32, 65, 71, 72, 85, 86, 87, 202,
    205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 216, 221, 221, 224, 226, 226, 233, 234, 234,
    238, 238, 239, 239, 242, 242, 243, 243, 244, 244, 259, 260, 307, 313, 319, 326, 333, 195, 196};
    iBaseEx_->setUsedSVsIds(usedSvs);

    if(!csvData_["SBAS_Correction"].empty()) {
        std::bitset<SBAS_COUNT> sbas = stoull(csvData_["SBAS_Correction"][0]);
        iBaseEx_->setSbasCorrection(sbas);
        csvData_["SBAS_Correction"].push_back(csvData_["SBAS_Correction"][0]);
        csvData_["SBAS_Correction"].erase(csvData_["SBAS_Correction"].begin());
    }

    if(!csvData_["Position_Tech"].empty()) {
        iBaseEx_->setPositionTechnology(stoul(csvData_["Position_Tech"][0]));
        csvData_["Position_Tech"].push_back(csvData_["Position_Tech"][0]);
        csvData_["Position_Tech"].erase(csvData_["Position_Tech"].begin());
    }

    telux::loc::GnssKinematicsData bodyFrameData;
    if(!csvData_["BodyFrameData_latAccel"].empty()) {
        bodyFrameData.latAccel = stof(csvData_["BodyFrameData_latAccel"][0]);
        csvData_["BodyFrameData_latAccel"].push_back(csvData_["BodyFrameData_latAccel"][0]);
        csvData_["BodyFrameData_latAccel"].erase(
            csvData_["BodyFrameData_latAccel"].begin());
    }
    if(!csvData_["BodyFrameData_longAccel"].empty()) {
        bodyFrameData.longAccel = stof(csvData_["BodyFrameData_longAccel"][0]);
        csvData_["BodyFrameData_longAccel"].push_back(csvData_["BodyFrameData_longAccel"][0]);
        csvData_["BodyFrameData_longAccel"].erase(
            csvData_["BodyFrameData_longAccel"].begin());
    }
    if(!csvData_["BodyFrameData_vertAccel"].empty()) {
        bodyFrameData.vertAccel = stof(csvData_["BodyFrameData_vertAccel"][0]);
        csvData_["BodyFrameData_vertAccel"].push_back(csvData_["BodyFrameData_vertAccel"][0]);
        csvData_["BodyFrameData_vertAccel"].erase(
            csvData_["BodyFrameData_vertAccel"].begin());
    }
    if(!csvData_["BodyFrameData_yawRate"].empty()) {
        bodyFrameData.yawRate = stof(csvData_["BodyFrameData_yawRate"][0]);
        csvData_["BodyFrameData_yawRate"].push_back(csvData_["BodyFrameData_yawRate"][0]);
        csvData_["BodyFrameData_yawRate"].erase(
            csvData_["BodyFrameData_yawRate"].begin());
    }
    if(!csvData_["BodyFrameData_pitch"].empty()) {
        bodyFrameData.pitch = stof(csvData_["BodyFrameData_pitch"][0]);
        csvData_["BodyFrameData_pitch"].push_back(csvData_["BodyFrameData_pitch"][0]);
        csvData_["BodyFrameData_pitch"].erase(
            csvData_["BodyFrameData_pitch"].begin());
    }
    if(!csvData_["BodyFrameData_latAccelUnc"].empty()) {
        bodyFrameData.latAccelUnc = stof(csvData_["BodyFrameData_latAccelUnc"][0]);
        csvData_["BodyFrameData_latAccelUnc"].push_back(csvData_["BodyFrameData_latAccelUnc"][0]);
        csvData_["BodyFrameData_latAccelUnc"].erase(
            csvData_["BodyFrameData_latAccelUnc"].begin());
    }
    if(!csvData_["BodyFrameData_longAccelUnc"].empty()) {
        bodyFrameData.longAccelUnc = stof(csvData_["BodyFrameData_longAccelUnc"][0]);
        csvData_["BodyFrameData_longAccelUnc"].push_back(csvData_["BodyFrameData_longAccelUnc"][0]);
        csvData_["BodyFrameData_longAccelUnc"].erase(
            csvData_["BodyFrameData_longAccelUnc"].begin());
    }
    if(!csvData_["BodyFrameData_vertAccelUnc"].empty()) {
        bodyFrameData.vertAccelUnc = stof(csvData_["BodyFrameData_vertAccelUnc"][0]);
        csvData_["BodyFrameData_vertAccelUnc"].push_back(csvData_["BodyFrameData_vertAccelUnc"][0]);
        csvData_["BodyFrameData_vertAccelUnc"].erase(
            csvData_["BodyFrameData_vertAccelUnc"].begin());
    }
    if(!csvData_["BodyFrameData_yawRateUnc"].empty()) {
        bodyFrameData.yawRateUnc = stof(csvData_["BodyFrameData_yawRateUnc"][0]);
        csvData_["BodyFrameData_yawRateUnc"].push_back(csvData_["BodyFrameData_yawRateUnc"][0]);
        csvData_["BodyFrameData_yawRateUnc"].erase(
            csvData_["BodyFrameData_yawRateUnc"].begin());
    }
    if(!csvData_["BodyFrameData_pitchUnc"].empty()) {
        bodyFrameData.pitchUnc = stof(csvData_["BodyFrameData_pitchUnc"][0]);
        csvData_["BodyFrameData_pitchUnc"].push_back(csvData_["BodyFrameData_pitchUnc"][0]);
        csvData_["BodyFrameData_pitchUnc"].erase(
            csvData_["BodyFrameData_pitchUnc"].begin());
    }
    if(!csvData_["BodyFrameData_pitchRate"].empty()) {
        bodyFrameData.pitchRate = stof(csvData_["BodyFrameData_pitchRate"][0]);
        csvData_["BodyFrameData_pitchRate"].push_back(csvData_["BodyFrameData_pitchRate"][0]);
        csvData_["BodyFrameData_pitchRate"].erase(
            csvData_["BodyFrameData_pitchRate"].begin());
    }
    if(!csvData_["BodyFrameData_pitchRateUnc"].empty()) {
        bodyFrameData.pitchRateUnc = stof(csvData_["BodyFrameData_pitchRateUnc"][0]);
        csvData_["BodyFrameData_pitchRateUnc"].push_back(csvData_["BodyFrameData_pitchRateUnc"][0]);
        csvData_["BodyFrameData_pitchRateUnc"].erase(
            csvData_["BodyFrameData_pitchRateUnc"].begin());
    }
    if(!csvData_["BodyFrameData_roll"].empty()) {
        bodyFrameData.roll = stof(csvData_["BodyFrameData_roll"][0]);
        csvData_["BodyFrameData_roll"].push_back(csvData_["BodyFrameData_roll"][0]);
        csvData_["BodyFrameData_roll"].erase(
            csvData_["BodyFrameData_roll"].begin());
    }
    if(!csvData_["BodyFrameData_rollUnc"].empty()) {
        bodyFrameData.rollUnc = stof(csvData_["BodyFrameData_rollUnc"][0]);
        csvData_["BodyFrameData_rollUnc"].push_back(csvData_["BodyFrameData_rollUnc"][0]);
        csvData_["BodyFrameData_rollUnc"].erase(
            csvData_["BodyFrameData_rollUnc"].begin());
    }
    if(!csvData_["BodyFrameData_rollRate"].empty()) {
        bodyFrameData.rollRate = stof(csvData_["BodyFrameData_rollRate"][0]);
        csvData_["BodyFrameData_rollRate"].push_back(csvData_["BodyFrameData_rollRate"][0]);
        csvData_["BodyFrameData_rollRate"].erase(
            csvData_["BodyFrameData_rollRate"].begin());
    }
    if(!csvData_["BodyFrameData_rollRateUnc"].empty()) {
        bodyFrameData.rollRateUnc = stof(csvData_["BodyFrameData_rollRateUnc"][0]);
        csvData_["BodyFrameData_rollRateUnc"].push_back(csvData_["BodyFrameData_rollRateUnc"][0]);
        csvData_["BodyFrameData_rollRateUnc"].erase(
            csvData_["BodyFrameData_rollRateUnc"].begin());
    }
    if(!csvData_["BodyFrameData_yaw"].empty()) {
        bodyFrameData.yaw = stof(csvData_["BodyFrameData_yaw"][0]);
        csvData_["BodyFrameData_yaw"].push_back(csvData_["BodyFrameData_yaw"][0]);
        csvData_["BodyFrameData_yaw"].erase(
            csvData_["BodyFrameData_yaw"].begin());
    }
    if(!csvData_["BodyFrameData_yawUnc"].empty()) {
        bodyFrameData.yawUnc = stof(csvData_["BodyFrameData_yawUnc"][0]);
        csvData_["BodyFrameData_yawUnc"].push_back(csvData_["BodyFrameData_yawUnc"][0]);
        csvData_["BodyFrameData_yawUnc"].erase(
            csvData_["BodyFrameData_yawUnc"].begin());
    }
    if(!csvData_["BodyFrameDataValidity"].empty()) {
        bodyFrameData.bodyFrameDataMask = stoul(csvData_["BodyFrameDataValidity"][0]);
        csvData_["BodyFrameDataValidity"].push_back(csvData_["BodyFrameDataValidity"][0]);
        csvData_["BodyFrameDataValidity"].erase(
            csvData_["BodyFrameDataValidity"].begin());
    }
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
    if(!csvData_["GnssSystemTime_numClockResets"].empty()) {
        timeInfo.bds.numClockResets = stoul(csvData_["GnssSystemTime_numClockResets"][0]);
        csvData_["GnssSystemTime_numClockResets"].push_back(csvData_["GnssSystemTime_numClockResets"][0]);
        csvData_["GnssSystemTime_numClockResets"].erase(
            csvData_["GnssSystemTime_numClockResets"].begin());
    }
    if(!csvData_["GnssSystemTime_refFCount"].empty()) {
        timeInfo.bds.refFCount = stoul(csvData_["GnssSystemTime_refFCount"][0]);
        csvData_["GnssSystemTime_refFCount"].push_back(csvData_["GnssSystemTime_refFCount"][0]);
        csvData_["GnssSystemTime_refFCount"].erase(
            csvData_["GnssSystemTime_refFCount"].begin());
    }
    if(!csvData_["GnssSystemTime_systemClkTimeUncMs"].empty()) {
        timeInfo.bds.systemClkTimeUncMs = stof(
            csvData_["GnssSystemTime_systemClkTimeUncMs"][0]);
        csvData_["GnssSystemTime_systemClkTimeUncMs"].push_back(csvData_["GnssSystemTime_systemClkTimeUncMs"][0]);
        csvData_["GnssSystemTime_systemClkTimeUncMs"].erase(
            csvData_["GnssSystemTime_systemClkTimeUncMs"].begin());
    }
    if(!csvData_["GnssSystemTime_systemClkTimeBias"].empty()) {
        timeInfo.bds.systemClkTimeBias = stoul(
            csvData_["GnssSystemTime_systemClkTimeBias"][0]);
        csvData_["GnssSystemTime_systemClkTimeBias"].push_back(csvData_["GnssSystemTime_systemClkTimeBias"][0]);
        csvData_["GnssSystemTime_systemClkTimeBias"].erase(
            csvData_["GnssSystemTime_systemClkTimeBias"].begin());
    }
    if(!csvData_["GnssSystemTime_systemMsec"].empty()) {
        timeInfo.bds.systemMsec = stof(
            csvData_["GnssSystemTime_systemMsec"][0]);
        csvData_["GnssSystemTime_systemMsec"].push_back(csvData_["GnssSystemTime_systemMsec"][0]);
        csvData_["GnssSystemTime_systemMsec"].erase(
            csvData_["GnssSystemTime_systemMsec"].begin());
    }
    if(!csvData_["GnssSystemTime_systemWeek"].empty()) {
        timeInfo.bds.systemWeek = stoul(
            csvData_["GnssSystemTime_systemWeek"][0]);
        csvData_["GnssSystemTime_systemWeek"].push_back(csvData_["GnssSystemTime_systemWeek"][0]);
        csvData_["GnssSystemTime_systemWeek"].erase(
            csvData_["GnssSystemTime_systemWeek"].begin());
    }
    systemTime.time = timeInfo;
    iBaseEx_->setGnssSystemTime(systemTime);

    if(!csvData_["Time_UncMs"].empty()) {
        iBaseEx_->setTimeUncMs(stof(csvData_["Time_UncMs"][0]));
        csvData_["Time_UncMs"].push_back(csvData_["Time_UncMs"][0]);
        csvData_["Time_UncMs"].erase(csvData_["Time_UncMs"].begin());
    }

    if(!csvData_["LeapSeconds"].empty()) {
        iBaseEx_->setLeapSeconds(stoul(csvData_["LeapSeconds"][0]));
        csvData_["LeapSeconds"].push_back(csvData_["LeapSeconds"][0]);
        csvData_["LeapSeconds"].erase(csvData_["LeapSeconds"].begin());
    }

    if(!csvData_["CalibrationConfidencePercent"].empty()) {
        iBaseEx_->setCalibrationConfidencePercent(stoul(
            csvData_["CalibrationConfidencePercent"][0]));
        csvData_["CalibrationConfidencePercent"].push_back(csvData_["CalibrationConfidencePercent"][0]);
        csvData_["CalibrationConfidencePercent"].erase(
            csvData_["CalibrationConfidencePercent"].begin());
    }

    if(!csvData_["CalibrationStatus"].empty()) {
        iBaseEx_->setCalibrationStatus(stoul(csvData_["CalibrationStatus"][0]));
        csvData_["CalibrationStatus"].push_back(csvData_["CalibrationStatus"][0]);
        csvData_["CalibrationStatus"].erase(csvData_["CalibrationStatus"].begin());
    }

    if(!csvData_["ConformityIndex"].empty()) {
        iBaseEx_->setConformityIndex(stof(csvData_["ConformityIndex"][0]));
        csvData_["ConformityIndex"].push_back(csvData_["ConformityIndex"][0]);
        csvData_["ConformityIndex"].erase(csvData_["ConformityIndex"].begin());
    }

    if(!csvData_["Loc_EngType"].empty()) {
        iBaseEx_->setLocOutputEngType(
            static_cast<telux::loc::LocationAggregationType>(
                stoi(csvData_["Loc_EngType"][0])));
        csvData_["Loc_EngType"].push_back(csvData_["Loc_EngType"][0]);
        csvData_["Loc_EngType"].erase(csvData_["Loc_EngType"].begin());
    }

    if(!csvData_["Loc_EngineMask"].empty()) {
        iBaseEx_->setLocOutputEngMask(stoul(csvData_["Loc_EngineMask"][0]));
        csvData_["Loc_EngineMask"].push_back(csvData_["Loc_EngineMask"][0]);
        csvData_["Loc_EngineMask"].erase(csvData_["Loc_EngineMask"].begin());
    }

    if(!csvData_["DR_SolutionStatus"].empty()) {
        iBaseEx_->setSolutionStatus(stoul(csvData_["DR_SolutionStatus"][0]));
        csvData_["DR_SolutionStatus"].push_back(csvData_["DR_SolutionStatus"][0]);
        csvData_["DR_SolutionStatus"].erase(csvData_["DR_SolutionStatus"].begin());
    }

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

    telux::loc::LLAInfo llaVRPInfo = {0};
    if(!csvData_["VRPBasedLLA_Lat"].empty()) {
        llaVRPInfo.latitude = stod(csvData_["VRPBasedLLA_Lat"][0]);
        csvData_["VRPBasedLLA_Lat"].push_back(csvData_["VRPBasedLLA_Lat"][0]);
        csvData_["VRPBasedLLA_Lat"].erase(csvData_["VRPBasedLLA_Lat"].begin());
    }
    if(!csvData_["VRPBasedLLA_Long"].empty()) {
        llaVRPInfo.longitude = stod(csvData_["VRPBasedLLA_Long"][0]);
        csvData_["VRPBasedLLA_Long"].push_back(csvData_["VRPBasedLLA_Long"][0]);
        csvData_["VRPBasedLLA_Long"].erase(csvData_["VRPBasedLLA_Long"].begin());
    }
    if(!csvData_["VRPBasedLLA_Alt"].empty()) {
        llaVRPInfo.altitude = stod(csvData_["VRPBasedLLA_Alt"][0]);
        csvData_["VRPBasedLLA_Alt"].push_back(csvData_["VRPBasedLLA_Alt"][0]);
        csvData_["VRPBasedLLA_Alt"].erase(csvData_["VRPBasedLLA_Alt"].begin());
    }
    iBaseEx_->setVRPBasedLLA(llaVRPInfo);

    std::vector<float> enuVelocity(3);
    if(!csvData_["VRPBased_Eastvelocity"].empty()) {
        enuVelocity[0] = stof(csvData_["VRPBased_Eastvelocity"][0]);
        csvData_["VRPBased_Eastvelocity"].push_back(csvData_["VRPBased_Eastvelocity"][0]);
        csvData_["VRPBased_Eastvelocity"].erase(
            csvData_["VRPBased_Eastvelocity"].begin());
    }
    if(!csvData_["VRPBased_Northvelocity"].empty()) {
        enuVelocity[1] = stof(csvData_["VRPBased_Northvelocity"][0]);
        csvData_["VRPBased_Northvelocity"].push_back(csvData_["VRPBased_Northvelocity"][0]);
        csvData_["VRPBased_Northvelocity"].erase(
            csvData_["VRPBased_Northvelocity"].begin());
    }
    if(!csvData_["VRPBased_Upvelocity"].empty()) {
        enuVelocity[2] = stof(csvData_["VRPBased_Upvelocity"][0]);
        csvData_["VRPBased_Upvelocity"].push_back(csvData_["VRPBased_Upvelocity"][0]);
        csvData_["VRPBased_Upvelocity"].erase(
            csvData_["VRPBased_Upvelocity"].begin());
    }
    iBaseEx_->setVRPBasedENUVelocity(enuVelocity);

    if(!csvData_["AltitudeType"].empty()) {
        iBaseEx_->setAltitudeType(convertAltitudeType(stoi(csvData_["AltitudeType"][0])));
        csvData_["AltitudeType"].push_back(csvData_["AltitudeType"][0]);
        csvData_["AltitudeType"].erase(csvData_["AltitudeType"].begin());
    }

    iBaseEx_->setReportStatus(telux::loc::ReportStatus::SUCCESS);

    if(!csvData_["IntegrityRiskUsed"].empty()) {
        iBaseEx_->setIntegrityRiskUsed(stoul(csvData_["IntegrityRiskUsed"][0]));
        csvData_["IntegrityRiskUsed"].push_back(csvData_["IntegrityRiskUsed"][0]);
        csvData_["IntegrityRiskUsed"].erase(csvData_["IntegrityRiskUsed"].begin());
    }

    if(!csvData_["ProtectionLevelAlongTrack"].empty()) {
        iBaseEx_->setProtectionLevelAlongTrack(stof(
            csvData_["ProtectionLevelAlongTrack"][0]));
        csvData_["ProtectionLevelAlongTrack"].push_back(csvData_["ProtectionLevelAlongTrack"][0]);
        csvData_["ProtectionLevelAlongTrack"].erase(
            csvData_["ProtectionLevelAlongTrack"].begin());
    }

    if(!csvData_["ProtectionLevelCrossTrack"].empty()) {
        iBaseEx_->setProtectionLevelCrossTrack(stof(
            csvData_["ProtectionLevelCrossTrack"][0]));
        csvData_["ProtectionLevelCrossTrack"].push_back(csvData_["ProtectionLevelCrossTrack"][0]);
        csvData_["ProtectionLevelCrossTrack"].erase(
            csvData_["ProtectionLevelCrossTrack"].begin());
    }

    if(!csvData_["ProtectionLevelVertical"].empty()) {
        iBaseEx_->setProtectionLevelVertical(stof(
            csvData_["ProtectionLevelVertical"][0]));
        csvData_["ProtectionLevelVertical"].push_back(csvData_["ProtectionLevelVertical"][0]);
        csvData_["ProtectionLevelVertical"].erase(
            csvData_["ProtectionLevelVertical"].begin());
    }
}

telux::loc::LocationReliability ReportReader::convertLocationReliability(
    size_t newLocReliability) {
  telux::loc::LocationReliability oldLocReliability = LocationReliability::UNKNOWN;
  switch (newLocReliability) {
    case -1:
         oldLocReliability = LocationReliability::UNKNOWN;
         break;
    case 0 :
         oldLocReliability = LocationReliability::NOT_SET;
         break;
    case 1 :
         oldLocReliability = LocationReliability::VERY_LOW;
         break;
    case 2 :
         oldLocReliability = LocationReliability::LOW;
         break;
    case 3 :
         oldLocReliability = LocationReliability::MEDIUM;
         break;
    case 4 :
         oldLocReliability = LocationReliability::HIGH;
         break;
  }
  return oldLocReliability;
}

telux::loc::AltitudeType ReportReader::convertAltitudeType(bool type) {
  telux::loc::AltitudeType altitudeType = telux::loc::AltitudeType::UNKNOWN;
  if (type == true) {
    altitudeType = telux::loc::AltitudeType::ASSUMED;
  } else {
    altitudeType = telux::loc::AltitudeType::CALCULATED;
  }
  return altitudeType;
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
    svUser.gps = 2629960192;
    svUser.glo = 7340225;
    svUser.gal = 4328788032;
    svUser.bds = 864706946862727154;
    svUser.qzss = 12;
    svUser.navic = 0;
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
    gnssMeasInfo.gnssSignalType = 1;  // GPS_L1CA
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
    timeInfo.bds.systemClkTimeBias = 0.00743252;
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

}

void reportCannedGnssSignalInfoInit(std::shared_ptr<GnssSignalInfo> gnssSignalInfo_) {
    auto gnssdata = gnssSignalInfo_->getGnssData();
    gnssdata.gnssDataMask[7] = 3;
    gnssdata.jammerInd[7] = 32;
    gnssdata.agc[7] = 2.24;
    gnssdata.gnssDataMask[9] = 3;
    gnssdata.jammerInd[9] = 56;
    gnssdata.agc[9] = 1.44;
    gnssdata.gnssDataMask[15] = 3;
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
    GnssMeasurementsDataValidity validMask = 0;
    measurementsData.svId = 24;
    validMask |= SV_ID_BIT;
    measurementsData.svType = GnssConstellationType::GPS;
    validMask |= SV_TYPE_BIT;
    measurementsData.timeOffsetNs = 0;
    GnssMeasurementsStateValidity validSMask = 0;
    validSMask |= CODE_LOCK_BIT;
    validSMask |= BIT_SYNC_BIT;
    validSMask |= SUBFRAME_SYNC_BIT;
    validSMask |= TOW_DECODED_BIT;
    measurementsData.stateMask = validSMask;
    measurementsData.receivedSvTimeNs = 388401344958305;
    validMask |= RECEIVED_SV_TIME_BIT;
    measurementsData.receivedSvTimeUncertaintyNs = 31;
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
    measurementsData.valid = validMask;
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
    locationSystemInfo_.info.current = (uint8_t)18;

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

}  // namespace loc
}  // namespace telux
