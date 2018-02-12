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

/**
 * Sample program to register and de-register for location fixes
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "telux/loc/LocationDefines.hpp"
#include "telux/loc/LocationFactory.hpp"
#include "telux/loc/LocationManager.hpp"
#include "telux/loc/LocationListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mCallback: \033[0m"

using namespace telux::loc;
using namespace telux::common;

class MyLocationListener : public telux::loc::ILocationListener {
public:
   void onLocationUpdate(const std::shared_ptr<ILocationInfo> &locationInfo) {
      std::cout << std::endl;
      std::cout << "*********************** Location Report *********************" << std::endl;
      time_t realtime;
      realtime = (time_t)(locationInfo->getTimeStamp());
      PRINT_NOTIFICATION << "Timestamp : " << ctime(&realtime) << std::endl;
      PRINT_NOTIFICATION << "Session status: " << logSessionStatus(locationInfo->getSessionStatus())
                         << std::endl;
      printPositionTech(locationInfo);
      PRINT_NOTIFICATION << "Latitude : " << locationInfo->getLatitude() << std::endl;
      PRINT_NOTIFICATION << "Longitude : " << locationInfo->getLongitude() << std::endl;
      PRINT_NOTIFICATION << "Altitude : " << locationInfo->getAltitude() << std::endl;
      PRINT_NOTIFICATION << "Heading : " << locationInfo->getHeading() << std::endl;
      PRINT_NOTIFICATION << "Vertical uncertainty : " << locationInfo->getVerticalUncertainty()
                         << std::endl;
      PRINT_NOTIFICATION
         << "Altitude with respect to mean sea level : " << locationInfo->getAltitudeMeanSeaLevel()
         << std::endl;
      PRINT_NOTIFICATION << "Position DOP : " << locationInfo->getPositionDop() << std::endl;
      PRINT_NOTIFICATION << "Horizontal DOP : " << locationInfo->getHorizontalDop() << std::endl;
      PRINT_NOTIFICATION << "Vertical DOP : " << locationInfo->getVerticalDop() << std::endl;
      PRINT_NOTIFICATION << "Magnetic deviation : " << locationInfo->getMagneticDeviation()
                         << std::endl;
      PRINT_NOTIFICATION << "Speed uncertainty : " << locationInfo->getSpeedUncertainty()
                         << std::endl;
      PRINT_NOTIFICATION << "Heading uncertainty : " << locationInfo->getHeadingUncertainty()
                         << std::endl;
      printHorizontalReliability(locationInfo->getHorizontalReliability());
      printVerticalReliability(locationInfo->getVerticalReliability());
      PRINT_NOTIFICATION
         << "HorizontalUncertaintySemiMajor : " << locationInfo->getHorizontalUncertaintySemiMajor()
         << std::endl;
      PRINT_NOTIFICATION
         << "HorizontalUncertaintySemiMinor : " << locationInfo->getHorizontalUncertaintySemiMinor()
         << std::endl;
      PRINT_NOTIFICATION
         << "HorizontalUncertaintyAzimuth : " << locationInfo->getHorizontalUncertaintyAzimuth()
         << std::endl;

      PRINT_NOTIFICATION << "Ids of used SVs : " << std::endl;
      std::vector<uint16_t> SVIds;
      locationInfo->getSVIds(SVIds);
      for(auto i = 0; i < SVIds.size(); ++i) {
         PRINT_NOTIFICATION << SVIds[i] << std::endl;
      }
      printSbasCorrection(locationInfo);

      uint8_t leapSeconds;
      if(locationInfo->getLeapSeconds(leapSeconds) == Status::SUCCESS) {
         PRINT_NOTIFICATION << "Leap seconds : " << leapSeconds << std::endl;
      }

      if(locationInfo->getGpsTime() != nullptr) {
         auto locGpsTime = locationInfo->getGpsTime();
         PRINT_NOTIFICATION << "Current GPS week : " << locGpsTime->getWeek() << std::endl;
         PRINT_NOTIFICATION << "GPS week in milliseconds : " << locGpsTime->getTimeOfWeekMsec()
                            << std::endl;
      }

      float circularHorizontalUncertainty;
      if(locationInfo->getCircularHorizontalUncertainty(circularHorizontalUncertainty)
         == Status::SUCCESS) {
         PRINT_NOTIFICATION << "Circular horizontal uncertainty : " << circularHorizontalUncertainty
                            << std::endl;
      }

      uint8_t horizontalConfidence;
      if(locationInfo->getHorizontalConfidence(horizontalConfidence) == Status::SUCCESS) {
         PRINT_NOTIFICATION
            << "Horizontal uncertainty confidence : " << unsigned(horizontalConfidence)
            << std::endl;
      }

      PRINT_NOTIFICATION << "Horizontal speed : " << locationInfo->getHorizontalSpeed()
                         << std::endl;

      uint8_t verticalConfidence;
      if(locationInfo->getVerticalConfidence(verticalConfidence) == Status::SUCCESS) {
         PRINT_NOTIFICATION << "Vertical uncertainty confidence : " << unsigned(verticalConfidence)
                            << std::endl;
      }

      PRINT_NOTIFICATION << "Vertical speed : " << locationInfo->getVerticalSpeed() << std::endl;

      std::shared_ptr<ISensorDataUsage> sensorDataUsage;
      if(sensorDataUsage != nullptr) {
         PRINT_NOTIFICATION << "Sensor data usage" << std::endl;
         if(locationInfo->getSensorDataUsage(sensorDataUsage) == Status::SUCCESS) {
            PRINT_NOTIFICATION << "Status of getSensorDataUsage : "
                               << (int)locationInfo->getSensorDataUsage(sensorDataUsage)
                               << std::endl;
            printSensorType(sensorDataUsage->getSensorType());
            printMeasurementType(locationInfo);
         }
      }

      uint32_t fixId;
      if(locationInfo->getFixId(fixId) == Status::SUCCESS) {
         PRINT_NOTIFICATION << "Fix Id : " << fixId << std::endl;
      }

      std::vector<float> velocityEastNorthUp;
      if(locationInfo->getVelocityEastNorthUp(velocityEastNorthUp) == Status::SUCCESS) {
         PRINT_NOTIFICATION << "East, North, Up velocity : ";
         for(auto i = 0; i < velocityEastNorthUp.size(); ++i) {
            std::cout << velocityEastNorthUp[i] << ", ";
         }
         std::cout << std::endl;
      }

      std::vector<float> velocityUncertaintyEastNorthUp;
      if(locationInfo->getVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp)
         == Status::SUCCESS) {
         PRINT_NOTIFICATION << "East, North, Up velocity uncertainty : ";
         for(auto i = 0; i < velocityEastNorthUp.size(); ++i) {
            std::cout << velocityUncertaintyEastNorthUp[i] << ", ";
         }
         std::cout << std::endl;
      }
   }

   void onGnssSVInfo(const std::shared_ptr<IGnssSVInfo> &gnssSVInfo) {
      std::cout << std::endl;
      std::cout << "**************** Satellite Vehicle Infomation ***************" << std::endl;
      printAltitudeType(gnssSVInfo->getAltitudeType());
      for(auto svInfo : gnssSVInfo->getSVInfoList()) {
         PRINT_NOTIFICATION << std::endl;
         PRINT_NOTIFICATION << "**** Gnss SV Id : " << svInfo->getId() << " ****" << std::endl;
         printConstellationType(svInfo->getConstellation());
         printSVHealthStatus(svInfo->getSVHealthStatus());
         printSVStatus(svInfo->getStatus());
         printEphimerisAvailability(svInfo->getHasEphemeris());
         printAlmanacAvailability(svInfo->getHasAlmanac());
         PRINT_NOTIFICATION << "Elevation : " << svInfo->getElevation() << std::endl;
         PRINT_NOTIFICATION << "Azimuth : " << svInfo->getAzimuth() << std::endl;
         PRINT_NOTIFICATION << "SNR : " << svInfo->getSnr() << std::endl;
      }

      std::cout << "*************************************************************" << std::endl;
   }

private:
   std::string logSessionStatus(SessionStatus sessionStatus) {
      std::string sessionStatusString = "UNKNOWN";
      switch(sessionStatus) {
         case SessionStatus::SUCCESS:
            sessionStatusString = "SUCCESS";
            break;
         case SessionStatus::IN_PROGRESS:
            sessionStatusString = "IN_PROGRESS";
            break;
         case SessionStatus::GENERAL_FAILURE:
            sessionStatusString = "GENERAL_FAILURE";
            break;
         case SessionStatus::TIMEOUT:
            sessionStatusString = "TIMEOUT";
            break;
         case SessionStatus::USER_END:
            sessionStatusString = "USER_END";
            break;
         case SessionStatus::BAD_PARAMETER:
            sessionStatusString = "BAD_PARAMETER";
            break;
         case SessionStatus::PHONE_OFFLINE:
            sessionStatusString = "PHONE_OFFLINE";
            break;
         case SessionStatus::ENGINE_LOCKED:
            sessionStatusString = "ENGINE_LOCKED";
            break;
         default:
            break;
      }
      return sessionStatusString;
   }

   void printSbasCorrection(std::shared_ptr<ILocationInfo> locationInfo) {
      SbasCorrection correction = locationInfo->getSbasCorrection();
      if(correction[SBAS_CORRECTION_IONO]) {
         PRINT_NOTIFICATION << "SBAS ionospheric correction is used" << std::endl;
      }

      if(correction[SBAS_CORRECTION_FAST]) {
         PRINT_NOTIFICATION << "SBAS fast correction is used" << std::endl;
      }

      if(correction[SBAS_CORRECTION_LONG]) {
         PRINT_NOTIFICATION << "SBAS long correction is used" << std::endl;
      }

      if(correction[SBAS_INTEGRITY]) {
         PRINT_NOTIFICATION << "SBAS integrity information is used" << std::endl;
      }
   }

   void printPositionTech(std::shared_ptr<ILocationInfo> locationInfo) {
      PositionTech positionTech = locationInfo->getPositionTechnology();
      PRINT_NOTIFICATION << "Position Technologies used: ";

      if(positionTech[SATELLITE]) {
         std::cout << "SATELLITE, ";
      }

      if(positionTech[CELLID]) {
         std::cout << "CELLID, ";
      }

      if(positionTech[WIFI]) {
         std::cout << "WIFI, ";
      }

      if(positionTech[SENSORS]) {
         std::cout << "SENSORS, ";
      }

      if(positionTech[REFERENCE_LOCATION]) {
         std::cout << "REFERENCE_LOCATION, ";
      }

      if(positionTech[INJECTED_COARSE_POSITION]) {
         std::cout << "INJECTED_COARSE_POSITION, ";
      }

      if(positionTech[AFLT]) {
         std::cout << "AFLT, ";
      }

      if(positionTech[HYBRID]) {
         std::cout << "HYBRID";
      }
      std::cout << std::endl;
   }

   void printMeasurementType(std::shared_ptr<ILocationInfo> locationInfo) {
      std::shared_ptr<ISensorDataUsage> sensorDataUsage;
      if(sensorDataUsage != nullptr) {
         PRINT_NOTIFICATION << "Sensor data usage: " << std::endl;
         if(locationInfo->getSensorDataUsage(sensorDataUsage) == Status::SUCCESS) {
            Measurement measurement = sensorDataUsage->getMeasurement();
            PRINT_NOTIFICATION << "Measurement type: " << std::endl;

            if(measurement[HEADING]) {
               PRINT_NOTIFICATION << "HEADING," << std::endl;
            }
            if(measurement[SPEED]) {
               PRINT_NOTIFICATION << "SPEED," << std::endl;
            }
            if(measurement[POSITION]) {
               PRINT_NOTIFICATION << "POSITION," << std::endl;
            }
            if(measurement[VELOCITY]) {
               PRINT_NOTIFICATION << "VELOCITY" << std::endl;
            }
         }
      }
   }

   void printHorizontalReliability(LocationReliability locReliability) {
      switch(locReliability) {
         case LocationReliability::NOT_SET:
            PRINT_NOTIFICATION << "Horizontal reliability: NOT_SET" << std::endl;
            break;
         case LocationReliability::VERY_LOW:
            PRINT_NOTIFICATION << "Horizontal reliability: VERY_LOW" << std::endl;
            break;
         case LocationReliability::LOW:
            PRINT_NOTIFICATION << "Horizontal reliability: LOW" << std::endl;
            break;
         case LocationReliability::MEDIUM:
            PRINT_NOTIFICATION << "Horizontal reliability: MEDIUM" << std::endl;
            break;
         case LocationReliability::HIGH:
            PRINT_NOTIFICATION << "Horizontal reliability: HIGH" << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "Horizontal reliability is UNKNOWN" << std::endl;
      }
   }

   void printVerticalReliability(LocationReliability locReliability) {
      switch(locReliability) {
         case LocationReliability::NOT_SET:
            PRINT_NOTIFICATION << "Vertical reliability: NOT_SET" << std::endl;
            break;
         case LocationReliability::VERY_LOW:
            PRINT_NOTIFICATION << "Vertical reliability: VERY_LOW" << std::endl;
            break;
         case LocationReliability::LOW:
            PRINT_NOTIFICATION << "Vertical reliability: LOW" << std::endl;
            break;
         case LocationReliability::MEDIUM:
            PRINT_NOTIFICATION << "Vertical reliability: MEDIUM" << std::endl;
            break;
         case LocationReliability::HIGH:
            PRINT_NOTIFICATION << "Vertical reliability: HIGH" << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "Vertical reliability is UNKNOWN" << std::endl;
      }
   }

   void printSensorType(SensorType sensorType) {
      switch(sensorType) {
         case SensorType::ACCELEROMETER:
            PRINT_NOTIFICATION << "Sensor type: ACCELEROMETER" << std::endl;
            break;
         case SensorType::GYROSCOPE:
            PRINT_NOTIFICATION << "Sensor type: GYROSCOPE" << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "Sensor type is UNKNOWN" << std::endl;
      }
   }

   void printAltitudeType(AltitudeType altitudeType) {
      switch(altitudeType) {
         case AltitudeType::CALCULATED:
            PRINT_NOTIFICATION << "Altitude type: CALCULATED" << std::endl;
            break;
         case AltitudeType::ASSUMED:
            PRINT_NOTIFICATION << "Altitude type: ASSUMED" << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "Altitude type is UNKNOWN" << std::endl;
      }
   }

   void printConstellationType(GnssConstellationType constellation) {
      switch(constellation) {
         case GnssConstellationType::GPS:
            PRINT_NOTIFICATION << "Constellation type: GPS" << std::endl;
            break;
         case GnssConstellationType::GALILEO:
            PRINT_NOTIFICATION << "Constellation type: GALILEO" << std::endl;
            break;
         case GnssConstellationType::SBAS:
            PRINT_NOTIFICATION << "Constellation type: SBAS" << std::endl;
            break;
         case GnssConstellationType::COMPASS:
            PRINT_NOTIFICATION << "Constellation type: COMPASS" << std::endl;
            break;
         case GnssConstellationType::GLONASS:
            PRINT_NOTIFICATION << "Constellation type: GLONASS" << std::endl;
            break;
         case GnssConstellationType::BDS:
            PRINT_NOTIFICATION << "Constellation type: BDS" << std::endl;
            break;
         case GnssConstellationType::QZSS:
            PRINT_NOTIFICATION << "Constellation type: QZSS" << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "Constellation type is UNKNOWN" << std::endl;
      }
   }

   void printSVHealthStatus(SVHealthStatus healthStatus) {
      switch(healthStatus) {
         case SVHealthStatus::UNHEALTHY:
            PRINT_NOTIFICATION << "SV health status: UNHEALTHY" << std::endl;
            break;
         case SVHealthStatus::HEALTHY:
            PRINT_NOTIFICATION << "SV health status: HEALTHY" << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "SV health status is UNKNOWN" << std::endl;
      }
   }
   void printSVStatus(SVStatus svStatus) {
      switch(svStatus) {
         case SVStatus::IDLE:
            PRINT_NOTIFICATION << "SV status: IDLE" << std::endl;
            break;
         case SVStatus::SEARCH:
            PRINT_NOTIFICATION << "SV status: SEARCH" << std::endl;
            break;
         case SVStatus::TRACK:
            PRINT_NOTIFICATION << "SV status: TRACK" << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "SV status is UNKNOWN" << std::endl;
      }
   }

   void printEphimerisAvailability(SVInfoAvailability availability) {
      switch(availability) {
         case SVInfoAvailability::YES:
            PRINT_NOTIFICATION << "Ephemeris availability: YES" << std::endl;
            break;
         case SVInfoAvailability::NO:
            PRINT_NOTIFICATION << "Ephemeris availability: NO " << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "Ephemeris availability is UNKNOWN" << std::endl;
      }
   }

   void printAlmanacAvailability(SVInfoAvailability availability) {
      switch(availability) {
         case SVInfoAvailability::YES:
            PRINT_NOTIFICATION << "Almanac availability: YES" << std::endl;
            break;
         case SVInfoAvailability::NO:
            PRINT_NOTIFICATION << "Almanac availability: NO " << std::endl;
            break;
         default:
            PRINT_NOTIFICATION << "Almanac availability is UNKNOWN" << std::endl;
      }
   }
};

/**
 * Main routine
 */
