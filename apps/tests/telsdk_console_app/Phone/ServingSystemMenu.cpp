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

#include <chrono>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include <string>
#include <sstream>

#include <telux/tel/PhoneFactory.hpp>

#include "MyServingSystemHandler.hpp"
#include "ServingSystemMenu.hpp"

ServingSystemMenu::ServingSystemMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

ServingSystemMenu::~ServingSystemMenu() {
   for (auto index = 0; index < servingSystemMgrs_.size(); index++) {
       servingSystemMgrs_[index]->deregisterListener(servingSystemListener_);
       servingSystemMgrs_[index] = nullptr;
   }
}

void ServingSystemMenu::init() {

   //  Get the PhoneFactory and ServingSystemManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   std::vector<int> phoneIds;
   if (phoneManager) {
       telux::common::Status status = phoneManager->getPhoneIds(phoneIds);
       if (status == telux::common::Status::SUCCESS) {
           for (auto index = 1; index <= phoneIds.size(); index++) {
               auto servingSystemMgr
                   = telux::tel::PhoneFactory::getInstance().getServingSystemManager(index);
               if (servingSystemMgr != nullptr) {
                   servingSystemMgrs_.emplace_back(servingSystemMgr);
               }
           }
       }

       for (auto index = 0; index < servingSystemMgrs_.size(); index++) {
           std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
           startTime = std::chrono::system_clock::now();
           //  Check if serving subsystem is ready
           bool subSystemStatus = servingSystemMgrs_[index]->isSubsystemReady();

           //  If serving subsystem is not ready, wait for it to be ready
           if(!subSystemStatus) {
              std::cout << "\n\nServing subsystem is not ready, Please wait!!!..." << std::endl;
              std::future<bool> f = servingSystemMgrs_[index]->onSubsystemReady();
              // If we want to wait unconditionally for serving subsystem to be ready
              subSystemStatus = f.get();
           }

           //  Exit the application, if SDK is unable to initialize serving subsystems
           if(subSystemStatus) {
              endTime = std::chrono::system_clock::now();
              std::chrono::duration<double> elapsedTime = endTime - startTime;
              std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                        << std::endl;
           } else {
              std::cout << " *** ERROR - Unable to initialize serving subsystem" << std::endl;
              exit(0);
           }

           servingSystemListener_ = std::make_shared<MyServingSystemListener>();
           auto status = servingSystemMgrs_[index]->registerListener(servingSystemListener_);
           if(status != telux::common::Status::SUCCESS) {
              std::cout << "Failed to registerListener for Serving system Manager" << std::endl;
           }
       }

       std::shared_ptr<ConsoleAppCommand> getRatModePreferenceCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "1", "Get_RAT_mode_preference", {},
             std::bind(&ServingSystemMenu::getRatModePreference, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> setRatModePreferenceCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "2", "Set_RAT_mode_preference", {},
             std::bind(&ServingSystemMenu::setRatModePreference, this, std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getServiceDomainPreferenceCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "3", "Get_service_domain_preference", {},
             std::bind(&ServingSystemMenu::getServiceDomainPreference, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> setServiceDomainPreferenceCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "4", "Set_service_domain_preference", {},
             std::bind(&ServingSystemMenu::setServiceDomainPreference, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> getDcStatusCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "5", "Get_NR_Dual_Connectivity_Status", {},
             std::bind(&ServingSystemMenu::getDualConnectivityStatus, this,
                std::placeholders::_1)));
       std::shared_ptr<ConsoleAppCommand> selectSimSlotCommand
          = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
             "6", "Select_sim_slot", {},
             std::bind(&ServingSystemMenu::selectSimSlot, this, std::placeholders::_1)));
       std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListNetworkSubMenu
          = {getRatModePreferenceCommand, setRatModePreferenceCommand,
             getServiceDomainPreferenceCommand, setServiceDomainPreferenceCommand,
             getDcStatusCommand };

       if (servingSystemMgrs_.size() > 1) {
           commandsListNetworkSubMenu.emplace_back(selectSimSlotCommand);
       }

       addCommands(commandsListNetworkSubMenu);
       ConsoleApp::displayMenu();
   } else {
     std::cout << " PhoneManager is NULL, failed to initialize ServingSystemMenu" << std::endl;
     exit(1);
   }
}

