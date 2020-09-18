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
 * PhoneMenu provides menu options to invoke Phone functions such as
 * requestSignalStrength.
 */

#include <chrono>
#include <iostream>

#include "MyCellInfoHandler.hpp"
#include <telux/tel/PhoneFactory.hpp>
#include <Utils.hpp>

#include "NetworkMenu.hpp"
#include "PhoneMenu.hpp"
#include "ServingSystemMenu.hpp"

PhoneMenu::PhoneMenu(std::string appName, std::string cursor)
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
      std::cout << "\n\nTelephony subsystem is not ready, Please wait" << std::endl;
      std::future<bool> f = phoneManager_->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemStatus = f.get();
   }

   //  Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemStatus) {
      endTime = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "Elapsed Time for Subsystem to ready : " << elapsedTime.count() << "s\n"
                << std::endl;
   } else {
      std::cout << "ERROR - Unable to initialize subsystem" << std::endl;
      exit(0);
   }

   if(subSystemStatus) {
      std::vector<int> phoneIds;
      telux::common::Status status = phoneManager_->getPhoneIds(phoneIds);
      if (status == telux::common::Status::SUCCESS) {
          for (auto index = 1; index <= phoneIds.size(); index++) {
              auto phone = phoneManager_->getPhone(index);
              if (phone != nullptr) {
                  phones_.emplace_back(phone);
              }
          }
      }
      // Turn on the radio if it's not available
      for (auto index = 0; index < phones_.size(); index++) {
         if(phones_[index]->getRadioState() != telux::tel::RadioState::RADIO_STATE_ON) {
            phones_[index]->setRadioPower(true);
         }
      }

      phoneListener_ = std::make_shared<MyPhoneListener>();

      subscriptionMgr_ = telux::tel::PhoneFactory::getInstance().getSubscriptionManager();
      subscriptionListener_ = std::make_shared<MySubscriptionListener>();
      if(!subscriptionMgr_->isSubsystemReady()) {
         subscriptionMgr_->onSubsystemReady().get();
      }

      status = subscriptionMgr_->registerListener(subscriptionListener_);
      if(status != telux::common::Status::SUCCESS) {
         std::cout << "Failed to registerListener" << std::endl;
      }

      status = phoneManager_->registerListener(phoneListener_);
      if(status != telux::common::Status::SUCCESS) {
         std::cout << "Failed to registerListener" << std::endl;
      }

      mySignalStrengthCb_ = std::make_shared<MySignalStrengthCallback>();
      myVoiceRadioTechCb_ = std::make_shared<MyVoiceRadioTechnologyCallback>();
      myVoiceSrvStateCb_ = std::make_shared<MyVoiceServiceStateCallback>();
      myCellularCapabilityCb_ = std::make_shared<MyCellularCapabilityCallback>();
      myGetOperatingModeCb_ = std::make_shared<MyGetOperatingModeCallback>();
      mySetOperatingModeCb_ = std::make_shared<MySetOperatingModeCallback>();
   }
}

PhoneMenu::~PhoneMenu() {
   phoneManager_->removeListener(phoneListener_);
   subscriptionMgr_->removeListener(subscriptionListener_);

   subscriptionMgr_ = nullptr;
   phoneManager_ = nullptr;
}

