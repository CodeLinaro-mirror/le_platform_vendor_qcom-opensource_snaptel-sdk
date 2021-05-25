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
 * @file: DataRoamingStatusApp.cpp
 *
 * @brief: Simple application to request roaming status and listens to roaming status change
           notifications
 */

//Function to log all Roaming Status Details
void logRoamingStatusDetails(const telux::data::RoamingStatus& status) {
   std::cout << " ** Roaming Status Details **\n";
   bool isRoaming = status.isRoaming;
   if(isRoaming) {
      std::cout << "System is in Roaming State" << std::endl;
      std::cout << "Roaming Type: ";
      switch(status.type)  {
         case telux::data::RoamingType::INTERNATIONAL:
             std::cout << "International" << std::endl;
         break;
         case telux::data::RoamingType::DOMESTIC:
             std::cout << "Domestic" << std::endl;
         break;
         default:
             std::cout << "Unknown" << std::endl;
      }
   } else {
      std::cout << "System is not in Roaming State" << std::endl;
   }
}

// Implementation of IServingSystemListener
class ServingSystemListener : public telux::data::IServingSystemListener {
public:
   ServingSystemListener(SlotId slotId) : slotId_(slotId) {}
   friend void logRoamingStatusDetails(const telux::data::ServiceStatus& status);
   void onRoamingStatusChanged(telux::data::RoamingStatus status) override {
      std::cout << "\n onRoamingStatusChanged on SlotId: "
                << static_cast<int>(slotId_) << std::endl;
      logRoamingStatusDetails(status);
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
            // [3] Check if data Serving System manager is ready
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
                     telux::data::RoamingStatus roamingStatus, telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                     << "requestRoamingStatus Response on slotid " << static_cast<int>(slotId);
         if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            logRoamingStatusDetails(roamingStatus);
         }
         else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         }
      };
      dataServingSystemMgr->requestRoamingStatus(respCb);

      // [6] Wait for request response and notifications

   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_roaming_status_app <slotId>\n";
      std::cout << "\n\t\t slot id        Slot id to get service status on";
      std::cout << "\n\t ./data_roaming_status_app 1   --> Get service status on slotId 1\n";
   }

   // [6] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [7] Cleanup
   if (dataServingSystemMgr) {
      dataServingSystemMgr->deregisterListener(dataListener);
      dataServingSystemMgr = nullptr;
   }
   return 0;
}
