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
#include <iomanip>

#include "DataResponseCallback.hpp"
#include "DataMenu.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MyDataProfilesCallback::onProfileListResponse(
   const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles,
   telux::common::ErrorCode error) {
   DataMenu::profiles_.clear();
   DataMenu::profiles_ = profiles;
   std::cout << std::endl << std::endl;
   PRINT_CB << " ** onProfileListResponse **" << std::endl;
   std::cout << std::setw(2)
             << "+-----------------------------------------------------------------+" << std::endl;
   std::cout << std::setw(14) << "| Profile # | " << std::setw(11) << "TechPref | " << std::setw(15)
             << "      APN      " << std::setw(17) << "|  ProfileName  |" << std::setw(10)
             << " IP Type |" << std::endl;
   std::cout << std::setw(2)
             << "+-----------------------------------------------------------------+" << std::endl;
   for(auto it : profiles) {
      std::cout << std::left << std::setw(4) << "  " << std::setw(10) << it->getId()
                << std::setw(11) << techPreferenceToString(it->getTechPreference()) << std::setw(15)
                << it->getApn() << std::setw(17) << it->getName() << std::setw(10)
                << ipFamilyTypeToString(it->getIpFamilyType()) << std::endl;
   }
   std::cout << "ErrorCode:" << (int)error << std::endl;
   std::cout << std::endl << std::endl;
}

std::string MyDataProfilesCallback::techPreferenceToString(telux::data::TechPreference techPref) {
   switch(techPref) {
      case telux::data::TechPreference::TP_3GPP:
         return "3gpp";
      case telux::data::TechPreference::TP_3GPP2:
         return "3gpp2";
      case telux::data::TechPreference::TP_ANY:
      default:
         return "Any";
   }
}

std::string MyDataProfilesCallback::ipFamilyTypeToString(telux::data::IpFamilyType ipType) {
   switch(ipType) {
      case telux::data::IpFamilyType::IPV4:
         return "IPv4";
      case telux::data::IpFamilyType::IPV6:
         return "IPv6";
      case telux::data::IpFamilyType::IPV4V6:
         return "IPv4v6";
      case telux::data::IpFamilyType::UNKNOWN:
      default:
         return "NA";
   }
}

void MyDataProfileCallback::onResponse(const std::shared_ptr<telux::data::DataProfile> &profile,
                                       telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << std::endl << std::endl;
      PRINT_CB << "onProfileResponse:" << std::endl;
      PRINT_CB << "ProfileID : " << profile->getId() << ", ProfileName : " << profile->getName()
               << ", TechPreference : " << (int)profile->getTechPreference()
               << ", APN : " << profile->getApn() << ", UserName : " << profile->getUserName()
               << ", Password : " << profile->getPassword()
               << ", AuthPreference : " << (int)profile->getAuthProtocolType()
               << ", IpFamilyType : " << (int)profile->getIpFamilyType() << std::endl;
   } else {
      PRINT_CB << "Unable to create profile or request profile by ID. " << std::endl;
   }
   std::cout << std::endl << std::endl;
}

void MyDataCreateProfileCallback::onResponse(int profileId, telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << std::endl << std::endl;
      PRINT_CB << "onResponse:" << std::endl;
      PRINT_CB << "ProfileID : " << profileId << std::endl;
   } else {
      PRINT_CB << "Unable to create profile or request profile by ID. " << std::endl;
   }
   std::cout << std::endl << std::endl;
}

void MyDeleteProfileCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   PRINT_CB << "onDeleteProfileResponse:" << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Delete Profile is successful " << std::endl;
   } else {
      PRINT_CB << " Delete Profile is failure " << std::endl;
   }
}

void MyModifyProfileCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   PRINT_CB << "onModifyProfileResponse:" << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << " Modify Profile is successful " << std::endl;
   } else {
      PRINT_CB << " Modify Profile is failure " << std::endl;
   }
}

// Implementation of My Data callback
void MyDataCallResponseCallback::startDataCallResponseCallBack(
   const std::shared_ptr<telux::data::IDataCall> &dataCall, telux::common::ErrorCode error) {
   std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "start DataCallResponseCb is successful " << std::endl;
   } else {
      PRINT_CB << "start DataCallResponseCb failed,  errorCode: " << static_cast<int>(error)
               << std::endl;
   }
   std::cout << std::endl;
}

void MyDataCallResponseCallback::stopDataCallResponseCallBack(
   const std::shared_ptr<telux::data::IDataCall> &dataCall, telux::common::ErrorCode error) {
   std::cout << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "stop DataCallResponseCb is successful " << std::endl;
   } else {
      PRINT_CB << "stop DataCallResponseCb failed,  errorCode: " << static_cast<int>(error)
               << std::endl;
   }
   std::cout << std::endl;
}

void DataCallStatisticsResponseCb::requestStatisticsResponse(
   const telux::data::DataCallStats dCallStats, telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "requestDataCallStatistics Response is successful \n";
      std::cout << " Number of packets transmitted: " << dCallStats.packetsTx << std::endl;
      std::cout << " Number of packets received: " << dCallStats.packetsRx << std::endl;
      std::cout << " Number of bytes transmitted: " << dCallStats.bytesTx << std::endl;
      std::cout << " Number of bytes received: " << dCallStats.bytesRx << std::endl;
      std::cout << " Number of transmit packets dropped: " << dCallStats.packetsDroppedTx
                << std::endl;
      std::cout << " Number of receive packets dropped: " << dCallStats.packetsDroppedRx
                << std::endl
                << std::endl;
   } else {
      PRINT_CB
         << "requestDataCallStatistics Response failed, errorCode: " << static_cast<int>(error)
         << std::endl;
   }
}