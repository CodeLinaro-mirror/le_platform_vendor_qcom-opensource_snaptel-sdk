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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>

#include <telux/tel/PhoneFactory.hpp>

#include "MyNetworkSelectionHandler.hpp"
#include "NetworkMenu.hpp"
#include "Utils.hpp"

#define UNKNOWN 0

NetworkMenu::NetworkMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

NetworkMenu::~NetworkMenu() {
   for (auto index = 0; index < networkManagers_.size(); index++) {
       networkManagers_[index]->deregisterListener(networkListener_);
       networkManagers_[index] = nullptr;
   }
}

void NetworkMenu::init() {

   //  Get the PhoneFactory and NetworkManger instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   std::vector<int> phoneIds;
   if (phoneManager) {
       telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
       if (status == telux::common::Status::SUCCESS) {
           for (auto index = 1; index <= phoneIds.size(); index++) {
               auto networkManager
                   = telux::tel::PhoneFactory::getInstance().getNetworkSelectionManager(index);
               if (networkManager != nullptr) {
                   networkManagers_.emplace_back(networkManager);
               }
           }
       }

       // Same listener used for both the slots
       networkListener_ = std::make_shared<MyNetworkSelectionListener>();
       for (auto index = 0; index < networkManagers_.size(); index++) {
           std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
           startTime = std::chrono::system_clock::now();

           //  Check if network subsystem is ready
           bool subSystemStatus = networkManagers_[index]->isSubsystemReady();

           //  If network subsystem is not ready, wait for it to be ready
           if(!subSystemStatus) {
              std::cout << "\n\n Network subsystem is not ready, Please wait." << std::endl;
              std::future<bool> f = networkManagers_[index]->onSubsystemReady();
              // If we want to wait unconditionally for network subsystem to be ready
              subSystemStatus = f.get();
           }

           //  Exit the application, if SDK is unable to initialize network subsystems
           if(subSystemStatus) {
              endTime = std::chrono::system_clock::now();
              std::chrono::duration<double> elapsedTime = endTime - startTime;
              std::cout << "Elapsed Time for Subsystems to ready: " << elapsedTime.count() << "s\n"
                        << std::endl;
           } else {
              std::cout << " *** ERROR - Unable to initialize network subsystem" << std::endl;
              exit(0);
           }

           auto status = networkManagers_[index]->registerListener(networkListener_);

           if(status != telux::common::Status::SUCCESS) {
              std::cout << "Failed to registerListener for network Manager" << std::endl;
           }
       }

       std::shared_ptr<ConsoleAppCommand> getNetworkSelectionModeCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "1", "get_selection_mode", {},
             std::bind(&NetworkMenu::getNetworkSelectionMode, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> setNetworkSelectionModeCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "2", "set_selection_mode", {},
             std::bind(&NetworkMenu::setNetworkSelectionMode, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getPreferredNetworksCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "3", "get_preferred_networks", {},
             std::bind(&NetworkMenu::getPreferredNetworks, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> setPreferredNetworksCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "4", "set_preferred_networks", {},
             std::bind(&NetworkMenu::setPreferredNetworks, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> performNetworkScanCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "5", "perform_network_scan", {},
             std::bind(&NetworkMenu::performNetworkScan, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "6", "select_sim_slot", {},
             std::bind(&NetworkMenu::selectSimSlot, this, std::placeholders::_1)));
       std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListNetworkSubMenu
          = {getNetworkSelectionModeCommand, setNetworkSelectionModeCommand,
             getPreferredNetworksCommand, setPreferredNetworksCommand, performNetworkScanCommand};

       if (networkManagers_.size() > 1) {
           commandsListNetworkSubMenu.emplace_back(selectSimSlotCommand);
       }

       addCommands(commandsListNetworkSubMenu);
       ConsoleApp::displayMenu();
   } else {
      std::cout << "Phone Manager is NULL, failed to initialize NetworkMenu" << std::endl;
      exit(1);
   }
}

void NetworkMenu::getNetworkSelectionMode(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if(networkManager) {
      auto ret = networkManager->requestNetworkSelectionMode(
         MySelectionModeResponseCallback::selectionModeResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet network selection mode request sent successfully\n";
      } else {
         std::cout << "\nGet network selection mode request failed \n";
      }
   }
}

void NetworkMenu::setNetworkSelectionMode(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if(networkManager) {
      bool selectionMode;
      std::string mcc;
      std::string mnc;
      telux::common::Status retStatus = telux::common::Status::FAILED;
      std::cout << "Enter Network Selection Mode(0-AUTOMATIC,1-MANUAL): ";
      std::cin >> selectionMode;
      Utils::validateInput(selectionMode);
      if(selectionMode == 1) {
         telux::tel::NetworkSelectionMode selectMode = telux::tel::NetworkSelectionMode::MANUAL;
         std::cout << "Enter MCC: ";
         std::cin >> mcc;
         Utils::validateInput(mcc);
         std::cout << "Enter MNC: ";
         std::cin >> mnc;
         Utils::validateInput(mnc);
         retStatus = networkManager->setNetworkSelectionMode(
            selectMode, mcc, mnc, &MyNetworkResponsecallback::setNetworkSelectionModeResponseCb);

      } else if(selectionMode == 0) {
         telux::tel::NetworkSelectionMode selectMode = telux::tel::NetworkSelectionMode::AUTOMATIC;
         mcc = "0";
         mnc = "0";
         retStatus = networkManager->setNetworkSelectionMode(
            selectMode, mcc, mnc, &MyNetworkResponsecallback::setNetworkSelectionModeResponseCb);

      } else {
         std::cout << "Invalid network selection mode input, Valid values are 0 or 1";
      }
      if(retStatus == telux::common::Status::SUCCESS) {
         std::cout << "\nSet network selection mode request sent successfully\n";
      } else {
         std::cout << "\nSet network selection mode request failed \n";
      }
   }
}

void NetworkMenu::getPreferredNetworks(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if(networkManager) {
      auto ret = networkManager->requestPreferredNetworks(
         MyPreferredNetworksResponseCallback::preferredNetworksResponse);
      if(ret != telux::common::Status::SUCCESS) {
         std::cout << "\nGet preferred networks request failed \n";
      }
   }
}

int NetworkMenu::convertToRatType(int input) {
   switch(input) {
      case 1:
         return telux::tel::RatType::GSM;
      case 2:
         return telux::tel::RatType::LTE;
      case 3:
         return telux::tel::RatType::UMTS;
      case 4:
         return telux::tel::RatType::NR5G;
      default:
         return UNKNOWN;
   }
}

telux::tel::PreferredNetworkInfo NetworkMenu::getNetworkInfoFromUser() {
   telux::tel::PreferredNetworkInfo networkInfo = {};
   telux::tel::RatMask rat;
   uint16_t mcc;
   uint16_t mnc;
   std::string preference;
   std::vector<int> options;
   std::cout << "Enter MCC: ";
   std::cin >> mcc;
   Utils::validateInput(mcc);
   networkInfo.mcc = mcc;
   std::cout << "Enter MNC: ";
   std::cin >> mnc;
   Utils::validateInput(mnc);
   networkInfo.mnc = mnc;
   std::cout << "Select RAT types (1-GSM, 2-LTE, 3-UMTS, 4-NR5G) \n";
   std::cout << "Enter RAT types\n(For example: enter 1,2 to set GSM & "
                "LTE RAT type): ";
   std::cin >> preference;
   Utils::validateNumericString(preference);
   std::stringstream ss(preference);
   int pref;
   while(ss >> pref) {
      options.push_back(pref);
      if(ss.peek() == ',' || ss.peek() == ' ')
         ss.ignore();
   }
   for(auto &opt : options) {
      if((opt == 1) || (opt == 2) || (opt == 3) || (opt == 4)) {
         rat.set(convertToRatType(opt));
      } else {
         std::cout << "Preference should not be out of range" << std::endl;
      }
   }
   options.clear();
   networkInfo.ratMask = rat;
   return networkInfo;
}

void NetworkMenu::setPreferredNetworks(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if(networkManager) {
      std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo;
      int numOfNetworks;
      bool clearPrevPreferredNetworks;
      std::cout << "Enter number of preferred networks: ";
      std::cin >> numOfNetworks;
      Utils::validateInput(numOfNetworks);

      for(int index = 0; index < numOfNetworks; index++) {
         auto nwInfo = getNetworkInfoFromUser();
         preferredNetworksInfo.emplace_back(nwInfo);
      }

      std::cout << "Clear previous preferred network(1 - Yes, 0 - No)?: ";
      std::cin >> clearPrevPreferredNetworks;
      Utils::validateInput(clearPrevPreferredNetworks);
      auto ret = networkManager->setPreferredNetworks(
         preferredNetworksInfo, clearPrevPreferredNetworks,
         MyNetworkResponsecallback::setPreferredNetworksResponseCb);

      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nSet preferred networks request sent successfully\n";
      } else {
         std::cout << "\nSet preferred networks request failed \n";
      }
   }
}

void NetworkMenu::performNetworkScan(std::vector<std::string> userInput) {
   auto networkManager = networkManagers_[slot_ - 1];
   if (networkManager) {
      auto ret = networkManager->performNetworkScan(
         MyPerformNetworkScanCallback::performNetworkScanResponseCb);
      if (ret == telux::common::Status::SUCCESS) {
         std::cout << "\nPerform network scan request sent successfully\n";
      } else {
         std::cout << "\nPerform network scan request failed \n";
      }
   } else {
      std::cout << " ERROR - Network manager is NULL" <<std::endl;
   }
}

void NetworkMenu::selectSimSlot(std::vector<std::string> userInput) {
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
            std::cout << "Successfully changed to slot " << slot << std::endl;
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
