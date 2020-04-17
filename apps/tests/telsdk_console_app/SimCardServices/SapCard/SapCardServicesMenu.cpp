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
 * Telematics SDK - Sample Application for sap card services
 */

#include <iostream>

#include <telux/tel/PhoneFactory.hpp>

#include "SapCardServicesMenu.hpp"
#include "Utils.hpp"

SapCardServicesMenu::SapCardServicesMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();
   //  Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   if(phoneManager) {
      //  Check if telephony subsystem is ready
      bool subSystemStatus = phoneManager->isSubsystemReady();

      //  If telephony subsystem is not ready, wait for it to be ready
      if(!subSystemStatus) {
         std::cout << "Telephony subsystem is not ready, Please wait" << std::endl;
         std::future<bool> f = phoneManager->onSubsystemReady();
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
           std::vector<int> phoneIds;
           telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
           if (status == telux::common::Status::SUCCESS) {
               for (auto index = 1; index <= phoneIds.size(); index++) {
                   auto sapMgr = phoneFactory.getSapCardManager(index);
                   if (sapMgr != nullptr) {
                       sapManagers_.emplace_back(sapMgr);
                   }
               }
           }
       }
   } else {
       std::cout << "ERROR - PhoneManager is NULL, failed to initialize SapCardServicesMenu"
                 << std::endl;
       exit(1);
   }
   mySapCmdResponseCb_ = std::make_shared<MySapCommandResponseCallback>();
   myTransmitApduResponseCb_ = std::make_shared<MySapTransmitApduResponseCallback>();
   mySapCardReaderCb_ = std::make_shared<MyCardReaderCallback>();
   myAtrCb_ = std::make_shared<MyAtrResponseCallback>();
}

SapCardServicesMenu::~SapCardServicesMenu() {
   mySapCmdResponseCb_ = nullptr;
   myTransmitApduResponseCb_ = nullptr;
   mySapCardReaderCb_ = nullptr;
   myAtrCb_ = nullptr;
}

