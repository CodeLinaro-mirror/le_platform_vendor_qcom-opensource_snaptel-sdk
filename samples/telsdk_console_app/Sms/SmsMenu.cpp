/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

/**
 * SmsMenu provides menu options to invoke SMS functions such as send SMS,
 * receive SMS etc.
 */

#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

#include <telux/tel/PhoneFactory.hpp>

#include "SmsMenu.hpp"

SmsMenu::SmsMenu(std::string appName, std::string cursor, int phoneId)
   : ConsoleApp(appName, cursor) {
   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();
   //  Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   phoneManager_ = phoneFactory.getPhoneManager();

   //  Check if telephony subsystem is ready
   bool subSystemStatus = phoneManager_->isSubsystemReady();

   //  If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "Telephony subsystem is not ready, Please wait" << std::endl;
      std::future<bool> f = phoneManager_->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemStatus = f.get();
   }

   //  Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemStatus) {
      endTime = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                << std::endl;
   } else {
      std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
      exit(0);
   }
   if(subSystemStatus) {
      mySmsCmdCb_ = std::make_shared<MySmsCommandCallback>();
      mySmscAddrCb_ = std::make_shared<MySmscAddressCallback>();
      mySmsDeliveryCb_ = std::make_shared<MySmsDeliveryCallback>();
      smsListener_ = std::make_shared<MySmsListener>();

      smsManager_ = phoneFactory.getSmsManager(phoneId);

      // add listeners for incoming SMS notification
      telux::common::Status status = smsManager_->registerListener(smsListener_);
      if(status != telux::common::Status::SUCCESS) {
         std::cout << "Unable to register Listener" << std::endl;
      }
   }
}

SmsMenu::~SmsMenu() {
   smsManager_->removeListener(smsListener_);
   mySmsCmdCb_ = nullptr;
   mySmscAddrCb_ = nullptr;
   smsListener_ = nullptr;
   mySmsDeliveryCb_ = nullptr;
}

void SmsMenu::init() {
   std::shared_ptr<ConsoleAppCommand> sendSmsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Send_SMS", {}, std::bind(&SmsMenu::sendSms, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getSmscAddrCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("2", "Get_SMSC_address", {},
                        std::bind(&SmsMenu::getSmscAddr, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setSmscAddrCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("3", "Set_SMSC_address", {},
                        std::bind(&SmsMenu::setSmscAddr, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getMsgEncodingSizeCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Calculate_message_attributes", {"message"},
         std::bind(&SmsMenu::calculateMessageAttributes, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListSmsSubMenu
      = {sendSmsCommand, getSmscAddrCommand, setSmscAddrCommand, getMsgEncodingSizeCommand};
   addCommands(commandsListSmsSubMenu);
   ConsoleApp::displayMenu();
}

// SMS Requests
void SmsMenu::sendSms(std::vector<std::string> userInput) {
   char delimiter = '\n';

   std::string receiverAddress;
   std::cout << "Enter phone number: ";
   std::getline(std::cin, receiverAddress, delimiter);

   std::string message;
   std::cout << "Enter message: ";
   std::getline(std::cin, message, delimiter);

   std::string deliveryAck;
   bool isDeliveryAck = false;
   do {
      std::cout << "Do you need delivery status (y/n): ";
      std::getline(std::cin, deliveryAck, delimiter);
      std::transform(deliveryAck.begin(), deliveryAck.end(), deliveryAck.begin(), ::tolower);
   } while((deliveryAck != "y") && (deliveryAck != "n"));

   telux::common::Status status = telux::common::Status::FAILED;
   if(deliveryAck == "y") {
      status = smsManager_->sendSms(message, receiverAddress, mySmsCmdCb_, mySmsDeliveryCb_);
   } else {
      status = smsManager_->sendSms(message, receiverAddress, mySmsCmdCb_);
   }

   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Send SMS request successful\n";
   } else {
      std::cout << "Send SMS request failed\n";
   }
}

void SmsMenu::getSmscAddr(std::vector<std::string> userInput) {
   auto ret = smsManager_->requestSmscAddress(mySmscAddrCb_);
   std::cout << (ret == telux::common::Status::SUCCESS ? "Request SmscAddress successful"
                                                       : "Request SmscAddress failed")
             << '\n';
}

void SmsMenu::setSmscAddr(std::vector<std::string> userInput) {
   std::cout << "set SMSC Address \n" << std::endl;
   char delimiter = '\n';

   std::string smscAddress;
   std::cout << "Enter SMSC number: ";
   std::getline(std::cin, smscAddress, delimiter);
   auto ret
      = smsManager_->setSmscAddress(smscAddress, MySetSmscAddressResponseCallback::setSmscResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Set SmscAddress request success" << std::endl;
   } else {
      std::cout << "Set SmscAddress request failed" << std::endl;
   }
}

void SmsMenu::calculateMessageAttributes(std::vector<std::string> userInput) {
   auto msgAttributes = smsManager_->calculateMessageAttributes(userInput[1]);
   std::cout
      << "Message Attributes \n encoding: " << (int)msgAttributes.encoding
      << "\n numberOfSegments: " << msgAttributes.numberOfSegments
      << "\n segmentSize: " << msgAttributes.segmentSize
      << "\n numberOfCharsLeftInLastSegment: " << msgAttributes.numberOfCharsLeftInLastSegment
      << std::endl;
}
