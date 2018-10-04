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

#include <chrono>
#include <future>
#include <iostream>
#include <memory>

#include <telux/loc/LocationFactory.hpp>

#include "LocationMenu.hpp"
#include "MyLocationListener.hpp"

LocationMenu::LocationMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   if(locationManager_ == nullptr) {
      auto &locationFactory = telux::loc::LocationFactory::getInstance();
      locationManager_ = locationFactory.getLocationManager();

      std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
      startTime = std::chrono::system_clock::now();
      bool subSystemsStatus = locationManager_->isSubsystemReady();
      if(!subSystemsStatus) {
         std::cout << "Location subsystem is not ready, Please wait!!!... " << std::endl;
         std::future<bool> f = locationManager_->onSubsystemReady();
         subSystemsStatus = f.get();
      }

      if(subSystemsStatus) {
         endTime = std::chrono::system_clock::now();
         std::chrono::duration<double> elapsedTime = endTime - startTime;
         std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                   << std::endl;
      } else {
         std::cout << " *** ERROR - Unable to initialize Location subsystem" << std::endl;
      }

      posListener_ = std::make_shared<MyLocationListener>();
      posListener_->setSvInfoFlag(false);
      posListener_->setLocationReportFlag(false);

      locationManager_->registerListener(posListener_);
   }
}

LocationMenu::~LocationMenu() {
   if(locationManager_ && posListener_) {
      locationManager_->removeListener(posListener_);
   }
   if(posListener_) {
      posListener_ = nullptr;
   }

   if(locationManager_) {
      locationManager_ = nullptr;
   }
}

void LocationMenu::init() {
   std::shared_ptr<ConsoleAppCommand> finalReportMinIntervalCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Set_min_interval", {},
         std::bind(&LocationMenu::finalReportMinInterval, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> positionReportTimeoutCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Set_position_report_timeout", {},
         std::bind(&LocationMenu::positionReportTimeout, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> horizontalAccuracyLevelCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Set_Horizontal_accuracy", {},
         std::bind(&LocationMenu::horizontalAccuracyLevel, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> enableLocationReportLogsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Location_notifications", {},
         std::bind(&LocationMenu::enableLocationReportLogs, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> enableSvInfoLogsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "SV_info_notifications", {},
                        std::bind(&LocationMenu::enableSvInfoLogs, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListGnssSubMenu
      = {finalReportMinIntervalCommand, positionReportTimeoutCommand,
         horizontalAccuracyLevelCommand, enableLocationReportLogsCommand, enableSvInfoLogsCommand};
   addCommands(commandsListGnssSubMenu);
   ConsoleApp::displayMenu();
}

void LocationMenu::positionReportTimeout(std::vector<std::string> userInput) {
   if(locationManager_) {
      char delimiter = '\n';
      std::string posReportTimeoutInput;
      std::cout << "Enter Position report timeout in Milliseconds (range: "
                   "1000ms-255000ms): ";
      std::getline(std::cin, posReportTimeoutInput, delimiter);
      int opt = -1;
      if(!posReportTimeoutInput.empty()) {
         try {
            opt = std::stoi(posReportTimeoutInput);
         } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << opt << std::endl;
         }
      } else {
         std::cout << "empty input using default min interval as 1000ms\n";
         opt = 1000;
      }

      if(opt >= 1000 && opt <= 255000) {
         myLocCmdResponseCb_
            = std::make_shared<MyLocationCommandCallback>("Position report timeout request");
         locationManager_->setPositionReportTimeout((uint32_t)opt, myLocCmdResponseCb_);
      } else {
         std::cout << " Invalid input \n";
      }
   }
}

void LocationMenu::finalReportMinInterval(std::vector<std::string> userInput) {
   if(locationManager_) {
      char delimiter = '\n';
      std::string minItervalInput;
      std::cout << "Enter Min Interval in Milliseconds (default: 1000ms): ";
      std::getline(std::cin, minItervalInput, delimiter);
      int opt = -1;
      if(!minItervalInput.empty()) {
         try {
            opt = std::stoi(minItervalInput);
         } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << opt << std::endl;
         }
      } else {
         std::cout << "empty input using default min interval as 1000ms\n";
         opt = 1000;
      }

      if(opt > 0) {
         myLocCmdResponseCb_
            = std::make_shared<MyLocationCommandCallback>("Final report min interval request");
         locationManager_->setMinIntervalForReports((uint32_t)opt, myLocCmdResponseCb_);
      } else {
         std::cout << " Invalid input \n";
      }
   }
}

void LocationMenu::horizontalAccuracyLevel(std::vector<std::string> userInput) {
   if(locationManager_) {
      int accuLevel = -1;
      std::cout << "Enter accuracy level (1 - Low, 2 - Medium, 3 - High): ";
      if(!(std::cin >> accuLevel)) {
         std::cout << "ERROR Invalid input " << std::endl;
         std::cin.clear();
         std::cin.ignore();
      }

      if(accuLevel == 1 || accuLevel == 2 || accuLevel == 3) {
         std::cout << "Setting accuracy level: " << accuLevel << std::endl;
         myLocCmdResponseCb_
            = std::make_shared<MyLocationCommandCallback>("Horizontal accuracy level request");
         locationManager_->setHorizontalAccuracyLevel(
            (telux::loc::HorizontalAccuracyLevel)accuLevel, myLocCmdResponseCb_);
      } else {
         std::cout << " Invalid input: " << accuLevel << std::endl;
      }
   }
}

void LocationMenu::enableLocationReportLogs(std::vector<std::string> userInput) {
   char delimiter = '\n';
   std::string usrInput;
   std::cout << "Enter 1-Enable/0-Disable: ";
   std::getline(std::cin, usrInput, delimiter);
   int opt = -1;
   if(!usrInput.empty()) {
      try {
         opt = std::stoi(usrInput);
      } catch(const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter numerical values " << opt << std::endl;
      }
   } else {
      std::cout << "empty input\n";
   }
   if((opt == 0) || (opt == 1)) {
      posListener_->setLocationReportFlag(opt);
   } else {
      std::cout << "ERROR: invalid input, please enter 0 or 1\n";
   }
}

void LocationMenu::enableSvInfoLogs(std::vector<std::string> userInput) {
   char delimiter = '\n';
   std::string usrInput;
   std::cout << "Enter 1-Enable/0-Disable: ";
   std::getline(std::cin, usrInput, delimiter);
   int opt = -1;
   if(!usrInput.empty()) {
      try {
         opt = std::stoi(usrInput);
      } catch(const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter numerical values " << opt << std::endl;
      }
   } else {
      std::cout << "empty input\n";
   }
   if((opt == 0) || (opt == 1)) {
      posListener_->setSvInfoFlag(opt);
   } else {
      std::cout << "ERROR: invalid input, please enter 0 or 1\n";
   }
}
