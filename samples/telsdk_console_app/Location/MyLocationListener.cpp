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

#include <iostream>
#include <memory>
#include <bitset>

#include <telux/loc/LocationDefines.hpp>

#include "MyLocationListener.hpp"

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"

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
   std::cout << std::endl << std::endl;
   telux::loc::SbasCorrection correction = locationInfo->getSbasCorrection();
   if(correction[telux::loc::SBAS_CORRECTION_IONO]) {
      print_notification << "SBAS ionospheric correction is used" << std::endl;
   }

   if(correction[telux::loc::SBAS_CORRECTION_FAST]) {
      print_notification << "SBAS fast correction is used" << std::endl;
   }

   if(correction[telux::loc::SBAS_CORRECTION_LONG]) {
      print_notification << "SBAS long correction is used" << std::endl;
   }

   if(correction[telux::loc::SBAS_INTEGRITY]) {
      print_notification << "SBAS integrity information is used" << std::endl;
   }
}

void MyLocationListener::printPositionTech(std::shared_ptr<telux::loc::ILocationInfo> locationInfo) {
   telux::loc::PositionTech positionTech = locationInfo->getPositionTechnology();
   print_notification << "Position Technologies used: ";

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
      print_notification << "Sensor data usage: " << std::endl;
      if(locationInfo->getSensorDataUsage(sensorDataUsage) == telux::common::Status::SUCCESS) {
         telux::loc::Measurement measurement = sensorDataUsage->getMeasurement();
         print_notification << "Measurement type: " << std::endl;

         if(measurement[telux::loc::HEADING]) {
            print_notification << "HEADING" << std::endl;
         }
         if(measurement[telux::loc::SPEED]) {
            print_notification << "SPEED" << std::endl;
         }
         if(measurement[telux::loc::POSITION]) {
            print_notification << "POSITION" << std::endl;
         }
         if(measurement[telux::loc::VELOCITY]) {
            print_notification << "VELOCITY" << std::endl;
         }
      }
   }
}

