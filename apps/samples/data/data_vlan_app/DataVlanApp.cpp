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
#include <telux/data/net/VlanManager.hpp>


/**
 * @file: DataVlanApp.cpp
 *
 * @brief: Simple application to creat Vlan
 *         ./vlan_sample_app <operation> <interface > <vlan id> <profile id> <acceleration>
 */

std::promise<int> promise;


int main(int argc, char *argv[]) {
   bool subSystemStatusUpdated = false;
   std::condition_variable initCv;
   std::mutex mtx;
   std::shared_ptr<telux::data::net::IVlanManager> dataVlanMgr = nullptr;
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

   if(argc == 7) {
      telux::data::OperationType opType = static_cast<telux::data::OperationType>
          (std::atoi(argv[1]));
      telux::data::InterfaceType infType = static_cast<telux::data::InterfaceType>
          (std::atoi(argv[2]));
      int vlanId         = std::atoi(argv[3]);
      SlotId slotId = static_cast<SlotId>(std::atoi(argv[4]));
      int profileId      = std::atoi(argv[5]);
      bool isAccelerated = false;
      if(std::atoi(argv[6])) {
         isAccelerated = true;
      }

      // [1] Instantiate initialization callback - this is optional
      auto initCb = [&](telux::common::ServiceStatus status) {
         std::lock_guard<std::mutex> lock(mtx);
         subSystemStatusUpdated = true;
         initCv.notify_all();
      };

      // [2] Get the DataFactory and VLAN Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      do {
         subSystemStatusUpdated = false;
         std::unique_lock<std::mutex> lck(mtx);
         dataVlanMgr  = dataFactory.getVlanManager(opType, initCb);
         if(dataVlanMgr) {
            // [3] Check if Vlan manager is ready
            std::cout <<
                  "\n\nInitializing Vlan Manager subsystem Please wait ..." << std::endl;
            initCv.wait(lck, [&]{return subSystemStatusUpdated;});
            subSystemStatus = dataVlanMgr->getServiceStatus();
         }
         if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** Vlan SubSystem is Ready *** " << std::endl;
            break;
         }
         else {
            std::cout << " *** Unable to initialize Vlan subsystem *** " << std::endl;
         }
      } while(1);

      // [4] Instantiate create vlan callback instance - this is optional
      auto respCbCreate = [](bool isAccelerated, telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                   << "createVlan Response"
                   << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                   << ". ErrorCode: " << static_cast<int>(error);
         std::cout << " Acceleration " << (isAccelerated ? "is allowed" : "is not allowed") << "\n";
         promise.set_value(1);
      };

      // [5] Create VLAN
      telux::data::VlanConfig config;
      config.iface = infType;
      config.vlanId = vlanId;
      config.isAccelerated = isAccelerated;
      std::future<int> future = promise.get_future();
      dataVlanMgr->createVlan(config, respCbCreate);

      // [6] Wait for create vlan callback - this is optional
      int tmp = future.get();
      promise = std::promise<int>();

      // [7] Instantiate bind vlan to profile id callback instance - this is optional
      auto respCbBind = [](telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                   << "bindWithProfile Response"
                   << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                   << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         promise.set_value(1);
      };

      // [8] Bind newly created vlan with modem profile
      future = promise.get_future();
      dataVlanMgr->bindWithProfile(profileId, vlanId, respCbBind, slotId);
      tmp = future.get();
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./vlan_sample_app <operation> <interface > <vlan id> <slotId> <profileId> <acc>";
      std::cout << std::endl;
      std::cout << "\n\t\t operation type (0-LOCAL, 1-REMOTE)";
      std::cout << "\n\t\t interface type (1-WLAN, 2-ETH, 3-ECM, 4-RNDIS, 5-MHI)";
      std::cout << "\n\t\t vlan id        id to be assigned to newly created vlan";
      std::cout << "\n\t\t slot id        Slot id that contains modem profile";
      std::cout << "\n\t\t profile id     modem profile id to be bind to newly created vlan";
      std::cout << "\n\t\t acceleration type       (0-false, 1-true)";
      std::cout << std::endl;
      std::cout << "\n\t ./vlan_sample_app 1 3 5 1 1 0 --> Creat remote vlan id 5 with ECM interface";
      std::cout << "\n\t                                   and slot 1 profile 1 no acceleration \n";
   }

   // [7] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
