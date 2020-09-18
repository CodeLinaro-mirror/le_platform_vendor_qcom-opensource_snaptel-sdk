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

#include <iostream>

#include "MyLocationCommandCallback.hpp"
#include "../telsdk_console_app/Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

// Implementation of My location callback
MyLocationCommandCallback::MyLocationCommandCallback(std::string cmdName) {
   commandName_ = cmdName;
}
void MyLocationCommandCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << commandName_ << " sent successfully" << std::endl;
   } else {
      PRINT_CB << commandName_ << " failed\n errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyLocationCommandCallback::onGnssEnergyConsumedInfo(telux::loc::GnssEnergyConsumedInfo
    gnssEnergyConsumed, telux::common::ErrorCode error) {
   std::cout << __FUNCTION__ << " : " << Utils::getErrorCodeAsString(error) << std::endl;

   PRINT_CB << "\n**************** Gnss Energy Consumed Information ***************"
       << std::endl;
   std::cout << "<<< onGnssEnergyConsumedInfoCb\n" << std::endl;
   std::cout << " GnssEnergyConsumedInfoValidity : " << std::endl;
   if(gnssEnergyConsumed.valid & telux::loc::ENERGY_CONSUMED_SINCE_FIRST_BOOT_BIT) {
     std::cout << " Energy consumed is valid" << std::endl;
   }
   else {
     std::cout << " Energy consumed is invalid" << std::endl;
   }
   std::cout << " Energy consumed : " << gnssEnergyConsumed.energySinceFirstBoot
       << std::endl;
   std::cout << "*******************************" << std::endl;
}

void MyLocationCommandCallback::onGetYearOfHwInfo(uint16_t yearOfHw,
    telux::common::ErrorCode error) {
   std::cout << __FUNCTION__ << " : " << Utils::getErrorCodeAsString(error) << std::endl;

   PRINT_CB << "\n**************** Year Of Hardware Information ***************"
       << std::endl;
   std::cout << "Year of Hardware is : " << yearOfHw << std::endl;
   std::cout << "*******************************" << std::endl;
}

void MyLocationCommandCallback::onMinGpsWeekInfo(uint16_t minGpsWeek,
    telux::common::ErrorCode error) {
  std::cout << __FUNCTION__ <<  " : " << Utils::getErrorCodeAsString(error) << std::endl;

  PRINT_CB << " ************ Request Minimum GPS Week ***************" << std::endl;
  std::cout << " Minimum Gps Week is : " << minGpsWeek << std::endl;
  std::cout << " ****************************************************" << std::endl;
}

void MyLocationCommandCallback::onMinSVElevationInfo(uint8_t minSVElevation,
    telux::common::ErrorCode error) {
  std::cout << __FUNCTION__ << " : " << Utils::getErrorCodeAsString(error) << std::endl;

  PRINT_CB << " ************ Request Minimum SV Elevation Angle ***************" << std::endl;
  std::cout << " Minimum SV Elevation is : " << (uint32_t)minSVElevation << std::endl;
}

void MyLocationCommandCallback::onRobustLocationInfo(const telux::loc::RobustLocationConfiguration
     rLConfig, telux::common::ErrorCode error) {
  std::cout << __FUNCTION__ << " : " << Utils::getErrorCodeAsString(error) << std::endl;

  PRINT_CB << " ************ Request Robust Location ***************" << std::endl;
  if (rLConfig.validMask & telux::loc::VALID_ENABLED) {
    std::cout << " Enabled is valid" << std::endl;
  }
  if (rLConfig.validMask & telux::loc::VALID_ENABLED_FOR_E911) {
    std::cout << " Enabled for E911 is valid" << std::endl;
  }
  if (rLConfig.validMask & telux::loc::VALID_VERSION) {
    std::cout << " Version is valid" << std::endl;
  }
  std::cout << " Enabled is : " << rLConfig.enabled << std::endl;
  std::cout << " Enabled for E911 is : " << rLConfig.enabledForE911 << std::endl;
  std::cout << " Major version is : " << unsigned(rLConfig.version.major) << std::endl;
  std::cout << " Minor version is : " << rLConfig.version.minor << std::endl;
  std::cout << " ****************************************************" << std::endl;
}

void MyLocationCommandCallback::onSecondaryBandInfo(telux::loc::ConstellationSet set,
     telux::common::ErrorCode error) {
  std::cout << __FUNCTION__ << " : " << Utils::getErrorCodeAsString(error) << std::endl;

  PRINT_CB << "************ Request Secondary Band Info ***************" << std::endl;
  std::cout << "Disabled secondary band constellations :" << std::endl;
  for (auto item : set) {
      if (item == telux::loc::GnssConstellationType::GPS) {
          std::cout << "GPS" << std::endl;
      } else if (item == telux::loc::GnssConstellationType::GALILEO) {
          std::cout << "GALILEO" << std::endl;
      } else if (item == telux::loc::GnssConstellationType::SBAS) {
          std::cout << "SBAS" << std::endl;
      } else if (item == telux::loc::GnssConstellationType::GLONASS) {
          std::cout << "GLONASS" << std::endl;
      } else if (item == telux::loc::GnssConstellationType::BDS) {
          std::cout << "BDS" << std::endl;
      } else if (item == telux::loc::GnssConstellationType::QZSS) {
          std::cout << "QZSS" << std::endl;
      } else if (item == telux::loc::GnssConstellationType::NAVIC) {
          std::cout << "NAVIC" << std::endl;
      } else {
          std::cout << "Not supported" << std::endl;
      }
  }
    std::cout << " ****************************************************" << std::endl;
}