void MyLocationListener::printHorizontalReliability(telux::loc::LocationReliability locReliability) {
   std::cout << std::endl << std::endl;
   switch(locReliability) {
      case telux::loc::LocationReliability::NOT_SET:
         print_notification << "Horizontal reliability: NOT_SET" << std::endl;
         break;
      case telux::loc::LocationReliability::VERY_LOW:
         print_notification << "Horizontal reliability: VERY_LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::LOW:
         print_notification << "Horizontal reliability: LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::MEDIUM:
         print_notification << "Horizontal reliability: MEDIUM" << std::endl;
         break;
      case telux::loc::LocationReliability::HIGH:
         print_notification << "Horizontal reliability: HIGH" << std::endl;
         break;
      default:
         print_notification << "Horizontal reliability is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printVerticalReliability(telux::loc::LocationReliability locReliability) {
   std::cout << std::endl << std::endl;
   switch(locReliability) {
      case telux::loc::LocationReliability::NOT_SET:
         print_notification << "Vertical reliability: NOT_SET" << std::endl;
         break;
      case telux::loc::LocationReliability::VERY_LOW:
         print_notification << "Vertical reliability: VERY_LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::LOW:
         print_notification << "Vertical reliability: LOW" << std::endl;
         break;
      case telux::loc::LocationReliability::MEDIUM:
         print_notification << "Vertical reliability: MEDIUM" << std::endl;
         break;
      case telux::loc::LocationReliability::HIGH:
         print_notification << "Vertical reliability: HIGH" << std::endl;
         break;
      default:
         print_notification << "Vertical reliability is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printSensorType(telux::loc::SensorType sensorType) {
   std::cout << std::endl << std::endl;
   switch(sensorType) {
      case telux::loc::SensorType::ACCELEROMETER:
         print_notification << "Sensor type: ACCELEROMETER" << std::endl;
         break;
      case telux::loc::SensorType::GYROSCOPE:
         print_notification << "Sensor type: GYROSCOPE" << std::endl;
         break;
      default:
         print_notification << "Sensor type is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printAltitudeType(telux::loc::AltitudeType altitudeType) {
   switch(altitudeType) {
      case telux::loc::AltitudeType::CALCULATED:
         print_notification << "Altitude type: CALCULATED" << std::endl;
         break;
      case telux::loc::AltitudeType::ASSUMED:
         print_notification << "Altitude type: ASSUMED" << std::endl;
         break;
      default:
         print_notification << "Altitude type is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printConstellationType(telux::loc::GnssConstellationType constellation) {
   switch(constellation) {
      case telux::loc::GnssConstellationType::GPS:
         print_notification << "Constellation type: GPS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::GALILEO:
         print_notification << "Constellation type: GALILEO" << std::endl;
         break;
      case telux::loc::GnssConstellationType::SBAS:
         print_notification << "Constellation type: SBAS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::COMPASS:
         print_notification << "Constellation type: COMPASS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::GLONASS:
         print_notification << "Constellation type: GLONASS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::BDS:
         print_notification << "Constellation type: BDS" << std::endl;
         break;
      case telux::loc::GnssConstellationType::QZSS:
         print_notification << "Constellation type: QZSS" << std::endl;
         break;
      default:
         print_notification << "Constellation type is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printSVHealthStatus(telux::loc::SVHealthStatus healthStatus) {
   switch(healthStatus) {
      case telux::loc::SVHealthStatus::UNHEALTHY:
         print_notification << "SV health status: UNHEALTHY" << std::endl;
         break;
      case telux::loc::SVHealthStatus::HEALTHY:
         print_notification << "SV health status: HEALTHY" << std::endl;
         break;
      default:
         print_notification << "SV health status is UNKNOWN" << std::endl;
   }
}
void MyLocationListener::printSVStatus(telux::loc::SVStatus svStatus) {
   switch(svStatus) {
      case telux::loc::SVStatus::IDLE:
         print_notification << "SV status: IDLE" << std::endl;
         break;
      case telux::loc::SVStatus::SEARCH:
         print_notification << "SV status: SEARCH" << std::endl;
         break;
      case telux::loc::SVStatus::TRACK:
         print_notification << "SV status: TRACK" << std::endl;
         break;
      default:
         print_notification << "SV status is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printEphimerisAvailability(telux::loc::SVInfoAvailability availability) {
   switch(availability) {
      case telux::loc::SVInfoAvailability::YES:
         print_notification << "Ephemeris availability: YES" << std::endl;
         break;
      case telux::loc::SVInfoAvailability::NO:
         print_notification << "Ephemeris availability: NO " << std::endl;
         break;
      default:
         print_notification << "Ephemeris availability is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::printAlmanacAvailability(telux::loc::SVInfoAvailability availability) {
   switch(availability) {
      case telux::loc::SVInfoAvailability::YES:
         print_notification << "Almanac availability: YES" << std::endl;
         break;
      case telux::loc::SVInfoAvailability::NO:
         print_notification << "Almanac availability: NO " << std::endl;
         break;
      default:
         print_notification << "Almanac availability is UNKNOWN" << std::endl;
   }
}

void MyLocationListener::onLocationUpdate(
   const std::shared_ptr<telux::loc::ILocationInfo> &locationInfo) {
   std::cout << std::endl;
   std::cout << "*********************** Location Report *********************" << std::endl;
   time_t realtime;
   realtime = (time_t)(locationInfo->getTimeStamp());
   print_notification << "Time stamp: " << ctime(&realtime);
   print_notification << "Session status: " << logSessionStatus(locationInfo->getSessionStatus())
                      << std::endl;
   printPositionTech(locationInfo);
   print_notification << "Latitude: " << locationInfo->getLatitude()
                      << ", Longitude: " << locationInfo->getLongitude() << std::endl;

   // NOTE: Uncomment following log statements for the detailed NOTIFICATION
   // print_notification << "Altitude : " << locationInfo->getAltitude() << std::endl;
   // print_notification << "Heading : " << locationInfo->getHeading() << std::endl;
   // print_notification << "Vertical uncertainty : " << locationInfo->getVerticalUncertainty()
   //                    << std::endl;
   // print_notification
   //    << "Altitude with respect to mean sea level : " << locationInfo->getAltitudeMeanSeaLevel()
   //    << std::endl;
   // print_notification << "Position DOP : " << locationInfo->getPositionDop() << std::endl;
   // print_notification << "Horizontal DOP : " << locationInfo->getHorizontalDop() << std::endl;
   // print_notification << "Vertical DOP : " << locationInfo->getVerticalDop() << std::endl;
   // print_notification << "Magnetic deviation : " << locationInfo->getMagneticDeviation()
   //                    << std::endl;
   // print_notification << "Speed uncertainty : " << locationInfo->getSpeedUncertainty() <<
   // std::endl;
   // print_notification << "Heading uncertainty : " << locationInfo->getHeadingUncertainty()
   //                    << std::endl;
   // printHorizontalReliability(locationInfo->getHorizontalReliability());
   // printVerticalReliability(locationInfo->getVerticalReliability());
   // print_notification
   //    << "HorizontalUncertaintySemiMajor : " << locationInfo->getHorizontalUncertaintySemiMajor()
   //    << std::endl;
   // print_notification
   //    << "HorizontalUncertaintySemiMinor : " << locationInfo->getHorizontalUncertaintySemiMinor()
   //    << std::endl;
   // print_notification
   //    << "HorizontalUncertaintyAzimuth : " << locationInfo->getHorizontalUncertaintyAzimuth()
   //    << std::endl;

   // print_notification << "Ids of used SVs : " << std::endl;
   // std::vector<uint16_t> SVIds;
   // locationInfo->getSVIds(SVIds);
   // for(auto i = 0; i < SVIds.size(); ++i) {
   //    print_notification << SVIds[i] << std::endl;
   // }
   // printSbasCorrection(locationInfo);

   // uint8_t leapSeconds;
   // if(locationInfo->getLeapSeconds(leapSeconds) == telux::common::Status::SUCCESS) {
   //    print_notification << "Leap seconds : " << leapSeconds << std::endl;
   // }

   // if(locationInfo->getGpsTime() != nullptr) {
   //    auto locGpsTime = locationInfo->getGpsTime();
   //    print_notification << "Current GPS week : " << locGpsTime->getWeek() << std::endl;
   //    print_notification << "GPS week in milliseconds : " << locGpsTime->getTimeOfWeekMsec()
   //                       << std::endl;
   // }

   // float circularHorizontalUncertainty;
   // if(locationInfo->getCircularHorizontalUncertainty(circularHorizontalUncertainty)
   //    == telux::common::Status::SUCCESS) {
   //    print_notification << "Circular horizontal uncertainty : " << circularHorizontalUncertainty
   //                       << std::endl;
   // }

   // uint8_t horizontalConfidence;
   // if(locationInfo->getHorizontalConfidence(horizontalConfidence)
   //    == telux::common::Status::SUCCESS) {
   //    print_notification << "Horizontal uncertainty confidence : " <<
   //    unsigned(horizontalConfidence)
   //                       << std::endl;
   // }

   // print_notification << "Horizontal speed : " << locationInfo->getHorizontalSpeed() <<
   // std::endl;

   // uint8_t verticalConfidence;
   // if(locationInfo->getVerticalConfidence(verticalConfidence) == telux::common::Status::SUCCESS)
   // {
   //    print_notification << "Vertical uncertainty confidence : " << unsigned(verticalConfidence)
   //                       << std::endl;
   // }

   // print_notification << "Vertical speed : " << locationInfo->getVerticalSpeed() << std::endl;

   // std::shared_ptr<telux::loc::ISensorDataUsage> sensorDataUsage;
   // if(sensorDataUsage != nullptr) {
   //    print_notification << "Sensor data usage" << std::endl;
   //    if(locationInfo->getSensorDataUsage(sensorDataUsage) == telux::common::Status::SUCCESS) {
   //       print_notification << "Status of getSensorDataUsage : "
   //                          << (int)locationInfo->getSensorDataUsage(sensorDataUsage) <<
   //                          std::endl;
   //       printSensorType(sensorDataUsage->getSensorType());
   //       printMeasurementType(locationInfo);
   //    }
   // }

   // uint32_t fixId;
   // if(locationInfo->getFixId(fixId) == telux::common::Status::SUCCESS) {
   //    print_notification << "Fix Id : " << fixId << std::endl;
   // }

   // std::vector<float> velocityEastNorthUp;
   // if(locationInfo->getVelocityEastNorthUp(velocityEastNorthUp) ==
   // telux::common::Status::SUCCESS) {
   //    print_notification << "East, North, Up velocity : ";
   //    for(auto i = 0; i < velocityEastNorthUp.size(); ++i) {
   //       std::cout << velocityEastNorthUp[i] << ", ";
   //    }
   //    std::cout << std::endl;
   // }

   // std::vector<float> velocityUncertaintyEastNorthUp;
   // if(locationInfo->getVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp)
   //    == telux::common::Status::SUCCESS) {
   //    print_notification << "East, North, Up velocity uncertainty : ";
   //    for(auto i = 0; i < velocityEastNorthUp.size(); ++i) {
   //       std::cout << velocityUncertaintyEastNorthUp[i] << ", ";
   //    }
   //    std::cout << std::endl;
   // }
   std::cout << "*************************************************************" << std::endl;
}

void MyLocationListener::onGnssSVInfo(const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
   std::cout << std::endl;
   std::cout << "**************** Satellite Vehicle Information ***************" << std::endl;
   printAltitudeType(gnssSVInfo->getAltitudeType());
   // NOTE: Uncomment following log statements for the detailed NOTIFICATION
   // for(auto svInfo : gnssSVInfo->getSVInfoList()) {
   //    print_notification << "**** GNSS SV Id : " << svInfo->getId() << " ****" << std::endl;
   //    printConstellationType(svInfo->getConstellation());
   //    printSVHealthStatus(svInfo->getSVHealthStatus());
   //    printSVStatus(svInfo->getStatus());
   //    printEphimerisAvailability(svInfo->getHasEphemeris());
   //    printAlmanacAvailability(svInfo->getHasAlmanac());
   //    print_notification << "Elevation : " << svInfo->getElevation() << std::endl;
   //    print_notification << "Azimuth : " << svInfo->getAzimuth() << std::endl;
   //    print_notification << "SNR : " << svInfo->getSnr() << std::endl;
   // }

   std::cout << "*************************************************************" << std::endl;
}
