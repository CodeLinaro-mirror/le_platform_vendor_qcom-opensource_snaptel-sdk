/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#include <bitset>
#include <iostream>
#include <memory>

#include <telux/loc/LocationDefines.hpp>

#include "MyLocationListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

std::string MyLocationListener::logSessionStatus(telux::loc::SessionStatus sessionStatus) {
   std::string sessionStatusString = "UNKNOWN";
   switch(sessionStatus) {
      case telux::loc::SessionStatus::SUCCESS:
         sessionStatusString = "SUCCESS";
         break;
      case telux::loc::SessionStatus::IN_PROGRESS:
         sessionStatusString = "IN_PROGRESS";
         break;
      case telux::loc::SessionStatus::GENERAL_FAILURE:
         sessionStatusString = "GENERAL_FAILURE";
         break;
      case telux::loc::SessionStatus::TIMEOUT:
         sessionStatusString = "TIMEOUT";
         break;
      case telux::loc::SessionStatus::USER_END:
         sessionStatusString = "USER_END";
         break;
      case telux::loc::SessionStatus::BAD_PARAMETER:
         sessionStatusString = "BAD_PARAMETER";
         break;
      case telux::loc::SessionStatus::PHONE_OFFLINE:
         sessionStatusString = "PHONE_OFFLINE";
         break;
      case telux::loc::SessionStatus::ENGINE_LOCKED:
         sessionStatusString = "ENGINE_LOCKED";
         break;
      default:
         break;
   }
   return sessionStatusString;
}

void MyLocationListener::printSbasCorrection(
   std::shared_ptr<telux::loc::ILocationInfo> locationInfo) {
   telux::loc::SbasCorrection correction = locationInfo->getSbasCorrection();
   if(correction[telux::loc::SBAS_CORRECTION_IONO]) {
      std::cout << "SBAS ionospheric correction is used" << std::endl;
   }

   if(correction[telux::loc::SBAS_CORRECTION_FAST]) {
      std::cout << "SBAS fast correction is used" << std::endl;
   }

   if(correction[telux::loc::SBAS_CORRECTION_LONG]) {
      std::cout << "SBAS long correction is used" << std::endl;
   }

   if(correction[telux::loc::SBAS_INTEGRITY]) {
      std::cout << "SBAS integrity information is used" << std::endl;
   }
}

void MyLocationListener::printPositionTech(std::shared_ptr<telux::loc::ILocationInfo> locationInfo) {
   telux::loc::PositionTech positionTech = locationInfo->getPositionTechnology();
   std::cout << "Position Technologies used: ";

   if(positionTech[telux::loc::SATELLITE]) {
      std::cout << "SATELLITE ";
   }

   if(positionTech[telux::loc::CELLID]) {
      std::cout << "CELLID ";
   }

   if(positionTech[telux::loc::WIFI]) {
      std::cout << "WIFI ";
   }

   if(positionTech[telux::loc::SENSORS]) {
      std::cout << "SENSORS ";
   }

   if(positionTech[telux::loc::REFERENCE_LOCATION]) {
      std::cout << "REFERENCE_LOCATION ";
   }

   if(positionTech[telux::loc::INJECTED_COARSE_POSITION]) {
      std::cout << "INJECTED_COARSE_POSITION ";
   }

   if(positionTech[telux::loc::AFLT]) {
      std::cout << "AFLT ";
   }

   if(positionTech[telux::loc::HYBRID]) {
      std::cout << "HYBRID";
   }
   std::cout << std::endl;
}

void MyLocationListener::printMeasurementType(
   std::shared_ptr<telux::loc::ILocationInfo> locationInfo) {
   std::shared_ptr<telux::loc::ISensorDataUsage> sensorDataUsage;
   std::cout << std::endl << std::endl;
   if(sensorDataUsage != nullptr) {
      std::cout << "Sensor data usage: " << std::endl;
      if(locationInfo->getSensorDataUsage(sensorDataUsage) == telux::common::Status::SUCCESS) {
         telux::loc::Measurement measurement = sensorDataUsage->getMeasurement();
         std::cout << "Measurement type: " << std::endl;

         if(measurement[telux::loc::HEADING]) {
            std::cout << "HEADING" << std::endl;
         }
         if(measurement[telux::loc::SPEED]) {
            std::cout << "SPEED" << std::endl;
         }
         if(measurement[telux::loc::POSITION]) {
            std::cout << "POSITION" << std::endl;
         }
         if(measurement[telux::loc::VELOCITY]) {
            std::cout << "VELOCITY" << std::endl;
         }
      }
   }
}

