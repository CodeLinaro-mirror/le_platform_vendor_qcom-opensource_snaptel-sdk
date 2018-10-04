/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#include <vector>
#include <string>
#include <memory>

#include <telux/tel/PhoneListener.hpp>
#include <telux/tel/SmsManager.hpp>
#include <telux/tel/PhoneFactory.hpp>

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::tel;

// [1] Implement ISmsListener interface to receive incoming SMS
class MySmsListener : public ISmsListener {
public:
   void onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> message) override;
};

void MySmsListener::onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> smsMsg) {
   PRINT_NOTIFICATION << "MySmsListener::onIncomingSms from PhoneId : " << phoneId << std::endl;
   PRINT_NOTIFICATION << "smsReceived: " << smsMsg->toString() << std::endl;
}

/**
 * Main routine
 */
int main(int, char **) {

   // [2] Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   // [3] Wait for the telephony subsystem initialization.
   bool subSystemsStatus = phoneManager->isSubsystemReady();
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      subSystemsStatus = f.get();
   }

   // [4] Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemsStatus) {
      std::cout << " *** Sub Systems Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }

   // [5] Instantiate global ISmsListener
   auto mySmsListener = std::make_shared<MySmsListener>();

   // [6] Get Default Phone and SMS manager instance
   std::shared_ptr<ISmsManager> smsMgr = phoneFactory.getSmsManager();

   // [7] Register of for incoming SMS messages
   if(smsMgr) {
      smsMgr->registerListener(mySmsListener);
   }

   // [8] wait for the onIncomingSms()
   std::cout << " *** wait for the onIncomingSms() *** " << std::endl;
   std::cout << "Press enter to exit" << std::endl;
   std::string input;
   std::getline(std::cin, input);
   std::cout << " Exiting application... " << std::endl;
   return 0;
}
