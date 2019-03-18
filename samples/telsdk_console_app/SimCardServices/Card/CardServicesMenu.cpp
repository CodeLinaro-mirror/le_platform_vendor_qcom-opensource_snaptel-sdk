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
 * CardServicesMenu provides menu options to invoke Card Services such as Transmit APDU.
 */

#include <chrono>
#include <iostream>

#include <telux/tel/PhoneFactory.hpp>

#include "CardServicesMenu.hpp"

CardServicesMenu::CardServicesMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {

   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();
   //  Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   cardManager_ = phoneFactory.getCardManager();

   //  Check if telephony subsystem is ready
   bool subSystemStatus = cardManager_->isSubsystemReady();

   //  If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "Telephony subsystem is not ready, Please wait" << std::endl;
      std::future<bool> f = cardManager_->onSubsystemReady();
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
      std::vector<int> slotIds;
      cardManager_->getSlotIds(slotIds);

      // get the default card object
      card_ = cardManager_->getCard();

      // listener
      cardListener_ = std::make_shared<MyCardListener>();

      // callbacks
      myOpenLogicalChannelCb_ = std::make_shared<MyOpenLogicalChannelCallback>();
      myTransmitApduCb_ = std::make_shared<MyTransmitApduResponseCallback>();
      myCloseLogicalChannelCb_ = std::make_shared<MyCardCommandResponseCallback>();

      // registering Listener
      telux::common::Status status = cardManager_->registerListener(cardListener_);
      if(status != telux::common::Status::SUCCESS) {
         std::cout << "Unable to registerListener" << std::endl;
      }
   }
}

CardServicesMenu::~CardServicesMenu() {
   if(cardListener_) {
      cardManager_->removeListener(cardListener_);
      cardListener_ = nullptr;
   }
   myOpenLogicalChannelCb_ = nullptr;
   myTransmitApduCb_ = nullptr;
   myCloseLogicalChannelCb_ = nullptr;
}

