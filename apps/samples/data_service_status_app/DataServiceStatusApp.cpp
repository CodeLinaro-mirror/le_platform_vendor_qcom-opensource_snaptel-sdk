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

#include <iostream>
#include <memory>
#include <cstdlib>

#include <telux/data/DataFactory.hpp>

/**
 * @file: DataServiceStatusApp.cpp
 *
 * @brief: Simple application to request service status and listens to service status change
           notifications
 */

//Function to log all Service Status Details
void logServiceStatusDetails(const telux::data::ServiceStatus& status) {
   std::cout << " ** Service Status Details **\n";
   if(status.serviceState == telux::data::DataServiceState::OUT_OF_SERVICE) {
      std::cout << "Current Status is Out Of Service" << std::endl;
   } else {
      std::cout << "Current Status is In Service" << std::endl;
      std::cout << "Preferred Rat is " ;
      switch (status.networkRat) {
         case telux::data::NetworkRat::CDMA_1X:
               std::cout << "CDMA 1X" << std::endl;
               break;
         case telux::data::NetworkRat::CDMA_EVDO:
               std::cout << "CDMA EVDO" << std::endl;
               break;
         case telux::data::NetworkRat::GSM:
              std::cout << "GSM" << std::endl;
               break;
         case telux::data::NetworkRat::WCDMA:
               std::cout << "WCDMA" << std::endl;
               break;
         case telux::data::NetworkRat::LTE:
               std::cout << "LTE" << std::endl;
               break;
         case telux::data::NetworkRat::TDSCDMA:
               std::cout << "TDSCDMA" << std::endl;
               break;
         case telux::data::NetworkRat::NR5G:
               std::cout << "NR5G" << std::endl;
               break;
         default:
               std::cout << "UNKNOWN" << std::endl;
               break;
      }
   }
}

// Implementation of IServingSystemListener
class ServingSystemListener : public telux::data::IServingSystemListener {
public:
   ServingSystemListener() {}
   friend void logServiceStatusDetails(const telux::data::ServiceStatus& status);
   void onServiceStateChanged(telux::data::ServiceStatus status) override {
      std::cout << "\n onServiceStateChanged " ;
      logServiceStatusDetails(status);
   }
};

int main(int argc, char *argv[]) {
   std::condition_variable cv_;
   bool subSystemStatusUpdated = false;
   std::shared_ptr<telux::data::IServingSystemManager> servingSystemMgr = nullptr;
   std::shared_ptr<telux::data::IServingSystemListener> servingSystemListener = nullptr;

   if (argc == 1) {
      servingSystemListener = std::make_shared<ServingSystemListener>();

      // [1] Get the DataFactory and data Serving System Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      servingSystemMgr = dataFactory.getServingSystemManager();

      // [2] Check if Serving Manger is ready
      bool subSystemStatus = servingSystemMgr->isSubsystemReady();

      // [2.1] If data subsystem is not ready, wait for it to be ready
      if(!subSystemStatus) {
         std::cout << "Serving System Manager is not ready" << std::endl;
         std::cout << "wait unconditionally for it to be ready " << std::endl;
         std::future<bool> f = servingSystemMgr->onSubsystemReady();
         // If we want to wait unconditionally for data subsystem to be ready
         subSystemStatus = f.get();
      }

      // [3] Exit the application, if SDK is unable to initialize data subsystems
      if(subSystemStatus) {
         std::cout << " *** Serving System Manager is Ready *** " << std::endl;
      } else {
         std::cout << " *** ERROR - Unable to initialize Serving System Manager *** " << std::endl;
         return 1;
      }

      // [4] Register for Serving System listener
      servingSystemMgr->registerListener(servingSystemListener);

      // [5] Get current Service Status
      // Callback
      auto respCb = [](telux::data::ServiceStatus serviceStatus, telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                     << "requestServiceStatus Response" ;
         if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            logServiceStatusDetails(serviceStatus);
         }
         else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         }
      };
      servingSystemMgr->requestServiceStatus(respCb);

      // [6] Wait for request response and notifications

   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_service_status_app\n";
      std::cout << "\n\t ./data_service_status_app - Get service status on slotId 1\n";
   }

   // [7] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [8] Cleanup
   if (servingSystemMgr) {
      servingSystemMgr->deregisterListener(servingSystemListener);
      servingSystemMgr = nullptr;
   }
   return 0;
}
