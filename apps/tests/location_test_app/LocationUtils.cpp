/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       LocationUtils.cpp
 *
 * @brief      Location Utility class
 */

#include <iostream>

#include "LocationUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void LocationUtils::displayCapabilities(telux::loc::LocCapability capabilityMask) {
   PRINT_NOTIFICATION << "\n************* Capabilities Information *************"
                      << std::endl;
  if (capabilityMask & telux::loc::TIME_BASED_TRACKING) {
    std::cout << "Time based tracking" << std::endl;
  }
  if (capabilityMask & telux::loc::DISTANCE_BASED_TRACKING) {
    std::cout << "Distance based tracking" << std::endl;
  }
  if (capabilityMask & telux::loc::GNSS_MEASUREMENTS) {
    std::cout << "GNSS Measurement" << std::endl;
  }
  if (capabilityMask & telux::loc::CONSTELLATION_ENABLEMENT) {
    std::cout << "Constellation enablement" << std::endl;
  }
  if (capabilityMask & telux::loc::CARRIER_PHASE) {
    std::cout << "Carrier phase" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_GNSS_SINGLE_FREQUENCY) {
    std::cout << "QWES GNSS single frequency" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_GNSS_MULTI_FREQUENCY) {
    std::cout << "QWES GNSS multi frequency" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_VPE) {
    std::cout << "QWES VPE" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_CV2X_LOCATION_BASIC) {
    std::cout << "QWES CV2X location basic" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_CV2X_LOCATION_PREMIUM) {
    std::cout << "QWES CV2X location premium" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_PPE) {
    std::cout << "QWES PPE" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_QDR2) {
    std::cout << "QWES QDR2" << std::endl;
  }
  if (capabilityMask & telux::loc::QWES_QDR3) {
    std::cout << "QWES QDR3" << std::endl;
  }
  std::cout << "*****************************************" << std::endl;
}

void LocationUtils::displayXtraStatus(telux::loc::XtraStatus xtraStatus) {
    std::cout << "Xtra Data Status: ";
    switch(xtraStatus.xtraDataStatus) {
        case telux::loc::XtraDataStatus::STATUS_UNKNOWN :
            std::cout << "Unknown \n";
            break;
        case telux::loc::XtraDataStatus::STATUS_NOT_AVAIL :
            std::cout << "Not available \n";
            break;
        case telux::loc::XtraDataStatus::STATUS_NOT_VALID :
            std::cout << "Invalid \n";
            break;
        case telux::loc::XtraDataStatus::STATUS_VALID :
            std::cout << "Valid \n";
            break;
    }
}

void LocationUtils::printGnssSignalType(telux::loc::GnssSignal signalTypeMask) {
   std::cout << "Gnss Signal Type :" << std::endl;
   if (signalTypeMask & telux::loc::GnssSignalType::GPS_L1CA) {
     std::cout << "GPS L1CA signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GPS_L1C) {
     std::cout << "GPS L1C signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GPS_L2) {
     std::cout << "GPS L2 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GPS_L5) {
     std::cout << "GPS L5 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GLONASS_G1) {
     std::cout << "Glonass G1 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GLONASS_G2) {
     std::cout << "Glonass G2 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GALILEO_E1) {
     std::cout << "Galileo E1 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GALILEO_E5A) {
     std::cout << "Galileo E5A signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::GALILIEO_E5B) {
     std::cout << "Galileo E5B signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B1) {
     std::cout << "Beidou B1 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2) {
     std::cout << "Beidou B2 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::QZSS_L1CA) {
     std::cout << "QZSS L1CA signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::QZSS_L1S) {
     std::cout << "QZSS L1S signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::QZSS_L2) {
     std::cout << "QZSS L2 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::QZSS_L5) {
     std::cout << "QZSS L5 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::SBAS_L1) {
     std::cout << "SBAS L1 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B1I) {
     std::cout << "Beidou B1I signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B1C) {
     std::cout << "Beidou B1C signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2I) {
     std::cout << "Beidou B2I signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2AI) {
     std::cout << "Beidou B2AI signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::NAVIC_L5) {
     std::cout << "Navic L5 signal is present" << std::endl;
   }
   if (signalTypeMask & telux::loc::GnssSignalType::BEIDOU_B2AQ) {
     std::cout << "Beidou B2AQ signal is present" << std::endl;
   }
   if (signalTypeMask == telux::loc::UNKNOWN_SIGNAL_MASK) {
     std::cout << " No signal present" << std::endl;
   }
}

void LocationUtils::displayDisasterCrisisReportType(telux::loc::GnssDisasterCrisisReport
    dcReportInfo) {
    std::cout << "Disaster Crisis Report type: ";
    switch(dcReportInfo.dcReportType) {
        case telux::loc::GnssReportDCType::QZSS_JMA_DISASTER_PREVENTION_INFO :
            std::cout << "QZSS_JMA_DISASTER_PREVENTION_INFO \n";
            break;
        case telux::loc::GnssReportDCType::QZSS_NON_JMA_DISASTER_PREVENTION_INFO :
            std::cout << "QZSS_NON_JMA_DISASTER_PREVENTION_INFO \n";
            break;
    }
}