void PhoneMenu::init() {
   std::shared_ptr<ConsoleAppCommand> getSignalStrengthCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Get_signal_strength", {},
         std::bind(&PhoneMenu::requestSignalStrength, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestRadioTechnologyCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Request_radio_technology", {},
         std::bind(&PhoneMenu::requestRadioTechnology, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestVoiceServiceStateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Request_voice_service_state", {},
         std::bind(&PhoneMenu::requestVoiceServiceState, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestCellularCapabilitiesCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Request_cellular_capabilities", {},
         std::bind(&PhoneMenu::requestCellularCapabilities, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> getSubscriptionCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "Get_subscription", {},
                        std::bind(&PhoneMenu::getSubscription, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getOperatingModeCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("6", "Get_operating_mode", {},
                        std::bind(&PhoneMenu::getOperatingMode, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setOperatingModeCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("7", "Set_operating_mode", {},
                        std::bind(&PhoneMenu::setOperatingMode, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestCellInfoListCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "8", "Request_cell_info_list", {},
         std::bind(&PhoneMenu::requestCellInfoList, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setCellInfoListRateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "9", "Set_cell_info_list_rate", {},
         std::bind(&PhoneMenu::setCellInfoListRate, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> networkMenuCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("10", "Network_Selection", {},
                        std::bind(&PhoneMenu::networkMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> servingSystemMenuCommand
      = std::make_shared<ConsoleAppCommand>(
         ConsoleAppCommand("11", "Serving_System", {},
                           std::bind(&PhoneMenu::servingSystemMenu, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> setECallOperatingModeCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "12", "Set_eCall_operating_mode", {},
         std::bind(&PhoneMenu::setECallOperatingMode, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> requestECallOperatingModeCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "13", "Request_eCall_operating_mode", {},
         std::bind(&PhoneMenu::requestECallOperatingMode, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("14", "Select_sim_slot", {},
                        std::bind(&PhoneMenu::selectSimSlot, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListPhoneSubMenu
      = {getSignalStrengthCommand,
         requestRadioTechnologyCommand,
         requestVoiceServiceStateCommand,
         requestCellularCapabilitiesCommand,
         getSubscriptionCommand,
         getOperatingModeCommand,
         setOperatingModeCommand,
         requestCellInfoListCommand,
         setCellInfoListRateCommand,
         networkMenuCommand,
         servingSystemMenuCommand,
         setECallOperatingModeCommand,
         requestECallOperatingModeCommand};

   if (phones_.size() > 1) {
       commandsListPhoneSubMenu.emplace_back(selectSimSlotCommand);
   }

   addCommands(commandsListPhoneSubMenu);
   ConsoleApp::displayMenu();
}

void PhoneMenu::requestSignalStrength(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto ret = phone->requestSignalStrength(mySignalStrengthCb_);
      std::cout
         << (ret == telux::common::Status::SUCCESS ? "Request Signal strength is successful \n"
                                                   : "Request Signal strength failed")
         << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

std::string PhoneMenu::getServiceStateAsString(telux::tel::ServiceState serviceState) {
   std::string serviceStateString = "";
   switch(serviceState) {
      case telux::tel::ServiceState::EMERGENCY_ONLY:
         serviceStateString = "Emergency Only";
         break;
      case telux::tel::ServiceState::IN_SERVICE:
         serviceStateString = "In Service";
         break;
      case telux::tel::ServiceState::OUT_OF_SERVICE:
         serviceStateString = "Out Of Service";
         break;
      case telux::tel::ServiceState::RADIO_OFF:
         serviceStateString = "Radio Off";
         break;
      default:
         break;
   }
   return serviceStateString;
}

void PhoneMenu::requestRadioTechnology(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto voiceTechResponseCb
         = std::bind(&MyVoiceRadioTechnologyCallback::voiceRadioTechnologyResponse,
                     myVoiceRadioTechCb_, std::placeholders::_1, std::placeholders::_2);
      auto ret = phone->requestVoiceRadioTechnology(voiceTechResponseCb);
      std::cout << (ret == telux::common::Status::SUCCESS
                       ? "Request Voice Radio Technology is successful \n"
                       : "Request Voice Radio Technology failed")
                << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

void PhoneMenu::requestVoiceServiceState(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto ret = phone->requestVoiceServiceState(myVoiceSrvStateCb_);
      std::cout
         << (ret == telux::common::Status::SUCCESS ? "Request Voice Service state is successful \n"
                                                   : "Request Voice Service state failed")
         << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

void PhoneMenu::getSubscription(std::vector<std::string> userInput) {

   telux::common::Status status;
   auto subscription = subscriptionMgr_->getSubscription(slot_, &status);
   if(subscription) {
      std::cout << "CarrierName : " << subscription->getCarrierName()
                << "\nPhoneNumber : " << subscription->getPhoneNumber()
                << "\nIccId : " << subscription->getIccId() << "\nMcc : " << subscription->getMcc()
                << "\nMnc : " << subscription->getMnc()
                << "\nSlotId : " << subscription->getSlotId()
                << "\nImsi : " << subscription->getImsi() << std::endl;
   } else {
      std::cout << "Subscription is empty" << std::endl;
   }
}

void PhoneMenu::requestCellularCapabilities(std::vector<std::string> userInput) {
   if(phoneManager_) {
      auto ret = phoneManager_->requestCellularCapabilityInfo(myCellularCapabilityCb_);
      std::cout << (ret == telux::common::Status::SUCCESS
                       ? "Cellular capabilities request is successful \n"
                       : "Cellular capabilities request failed")
                << '\n';
   }
}
void PhoneMenu::getOperatingMode(std::vector<std::string> userInput) {
   if(phoneManager_) {
      auto ret = phoneManager_->requestOperatingMode(myGetOperatingModeCb_);
      std::cout
         << (ret == telux::common::Status::SUCCESS ? "Get Operating mode request is successful \n"
                                                   : "Get Operating mode request failed")
         << '\n';
   }
}

void PhoneMenu::setOperatingMode(std::vector<std::string> userInput) {
   if(phoneManager_) {
      int operatingMode;
      std::cout << "Enter Operating Mode (0-Online, 1-Airplane, 2-Factory Test,\n"
                << "3-Offline, 4-Resetting, 5-Shutting Down, 6-Persistent Low "
                   "Power) : ";
      std::cin >> operatingMode;
      Utils::validateInput(operatingMode);
      if(operatingMode >= 0 && operatingMode <= 6) {

         auto responseCb = std::bind(&MySetOperatingModeCallback::setOperatingModeResponse,
                                     mySetOperatingModeCb_, std::placeholders::_1);
         auto ret = phoneManager_->setOperatingMode(
            static_cast<telux::tel::OperatingMode>(operatingMode), responseCb);
         std::cout << (ret == telux::common::Status::SUCCESS
                          ? "Set Operating mode request is successful \n"
                          : "Set Operating mode request failed")
                   << '\n';
      } else {
         std::cout << " Invalid input " << std::endl;
      }
   }
}

void PhoneMenu::requestCellInfoList(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto ret = phone->requestCellInfo(MyCellInfoCallback::cellInfoListResponse);
      std::cout << (ret == telux::common::Status::SUCCESS ? "CellInfo list request is successful \n"
                                                          : "CellInfo list request failed")
                << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

void PhoneMenu::setCellInfoListRate(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      char delimiter = '\n';
      std::string timeIntervalInput;
      std::cout
         << "Enter time interval in Milliseconds(0 for default or notify when any changes): ";
      std::getline(std::cin, timeIntervalInput, delimiter);
      uint32_t opt = -1;
      if(!timeIntervalInput.empty()) {
         try {
            opt = std::stoi(timeIntervalInput);
         } catch(const std::exception &e) {
            std::cout << "ERROR: Invalid input, Enter numerical value " << opt << std::endl;
         }
      } else {
         opt = 0;
      }
      auto ret = phone->setCellInfoListRate(opt, MyCellInfoCallback::cellInfoListRateResponse);
      std::cout
         << (ret == telux::common::Status::SUCCESS ? "Set cell info rate request is successful \n"
                                                   : "Set cell info rate request failed")
         << '\n';
   } else {
      std::cout << "No default phone found" << std::endl;
   }
}

void PhoneMenu::servingSystemMenu(std::vector<std::string> userInput) {
   ServingSystemMenu servingSystemMenu("Serving System Menu", "ServingSystem> ");
   servingSystemMenu.init();
   servingSystemMenu.mainLoop();
}

void PhoneMenu::networkMenu(std::vector<std::string> userInput) {
   NetworkMenu networkMenu("Network Menu", "Network> ");
   networkMenu.init();
   networkMenu.mainLoop();
}

void PhoneMenu::setECallOperatingMode(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      int eCallMode;
      std::cout << std::endl;
      std::cout << "Enter eCall Operating Mode(0-NORMAL, 1-ECALL_ONLY): ";
      std::cin >> eCallMode;

      if(eCallMode == 0 || eCallMode == 1) {
         auto ret = phone->setECallOperatingMode(
            static_cast<telux::tel::ECallMode>(eCallMode),
            MySetECallOperatingModeCallback::setECallOperatingModeResponse);
         if(ret == telux::common::Status::SUCCESS) {
            std::cout << "Set eCall operating mode request sent successfully \n";
         } else {
            std::cout << "Set eCall operating mode request failed \n";
         }
      } else {
         std::cout << "Invalid input \n";
      }
   } else {
      std::cout << "No phone found corresponding to default phoneId" << std::endl;
   }
}

void PhoneMenu::requestECallOperatingMode(std::vector<std::string> userInput) {
   auto phone = phones_[slot_ - 1];
   if(phone) {
      auto ret = phone->requestECallOperatingMode(
         MyGetECallOperatingModeCallback::getECallOperatingModeResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "Get eCall Operating mode request sent successfully\n";
      } else {
         std::cout << "Get eCall Operating mode request failed \n";
      }
   } else {
      std::cout << "No phone found corresponding to default phoneId" << std::endl;
   }
}

void PhoneMenu::selectSimSlot(std::vector<std::string> userInput) {
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
