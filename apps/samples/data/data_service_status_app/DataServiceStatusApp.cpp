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
   ServingSystemListener(SlotId slotId) : slotId_(slotId) {}
   friend void logServiceStatusDetails(const telux::data::ServiceStatus& status);
   void onServiceStateChanged(telux::data::ServiceStatus status) override {
      std::cout << "\n onServiceStateChanged on SlotId: " << static_cast<int>(slotId_);
      logServiceStatusDetails(status);
   }

private:
   SlotId slotId_;
};

int main(int argc, char *argv[]) {
   std::mutex mtx_;
   std::condition_variable cv_;
   bool subSystemStatusUpdated = false;
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
   std::shared_ptr<telux::data::IServingSystemManager> dataServingSystemMgr = nullptr;
   std::shared_ptr<telux::data::IServingSystemListener> dataListener = nullptr;

   if (argc == 2) {
      SlotId slotId = static_cast<SlotId>(std::atoi(argv[1]));

      dataListener = std::make_shared<ServingSystemListener>(slotId);
      // [1] Instantiate initialization callback - this is optional
      auto initCb = [&](telux::common::ServiceStatus status) {
         subSystemStatus = status;
         subSystemStatusUpdated = true;
         cv_.notify_all();
      };

      // [2] Get the DataFactory and data Serving System Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      do {
         subSystemStatusUpdated = false;
         std::unique_lock<std::mutex> lck(mtx_);
         dataServingSystemMgr = dataFactory.getServingSystemManager(slotId, initCb);
         if (dataServingSystemMgr) {
            // [3] Check if data serving system manager is ready
            std::cout << "\n\nInitializing Data Serving System manager subsystem on slot " <<
                  slotId << ", Please wait ..." << std::endl;
            cv_.wait(lck, [&]{return subSystemStatusUpdated;});
            subSystemStatus = dataServingSystemMgr->getServiceStatus();
         }
         if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** DATA Serving System is Ready *** " << std::endl;
            break;
         }
         else {
            std::cout << " *** Unable to initialize data Serving System *** " << std::endl;
         }
      } while (1);

      // [4] Register for Serving System listener
      dataServingSystemMgr->registerListener(dataListener);

      // [5] Get current Service Status
      // Callback
      auto respCb = [&slotId](
                     telux::data::ServiceStatus serviceStatus, telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                     << "requestServiceStatus Response on slotid " << static_cast<int>(slotId);
         if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            logServiceStatusDetails(serviceStatus);
         }
         else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         }
      };
      dataServingSystemMgr->requestServiceStatus(respCb);

      // [6] Wait for request response and notifications

   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_service_status_app <slotId>\n";
      std::cout << "\n\t\t slot id        Slot id to get service status on";
      std::cout << "\n\t ./data_service_status_app 1   --> Get service status on slotId 1\n";
   }

   // [7] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [8] Cleanup
   if (dataServingSystemMgr) {
      dataServingSystemMgr->deregisterListener(dataListener);
      dataServingSystemMgr = nullptr;
   }
   return 0;
}
