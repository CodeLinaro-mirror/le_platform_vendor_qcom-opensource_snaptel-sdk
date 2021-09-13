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
 * @file: DataDrbStatusApp.cpp
 *
 * @brief: Simple application to get current dedicated radio bearer status and listens to status
 *         change notifications
 */

// Implementation of IServingSystemListener
class ServingSystemListener : public telux::data::IServingSystemListener {
public:
   ServingSystemListener(SlotId slotId) : slotId_(slotId) {}
   void onDrbStatusChanged(telux::data::DrbStatus status) override {
      std::cout << "\n onDrbStatusChanged on SlotId: "
                << static_cast<int>(slotId_) << std::endl;
      switch(status) {
         case telux::data::DrbStatus::ACTIVE:
            std::cout << "Current Drb Status is Active" << std::endl;
            break;
         case telux::data::DrbStatus::DORMANT:
            std::cout << "Current Drb Status is Dormant" << std::endl;
            break;
         case telux::data::DrbStatus::UNKNOWN:
            std::cout << "Current Drb Status is Unknown" << std::endl;
            break;
         default:
            std::cout << "Error: Unexpected Drb Status is reported" << std::endl;
            break;
      }
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
            // [3] Check if data Serving System manager is ready
         if (dataServingSystemMgr) {
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

      // [5] Get current drb Status
      telux::data::DrbStatus drbStatus = dataServingSystemMgr->getDrbStatus();
      switch(drbStatus) {
         case telux::data::DrbStatus::ACTIVE:
            std::cout << "Current Drb Status is Active" << std::endl;
            break;
         case telux::data::DrbStatus::DORMANT:
            std::cout << "Current Drb Status is Dormant" << std::endl;
            break;
         case telux::data::DrbStatus::UNKNOWN:
            std::cout << "Current Drb Status is Unknown" << std::endl;
            break;
         default:
            std::cout << "Error: Unexpected Drb Status is reported" << std::endl;
            break;
      }

      // [6] Wait for request response and notifications

   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_drb_status_app <slotId>\n";
      std::cout << "\n\t\t slot id        Slot id to get drb status on";
      std::cout << "\n\t ./data_drb_status_app 1   --> Get drb status on slotId 1\n";
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
