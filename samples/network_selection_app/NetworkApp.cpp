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
#include <thread>
#include <chrono>

#include <telux/tel/NetworkSelectionManager.hpp>
#include <telux/tel/PhoneFactory.hpp>

#define DEFAULT_SLOT_ID 1

#define PRINT_CB std::cout << "\033[1;35mCALLBACK: \033[0m"

/**
 * @file: NetworkApp.cpp
 *
 * @brief: Simple application to get network selection mode
 */

// Response callback for get network selection mode
class SelectionModeResponseCallback {
public:
   static void selectionModeResponse(telux::tel::NetworkSelectionMode networkSelectionMode,
                                     telux::common::ErrorCode error);

private:
   static std::string logNetworkSelectionMode(telux::tel::NetworkSelectionMode mode);
};

void SelectionModeResponseCallback::selectionModeResponse(
   telux::tel::NetworkSelectionMode networkSelectionMode, telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Network selection mode: " << logNetworkSelectionMode(networkSelectionMode)
               << std::endl;
   } else {
      PRINT_CB << "\n setNetworkSelectionMode failed, ErrorCode: " << static_cast<int>(error)
               << std::endl;
   }
}

std::string
   SelectionModeResponseCallback::logNetworkSelectionMode(telux::tel::NetworkSelectionMode mode) {
   std::string modeString = "UNKNOWN";
   switch(mode) {
      case telux::tel::NetworkSelectionMode::AUTOMATIC:
         modeString = "AUTOMATIC";
         break;
      case telux::tel::NetworkSelectionMode::MANUAL:
         modeString = "MANUAL";
         break;
      default:
         break;
   }
   return modeString;
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
      std::cout << "network selection subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = networkMgr->onSubsystemReady();
      // If we want to wait unconditionally for network selection subsystem to be ready
      subSystemStatus = f.get();
   }

   // [3] Exit the application, if SDK is unable to initialize network selection subsystem
   if(subSystemStatus) {
      std::cout << " *** network selection subsystem is ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize network selection subsystem *** "
                << std::endl;
      return 1;
   }

   // [4] Get network selection mode
   networkMgr->requestNetworkSelectionMode(SelectionModeResponseCallback::selectionModeResponse);

   // [5] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit \n\n";
   std::cin.ignore();
   return 0;
}