void SapCardServicesMenu::init() {
   std::shared_ptr<ConsoleAppCommand> openSapConnectionCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Open_sap_connection", {},
         std::bind(&SapCardServicesMenu::openSapConnection, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getSapAtrCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("2", "Get_sap_ATR", {},
                        std::bind(&SapCardServicesMenu::getSapAtr, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestSapStateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Request_sap_state", {},
         std::bind(&SapCardServicesMenu::requestSapState, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> transmitSapApduCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Transmit_sap_APDU", {},
         std::bind(&SapCardServicesMenu::transmitSapApdu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> sapSimPowerOffCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "5", "Sap_sim_power_off", {},
         std::bind(&SapCardServicesMenu::sapSimPowerOff, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> sapSimPowerOnCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("6", "Sap_sim_power_on", {}, std::bind(&SapCardServicesMenu::sapSimPowerOn,
                                                               this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> sapSimResetCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("7", "Sap_sim_reset", {},
                        std::bind(&SapCardServicesMenu::sapSimReset, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> sapCardReaderStatusCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "8", "Sap_card_reader_status", {},
         std::bind(&SapCardServicesMenu::sapCardReaderStatus, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> closeSapConnectionCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "9", "Close_sap_connection", {},
         std::bind(&SapCardServicesMenu::closeSapConnection, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getStateCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("10", "Get_sap_state", {},
                        std::bind(&SapCardServicesMenu::getState, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("11", "Select_sim_slot", {}, std::bind(&SapCardServicesMenu::selectSimSlot,
                                                               this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListSapManagerSubMenu
      = {openSapConnectionCommand, getSapAtrCommand,           requestSapStateCommand,
         transmitSapApduCommand,   sapSimPowerOffCommand,      sapSimPowerOnCommand,
         sapSimResetCommand,       sapCardReaderStatusCommand, closeSapConnectionCommand,
         getStateCommand};

   if (sapManagers_.size() > 1) {
       commandsListSapManagerSubMenu.emplace_back(selectSimSlotCommand);
   }

   addCommands(commandsListSapManagerSubMenu);
   ConsoleApp::displayMenu();
}

void SapCardServicesMenu::logSapState(telux::tel::SapState sapState) {
   if(sapState == telux::tel::SapState::SAP_STATE_NOT_ENABLED) {
      std::cout << "Sap state not enabled \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_CONNECTING) {
      std::cout << "Sap state connecting \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_CONNECTED_SUCCESSFULLY) {
      std::cout << "Sap state connected successfully \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_CONNECTION_ERROR) {
      std::cout << "Sap state connection error \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_DISCONNECTING) {
      std::cout << "Sap state disconnecting \n";
   } else if(sapState == telux::tel::SapState::SAP_STATE_DISCONNECTED_SUCCESSFULLY) {
      std::cout << "Sap state disconnected successfully \n";
   }
}

void SapCardServicesMenu::openSapConnection(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   sapCardMgr->openConnection(telux::tel::SapCondition::SAP_CONDITION_BLOCK_VOICE_OR_DATA,
                               mySapCmdResponseCb_);
}
void SapCardServicesMenu::getSapAtr(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   sapCardMgr->requestAtr(myAtrCb_);
}

void SapCardServicesMenu::transmitSapApdu(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   int cla, instruction, p1, p2, lc, tmpInp;
   std::vector<uint8_t> data;

   cla = 0;
   instruction = 0;
   p1 = 0;
   p2 = 0;
   lc = 0;

   std::string user_input;
   std::cout << std::endl;
   std::cout << "Enter CLA : ";
   std::cin >> cla;
   Utils::validateInput(cla);
   std::cout << "Enter INS : ";
   std::cin >> instruction;
   Utils::validateInput(instruction);
   std::cout << "Enter P1 : ";
   std::cin >> p1;
   Utils::validateInput(p1);
   std::cout << "Enter P2 : ";
   std::cin >> p2;
   Utils::validateInput(p2);
   std::cout << "Enter Lc : ";
   std::cin >> lc;
   Utils::validateInput(lc);
   for(int i = 0; i < lc; i++) {
      std::cout << "Enter DATA (" << i + 1 << ") :";
      std::cin >> tmpInp;
      Utils::validateInput(tmpInp);
      data.emplace_back((uint8_t)tmpInp);
   }
   auto ret
      = sapCardMgr->transmitApdu((uint8_t)cla, (uint8_t)instruction, (uint8_t)p1, (uint8_t)p2,
                                  (uint8_t)lc, data, 0, myTransmitApduResponseCb_);
   if(ret == telux::common::Status::SUCCESS) {
      std::cout << "Sap transmit APDU is successful \n";
   } else {
      std::cout << "Sap transmit APDU failed \n";
   }
}

void SapCardServicesMenu::sapSimPowerOff(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   sapCardMgr->requestSimPowerOff(mySapCmdResponseCb_);
}

void SapCardServicesMenu::sapSimPowerOn(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   sapCardMgr->requestSimPowerOn(mySapCmdResponseCb_);
}

void SapCardServicesMenu::sapSimReset(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   sapCardMgr->requestSimReset(mySapCmdResponseCb_);
}

void SapCardServicesMenu::sapCardReaderStatus(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   sapCardMgr->requestCardReaderStatus(mySapCardReaderCb_);
}

void SapCardServicesMenu::closeSapConnection(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   sapCardMgr->closeConnection(mySapCmdResponseCb_);
}

void SapCardServicesMenu::requestSapState(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   telux::tel::SapState sapstate;
   if(sapCardMgr->requestSapState(MySapStateCallback::sapStateResponse)
      == telux::common::Status::SUCCESS) {
      std::cout << "Request sap state success \n";
   } else {
      std::cout << "Request sap state failed \n";
   }
}

void SapCardServicesMenu::getState(std::vector<std::string> userInput) {
   auto sapCardMgr = sapManagers_[slot_ - 1];
   telux::tel::SapState sapstate;
   if(sapCardMgr->getState(sapstate) == telux::common::Status::SUCCESS) {
      logSapState(sapstate);
      std::cout << "Get sap state success \n";
   } else {
      std::cout << "Get sap state failed \n";
   }
}

void SapCardServicesMenu::selectSimSlot(std::vector<std::string> userInput) {
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
