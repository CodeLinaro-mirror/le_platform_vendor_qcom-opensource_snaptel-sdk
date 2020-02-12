/*
 *  Copyright (c) 2018-2020 The Linux Foundation. All rights reserved.
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

#include "MyServingSystemHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

std::string MyServingSystemHelper::getRatPreference(telux::tel::RatPreference preference) {
   std::string ratPrefString = "";
   if(preference[telux::tel::PREF_CDMA_1X]) {
      ratPrefString += " CDMA_1X \n";
   }

   if(preference[telux::tel::PREF_CDMA_EVDO]) {
      ratPrefString += " CDMA_EVDO \n";
   }

   if(preference[telux::tel::PREF_GSM]) {
      ratPrefString += " GSM \n";
   }

   if(preference[telux::tel::PREF_WCDMA]) {
      ratPrefString += " WCDMA \n";
   }

   if(preference[telux::tel::PREF_LTE]) {
      ratPrefString += " LTE \n";
   }

   if(preference[telux::tel::PREF_TDSCDMA]) {
      ratPrefString += " TDSCDMA \n";
   }

   if(preference[telux::tel::PREF_NR5G]) {
      ratPrefString += " NR5G \n";
   }
   return ratPrefString;
}

std::string MyServingSystemHelper::getEndcAvailability(telux::tel::EndcAvailability isAvailable) {
   std::string availabilityString = "";
   if(isAvailable == telux::tel::EndcAvailability::AVAILABLE) {
      availabilityString += " AVAILABLE \n";
   } else if(isAvailable == telux::tel::EndcAvailability::UNAVAILABLE){
       availabilityString += " NOT AVAILABLE \n";
   } else {
       availabilityString += " UNKNOWN \n";
   }
   return availabilityString;
}

std::string MyServingSystemHelper::getDcnrRestriction(telux::tel::DcnrRestriction isRestricted) {
   std::string restrictedString = "";
   if(isRestricted == telux::tel::DcnrRestriction::RESTRICTED) {
      restrictedString += " RESTRICTED \n";
   } else if(isRestricted == telux::tel::DcnrRestriction::UNRESTRICTED) {
       restrictedString += " NOT RESTRICTED \n";
   } else {
       restrictedString += " UNKNOWN \n";
   }
   return restrictedString;
}

void MyRatPreferenceResponseCallback::ratPreferenceResponse(telux::tel::RatPreference preference,
                                                            telux::common::ErrorCode error) {
   std::cout << "\n\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\nRAT mode preference: \n"
               << MyServingSystemHelper::getRatPreference(preference);
   } else {
      PRINT_CB << "ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

std::string MyServingSystemHelper::getServiceDomain(telux::tel::ServiceDomainPreference preference) {
   std::string prefString = " Unknown";
   switch(preference) {
      case telux::tel::ServiceDomainPreference::CS_ONLY:
         prefString = " Circuit Switched(CS) only";
         break;
      case telux::tel::ServiceDomainPreference::PS_ONLY:
         prefString = " Packet Switched(PS) only";
         break;
      case telux::tel::ServiceDomainPreference::CS_PS:
         prefString = " Circuit Switched and Packet Switched ";
         break;
      default:
         break;
   }
   return prefString;
}

void MyServiceDomainResponseCallback::serviceDomainResponse(
   telux::tel::ServiceDomainPreference preference, telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\n requestServiceDomainPreference is successful.\n Service domain is "
               << MyServingSystemHelper::getServiceDomain(preference) << std::endl;
   } else {
      PRINT_CB << "\n requestServiceDomainPreference failed, ErrorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyServingSystemResponsecallback::servingSystemResponse(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "setRatPreference is successful" << std::endl;
   } else {
      PRINT_CB << "setRatPreference Request failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MyServingSystemListener::onRatPreferenceChanged(telux::tel::RatPreference preference) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << "RAT mode preference: \n"
                      << MyServingSystemHelper::getRatPreference(preference);
}

void MyServingSystemListener::onServiceDomainPreferenceChanged(
   telux::tel::ServiceDomainPreference preference) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << "\nService domain preference is"
                      << MyServingSystemHelper::getServiceDomain(preference) << std::endl;
}

void MyServingSystemListener::onDcStatusChanged(telux::tel::DcStatus dcStatus) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << "\nENDC Availability: \n"
                      << MyServingSystemHelper::getEndcAvailability(dcStatus.endcAvailability);
   PRINT_NOTIFICATION << "\nDCNR Restriction: \n"
                      << MyServingSystemHelper::getDcnrRestriction(dcStatus.dcnrRestriction);
}