void MyLocationListener::printHorizontalReliability(telux::loc::LocationReliability locReliability) {
   switch(locReliability) {
      case telux::loc::LocationReliability::NOT_SET:
         std::cout << "Horizontal reliability: NOT_SET" << std::endl;
         break;
      case telux::loc::LocationReliability::VERY_LOW:
         std::cout << "Horizontal reliability: VERY_LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::LOW:
         std::cout << "Horizontal reliability: LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::MEDIUM:
         std::cout << "Horizontal reliability: MEDIUM" << std::endl;
         break;
      case telux::loc::LocationReliability::HIGH:
         std::cout << "Horizontal reliability: HIGH" << std::endl;
         break;
      default:
         std::cout << "Horizontal reliability is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printVerticalReliability(telux::loc::LocationReliability locReliability) {
   switch(locReliability) {
      case telux::loc::LocationReliability::NOT_SET:
         std::cout << "Vertical reliability: NOT_SET" << std::endl;
         break;
      case telux::loc::LocationReliability::VERY_LOW:
         std::cout << "Vertical reliability: VERY_LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::LOW:
         std::cout << "Vertical reliability: LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::MEDIUM:
         std::cout << "Vertical reliability: MEDIUM" << std::endl;
         break;
      case telux::loc::LocationReliability::HIGH:
         std::cout << "Vertical reliability: HIGH" << std::endl;
         break;
      default:
         std::cout << "Vertical reliability is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printSensorType(telux::loc::SensorType sensorType) {
   std::cout << std::endl << std::endl;
   switch(sensorType) {
      case telux::loc::SensorType::ACCELEROMETER:
         std::cout << "Sensor type: ACCELEROMETER" << std::endl;
         break;
      case telux::loc::SensorType::GYROSCOPE:
         std::cout << "Sensor type: GYROSCOPE" << std::endl;
         break;
      default:
         std::cout << "Sensor type is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printAltitudeType(telux::loc::AltitudeType altitudeType) {
   switch(altitudeType) {
      case telux::loc::AltitudeType::CALCULATED:
         std::cout << "Altitude type: CALCULATED, ";
         break;
      case telux::loc::AltitudeType::ASSUMED:
         std::cout << "Altitude type: ASSUMED, ";
         break;
      default:
         std::cout << "Altitude type is UNKNOWN, ";
   }
}

void MyLocationListener::printConstellationType(telux::loc::GnssConstellationType constellation) {
   switch(constellation) {
      case telux::loc::GnssConstellationType::GPS:
         std::cout << "Constellation type: GPS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::GALILEO:
         std::cout << "Constellation type: GALILEO" << std::endl;
         break;
      case telux::loc::GnssConstellationType::SBAS:
         std::cout << "Constellation type: SBAS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::COMPASS:
         std::cout << "Constellation type: COMPASS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::GLONASS:
         std::cout << "Constellation type: GLONASS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::BDS:
         std::cout << "Constellation type: BDS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::QZSS:
         std::cout << "Constellation type: QZSS" << std::endl;
         break;
      default:
         std::cout << "Constellation type: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printSVHealthStatus(telux::loc::SVHealthStatus healthStatus) {
   switch(healthStatus) {
      case telux::loc::SVHealthStatus::UNHEALTHY:
         std::cout << "SV health status: UNHEALTHY, ";
         break;
      case telux::loc::SVHealthStatus::HEALTHY:
         std::cout << "SV health status: HEALTHY, ";
         break;
      default:
         std::cout << "SV health status: UNKNOWN, ";
   }
}
void MyLocationListener::printSVStatus(telux::loc::SVStatus svStatus) {
   switch(svStatus) {
      case telux::loc::SVStatus::IDLE:
         std::cout << "SV status: IDLE" << std::endl;
         break;
      case telux::loc::SVStatus::SEARCH:
         std::cout << "SV status: SEARCH" << std::endl;
         break;
      case telux::loc::SVStatus::TRACK:
         std::cout << "SV status: TRACK" << std::endl;
         break;
      default:
         std::cout << "SV status: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printEphimerisAvailability(telux::loc::SVInfoAvailability availability) {
   switch(availability) {
      case telux::loc::SVInfoAvailability::YES:
         std::cout << "Ephemeris availability: YES, ";
         break;
      case telux::loc::SVInfoAvailability::NO:
         std::cout << "Ephemeris availability: NO,  ";
         break;
      default:
         std::cout << "Ephemeris availability: UNKNOWN, ";
   }
}

void MyLocationListener::printAlmanacAvailability(telux::loc::SVInfoAvailability availability) {
   switch(availability) {
      case telux::loc::SVInfoAvailability::YES:
         std::cout << "Almanac availability: YES" << std::endl;
         break;
      case telux::loc::SVInfoAvailability::NO:
         std::cout << "Almanac availability: NO " << std::endl;
         break;
      default:
         std::cout << "Almanac availability: UNKNOWN" << std::endl;
   }
}

void MyLocationListener::onLocationUpdate(
   const std::shared_ptr<telux::loc::ILocationInfo> &locationInfo) {
   isTimerExpired = true;
   if(!isLocReportFlagEnabled_) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n*********************** Location Report *********************"
                      << std::endl;
   time_t realtime;
   realtime = (time_t)((locationInfo->getTimeStamp() / 1000));
   std::cout << "Time stamp: " << locationInfo->getTimeStamp() << " mSec" << std::endl;
   std::cout << "GMT Time stamp: " << ctime(&realtime);
   std::cout << "Session status: " << logSessionStatus(locationInfo->getSessionStatus())
             << std::endl;
   printPositionTech(locationInfo);
   std::cout
      << "Latitude: " << locationInfo->getLatitude()
      << "  Longitude: " << locationInfo->getLongitude() << std::endl
      << "Altitude: " << locationInfo->getAltitude() << std::endl
      << "Heading: " << locationInfo->getHeading() << std::endl
      << "Vertical uncertainty: " << locationInfo->getVerticalUncertainty() << std::endl
      << "Altitude with respect to mean sea level: " << locationInfo->getAltitudeMeanSeaLevel()
      << std::endl
      << "Position DOP: " << locationInfo->getPositionDop() << std::endl
      << "Horizontal DOP: " << locationInfo->getHorizontalDop() << std::endl
      << "Vertical DOP: " << locationInfo->getVerticalDop() << std::endl
      << "Magnetic deviation: " << locationInfo->getMagneticDeviation() << std::endl
      << "Speed uncertainty: " << locationInfo->getSpeedUncertainty() << std::endl
      << "Heading uncertainty: " << locationInfo->getHeadingUncertainty() << std::endl
      << "HorizontalUncertainty\nSemiMajor: " << locationInfo->getHorizontalUncertaintySemiMajor()
      << ", SemiMinor: " << locationInfo->getHorizontalUncertaintySemiMinor()
      << ", Azimuth: " << locationInfo->getHorizontalUncertaintyAzimuth() << std::endl;
   printHorizontalReliability(locationInfo->getHorizontalReliability());
   printVerticalReliability(locationInfo->getVerticalReliability());
   std::vector<uint16_t> SVIds;
   locationInfo->getSVIds(SVIds);
   if(SVIds.size() > 0) {
      std::cout << "Ids of used SVs : " << std::endl;
   }
   for(auto i = 0; i < SVIds.size() - 1; ++i) {
      std::cout << SVIds[i] << ", ";
   }
   if(SVIds.size() > 0) {
      std::cout << SVIds[SVIds.size() - 1] << std::endl;
   }
   printSbasCorrection(locationInfo);

   uint8_t leapSeconds;
   if(locationInfo->getLeapSeconds(leapSeconds) == telux::common::Status::SUCCESS) {
      std::cout << "Leap seconds: " << static_cast<int>(leapSeconds) << std::endl;
   }

   if(locationInfo->getGpsTime() != nullptr) {
      auto locGpsTime = locationInfo->getGpsTime();
      std::cout << "Current GPS week: " << locGpsTime->getWeek() << std::endl;
      std::cout << "GPS week in milliseconds: " << locGpsTime->getTimeOfWeekMsec() << std::endl;
   }

   float circularHorizontalUncertainty;
   if(locationInfo->getCircularHorizontalUncertainty(circularHorizontalUncertainty)
      == telux::common::Status::SUCCESS) {
      std::cout << "Circular horizontal uncertainty: " << circularHorizontalUncertainty
                << std::endl;
   }

   uint8_t horizontalConfidence;
   if(locationInfo->getHorizontalConfidence(horizontalConfidence)
      == telux::common::Status::SUCCESS) {
      std::cout << "Horizontal uncertainty confidence: " << unsigned(horizontalConfidence)
                << std::endl;
   }

   std::cout << "Horizontal speed: " << locationInfo->getHorizontalSpeed() << std::endl;

   uint8_t verticalConfidence;
   if(locationInfo->getVerticalConfidence(verticalConfidence) == telux::common::Status::SUCCESS) {
      std::cout << "Vertical uncertainty confidence: " << unsigned(verticalConfidence) << std::endl;
   }

   std::cout << "Vertical speed: " << locationInfo->getVerticalSpeed() << std::endl;

   std::shared_ptr<telux::loc::ISensorDataUsage> sensorDataUsage;
   if(sensorDataUsage != nullptr) {
      std::cout << "Sensor data usage" << std::endl;
      if(locationInfo->getSensorDataUsage(sensorDataUsage) == telux::common::Status::SUCCESS) {
         std::cout << "Status of getSensorDataUsage: "
                   << (int)locationInfo->getSensorDataUsage(sensorDataUsage) << std::endl;
         printSensorType(sensorDataUsage->getSensorType());
         printMeasurementType(locationInfo);
      }
   }

   uint32_t fixId;
   if(locationInfo->getFixId(fixId) == telux::common::Status::SUCCESS) {
      std::cout << "Fix Id: " << fixId << std::endl;
   }

   std::vector<float> velocityEastNorthUp;
   if(locationInfo->getVelocityEastNorthUp(velocityEastNorthUp) == telux::common::Status::SUCCESS) {
      std::cout << "East, North, Up velocity: ";
      for(auto i = 0; i < velocityEastNorthUp.size() - 1; ++i) {
         std::cout << velocityEastNorthUp[i] << ", ";
      }
      if(velocityEastNorthUp.size() > 0) {
         std::cout << velocityEastNorthUp[velocityEastNorthUp.size() - 1];
      }
      std::cout << std::endl;
   }

   std::vector<float> velocityUncertaintyEastNorthUp;
   if(locationInfo->getVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp)
      == telux::common::Status::SUCCESS) {
      std::cout << "East, North, Up velocity uncertainty: " << std::endl;
      for(auto i = 0; i < velocityEastNorthUp.size() - 1; ++i) {
         std::cout << velocityUncertaintyEastNorthUp[i] << ", ";
      }
      if(velocityEastNorthUp.size() > 0) {
         std::cout << velocityUncertaintyEastNorthUp[velocityEastNorthUp.size() - 1];
      }
      std::cout << std::endl;
   }
   std::cout << "*************************************************************" << std::endl;
}

void MyLocationListener::onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
   if(!isSvInfoFlagEnabled_ || !isTimerExpired) {
      return;
   }
   std::cout << std::endl;
   PRINT_NOTIFICATION << "\n**************** Satellite Vehicle Information ***************"
                      << std::endl;
   printAltitudeType(gnssSVInfo->getAltitudeType());
   for(auto svInfo : gnssSVInfo->getSVInfoList()) {
      std::cout << "**** GNSS SV Id : " << svInfo->getId() << " ****" << std::endl;
      printConstellationType(svInfo->getConstellation());
      printSVHealthStatus(svInfo->getSVHealthStatus());
      printSVStatus(svInfo->getStatus());
      printEphimerisAvailability(svInfo->getHasEphemeris());
      printAlmanacAvailability(svInfo->getHasAlmanac());
      std::cout << "Elevation: " << svInfo->getElevation() << ", Azimuth: " << svInfo->getAzimuth()
                << ", SNR: " << svInfo->getSnr() << std::endl;
   }
   isTimerExpired = false;
   std::cout << "*************************************************************" << std::endl;
}

void MyLocationListener::setLocationReportFlag(bool enable) {
   isLocReportFlagEnabled_ = enable;
}

void MyLocationListener::setSvInfoFlag(bool enable) {
   isSvInfoFlagEnabled_ = enable;
}
