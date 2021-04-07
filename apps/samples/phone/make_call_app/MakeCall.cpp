/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

#include <telux/tel/PhoneFactory.hpp>

#include "ConfigParser.hpp"

#define DEFAULT_PHONE_NUMBER "+18583562961"

using namespace telux::tel;
using namespace telux::common;

// ##### 5.1 implement IMakeCallCallback interface to receive response for the
// dial request -
// optional
class DialCallback : public IMakeCallCallback {
public:
   void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) override;
};

void DialCallback::makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
   std::cout << "DialCallback::makeCallResponse" << std::endl;
   std::cout << "makeCallResponse ErrorCode: " << int(error) << std::endl;
   if(call) {
      std::cout << "makeCallResponse RemotePartyNumber : " << call->getRemotePartyNumber()
                << std::endl;
      std::cout << "makeCallResponse getCallIndex : " << call->getCallIndex() << std::endl;
   }
}

/**
 * Main routine
 */
int main(int argc, char *argv[]) {

   // ### 1. Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   // ### 2. Check if telephony subsystem is ready
   bool subSystemsStatus = phoneManager->isSubsystemReady();

   // #### 2.1 If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemsStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemsStatus) {
      std::cout << " *** Sub Systems Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }

   // ### 3. Instantiate Phone and call manager
   auto phone = phoneManager->getPhone();
   std::shared_ptr<ICallManager> callManager = phoneFactory.getCallManager();

   // ### 4. Get unique id of the phone
   int phoneId = DEFAULT_PHONE_ID;

   // ### 5. Instantiate dial callback instance - this is optional
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback>();

   // ### 6. Send a dial request
   if(callManager) {
      std::string configFile;
      std::string phoneNumber;
      std::shared_ptr<ConfigParser> configParser;

      // [6.1] User can send a dial request by taking receiver's phone number from the user
      // created config file. If user did not provide any config file then it will take
      // parameters from default config file(i.e. SampleAppConfig.conf) which is located
      // under(/usr/data) whereapplication is running.
      if(argc == 2) {
         configFile = argv[1];
         configParser = std::make_shared<ConfigParser>(configFile);
      } else {
         configParser = std::make_shared<ConfigParser>();
      }
      phoneNumber = configParser->getValue(std::string("DIAL_NUMBER"));

      // [6.3] If default config file is also not found then it will take default
      // receiver's phone number and text message which is defined in the sample application.
      if(phoneNumber.empty()) {
         phoneNumber = DEFAULT_PHONE_NUMBER;
         std::cout << "Using default phoneNumber:" << phoneNumber << std::endl;
      }

      auto makeCallStatus = callManager->makeCall(phoneId, phoneNumber, dialCb);
      std::cout << "Dial Call Status:" << (int)makeCallStatus << std::endl;
   }

   // ### 7. Exit logic is specific to an application
   std::cout << "Press enter to exit" << std::endl;
   std::string input;
   std::getline(std::cin, input);
   std::cout << "Exiting application..." << std::endl;
   return 0;
}
