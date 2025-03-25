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
/*
 *  Changes from Qualcomm Technologies, Inc. are provided under the following license:
 *
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 */
/**
 * This is a IPA Manager Sample Application using Telematics SDK.
 * It is used to demonstrate various APIs for IPA related functionalities.
 */

#ifndef IPAMENU_HPP
#define IPAMENU_HPP

#include "console_app_framework/ConsoleApp.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <telux/data/DataFactory.hpp>
#include <telux/data/IpaManager.hpp>

using namespace telux::data;
using namespace telux::common;

class IpaMenu : public ConsoleApp ,
                 public IIpaListener,
                 public std::enable_shared_from_this<IpaMenu> {
 public:
   // initialize menu and sdk
   bool init();

   // Ipa Manager APIs
   void setIpPassthrough(std::vector<std::string> inputCommand);
   void setVlanConfig(std::vector<std::string> inputCommand);
   void setIpCollision(std::vector<std::string> inputCommand);
   void setFactoryReset(std::vector<std::string> inputCommand);
   void monitorLanStatistics(std::vector<std::string> inputCommand);
   void requestLanStatistics(std::vector<std::string> inputCommand);
   void updateWlanMode(std::vector<std::string> inputCommand);
   void updateInterfaceType(std::vector<std::string> inputCommand);
   void setPacketThreshold(std::vector<std::string> inputCommand);
   void setMacBasedSwFiltering(std::vector<std::string> inputCommand);
   void setIpBasedSwFiltering(std::vector<std::string> inputCommand);
   void setInterfaceBasedSwFiltering(std::vector<std::string> inputCommand);
   void enableFileBasedSwFiltering(std::vector<std::string> inputCommand);


   //Initialization callback
   void onInitComplete(telux::common::ServiceStatus status);
   IpaMenu(std::string appName, std::string cursor);
   ~IpaMenu();
 private:
   bool menuOptionsAdded_;
   bool subSystemStatusUpdated_;
   std::mutex mtx_;
   std::condition_variable cv_;
   std::shared_ptr<telux::data::IIpaManager> ipaManager_;
};

#endif
