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
      }

      posListener_ = std::make_shared<MyLocationListener>();
      posListener_->setSvInfoFlag(false);
      posListener_->setDetailedLocationReportFlag(false);
      posListener_->setLocationReportFlag(false);
      posListener_->setBasicLocationReportFlag(false);
      posListener_->setDataInfoFlag(false);
   }
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

void LocationMenu::init() {
   std::shared_ptr<ConsoleAppCommand> registerListenerExCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Register_listener Ex", {},
         std::bind(&LocationMenu::registerListenerEx, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> deRegisterListenerCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "DeRegister_listener Ex", {},
         std::bind(&LocationMenu::deRegisterListenerEx, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> startDetailedReportsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Start_Detailed_Reports", {},
         std::bind(&LocationMenu::startDetailedReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> startBasicReportsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Start_Basic_Reports", {},
         std::bind(&LocationMenu::startBasicReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> stopReportsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "Stop_Reports", {},
                        std::bind(&LocationMenu::stopReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> registerListenerCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("6", "Register_listener", {},
                        std::bind(&LocationMenu::registerListener, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> removeListenerCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("7", "Remove_listener", {},
                        std::bind(&LocationMenu::removeListener, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> minIntervalCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("8", "Min_Inteval", {},
                        std::bind(&LocationMenu::minInterval, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> positionReportCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("9", "Position_Report", {},
                        std::bind(&LocationMenu::positionReport, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> horizontalAccuracyCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "10", "Horizontal_Accuracy", {},
         std::bind(&LocationMenu::horizontalAccuracy, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> enableReportLogsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("11", "Report_notifications", {},
                        std::bind(&LocationMenu::enableReportLogs, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListGnssSubMenu
      = {registerListenerExCommand, deRegisterListenerCommand, startDetailedReportsCommand,
         startBasicReportsCommand,  stopReportsCommand,        registerListenerCommand,
         removeListenerCommand,     minIntervalCommand,        positionReportCommand,
         horizontalAccuracyCommand, enableReportLogsCommand};
   addCommands(commandsListGnssSubMenu);
   ConsoleApp::displayMenu();
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

void LocationMenu::registerListenerEx(std::vector<std::string> userInput) {
   locationManager_->registerListenerEx(posListener_);
}

void LocationMenu::deRegisterListenerEx(std::vector<std::string> userInput) {
   locationManager_->deRegisterListenerEx(posListener_);
}

void LocationMenu::stopReports(std::vector<std::string> userInput) {
   myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Stop request");
   locationManager_->stopReports(std::bind(&MyLocationCommandCallback::commandResponse,
                                           myLocCmdResponseCb_, std::placeholders::_1));
}

void LocationMenu::registerListener(std::vector<std::string> userInput) {
   locationManager_->registerListener(posListener_);
}

void LocationMenu::removeListener(std::vector<std::string> userInput) {
   locationManager_->removeListener(posListener_);
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

   char delimiter = '\n';
   std::string usrInput;
   int option = -1;
   std::cout << "------------------------------------------------" << std::endl;
   std::cout << "           "
             << "LOCATION NOTIFICATION MENU" << std::endl;
   std::cout << "------------------------------------------------" << std::endl << std::endl;
   std::cout << "  1 - Location_notifications" << std::endl;
   std::cout << "  2 - Basic_location_notifications" << std::endl;
   std::cout << "  3 - Detailed_location_notifications" << std::endl;
   std::cout << "  4 - SV_info_notifications" << std::endl;
   std::cout << "  5 - Data_info_notifications" << std::endl << std::endl;
   std::cout << "------------------------------------------------" << std::endl << std::endl;
   std::getline(std::cin, usrInput, delimiter);
   if(!usrInput.empty()) {
      try {
         option = std::stoi(usrInput);
      } catch(const std::exception &e) {
         std::cout << "ERROR: invalid input, please enter numerical values " << option << std::endl;
      }
   } else {
      std::cout << "empty input\n";
   }
   switch(option) {
      case 1:
         enableLocationReportLogs();
         break;
      case 2:
         enableBasicLocationReportLogs();
         break;
      case 3:
         enableDetailedLocationReportLogs();
         break;
      case 4:
         enableSvInfoLogs();
         break;
      case 5:
         enableDataInfoLogs();
         break;
      default:
         std::cout << "Not a valid entry, enter value between 1-5 " << std::endl;
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

void LocationMenu::enableDataInfoLogs() {
   int opt = enableReportLogsUtility();
   if((opt == 0) || (opt == 1)) {
      posListener_->setDataInfoFlag(opt);
   } else {
      std::cout << "ERROR: invalid input, please enter 0 or 1\n";
   }
}
