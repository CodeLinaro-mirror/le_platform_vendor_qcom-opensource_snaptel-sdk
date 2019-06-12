/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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

class MyLocationListener : public telux::loc::ILocationListener {
public:
   void onLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfo> &locationInfo) override;

   void onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) override;

   std::string logSessionStatus(telux::loc::SessionStatus sessionStatus);
   void setLocationReportFlag(bool enable);
   void setSvInfoFlag(bool enable);

   ~MyLocationListener() {
   }

private:
   bool isSvInfoFlagEnabled_ = false;
   bool isLocReportFlagEnabled_ = false;
   bool isTimerExpired = false;
   void printOnLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfo> &locationInfo);
   void printSbasCorrection(std::shared_ptr<telux::loc::ILocationInfo> locationInfo);
   void printPositionTech(std::shared_ptr<telux::loc::ILocationInfo> locationInfo);
   void printMeasurementType(std::shared_ptr<telux::loc::ILocationInfo> locationInfo);
   void printHorizontalReliability(telux::loc::LocationReliability locReliability);
   void printVerticalReliability(telux::loc::LocationReliability locReliability);
   void printSensorType(telux::loc::SensorType sensorType);
   void printAltitudeType(telux::loc::AltitudeType altitudeType);
   void printConstellationType(telux::loc::GnssConstellationType constellation);
   void printSVHealthStatus(telux::loc::SVHealthStatus healthStatus);
   void printSVStatus(telux::loc::SVStatus svStatus);
   void printEphimerisAvailability(telux::loc::SVInfoAvailability availability);
   void printAlmanacAvailability(telux::loc::SVInfoAvailability availability);
};

#endif  // MYLOCATIONLISTENER_HPP