void CardServicesMenu::init() {
   std::shared_ptr<ConsoleAppCommand> getCardStateCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", "Get_card_state", {},
                        std::bind(&CardServicesMenu::getCardState, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getSupportedAppsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Get_supported_apps", {},
         std::bind(&CardServicesMenu::getSupportedApps, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> openLogicalChannelCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Open_logical_channel", {"aid"},
         std::bind(&CardServicesMenu::openLogicalChannel, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> closeLogicalChannelCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Close_logical_channel", {"channel"},
         std::bind(&CardServicesMenu::closeLogicalChannel, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> transmitApduCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "Transmit_APDU", {},
                        std::bind(&CardServicesMenu::transmitApdu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> basicTransmitApduCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "6", "Basic_transmit_APDU", {},
         std::bind(&CardServicesMenu::basicTransmitApdu, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListCardServicesSubMenu
      = {getCardStateCommand,        getSupportedAppsCommand, openLogicalChannelCommand,
         closeLogicalChannelCommand, transmitApduCommand,     basicTransmitApduCommand};
   addCommands(commandsListCardServicesSubMenu);
   ConsoleApp::displayMenu();
}

void CardServicesMenu::getCardState(std::vector<std::string> userInput) {
   if(card_) {
      telux::tel::CardState cardState;
      card_->getState(cardState);
      std::cout << "\ngetCardState : " << (int)cardState << std::endl;
      switch(cardState) {
         case telux::tel::CardState::CARDSTATE_ABSENT:
            std::cout << "Card state is Absent" << std::endl;
            break;
         case telux::tel::CardState::CARDSTATE_PRESENT:
            std::cout << "Card state is Present" << std::endl;
            break;
         case telux::tel::CardState::CARDSTATE_ERROR:
            std::cout << "Card state is either Error or Absent" << std::endl;
            break;
         case telux::tel::CardState::CARDSTATE_RESTRICTED:
            std::cout << "Card state is Restricted" << std::endl;
            break;
         default:
            std::cout << "Unknown Card State" << std::endl;
            break;
      }
   }
}

void CardServicesMenu::getSupportedApps(std::vector<std::string> userInput) {
   if(card_) {
      std::vector<std::shared_ptr<telux::tel::ICardApp>> applications;
      applications = card_->getApplications();
      for(auto cardApp : applications) {
         std::cout << "\nAppType : " << cardApp->getAppType() << std::endl;
         std::cout << "AppState : " << cardApp->getAppState() << std::endl;
         std::cout << "AppId : " << cardApp->getAppId() << std::endl;
      }
   }
}

void CardServicesMenu::openLogicalChannel(std::vector<std::string> userInput) {
   if(card_) {
      std::string aid = userInput[1];
      std::cout << "Open logical channel with aid:" << aid << std::endl;
      card_->openLogicalChannel(aid, myOpenLogicalChannelCb_);
   }
}

void CardServicesMenu::transmitApdu(std::vector<std::string> userInput) {
   if(card_) {
      int channel;
      int cla, instruction, p1, p2, p3;
      std::vector<uint8_t> data;

      cla = 0;
      instruction = 0;
      p1 = 0;
      p2 = 0;
      p3 = 0;

      std::cout << std::endl;
      std::cout << "Enter the channel : ";
      std::cin >> channel;
      std::cout << "Enter CLA : ";
      std::cin >> cla;
      std::cout << "Enter INS : ";
      std::cin >> instruction;
      std::cout << "Enter P1 : ";
      std::cin >> p1;
      std::cout << "Enter P2 : ";
      std::cin >> p2;
      std::cout << "Enter P3 : ";
      std::cin >> p3;
      int dataInput;
      for(int i = 0; i < p3; i++) {
         std::cout << "Enter DATA (" << i + 1 << ") :";
         std::cin >> dataInput;
         data.emplace_back((uint8_t)dataInput);
      }

      auto ret = card_->transmitApduLogicalChannel(channel, (uint8_t)cla, (uint8_t)instruction,
                                                   (uint8_t)p1, (uint8_t)p2, (uint8_t)p3, data,
                                                   myTransmitApduCb_);
      std::cout << (ret == telux::common::Status::SUCCESS ? "Transmit APDU successful"
                                                          : "Transmit APDU failed")
                << '\n';
   }
}

void CardServicesMenu::basicTransmitApdu(std::vector<std::string> userInput) {
   if(card_) {
      int cla, instruction, p1, p2, p3;
      std::vector<uint8_t> data;

      cla = 0;
      instruction = 0;
      p1 = 0;
      p2 = 0;
      p3 = 0;

      std::string user_input;
      std::cout << std::endl;
      std::cout << "Enter CLA : ";
      std::cin >> cla;
      std::cout << "Enter INS : ";
      std::cin >> instruction;
      std::cout << "Enter P1 : ";
      std::cin >> p1;
      std::cout << "Enter P2 : ";
      std::cin >> p2;
      std::cout << "Enter P3 : ";
      std::cin >> p3;
      int tmpInp;
      for(int i = 0; i < p3; i++) {
         std::cout << "Enter DATA (" << i + 1 << ") :";
         std::cin >> tmpInp;
         data.emplace_back((uint8_t)tmpInp);
      }
      auto ret = card_->transmitApduBasicChannel((uint8_t)cla, (uint8_t)instruction, (uint8_t)p1,
                                                 (uint8_t)p2, (uint8_t)p3, data, myTransmitApduCb_);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "Basic transmit APDU successful\n";
      } else {
         std::cout << "Basic transmit APDU failed\n";
      }
   }
}

void CardServicesMenu::closeLogicalChannel(std::vector<std::string> userInput) {
   if(card_) {
      int channel = std::stoi(userInput[1]);
      std::cout << "Close logical channel with channel:" << channel << std::endl;
      card_->closeLogicalChannel(channel, myCloseLogicalChannelCb_);
   }
}
