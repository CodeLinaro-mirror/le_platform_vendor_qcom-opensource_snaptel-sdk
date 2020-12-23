/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
#include <telux/data/net/NatManager.hpp>


/**
 * @file: DataSnatApp.cpp
 *
 * @brief: Simple application to creat Static NAT
 *         ./snat_sample_app <operation type> <profile id> <ip address> <private port>
 *                           <global port> <protocol>
 */

std::promise<int> promise;

int main(int argc, char *argv[]) {
   bool subSystemStatusUpdated = false;
   std::condition_variable initCv;
   std::mutex mtx;
   std::shared_ptr<telux::data::net::INatManager> dataSnatMgr = nullptr;
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

   if(argc == 8) {
      telux::data::OperationType opType = static_cast<telux::data::OperationType>
          (std::atoi(argv[1]));
      SlotId slotId = static_cast<SlotId>(std::atoi(argv[2]));
      int profileId = std::atoi(argv[3]);
      std::string ipAddr = static_cast<std::string>(argv[4]);
      int localIpPort = std::atoi(argv[5]);
      int globalIpPort = std::atoi(argv[6]);
      int proto = std::atoi(argv[7]);

      // [1] Instantiate initialization callback - this is optional
      auto initCb = [&](telux::common::ServiceStatus status) {
         std::lock_guard<std::mutex> lock(mtx);
         subSystemStatusUpdated = true;
         initCv.notify_all();
      };

      // [2] Get the DataFactory and Nat Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      do {
         subSystemStatusUpdated = false;
         dataSnatMgr  = dataFactory.getNatManager(opType, initCb);
         if (dataSnatMgr) {
            // [3] Check if Nat manager is ready
            subSystemStatus = dataSnatMgr->getServiceStatus();

            // [3.1] If Nat manager is not ready, wait for it to be ready
            if (subSystemStatus == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
               std::cout <<
                   "\n\nInitializing Nat Manager subsystem Please wait ..." << std::endl;
               std::unique_lock<std::mutex> lck(mtx);
               initCv.wait(lck, [&]{return subSystemStatusUpdated;});
               subSystemStatus = dataSnatMgr->getServiceStatus();
            }
         }
         if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** Nat SubSystem is Ready *** " << std::endl;
            break;
         }
         else {
            std::cout << " *** Unable to initialize Nat subsystem *** " << std::endl;
         }
      } while(1);

      // [4] Instantiate create static NAT callback instance - this is optional
      auto respCb = [](telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                   << "addStaticNatEntry"
                   << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
         promise.set_value(1);
      };

      // [5] Create Static NAT entry
      struct telux::data::net::NatConfig natConfig;
      natConfig.addr = ipAddr;
      natConfig.port = (uint16_t)localIpPort;
      natConfig.globalPort = (uint16_t)globalIpPort;
      natConfig.proto = (uint8_t)proto;
      std::future<int> future = promise.get_future();
      dataSnatMgr->addStaticNatEntry(profileId, natConfig, respCb);

      // [6] Wait for callback - this is optional
      int tmp = future.get();
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./snat_sample_app <operation type> <slotid> <profileid> <ip address> "
                   "\n\t                   <private port> <global port> <protocol>";
      std::cout << std::endl;
      std::cout << "\n\t\t operation type (0-LOCAL, 1-REMOTE)";
      std::cout << "\n\t\t slot id        Slot id that contains modem profile";
      std::cout << "\n\t\t profile id     modem profile id to add static entry on ";
      std::cout << "\n\t\t ip address (IPv4 or IPv6 format)";
      std::cout << "\n\t\t protocol (1-ICMP, 2-IGMP, 6-TCP, 17-UDP, 50-ESP)";
      std::cout << std::endl;
      std::cout << "\n\t ./snat_sample_app 1 1 5 192.168.225.22 500 500 6 --> to add Static NAT"
                   "\n\t                   entry on slot 1 profile id 5 for specified IPv4 address"
                   "\n\t                   over TCP protocol and map local port 500 to global port";
                   "\n\t                   500\n";
   }

   // [7] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