void ServingSystemMenu::getRatModePreference(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      auto ret = servingSystemMgr->requestRatPreference(
         MyRatPreferenceResponseCallback::ratPreferenceResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet RAT mode preference request sent successfully\n";
      } else {
         std::cout << "\nGet RAT mode preference request failed \n";
      }
   }
}

void ServingSystemMenu::setRatModePreference(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      char delimiter = '\n';
      std::string preference;
      telux::tel::RatPreference pref;
      std::vector<int> options;
      std::cout
         << "Available RAT mode preferences: \n"
            "(0 - CDMA_1X\n 1 - CDMA_EVDO\n 2 - GSM\n 3 - WCDMA\n 4 - LTE\n 5 - TDSCDMA\n" <<
            "6 - NR5G) \n\n";
      std::cout
         << "Enter RAT mode preferences\n(For example: enter 2,4 to prefer GSM & LTE mode): ";
      std::getline(std::cin, preference, delimiter);

      std::stringstream ss(preference);
      int i;
      while(ss >> i) {
         options.push_back(i);
         if(ss.peek() == ',' || ss.peek() == ' ')
            ss.ignore();
      }

      for(auto &opt : options) {
         if(opt >= 0 && opt <= 6) {
            try {
               pref.set(opt);
            } catch(const std::exception &e) {
               std::cout << "ERROR: invalid input, please enter numerical values " << opt
                         << std::endl;
            }
         } else {
            std::cout << "Preference should not be out of range" << std::endl;
         }
      }
      auto ret = servingSystemMgr->setRatPreference(
         pref, MyServingSystemResponsecallback::servingSystemResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nSet RAT mode preference request sent successfully\n";
      } else {
         std::cout << "\nSet RAT mode preference request failed \n";
      }
   }
}

void ServingSystemMenu::getServiceDomainPreference(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      auto ret = servingSystemMgr->requestServiceDomainPreference(
         MyServiceDomainResponseCallback::serviceDomainResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nGet service domain preference request sent successfully\n";
      } else {
         std::cout << "\nGet service domain preference request failed \n";
      }
   }
}

void ServingSystemMenu::setServiceDomainPreference(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      std::string serviceDomain;
      int opt = -1;
      std::cout << "Enter service domain preference: (0 - CS, 1 - PS, 2 - CS/PS): ";
      std::cin >> serviceDomain;
      if(!serviceDomain.empty()) {
         try {
            opt = std::stoi(serviceDomain);
         } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input " << opt << std::endl;
         }
      } else {
         std::cout << "Service domain should not be empty";
      }
      telux::tel::ServiceDomainPreference domainPref
         = static_cast<telux::tel::ServiceDomainPreference>(opt);
      auto ret = servingSystemMgr->setServiceDomainPreference(
         domainPref, MyServingSystemResponsecallback::servingSystemResponse);
      if(ret == telux::common::Status::SUCCESS) {
         std::cout << "\nSet service domain preference request sent successfully\n";
      } else {
         std::cout << "\nSet service domain preference request failed \n";
      }
   }
}

void ServingSystemMenu::selectSimSlot(std::vector<std::string> userInput) {
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

void ServingSystemMenu::getDualConnectivityStatus(std::vector<std::string> userInput) {
   auto servingSystemMgr = servingSystemMgrs_[slot_ - 1];
   if(servingSystemMgr) {
      telux::tel::DcStatus dcStatus = servingSystemMgr->getDcStatus();
      std::cout << "\nENDC Availability: \n"
               << MyServingSystemHelper::getEndcAvailability(dcStatus.endcAvailability);
      std::cout << "\nDCNR Restriction: \n"
               << MyServingSystemHelper::getDcnrRestriction(dcStatus.dcnrRestriction);
   }
}
