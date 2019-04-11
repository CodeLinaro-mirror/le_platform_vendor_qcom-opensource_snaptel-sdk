/*
 *  Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
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
#include <memory>
#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>

#include "ConfigParser.hpp"

#define DEFAULT_RECEIVER_PHONE_NUMBER "+18583562961"
#define DEFAULT_MESSAGE "default test msg"

// [3.1] Implement ICommandResponseCallback interface to know
// SMS sent and Delivery status
class SmsCallback : public telux::common::ICommandResponseCallback {
public:
   void commandResponse(telux::common::ErrorCode error) override;
};

void SmsCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << "onSmsSent successfully" << std::endl;
   } else {
      std::cout << "onSmsSent failed" << std::endl;
   }
   std::cout << "onSmsSent error = " << (int)error << std::endl;
}

class SmsDeliveryCallback : public telux::common::ICommandResponseCallback {
public:
   void commandResponse(telux::common::ErrorCode error) override;
};

void SmsDeliveryCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << "SMS Delivered successfully" << std::endl;
   } else {
      std::cout << "SMS Delivery failed" << std::endl;
   }
   std::cout << "SMS Delivery error = " << (int)error << std::endl;
}

/**
 * Main routine
 */
int main(int argc, char *argv[]) {

   // [1] Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   // [2] Check if telephony subsystem is ready
   bool subSystemsStatus = phoneManager->isSubsystemReady();

   // [2.1] If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemsStatus = f.get();
   }

   // [3] Exit the application, if SDK is unable to initialize telephony
   // subsystems
   if(subSystemsStatus) {
      std::cout << " *** Sub Systems Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }

   // [4] Instantiate SMS sent and delivery callback
   auto smsSentCb = std::make_shared<SmsCallback>();
   auto smsDeliveryCb = std::make_shared<SmsDeliveryCallback>();

   // [5] Get Default SMS manager instance
   std::shared_ptr<telux::tel::ISmsManager> smsManager = phoneFactory.getSmsManager();

   // [6] Send an SMS using ISmsManager by passing the text and receiver number
   // along with required callback
   if(smsManager) {
      std::string configFile;
      std::string receiverAddress;
      std::string message;
      std::shared_ptr<ConfigParser> configParser;

      // [6.1] User can send an SMS by taking receiver's phone number and text message
      // from the user created config file. If user did not provide any config file then
      // it will take parameters from default config file(i.e SampleAppConfig.conf)
      // which is located under(/usr/data) where application is running.
      if(argc == 2) {
         configFile = argv[1];
         configParser = std::make_shared<ConfigParser>(configFile);
      } else {
         configParser = std::make_shared<ConfigParser>();
      }

      receiverAddress = configParser->getValue(std::string("RECEIVER_NUMBER"));
      message = configParser->getValue(std::string("MESSAGE"));

      // [6.2] If default config file is also not found then will take default
      // receiver's phone number and text message which is defined in the sample application.
      if(receiverAddress.empty() || message.empty()) {
         receiverAddress = DEFAULT_RECEIVER_PHONE_NUMBER;
         message = DEFAULT_MESSAGE;
         std::cout << "Using default receiverAddress:" << std::endl;
      }
      smsManager->sendSms(message, receiverAddress, smsSentCb, smsDeliveryCb);
   }

   // [7] Receive responses for sendSms request

   // [8] Exit logic is specific to an application
   std::cout << "Press enter to exit" << std::endl;
   std::string input;
   std::getline(std::cin, input);
   std::cout << "Exiting application..." << std::endl;
   return 0;
}
