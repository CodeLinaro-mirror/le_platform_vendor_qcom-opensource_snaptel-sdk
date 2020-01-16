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
#include <telux/data/net/FirewallManager.hpp>


/**
 * @file: DataFwlEnApp.cpp
 *
 * @brief: Simple application to Enable Firewall
 *         ./fwlEnbl_sample_app <operation type> <profile id > <enable/disable> <allow/drop packets>
 */

std::promise<int> promise;

int main(int argc, char *argv[]) {
   if(argc == 5) {
      telux::data::OperationType opType = static_cast<telux::data::OperationType>
          (std::atoi(argv[1]));
      bool fwEnable = false;
      int profileId = std::atoi(argv[2]);
      if(std::atoi(argv[3])) {
         fwEnable = true;
      }
      bool allowPackets = false;
      if(std::atoi(argv[4])) {
         allowPackets = true;
      }

      // [1] Get the DataFactory and Firewall Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      auto dataFwMgr  = dataFactory.getFirewallManager(opType);

      // [2] Check if data subsystem is ready
      bool subSystemStatus = dataFwMgr->isSubsystemReady();

      // [2.1] If data subsystem is not ready, wait for it to be ready
      if(!subSystemStatus) {
         std::cout << "Firewall subsystem is not ready" << std::endl;
         std::cout << "wait unconditionally for it to be ready " << std::endl;
         std::future<bool> f = dataFwMgr->onSubsystemReady();
         // If we want to wait unconditionally for data subsystem to be ready
         subSystemStatus = f.get();
      }

      // [3] Exit the application, if SDK is unable to initialize firewall subsystems
      if(subSystemStatus) {
         std::cout << " *** Firewall Sub System is Ready *** " << std::endl;
      } else {
         std::cout << " *** ERROR - Unable to initialize Firewall subsystem *** " << std::endl;
         return 1;
      }

      // [4] Instantiate set firewall callback instance - this is optional
      auto respCb = [](telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                   << "setFirewall Response"
                   << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
         promise.set_value(1);
      };

      // [5] Configure firewall
      std::future<int> future = promise.get_future();
      dataFwMgr->setFirewall(profileId, fwEnable, allowPackets, respCb);

      // [6] Wait for callback - this is optional
      int tmp = future.get();
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./fwlEnbl_sample_app <operation type> <enable> <allow>";
      std::cout << std::endl;
      std::cout << "\n\t\t operation type (0-LOCAL, 1-REMOTE)";
      std::cout << "\n\t\t profile id     modem profile id to enable firewall on";
      std::cout << "\n\t\t enable (1-On, 0-Off)";
      std::cout << "\n\t\t allow (1-Accept, 0-Drop)";
      std::cout << std::endl;
      std::cout << "\n\t ./fwlEnbl_sample_app 1 5 1 1 ->enable firewall on remote host and allow ";
      std::cout << "\n\t                      packets";
   }

   // [7] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
