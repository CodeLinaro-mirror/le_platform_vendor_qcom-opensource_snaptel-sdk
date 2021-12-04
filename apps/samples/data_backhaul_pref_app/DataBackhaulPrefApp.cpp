/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <memory>
#include <cstdlib>

#include <telux/data/DataFactory.hpp>

/**
 * @file: DataBackhaulPrefApp.cpp
 *
 * @brief: Simple application to request backhaul preference
 */

int main(int argc, char *argv[]) {
   std::mutex mtx_;
   std::condition_variable cv_;
   bool subSystemStatusUpdated = false;
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
   std::shared_ptr<telux::data::IDataSettingsManager> dataSettingsMgr = nullptr;

   if (argc == 2) {
      telux::data::OperationType opType = static_cast<telux::data::OperationType>
          (std::atoi(argv[1]));

      auto initCb = [&](telux::common::ServiceStatus status) {
         subSystemStatus = status;
         subSystemStatusUpdated = true;
         cv_.notify_all();
      };

      // [2] Get the DataFactory and data settings Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      do {
         subSystemStatusUpdated = false;
         std::unique_lock<std::mutex> lck(mtx_);
         dataSettingsMgr = dataFactory.getDataSettingsManager(opType, initCb);
         if (dataSettingsMgr) {
            // [3] Check if data settings manager is ready
            std::cout << "\n\nInitializing Data Settings manager subsystem "
                      << ", Please wait ..." << std::endl;
            cv_.wait(lck, [&]{return subSystemStatusUpdated;});
            subSystemStatus = dataSettingsMgr->getServiceStatus();
         }
         if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** DATA Settings Manager is Ready *** " << std::endl;
            break;
         }
         else {
            std::cout << " *** Unable to initialize data Settings System *** " << std::endl;
         }
      } while (1);

      // [4] Callback for backhaul preference
      auto respCb = [](
         std::vector<telux::data::BackhaulType> backhaulPref, telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: Response"
                   << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                   << std::endl;
         if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Current Backhaul Preference is: " << std::endl;
            for(size_t i=0; i<backhaulPref.size(); ++i) {
                  switch(backhaulPref[i]) {
                     case telux::data::BackhaulType::ETH:
                        std::cout << "Ethernet" << std::endl;
                        break;
                     case telux::data::BackhaulType::USB:
                        std::cout << "USB" << std::endl;
                        break;
                     case telux::data::BackhaulType::WLAN:
                        std::cout << "WLAN" << std::endl;
                        break;
                     case telux::data::BackhaulType::WWAN:
                        std::cout << "WWAN" << std::endl;
                        break;
                     case telux::data::BackhaulType::BLE:
                        std::cout << "BLE" << std::endl;
                        break;
                     default:
                        std::cout << "Unsupported Backhaul" << std::endl;
                  }
            }
         }
      };
      // [5] Get current backhaul preference
      telux::common::Status stat = dataSettingsMgr->requestBackhaulPreference(respCb);
      if(telux::common::Status::SUCCESS != stat) {
         std::cout << "Request Backhaul returned error: " << static_cast<int>(stat) << std::endl;
      }

      // [6] Wait for request response and notifications

   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_backhaul_pref_app <operation type>\n";
      std::cout << "\n\t\t operation type (0-LOCAL, 1-REMOTE) ";
      std::cout << "\n\t ./data_backhaul_pref_app 0   --> Get backhaul preference when running";
      std::cout << "\n\t                                  this app from modem";
   }

   // [7] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [8] Cleanup
   if (dataSettingsMgr) {
      dataSettingsMgr = nullptr;
   }
   return 0;
}
