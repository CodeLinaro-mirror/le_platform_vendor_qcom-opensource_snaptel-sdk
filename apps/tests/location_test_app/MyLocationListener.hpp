/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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

#ifndef MYLOCATIONLISTENER_HPP
#define MYLOCATIONLISTENER_HPP

#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationListener.hpp>

class MyLocationListener : public telux::loc::ILocationListener, public
    telux::loc::ILocationSystemInfoListener {
public:
   void onBasicLocationUpdate(
      const std::shared_ptr<telux::loc::ILocationInfoBase> &locationInfo) override;
   void onDetailedLocationUpdate(
      const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) override;

   void onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) override;

   void onGnssSignalInfo(const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) override;

   void onGnssNmeaInfo(uint64_t timestamp, const std::string &nmea) override;

   void onDetailedEngineLocationUpdate(const std::vector<std::shared_ptr<telux::loc::ILocationInfoEx>>
       &locationEngineInfo) override;

   void onGnssMeasurementsInfo(const telux::loc::GnssMeasurements &measurementInfo) override;

   void onLocationSystemInfo(const telux::loc::LocationSystemInfo &locationSystemInfo) override;

   void setDetailedLocationReportFlag(bool enable);
   void setDetailedEngineLocReportFlag(bool enable);
   void setBasicLocationReportFlag(bool enable);
   void setSvInfoFlag(bool enable);
   void setDataInfoFlag(bool enable);
   void setNmeaInfoFlag(bool enable);
   void setMeasurementsInfoFlag(bool enable);
   void setLocSystemInfoFlag(bool enable);

   ~MyLocationListener() {
   }

private:
   bool isSvInfoFlagEnabled_ = false, isDetailedReportFlagEnabled_ = false;
   bool isBasicReportFlagEnabled_ = false, isDataInfoFlagEnabled_ = false;
   bool isNmeaInfoFlagEnabled_ = false, isDetailedEngineReportFlagEnabled_ = false;
   bool isMeasurementsInfoFlagEnabled_ = false;
   bool isLocSysInfoFlagEnabled_ = false;
   void printSbasCorrectionEx(std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo);
   void printHorizontalReliability(telux::loc::LocationReliability locReliability);
   void printLocationPositionTech(std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo);
   void printLocationPositionDynamics(std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo);
   void printGnssMeasurementInfo(std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo);
   void printGnssSystemTime(std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo);
   void printLocationValidity(telux::loc::LocationInfoValidity validityMask);
   void printLocationExValidity(telux::loc::LocationInfoExValidity validityMask);
   void printLocationTech(telux::loc::LocationTechnology techMask);
   void printVerticalReliability(telux::loc::LocationReliability locReliability);
   void printConstellationType(telux::loc::GnssConstellationType constellation);
   void printGnssSignalType(telux::loc::GnssSignal signalTypeMask);
   void printEphimerisAvailability(telux::loc::SVInfoAvailability availability);
   void printAlmanacAvailability(telux::loc::SVInfoAvailability availability);
   void printFixAvailability(telux::loc::SVInfoAvailability availability);
   void printSvUsedInPosition(telux::loc::SvUsedInPosition svUsedInPosition);
   void printCalibrationStatus(std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo);
   void printLocOutputEngineType(std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo);
   void printLocOutputEngineMask(std::shared_ptr<telux::loc::ILocationInfoEx> locationInfo);
   void printMeasurementsClockValidity(telux::loc::GnssMeasurementsClockValidity flags);
   void printMeasurementsDataValidity(telux::loc::GnssMeasurementsDataValidity flags);
   void printMeasurementState(telux::loc::GnssMeasurementsStateValidity mask);
   void printMeasurementAdrState(telux::loc::GnssMeasurementsAdrStateValidity mask);
   void printMeasurementsMultipathIndicator(telux::loc::
       GnssMeasurementsMultipathIndicator indicator);
   void printLLAVRPBasedInfo(telux::loc::LLAInfo llaInfo);
   void printENUVelocityVRPBased(std::vector<float> enuVelocityVRPBased);
};

#endif  // MYLOCATIONLISTENER_HPP
