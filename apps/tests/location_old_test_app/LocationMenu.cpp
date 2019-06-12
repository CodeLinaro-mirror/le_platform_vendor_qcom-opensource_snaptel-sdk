/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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

int LocationMenu::init() {

   std::shared_ptr<ConsoleAppCommand> minIntervalCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", "Min_Inteval", {},
                        std::bind(&LocationMenu::minInterval, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> positionReportCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("2", "Position_Report", {},
                        std::bind(&LocationMenu::positionReport, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> horizontalAccuracyCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Horizontal_Accuracy", {},
         std::bind(&LocationMenu::horizontalAccuracy, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> enableReportLogsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("4", "Filter_notifications", {},
                        std::bind(&LocationMenu::enableReportLogs, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListGnssSubMenu
      = {minIntervalCommand, positionReportCommand, horizontalAccuracyCommand,
         enableReportLogsCommand};
   addCommands(commandsListGnssSubMenu);
   ConsoleApp::displayMenu();

   if(locationManager_ == nullptr) {
      auto &locationFactory = telux::loc::LocationFactory::getInstance();
      locationManager_ = locationFactory.getLocationManager();

      std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
      startTime = std::chrono::system_clock::now();
      bool subSystemsStatus = locationManager_->isSubsystemReady();
      if(!subSystemsStatus) {
         std::cout << "Location subsystem is not ready, Please wait" << std::endl;
         std::future<bool> f = locationManager_->onSubsystemReady();
         subSystemsStatus = f.get();
      }

      if(subSystemsStatus) {
          endTime = std::chrono::system_clock::now();
          std::chrono::duration<double> elapsedTime = endTime - startTime;
          std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                            << std::endl;
      } else {
          std::cout << "ERROR - Unable to initialize Location subsystem" << std::endl;
          return -1;
      }

      posListener_ = std::make_shared<MyLocationListener>();
      posListener_->setSvInfoFlag(false);
      posListener_->setLocationReportFlag(false);

      //Registering the listener for fixes
      locationManager_->registerListener(posListener_);
   }

   return 0;
}

void LocationMenu::minInterval(std::vector<std::string> userInput) {
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
         opt = 1000;
      }

      if(opt > 0) {
         myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("min Interval request");
         locationManager_->setMinIntervalForReports((uint32_t)opt, myLocCmdResponseCb_);
      } else {
         std::cout << " Invalid input \n";
      }
   }
}

void LocationMenu::positionReport(std::vector<std::string> userInput) {
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

void LocationMenu::horizontalAccuracy(std::vector<std::string> userInput) {
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

int LocationMenu::enableReportLogsUtility() {
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

   return opt;
}

void LocationMenu::enableReportLogs(std::vector<std::string> userInput) {

   while(true) {
     char delimiter = '\n';
     std::string usrInput;
     std::cout << "------------------------------------------------" << std::endl;
     std::cout << "           "
               << "FILTER NOTIFICATION MENU" << std::endl;
     std::cout << "------------------------------------------------" << std::endl << std::endl;
     std::cout << "  1 - Location_notifications" << std::endl;
     std::cout << "  2 - SV_info_notifications" << std::endl << std::endl;
     std::cout << "  ? / h - help" << std::endl;
     std::cout << "  q / 0 - exit" << std::endl << std::endl;
     std::cout << "------------------------------------------------" << std::endl << std::endl;
     std::cout << "notification> ";
     std::getline(std::cin, usrInput, delimiter);
     if(usrInput.empty()) {
         std::cout << " Empty input, enter value again" << std::endl;
         continue;
     }
     if(usrInput == "1") {
         LocationMenu::enableLocationReportLogs();
     } else if(usrInput == "2") {
         LocationMenu::enableSvInfoLogs();
     } else if(usrInput == "?" || usrInput == "h" || usrInput == "help") {
         continue;
     } else if(usrInput == "q" || usrInput == "0" || usrInput == "exit" || usrInput == "quit"
               || usrInput == "back") {
         break;
     } else {
         std::cout << "Not a valid entry, enter value again" << std::endl;
         continue;
     }
   }
}

void LocationMenu::enableLocationReportLogs() {
   int opt = enableReportLogsUtility();
   if((opt == 0) || (opt == 1)) {
      posListener_->setLocationReportFlag(opt);
   } else {
      std::cout << "ERROR: invalid input, please enter 0 or 1\n";
   }
}

void LocationMenu::enableSvInfoLogs() {
   int opt = enableReportLogsUtility();
   if((opt == 0) || (opt == 1)) {
      posListener_->setSvInfoFlag(opt);
   } else {
      std::cout << "ERROR: invalid input, please enter 0 or 1\n";
   }
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {

   LocationMenu locationMenu("Location Menu", "location> ");
   if( locationMenu.init() == -1) {
       std::cout << "ERROR - Subsystem not ready, Exiting !!!" << std::endl;
       return -1;
   }
   locationMenu.mainLoop();
   return 0;
}