int main(int, char **) {
   std::shared_ptr<telux::loc::ILocationListener> myLocationListener
      = std::make_shared<MyLocationListener>();
   std::shared_ptr<telux::loc::ILocationManager> locationManager;
   if(locationManager == nullptr) {
      // Get location manager object
      auto &locationFactory = LocationFactory::getInstance();
      locationManager = locationFactory.getLocationManager();

      std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
      startTime = std::chrono::system_clock::now();
      bool subSystemsStatus = locationManager->isSubsystemReady();
      if(!subSystemsStatus) {
         std::cout << "Location subsystem is not ready, wait for it to be ready " << std::endl;
         std::future<bool> f = locationManager->onSubsystemReady();
         subSystemsStatus = f.get();
      }

      if(subSystemsStatus) {
         endTime = std::chrono::system_clock::now();
         std::chrono::duration<double> elapsedTime = endTime - startTime;
         std::cout << "\nElapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                   << std::endl;
      } else {
         std::cout << " *** ERROR - Unable to initialize Location subsystem" << std::endl;
      }
   }

   // Registering a listener to get location fixes
   locationManager->registerListener(myLocationListener);

   // [9] exit logic is specific to an application
   // std::cout << " *** Press [ENTER] or type [quit] to exit the application *** " << std::endl;
   std::string input;
   std::getline(std::cin, input);
   if(input != "quit") {
      locationManager->removeListener(myLocationListener);
      return 0;
   }
}