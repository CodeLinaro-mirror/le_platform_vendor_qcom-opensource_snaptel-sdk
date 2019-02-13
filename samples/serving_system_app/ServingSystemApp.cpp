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
#include <memory>

#include <telux/tel/ServingSystemManager.hpp>
#include <telux/tel/PhoneFactory.hpp>

#define DEFAULT_SLOT_ID 1

#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"

/**
 * @file: ServingSystemApp.cpp
 *
 * @brief: Simple application to get service domain preference
 */

class ServiceDomainResponseCallback {
public:
   static void serviceDomainResponse(telux::tel::ServiceDomainPreference preference,
                                     telux::common::ErrorCode error);

private:
   static std::string getServiceDomain(telux::tel::ServiceDomainPreference preference);
};

void ServiceDomainResponseCallback::serviceDomainResponse(
   telux::tel::ServiceDomainPreference preference, telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "\nService domain preference:  " << getServiceDomain(preference) << std::endl;
   } else {
      PRINT_CB << "\n requestServiceDomainPreference failed, ErrorCode: " << static_cast<int>(error)
               << std::endl;
   }
}

std::string
   ServiceDomainResponseCallback::getServiceDomain(telux::tel::ServiceDomainPreference preference) {
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

int main(int argc, char *argv[]) {
   // [1] Get phone factory and serving system manager instances
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto servingSystemMgr = phoneFactory.getServingSystemManager(DEFAULT_SLOT_ID);

   // [2] Check if serving subsystem is ready
   bool subSystemStatus = false;

   if(servingSystemMgr) {
      subSystemStatus = servingSystemMgr->isSubsystemReady();
   }

   // [2.1] If serving subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "serving subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      if(servingSystemMgr) {
         std::future<bool> f = servingSystemMgr->onSubsystemReady();
         // If we want to wait unconditionally for serving subsystem to be ready
         subSystemStatus = f.get();
      }
   }

   // [3] Exit the application, if SDK is unable to initialize serving subsystem
   if(subSystemStatus) {
      std::cout << " *** serving subsystem is ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize serving subsystem *** " << std::endl;
      return 1;
   }

   // [5] Get service domain prefernece
   servingSystemMgr->requestServiceDomainPreference(
      ServiceDomainResponseCallback::serviceDomainResponse);

   // [6] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit \n\n";
   std::cin.ignore();
   return 0;
}
