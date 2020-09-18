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

#ifndef PHONEMENU_HPP
#define PHONEMENU_HPP

#include "telux/tel/PhoneListener.hpp"
#include "telux/tel/PhoneManager.hpp"
#include "telux/tel/SubscriptionManager.hpp"
#include <telux/tel/Phone.hpp>

#include "MyPhoneListener.hpp"
#include "MySignalStrengthHandler.hpp"
#include "MySubscriptionListener.hpp"

#include "console_app_framework/ConsoleApp.hpp"

class PhoneMenu : public ConsoleApp {
public:
   PhoneMenu(std::string appName, std::string cursor);
   ~PhoneMenu();
   void init();

private:
   void requestSignalStrength(std::vector<std::string> userInput);
   void requestRadioTechnology(std::vector<std::string> userInput);
   void getSubscription(std::vector<std::string> userInput);
   void requestVoiceServiceState(std::vector<std::string> userInput);
   void requestCellularCapabilities(std::vector<std::string> userInput);
   void getOperatingMode(std::vector<std::string> userInput);
   void setOperatingMode(std::vector<std::string> userInput);
   void requestCellInfoList(std::vector<std::string> userInput);
   void setCellInfoListRate(std::vector<std::string> userInput);
   void servingSystemMenu(std::vector<std::string> userInput);
   void networkMenu(std::vector<std::string> userInput);
   void setECallOperatingMode(std::vector<std::string> userInput);
   void requestECallOperatingMode(std::vector<std::string> userInput);
   void selectSimSlot(std::vector<std::string> userInput);

   std::string getRadioStateAsString(telux::tel::RadioState radioState);
   std::string getServiceStateAsString(telux::tel::ServiceState serviceState);
   // Member variable to keep the Listener object alive till application ends.
   std::shared_ptr<telux::tel::IPhoneListener> phoneListener_;
   std::shared_ptr<telux::tel::IPhoneManager> phoneManager_;
   std::shared_ptr<telux::tel::ISubscriptionManager> subscriptionMgr_;
   std::shared_ptr<MySubscriptionListener> subscriptionListener_;
   std::shared_ptr<MySignalStrengthCallback> mySignalStrengthCb_;
   std::shared_ptr<MyVoiceRadioTechnologyCallback> myVoiceRadioTechCb_;
   std::shared_ptr<MyVoiceServiceStateCallback> myVoiceSrvStateCb_;
   std::shared_ptr<MyCellularCapabilityCallback> myCellularCapabilityCb_;
   std::shared_ptr<MyGetOperatingModeCallback> myGetOperatingModeCb_;
   std::shared_ptr<MySetOperatingModeCallback> mySetOperatingModeCb_;
   int slot_ = DEFAULT_SLOT_ID;
   std::vector<std::shared_ptr<telux::tel::IPhone>> phones_;
};

#endif  // PHONEMENU_HPP
