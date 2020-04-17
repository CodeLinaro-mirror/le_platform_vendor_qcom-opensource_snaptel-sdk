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

SmsMenu::SmsMenu(std::string appName, std::string cursor)
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

      std::vector<int> phoneIds;
      telux::common::Status status = phoneManager_->getPhoneIds(phoneIds);
      if (status == telux::common::Status::SUCCESS) {
          for (auto index = 1; index <= phoneIds.size(); index++) {
              auto smsMgr = phoneFactory.getSmsManager(index);
              if (smsMgr) {
                  // add listeners for incoming SMS notification
                  telux::common::Status status = smsMgr->registerListener(smsListener_);
                  if(status != telux::common::Status::SUCCESS) {
                     std::cout << "Unable to register Listener" << std::endl;
                  }
                  smsManagers_.emplace_back(smsMgr);
              } else {
                  std::cout << " SMS Manager is NULL,"
                            <<" so cannot register a listener to receive incoming SMS"
                            << std::endl;
              }
          }
      }
   }
}

SmsMenu::~SmsMenu() {
   for (auto index = 0; index < smsManagers_.size(); index++) {
       smsManagers_[index]->removeListener(smsListener_);
   }
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
         "4", "Calculate_message_attributes", {},
         std::bind(&SmsMenu::calculateMessageAttributes, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "Select_sim_slot", {},
                        std::bind(&SmsMenu::selectSimSlot, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListSmsSubMenu
      = {sendSmsCommand, getSmscAddrCommand, setSmscAddrCommand, getMsgEncodingSizeCommand};

   if (smsManagers_.size() > 1) {
       commandsListSmsSubMenu.emplace_back(selectSimSlotCommand);
   }

   addCommands(commandsListSmsSubMenu);
   ConsoleApp::displayMenu();
   std::cout << "Device is listening for any incoming messages" << std::endl;
}

// SMS Requests
void SmsMenu::sendSms(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
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
      status = smsManager->sendSms(message, receiverAddress, mySmsCmdCb_, mySmsDeliveryCb_);
   } else {
      status = smsManager->sendSms(message, receiverAddress, mySmsCmdCb_);
   }

   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Send SMS request successful\n";
   } else {
      std::cout << "Send SMS request failed\n";
   }
}

void SmsMenu::getSmscAddr(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   auto ret = smsManager->requestSmscAddress(mySmscAddrCb_);
   std::cout << (ret == telux::common::Status::SUCCESS ? "Request SmscAddress successful"
                                                       : "Request SmscAddress failed")
             << '\n';
}

void SmsMenu::setSmscAddr(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::cout << "set SMSC Address \n" << std::endl;
   char delimiter = '\n';

   std::string smscAddress;
   std::cout << "Enter SMSC number: ";
   std::getline(std::cin, smscAddress, delimiter);
   auto ret
      = smsManager->setSmscAddress(smscAddress, MySetSmscAddressResponseCallback::setSmscResponse);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Set SmscAddress request success" << std::endl;
   } else {
      std::cout << "Set SmscAddress request failed" << std::endl;
   }
}

void SmsMenu::calculateMessageAttributes(std::vector<std::string> userInput) {
   auto smsManager = smsManagers_[slot_ - 1];
   std::string smsMessage;
   char delimiter = '\n';

   std::cout << "Enter Message: ";
   std::getline(std::cin, smsMessage, delimiter);

   auto msgAttributes = smsManager->calculateMessageAttributes(smsMessage);
   std::cout
      << "Message Attributes \n encoding: " << (int)msgAttributes.encoding
      << "\n numberOfSegments: " << msgAttributes.numberOfSegments
      << "\n segmentSize: " << msgAttributes.segmentSize
      << "\n numberOfCharsLeftInLastSegment: " << msgAttributes.numberOfCharsLeftInLastSegment
      << std::endl;
}

void SmsMenu::selectSimSlot(std::vector<std::string> userInput) {
   std::string slotSelection;
   char delimiter = '\n';

   std::cout << "Enter the desired SIM slot: ";
   std::getline(std::cin, slotSelection, delimiter);

   if (!slotSelection.empty()) {
      try {
         int slot = std::stoi(slotSelection);
         if (slot > 2) {
            std::cout << "Invalid slot entered, using default slot" << std::endl;
            slot_ = DEFAULT_SLOT_ID;
         } else {
            slot_ = slot;
         }
      } catch (const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
            << slotSelection << std::endl;
         return;
      }
   } else {
      std::cout << "Empty input, enter the correct slot" << std::endl;
   }
}
