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

NetworkMenu::NetworkMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {

   //  Get the PhoneFactory and NetworkManger instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   networkManager_
      = telux::tel::PhoneFactory::getInstance().getNetworkSelectionManager(DEFAULT_SLOT_ID);

   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();

   //  Check if network subsystem is ready
   bool subSystemStatus = networkManager_->isSubsystemReady();

   //  If network subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "\n\n Network subsystem is not ready, Please wait!!!..." << std::endl;
      std::future<bool> f = networkManager_->onSubsystemReady();
      // If we want to wait unconditionally for network subsystem to be ready
      subSystemStatus = f.get();
   }

   //  Exit the application, if SDK is unable to initialize network subsystems
   if(subSystemStatus) {
      endTime = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize network subsystem" << std::endl;
      exit(0);
   }

   networkListener_ = std::make_shared<MyNetworkSelectionListener>();
   telux::common::Status status = networkManager_->registerListener(networkListener_);

   if(status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to registerListener for network Manager" << std::endl;
   }
}

NetworkMenu::~NetworkMenu() {
   networkManager_->deregisterListener(networkListener_);
   networkManager_ = nullptr;
}

void NetworkMenu::init() {
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
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListNetworkSubMenu
      = {getNetworkSelectionModeCommand, setNetworkSelectionModeCommand,
         getPreferredNetworksCommand, setPreferredNetworksCommand, performNetworkScanCommand};
   addCommands(commandsListNetworkSubMenu);
   ConsoleApp::displayMenu();
}

void NetworkMenu::getNetworkSelectionMode(std::vector<std::string> userInput) {
   if(networkManager_) {
      auto ret = networkManager_->requestNetworkSelectionMode(
         MySelectionModeResponseCallback::selectionModeResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet network selection mode request sent successfully\n";
      } else {
         std::cout << "\nGet network selection mode request failed \n";
      }
   }
}

void NetworkMenu::setNetworkSelectionMode(std::vector<std::string> userInput) {
   if(networkManager_) {
      bool selectionMode;
      std::string mcc;
      std::string mnc;
      telux::common::Status retStatus;
      std::cout << "Enter Network Selection Mode(0-AUTOMATIC,1-MANUAL): ";
      while(!(std::cin >> selectionMode)) {
         std::cin.clear();
         std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
         std::cout << "Invalid input. Enter only boolean: ";
      }
      if(selectionMode == 1) {
         telux::tel::NetworkSelectionMode selectMode = telux::tel::NetworkSelectionMode::MANUAL;
         std::cout << "Enter MCC: ";
         std::cin >> mcc;
         std::cout << "Enter MNC: ";
         std::cin >> mnc;
         retStatus = networkManager_->setNetworkSelectionMode(
            selectMode, mcc, mnc, &MyNetworkResponsecallback::setNetworkSelectionModeResponseCb);

      } else if(selectionMode == 0) {
         telux::tel::NetworkSelectionMode selectMode = telux::tel::NetworkSelectionMode::AUTOMATIC;
         mcc = "0";
         mnc = "0";
         retStatus = networkManager_->setNetworkSelectionMode(
            selectMode, mcc, mnc, &MyNetworkResponsecallback::setNetworkSelectionModeResponseCb);

      } else {
         std::cout << "Network selection mode shouldn't be empty. Enter only 0 or 1";
      }
      if(retStatus == telux::common::Status::SUCCESS) {
         std::cout << "\nSet network selection mode request sent successfully\n";
      } else {
         std::cout << "\nSet network selection mode request failed \n";
      }
   }
}

void NetworkMenu::getPreferredNetworks(std::vector<std::string> userInput) {
   if(networkManager_) {
      auto ret = networkManager_->requestPreferredNetworks(
         MyPreferredNetworksResponseCallback::preferredNetworksResponse);
      if(ret != telux::common::Status::SUCCESS) {
         std::cout << "\nGet preferred networks request failed \n";
      }
   }
}

void NetworkMenu::setPreferredNetworks(std::vector<std::string> userInput) {
   if(networkManager_) {
      std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo;
      int numOfNetworks;
      uint16_t mcc;
      uint16_t mnc;
      std::bitset<16> prefNetworks;
      int pref;
      bool clearPrevPreferredNetworks;
      bool flag = 0;
      bool prefFlag;

      std::cout << "Enter number of preferred networks you want to set: ";
      while(!(std::cin >> numOfNetworks)) {
         std::cin.clear();
         std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
         std::cout << "Invalid input. Enter only integer: ";
      }
      for(int index = 0; index < numOfNetworks; index++) {
         telux::tel::PreferredNetworkInfo networkInfo;
         std::cout << "Enter MCC: ";
         while(!(std::cin >> mcc)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Enter only integer: ";
         }
         networkInfo.mcc = mcc;
         std::cout << "Enter MNC: ";
         while(!(std::cin >> mnc)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Enter only integer: ";
         }
         networkInfo.mnc = mnc;
         std::cout << "Select RAT type (15-UMTS, 14-LTE, 7-GSM): ";
         std::vector<int> preferredNetworks = {7, 14, 15};
         while(!(std::cin >> pref)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid input. Enter only integer: ";
         }
         while(!flag) {
            std::vector<int>::iterator it;
            it = std::find(preferredNetworks.begin(), preferredNetworks.end(), pref);
            if(it != preferredNetworks.end()) {
               if(!(prefNetworks.test(pref))) {
                  prefNetworks.set(pref);
                  flag = 1;
               } else {
                  std::cout << " RAT type already set" << std::endl;
                  flag = 1;
               }
               std::cout << "Do you want to set another RAT type? 1-Yes, 0-No" << std::endl;
               while(!(std::cin >> prefFlag)) {
                  std::cin.clear();
                  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                  std::cout << "Invalid input. Enter only integer: ";
               }
               if(prefFlag) {
                  std::cout << "Enter RAT type (15-UMTS, 14-LTE, 7-GSM): " << std::endl;
                  while(!(std::cin >> pref)) {
                     std::cin.clear();
                     std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                     std::cout << "Invalid input. Enter only integer: ";
                  }
                  flag = 0;
               } else {
                  flag = 1;
               }
            } else {
               std::cout << "Enter valid  RAT type: ";
               while(!(std::cin >> pref)) {
                  std::cin.clear();
                  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                  std::cout << "Invalid input. Enter only integer: ";
               }
               flag = 0;
            }
         }
         flag = 0;
         networkInfo.ratMask = prefNetworks;
         preferredNetworksInfo.emplace_back(networkInfo);
      }
      std::cout << "Clear previous preferred network(1-yes, 0-no): ";
      while(!(std::cin >> clearPrevPreferredNetworks)) {
         std::cin.clear();
         std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
         std::cout << "Invalid input. Enter only boolean: ";
      }
      auto ret = networkManager_->setPreferredNetworks(
         preferredNetworksInfo, clearPrevPreferredNetworks,
         MyNetworkResponsecallback::setPreferredNetworksResponseCb);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet preferred networks request sent successfully\n";
      } else {
         std::cout << "\nGet preferred networks request failed \n";
      }
   }
}

void NetworkMenu::performNetworkScan(std::vector<std::string> userInput) {
   if(networkManager_) {
      auto ret = networkManager_->performNetworkScan(
         MyPerformNetworkScanCallback::performNetworkScanResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nPerform network scan request sent successfully\n";
      } else {
         std::cout << "\nPerform network scan request failed \n";
      }
   }
}