/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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

#ifndef SERVINGSYSTEMMENU_HPP
#define SERVINGSYSTEMMENU_HPP

#include <memory>
#include <string>
#include <vector>

#include "telux/tel/PhoneManager.hpp"
#include <telux/tel/ServingSystemManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

class ServingSystemMenu : public ConsoleApp {
public:
   /**
    * Initialize commands and SDK
    */
   void init();

   ServingSystemMenu(std::string appName, std::string cursor);

   ~ServingSystemMenu();

   void getRatModePreference(std::vector<std::string> userInput);
   void setRatModePreference(std::vector<std::string> userInput);
   void getServiceDomainPreference(std::vector<std::string> userInput);
   void setServiceDomainPreference(std::vector<std::string> userInput);
   void selectSimSlot(std::vector<std::string> userInput);
   void getDualConnectivityStatus(std::vector<std::string> userInput);

private:
   // Member variable to keep the Listener object alive till application ends.
   std::shared_ptr<telux::tel::IServingSystemListener> servingSystemListener_;
   int slot_ = DEFAULT_SLOT_ID;
   std::vector<std::shared_ptr<telux::tel::IServingSystemManager>> servingSystemMgrs_;
};

#endif  // SERVINGSYSTEMMENU_HPP
