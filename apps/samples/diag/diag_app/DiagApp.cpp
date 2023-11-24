/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <string>

#include <chrono>
#include <thread>

#include <telux/platform/diag/DiagLogManager.hpp>
#include <telux/platform/diag/DiagnosticsFactory.hpp>


/**
 * @file: DiagApp.cpp
 *
 * @brief: Simple application to Configure and start/stop logging
 *         ./diag_app <path to mask file>
 *         Application uses File method to capture logs for all peripherals in MDM
 *         It takes path to modem mask file and capture logs to output file.
 *         location of output file will be in directory set in key
 *         platform.diag.diag_output_log_path in /etc/tel.conf file. If key does not exist,
 *         output file will be located in /tmp/diag
 */

int main(int argc, char *argv[]) {
   std::promise<telux::common::ServiceStatus> initPromise;
   telux::common::ErrorCode errCode = telux::common::ErrorCode::SUCCESS;
   std::shared_ptr<telux::platform::diag::IDiagLogManager> diagMgr = nullptr;
   telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

   if(argc == 2) {
      std::string maskFile = argv[1];

      // [1] Instantiate initialization callback - this is optional
      auto initCb = [&](telux::common::ServiceStatus status) {
         initPromise.set_value(status);
      };

      // [2] Get the DiagnosticsFactory and DiagLogManager instance
      auto &diagFactory = telux::platform::diag::DiagnosticsFactory::getInstance();
      do {
         diagMgr  = diagFactory.getDiagLogManager(initCb);
         if (diagMgr) {
            // [3] Check if DiagLogManager is ready
            std::cout <<
                  "\nInitializing Diagnostics subsystem Please wait ..." << std::endl;
            subSystemStatus = initPromise.get_future().get();
         }
         if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** Diagnostics SubSystem is Ready *** " << std::endl;
         }
         else {
            std::cout << " *** Unable to initialize Diagnostics subsystem *** " << std::endl;
            break;
         }

         // [4] If Diagnostics logging is already in progress, exit
         telux::platform::diag::DiagStatus diagStatus = diagMgr->getStatus();
         if(diagStatus.isLoggingInProgress) {
            std::cout << " *** Logging already in progress - quitting application *** " << std::endl;
            break;
         }

         // [5] Set Diagnostics configuration
         std::cout << "Setting Diagnostics Config .... Please wait" << std::endl;
         telux::platform::diag::DiagConfig fileMethodCfg {};
         fileMethodCfg.method = telux::platform::diag::LogMethod::FILE;
         fileMethodCfg.srcType = telux::platform::diag::SourceType::DEVICE;
         fileMethodCfg.srcInfo.device = static_cast<uint8_t>(2);
         fileMethodCfg.mdmLogMaskFile = maskFile;
         fileMethodCfg.modeType = telux::platform::diag::DiagLogMode::STREAMING;
         fileMethodCfg.methodConfig.fileConfig.maxSize = 0;
         fileMethodCfg.methodConfig.fileConfig.maxNumber = 0;

         errCode = diagMgr->setConfig(fileMethodCfg);
         if(telux::common::ErrorCode::SUCCESS != errCode) {
            std::cout << "Failed to set Diagnostics configuration... Error Code: "
                      << static_cast<int>(errCode) << ". quitting application" <<std::endl;
            break;
         }

         // [6] Start logging
         errCode = diagMgr->startLogCollection();
         if(telux::common::ErrorCode::SUCCESS != errCode) {
            std::cout << "Failed to start logging... Error Code: "
                      << static_cast<int>(errCode) << ". quitting application" <<std::endl;
            break;
         }

         std::this_thread::sleep_for(std::chrono::milliseconds(10000));

         // [7] Stop logging
         errCode = diagMgr->stopLogCollection();
         if(telux::common::ErrorCode::SUCCESS != errCode) {
            std::cout << "Failed to stop logging... Error Code: "
                      << static_cast<int>(errCode) << ". quitting application" <<std::endl;
            break;
         }

      } while(0);

      // [8] Cleanup
      if(diagMgr) {
         diagMgr = nullptr;
      }
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./diag_app <path to MDM mask file>";
      std::cout << std::endl;
      std::cout << "\n\t\t path to mask file   Absolute path to MDM mask file";
      std::cout << std::endl;
      std::cout << "\n\t ./diag_app /tmp/modem.cfg \n";
   }

   // [9] Cleaning up and exit the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   return 0;
}
