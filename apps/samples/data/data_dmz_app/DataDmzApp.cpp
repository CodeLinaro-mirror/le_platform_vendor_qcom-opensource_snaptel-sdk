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

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/FirewallManager.hpp>


/**
 * @file: DataDmzApp.cpp
 *
 * @brief: Simple application to creat DMZ
 *         ./dmz_sample_app <operation type> <profile id> <ip address>
 */

int main(int argc, char *argv[]) {
   std::promise<int> promise;
   bool subSystemStatusUpdated = false;
   std::condition_variable initCv;
   std::mutex mtx;
   std::shared_ptr<telux::data::net::IFirewallManager> dataFwMgr = nullptr;
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

   if(argc == 5) {
      telux::data::OperationType opType = static_cast<telux::data::OperationType>
          (std::atoi(argv[1]));
      SlotId slotId = static_cast<SlotId>(std::atoi(argv[2]));
      int profileId = std::atoi(argv[3]);
      std::string ipAddr = static_cast<std::string>(argv[4]);

      // [1] Instantiate initialization callback - this is optional
      auto initCb = [&](telux::common::ServiceStatus status) {
         std::lock_guard<std::mutex> lock(mtx);
         subSystemStatusUpdated = true;
         initCv.notify_all();
      };

      // [2] Get the DataFactory and Firewall Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      do {
         subSystemStatusUpdated = false;
         std::unique_lock<std::mutex> lck(mtx);
         dataFwMgr  = dataFactory.getFirewallManager(opType, initCb);
         if (dataFwMgr) {
            // [3] Check if Firewall manager is ready
            std::cout <<
                  "\n\nInitializing Firewall Manager subsystem Please wait ..." << std::endl;
            initCv.wait(lck, [&]{return subSystemStatusUpdated;});
            subSystemStatus = dataFwMgr->getServiceStatus();
         }
         if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** Firewall Sub System is Ready *** " << std::endl;
            break;
         }
         else {
            std::cout << " *** Unable to initialize Firewall subsystem *** " << std::endl;
         }
      } while(1);

      // [4] Instantiate add DMZ callback instance - this is optional
      auto respCb = [&](telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                   << "addDmz Response"
                   << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
         promise.set_value(1);
      };

      // [5] Add DMZ entry
      std::future<int> future = promise.get_future();
      dataFwMgr->enableDmz(profileId, ipAddr, respCb, slotId);

      // [6] Wait for callback - this is optional
      int tmp = future.get();
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./dmz_sample_app <operation type> <slotId> <profileId> <ip address>";
      std::cout << std::endl;
      std::cout << "\n\t\t operation type (0-LOCAL, 1-REMOTE)";
      std::cout << "\n\t\t slot id        Slot id that contains modem profile";
      std::cout << "\n\t\t profile id     modem profile id to enable dmz on";
      std::cout << "\n\t\t ip address (IPv4 or IPv6 format)";
      std::cout << std::endl;
      std::cout << "\n\t ./dmz_sample_app 0 1 5 192.168.225.22 --> to enable local DMZ on specified";
      std::cout << "\n\t                  I                        IPv4 address\n";
   }

   // [7] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
