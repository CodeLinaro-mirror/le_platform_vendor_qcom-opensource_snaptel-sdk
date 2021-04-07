/*
 *  Copyright (c) 2020-2021, The Linux Foundation. All rights reserved.
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
 * @file: data_serving_system_app.cpp
 *
 * @brief: Simple application to register for data serving system drb status change
 */


// Implementation of IServingSystemListener
class ServingSystemListener : public telux::data::IServingSystemListener {
public:
   void onDrbStatusChanged(telux::data::DrbStatus status) override {
      std::cout << "Data ServingSystem Drb status is :"
               << static_cast<int>(status) << std::endl;;
   }
};

int main(int argc, char *argv[]) {
   std::mutex mtx_;
   std::condition_variable cv_;
   bool subSystemStatusUpdated = false;
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

   //Init callback
   auto initCb = [&](telux::common::ServiceStatus status) {
      subSystemStatus = status;
      subSystemStatusUpdated = true;
      cv_.notify_all();
   };

   // [1] Get the DataFactory
   auto &dataFactory = telux::data::DataFactory::getInstance();
   std::unique_lock<std::mutex> lck(mtx_);
   auto dataServingMgr = dataFactory.getServingSystemManager(DEFAULT_SLOT_ID, initCb);

   // [2] Wait for data serving system subsystem initialization
   std::cout << " Checking Data Serving System Manager readiness .. please wait " << std::endl;
   if (dataServingMgr) {
      cv_.wait(lck, [&]{return subSystemStatusUpdated;});
      subSystemStatus = dataServingMgr->getServiceStatus();
   }

   // [2.1] Check for data serving system subsystem readiness
   if(subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << " *** Data Serving System Manager is Ready *** " << std::endl;
   }
   else {
      std::cout << " *** Data Serving System Manager is not Ready *** " << std::endl;
      return 1;
   }

   // [3] Register for Serving System listener
   std::shared_ptr<telux::data::IServingSystemListener> servingSystemListener
      = std::make_shared<ServingSystemListener>();
   dataServingMgr->registerListener(servingSystemListener);

   // [4] Wait for Drb events or exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [6] Cleanup
   dataServingMgr->deregisterListener(servingSystemListener);
   dataServingMgr = nullptr;
   return 0;
}
