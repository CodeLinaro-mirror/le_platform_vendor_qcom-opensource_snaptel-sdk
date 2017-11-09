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

#include <chrono>
#include <iostream>
#include <string>
#include <memory>
#include <vector>

#include <telux/tel/PhoneListener.hpp>
#include <telux/tel/SmsManager.hpp>
#include <telux/tel/PhoneFactory.hpp>

using namespace telux::tel;
using namespace telux::common;

// [1] Implement ICommandResponseCallback interface to know
// SMS sent status

class SmsCallback : public ICommandResponseCallback {
public:
   void commandResponse(ErrorCode error) override;
};

void SmsCallback::commandResponse(ErrorCode error) {
   if(error == ErrorCode::SUCCESS) {
      std::cout << "onSmsSent successfully" << std::endl;
   } else {
      std::cout << "onSmsSent failed" << std::endl;
   }
   std::cout << "onSmsSent error = " << (int)error << std::endl;
}

/**
 * Main routine
 */
int main(int, char **) {

   // [2] Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   // [3] Check if telephony subsystem is ready
   bool subSystemsStatus = phoneManager->isSubsystemReady();

   // [3.1] If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemsStatus = f.get();
   }

   // [4] Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemsStatus) {
      std::cout << " *** Sub Systems Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }

   // [5] Instantiate ICommandResponseCallback
   auto smsCb = std::make_shared<SmsCallback>();

   // [6] Get Default SMS manager instance
   std::shared_ptr<ISmsManager> smsManager = phoneFactory.getSmsManager();

   // [7] if smsManager is not empty then send an sms
   if(smsManager) {
      std::string message("Test Message");
      std::string receiverAddress("+18588451326");
      smsManager->sendSms(message, receiverAddress, smsCb);
   }

   // [8] exit logic is specific to an application
   std::cout << " *** Press [ENTER] or type [quit] to exit the application *** " << std::endl;
   std::string input;
   std::getline(std::cin, input);
   if(input != "quit") {
      return 0;
   }
}
