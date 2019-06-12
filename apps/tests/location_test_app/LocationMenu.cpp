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
      locationManager_->deRegisterListenerEx(posListener_);
   }
   if(posListener_) {
      posListener_ = nullptr;
   }

   if(locationManager_) {
      locationManager_ = nullptr;
   }
}

int LocationMenu::init() {

   std::shared_ptr<ConsoleAppCommand> startDetailedReportsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Start_Detailed_Reports", {},
         std::bind(&LocationMenu::startDetailedReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> startBasicReportsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Start_Basic_Reports", {},
         std::bind(&LocationMenu::startBasicReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> stopReportsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("3", "Stop_Reports", {},
                        std::bind(&LocationMenu::stopReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> enableReportLogsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("4", "Filter_notifications", {},
                        std::bind(&LocationMenu::enableReportLogs, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListGnssSubMenu
      = {startDetailedReportsCommand, startBasicReportsCommand, stopReportsCommand,
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
      posListener_->setDetailedLocationReportFlag(false);
      posListener_->setBasicLocationReportFlag(false);
      posListener_->setDataInfoFlag(false);

      //Registering listener for fixes
      locationManager_->registerListenerEx(posListener_);

   }

   return 0;
}

void LocationMenu::startDetailedReports(std::vector<std::string> userInput) {
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
         myLocCmdResponseCb_
            = std::make_shared<MyLocationCommandCallback>("Detailed report request");
         locationManager_->startDetailedReports(
            (uint32_t)opt, std::bind(&MyLocationCommandCallback::commandResponse,
                                     myLocCmdResponseCb_, std::placeholders::_1));
      } else {
         std::cout << " Invalid input \n";
      }
   }
}

void LocationMenu::startBasicReports(std::vector<std::string> userInput) {
   if(locationManager_) {
      char delimiter = '\n';
      std::string minItervalInput;
      std::string distanceInput;
      std::cout << "Enter Interval in Milliseconds (default: 1000ms): ";
      std::getline(std::cin, minItervalInput, delimiter);
      std::cout << "Enter Distance in Meters (default: 0m): ";
      std::getline(std::cin, distanceInput, delimiter);
      int optInterval = -1;
      int optDistance = -1;
      if(!minItervalInput.empty()) {
         try {
            optInterval = std::stoi(minItervalInput);
         } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << optInterval
                      << std::endl;
         }
      } else {
         optInterval = 1000;
      }
      if(!distanceInput.empty()) {
         try {
            optDistance = std::stoi(distanceInput);
         } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << optDistance
                      << std::endl;
         }
      } else {
         optDistance = 0;
      }

      if(optInterval > 0 && optDistance >= 0) {
         myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Basic report request");
         locationManager_->startBasicReports((uint32_t)optDistance, (uint32_t)optInterval,
                                             std::bind(&MyLocationCommandCallback::commandResponse,
                                                       myLocCmdResponseCb_, std::placeholders::_1));
      } else {
         std::cout << " Invalid input \n";
      }
   }
}

void LocationMenu::stopReports(std::vector<std::string> userInput) {
   myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Stop request");
   locationManager_->stopReports(std::bind(&MyLocationCommandCallback::commandResponse,
                                           myLocCmdResponseCb_, std::placeholders::_1));
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

void LocationMenu::enableDetailedLocationReportLogs() {
   int opt = enableReportLogsUtility();
   if((opt == 0) || (opt == 1)) {
      posListener_->setDetailedLocationReportFlag(opt);
   } else {
      std::cout << "ERROR: invalid input, please enter 0 or 1\n";
   }
}

void LocationMenu::enableBasicLocationReportLogs() {
   int opt = enableReportLogsUtility();
   if((opt == 0) || (opt == 1)) {
      posListener_->setBasicLocationReportFlag(opt);
   } else {
      std::cout << "ERROR: invalid input, please enter 0 or 1\n";
   }
}

void LocationMenu::enableReportLogs(std::vector<std::string> userInput) {

   while(true) {
     char delimiter = '\n';
     std::string usrInput;
     std::cout << "------------------------------------------------" << std::endl;
     std::cout << "           "
               << "FILTER NOTIFICATION MENU" << std::endl;
     std::cout << "------------------------------------------------" << std::endl << std::endl;
     std::cout << "  1 - Basic_location_notifications" << std::endl;
     std::cout << "  2 - Detailed_location_notifications" << std::endl;
     std::cout << "  3 - SV_info_notifications" << std::endl;
     std::cout << "  4 - Data_info_notifications" << std::endl << std::endl << std::endl;
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
         LocationMenu::enableBasicLocationReportLogs();
     } else if(usrInput == "2") {
         LocationMenu::enableDetailedLocationReportLogs();
     } else if(usrInput == "3") {
         LocationMenu::enableSvInfoLogs();
     } else if(usrInput == "4") {
         LocationMenu::enableDataInfoLogs();
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


void LocationMenu::enableSvInfoLogs() {
   int opt = enableReportLogsUtility();
   if((opt == 0) || (opt == 1)) {
      posListener_->setSvInfoFlag(opt);
   } else {
      std::cout << "ERROR: invalid input, please enter 0 or 1\n";
   }
}

void LocationMenu::enableDataInfoLogs() {
   int opt = enableReportLogsUtility();
   if((opt == 0) || (opt == 1)) {
      posListener_->setDataInfoFlag(opt);
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
