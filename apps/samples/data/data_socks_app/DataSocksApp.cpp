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

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <memory>
#include <cstdlib>

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/net/SocksManager.hpp>


/**
 * @file: DataSocksApp.cpp
 *
 * @brief: Simple application to enable Socks Proxy
 *         ./socks_sample_app <operation> <enable/disable>
 * @note: Please follow instructions in readme.txt file located in same folder as this app
 *        to setup Socks Proxy before running this app.
 */

std::promise<int> promise;


int main(int argc, char *argv[]) {
   bool subSystemStatusUpdated = false;
   std::condition_variable initCv;
   std::mutex mtx;
   std::shared_ptr<telux::data::net::ISocksManager> dataSocksMgr = nullptr;
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

   if(argc == 3) {
      telux::data::OperationType opType = static_cast<telux::data::OperationType>
          (std::atoi(argv[1]));
      bool enable = false;
      if(std::atoi(argv[2])) {
         enable = true;
      }

      // [1] Instantiate initialization callback - this is optional
      auto initCb = [&](telux::common::ServiceStatus status) {
         std::lock_guard<std::mutex> lock(mtx);
         subSystemStatusUpdated = true;
         initCv.notify_all();
      };

      // [2] Get the DataFactory and Socks Manager instance
      auto &dataFactory = telux::data::DataFactory::getInstance();
      do {
         subSystemStatusUpdated = false;
         std::unique_lock<std::mutex> lck(mtx);
         dataSocksMgr  = dataFactory.getSocksManager(opType, initCb);
         if (dataSocksMgr) {
            // [3] Check if Socks manager is ready
            std::cout <<
                  "\n\nInitializing Socks Manager subsystem Please wait ..." << std::endl;
            initCv.wait(lck, [&]{return subSystemStatusUpdated;});
            subSystemStatus = dataSocksMgr->getServiceStatus();
         }
         if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** Socks SubSystem is Ready *** " << std::endl;
            break;
         }
         else {
            std::cout << " *** Unable to initialize Socks subsystem *** " << std::endl;
         }
      } while (1);

      // [4] Instantiate enable Socks callback instance - this is optional
      auto respCb = [](telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                   << "enableSocks Response"
                   << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                   << ". ErrorCode: " << static_cast<int>(error) << "\n";
         promise.set_value(1);
      };

      std::future<int> future = promise.get_future();
      // [5] enable Socks
      telux::common::Status status = dataSocksMgr->enableSocks(enable, respCb);

      // [6] Wait for enable Socks callback - this is optional
      if (status == telux::common::Status::SUCCESS) {
         int tmp = future.get();
      } else {
         std::cout << " *** ERROR - Unable to enableSocks *** " << std::endl;
      }
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./socks_sample_app <operation type> <enable>";
      std::cout << std::endl;
      std::cout << "\n\t\t operation type (0-LOCAL, 1-REMOTE)";
      std::cout << "\n\t\t enable (1-enable, 0-disable)";
      std::cout << std::endl;
      std::cout << "\n\t ./socks_sample_app 1 1 ->enable Socks on remote host";
   }

   // [7] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
