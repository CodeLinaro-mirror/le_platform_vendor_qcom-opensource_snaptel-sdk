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

#include <telux/tel/NetworkSelectionManager.hpp>
#include <telux/tel/PhoneFactory.hpp>

#define DEFAULT_SLOT_ID 1
#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"

/**
 * @file: NetworkApp.cpp
 *
 * @brief: Sample application to perform network scan
 */

void logInUseStatus(telux::tel::InUseStatus status) {
   switch(status) {
      case telux::tel::InUseStatus::UNKNOWN:
         std::cout << "In-use status: UNKNOWN, ";
         break;
      case telux::tel::InUseStatus::CURRENT_SERVING:
         std::cout << "In-use status: CURRENT_SERVING, ";
         break;
      case telux::tel::InUseStatus::AVAILABLE:
         std::cout << "In-use status: AVAILABLE, ";
         break;
      default:
         break;
   }
}

void logRoamingStatus(telux::tel::RoamingStatus status) {
   switch(status) {
      case telux::tel::RoamingStatus::UNKNOWN:
         std::cout << "Roaming status: UNKNOWN, ";
         break;
      case telux::tel::RoamingStatus::HOME:
         std::cout << "Roaming status: HOME, ";
         break;
      case telux::tel::RoamingStatus::ROAM:
         std::cout << "Roaming status: ROAM, ";
         break;
      default:
         break;
   }
}

void logForbiddenStatus(telux::tel::ForbiddenStatus status) {
   switch(status) {
      case telux::tel::ForbiddenStatus::UNKNOWN:
         std::cout << "Forbidden status: UNKNOWN, ";
         break;
      case telux::tel::ForbiddenStatus::FORBIDDEN:
         std::cout << "Forbidden status: FORBIDDEN, ";
         break;
      case telux::tel::ForbiddenStatus::NOT_FORBIDDEN:
         std::cout << "Forbidden status: NOT_FORBIDDEN, ";
         break;
      default:
         break;
   }
}

void logPreferredStatus(telux::tel::PreferredStatus status) {
   switch(status) {
      case telux::tel::PreferredStatus::UNKNOWN:
         std::cout << "Preferred status: UNKNOWN" << std::endl;
         break;
      case telux::tel::PreferredStatus::PREFERRED:
         std::cout << "Preferred status: PREFERRED" << std::endl;
         break;
      case telux::tel::PreferredStatus::NOT_PREFERRED:
         std::cout << "Preferred status: NOT_PREFERRED" << std::endl;
         break;
      default:
         break;
   }
}

void responseCallback(
   std::vector<telux::tel::OperatorInfo> operatorInfos, telux::common::ErrorCode error) {
   std::cout << std::endl;
   PRINT_CB << "\n************ Received network scan response ************" << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << "Operator info size: " << operatorInfos.size() << std::endl;

      for(auto it : operatorInfos) {
         std::cout << "Operator name: " << it.getName() << "\nMcc: " << it.getMcc()
                   << "\nMnc: " << it.getMnc() << std::endl;
         logInUseStatus(it.getStatus().inUse);
         logRoamingStatus(it.getStatus().roaming);
         std::cout << std::endl;
         logForbiddenStatus(it.getStatus().forbidden);
         logPreferredStatus(it.getStatus().preferred);
         std::cout << std::endl;
      }
      std::cout << "\n*********************************************************\n";
   } else {
      PRINT_CB << "Network scan failed, ErrorCode : " << static_cast<int>(error) << std::endl;
   }
}

int main(int argc, char **argv) {
   // [1] Get phone factory and network selection manager instances
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::shared_ptr<telux::tel::INetworkSelectionManager> networkMgr
      = phoneFactory.getNetworkSelectionManager(DEFAULT_SLOT_ID);

   // [2] Check if network selection subsystem is ready
   bool subSystemStatus = networkMgr->isSubsystemReady();

   // [2.1] If network selection subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "\n Network selection subsystem is not ready, please wait" << std::endl;
      std::future<bool> f = networkMgr->onSubsystemReady();
      // If we want to wait unconditionally for network selection subsystem to be ready
      subSystemStatus = f.get();
   }

   // [3] Exit the application, if SDK is unable to initialize network selection subsystem
   if(subSystemStatus) {
      std::cout << "\n *** Network selection subsystem is ready *** " << std::endl;
   } else {
      std::cout << "\n *** ERROR - Unable to initialize network selection subsystem *** "
                << std::endl;
      return 1;
   }

   std::cout << "\n Scanning network... Please wait. \n";

   // [4] Perform network scan
   networkMgr->performNetworkScan(responseCallback);

   // [5] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit \n";
   std::cin.ignore();
   return 0;
}
