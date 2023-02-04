/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <future>

#include <telux/wlan/WlanDefines.hpp>
#include <telux/wlan/WlanFactory.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>


/**
 * @file: WlanConfigApp.cpp
 *
 * @brief: Simple application to Configure and enable Wlan
 *         ./wlan_config_app <Num of APs> <Num of Sta>
 *         Settings of AP and Sta will be encapsulated in
 *         hostapd.conf and wpa_supplicant.conf correspondingly.
 *         See readme file for location of those files.
 */

class NotificationListener: public telux::wlan::IWlanListener {
public:
   void onEnableChanged(bool enable) {
      promise_.set_value(enable);
   }

   bool getEnableStatus() {
      return promise_.get_future().get();
   }

   void resetPromise() {
      promise_ = std::promise<bool>();
   }
private:
   std::promise<bool> promise_;
};

int main(int argc, char *argv[]) {
   std::promise<telux::common::ServiceStatus> initPromise;
   telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
   std::shared_ptr<telux::wlan::IWlanDeviceManager> wlanDevMgr = nullptr;
   auto listener = std::make_shared<NotificationListener>();
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

   if(argc == 3) {
      int numAp = std::atoi(argv[1]);
      int numSta = std::atoi(argv[2]);

      // [1] Instantiate initialization callback - this is optional
      auto initCb = [&](telux::common::ServiceStatus status) {
         initPromise.set_value(status);
      };

      // [2] Get the WlanFactory and Device Manager instance
      auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
      do {
         wlanDevMgr  = wlanFactory.getWlanDeviceManager(initCb);
         if (wlanDevMgr) {
            // [3] Check if Device manager is ready
            std::cout <<
                  "\nInitializing Wlan subsystem Please wait ..." << std::endl;
            subSystemStatus = initPromise.get_future().get();
         }
         if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** Wlan SubSystem is Ready *** " << std::endl;
         }
         else {
            std::cout << " *** Unable to initialize Wlan subsystem *** " << std::endl;
            break;
         }

         // [4] If Wlan is enabled, disable Wlan - this necessary before changing configuration
         bool enableStat = false;
         std::cout << "Checking Wlan Status... Please wait" << std::endl;
         std::vector<telux::wlan::InterfaceStatus> ifStatus;
         if(telux::common::ErrorCode::SUCCESS != wlanDevMgr->getStatus(enableStat, ifStatus)) {
            std::cout << "Failed to retrieve Wlan status... quitting application" <<std::endl;
            break;
         }
         std::cout << "Wlan is " << ((enableStat)? "Enabled":"Disabled") << std::endl;
         if(enableStat) {
            std::cout << "Disabling Wlan before changing configuration... Please wait" << std::endl;
            wlanDevMgr->registerListener(listener);
            //Ensure callback returns SUCCESS and indication for Wlan disablement is received
            if((telux::common::ErrorCode::SUCCESS != wlanDevMgr->enable(false)) ||
               (true == listener->getEnableStatus())) {
               std::cout << "Failed to disable Wlan ... quitting application" <<std::endl;
               break;
            }
         }
         // [5] Set Wlan configuration
         std::cout << "Setting Wlan Config .... Please wait" << std::endl;
         errCode = wlanDevMgr->setMode(numAp, numSta);
         if(telux::common::ErrorCode::SUCCESS != errCode) {
            std::cout << "Failed to set Wlan configuration... Error Code: "
                      << static_cast<int>(errCode) << ". quitting application" <<std::endl;
            break;
         }

         // [6] Enable Wlan for new configuration to take effect
         listener->resetPromise();
         std::cout << "Enabling Wlan to activate new configuration... Please wait" << std::endl;
         //Ensure callback returns SUCCESS and indication for Wlan enablement is received
         if((telux::common::ErrorCode::SUCCESS != wlanDevMgr->enable(true)) ||
            (false == listener->getEnableStatus())) {
            std::cout << "Failed to enable Wlan ... quitting application" <<std::endl;
            break;
         }
      } while(0);

      // [7] Cleanup
      if(wlanDevMgr) {
         wlanDevMgr->deregisterListener(listener);
         wlanDevMgr = nullptr;
      }
      if(listener) {
         listener = nullptr;
      }
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./wlan_config_app <Num of APs> <Num of Sta>";
      std::cout << std::endl;
      std::cout << "\n\t\t Num of APs     Number of Access Points to be configured";
      std::cout << "\n\t\t Num of Sta     Number of Stations to be configured";
      std::cout << std::endl;
      std::cout << "\n\t ./wlan_config_app 1 1 --> to configure and enable AP+STA\n";
   }

   // [7] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
