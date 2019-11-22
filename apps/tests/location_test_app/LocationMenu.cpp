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
#include <sstream>

#include <telux/loc/LocationFactory.hpp>

#include "LocationMenu.hpp"
#include "MyLocationListener.hpp"

const int DEFAULT_UNKNOWN = 0;

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

telux::common::Status LocationMenu::initLocationManager(std::shared_ptr<ILocationManager>
        &locationManager, std::shared_ptr<MyLocationListener> &posListener) {
    if(locationManager == nullptr) {
      auto &locationFactory = LocationFactory::getInstance();
      locationManager = locationFactory.getLocationManager();

      std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
      startTime = std::chrono::system_clock::now();
      bool subSystemsStatus = locationManager->isSubsystemReady();
      if(!subSystemsStatus) {
         std::cout << "Location subsystem is not ready, Please wait" << std::endl;
         std::future<bool> f = locationManager->onSubsystemReady();
         subSystemsStatus = f.get();
      }

      if(subSystemsStatus) {
          endTime = std::chrono::system_clock::now();
          std::chrono::duration<double> elapsedTime = endTime - startTime;
          std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                            << std::endl;
      } else {
          std::cout << "ERROR - Unable to initialize Location subsystem" << std::endl;
          return telux::common::Status::NOTREADY;
      }

      posListener = std::make_shared<MyLocationListener>();
      posListener->setSvInfoFlag(false);
      posListener->setDetailedLocationReportFlag(false);
      posListener->setBasicLocationReportFlag(false);
      posListener->setDataInfoFlag(false);

      //Registering listener for fixes
      locationManager->registerListenerEx(posListener_);
   } else {
       std::cout<< "Location manager already initialized" << std::endl;
   }
   return telux::common::Status::SUCCESS;
}

telux::common::Status LocationMenu::initLocationConfigurator(std::shared_ptr<ILocationConfigurator>
        &locationConfigurator) {
    if(locationConfigurator == nullptr) {
        auto &locationFactory = LocationFactory::getInstance();
        locationConfigurator = locationFactory.getLocationConfigurator();
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
        startTime = std::chrono::system_clock::now();
        bool subSystemsStatus = locationConfigurator->isSubsystemReady();
        if(!subSystemsStatus) {
            std::cout << "Location configuration subsystem is not ready, Please wait" << std::endl;
            std::future<bool> f = locationConfigurator->onSubsystemReady();
            subSystemsStatus = f.get();
        }

        if(subSystemsStatus) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            std::cout << "Elapsed Time for configuration subsystems to ready : "
                << elapsedTime.count() << "s\n"  << std::endl;
        } else {
            std::cout << "ERROR - Unable to initialize Location configuration subsystem"
                << std::endl;
            return telux::common::Status::NOTREADY;
        }
   } else {
       std::cout<< "Location configurator is already initialized" << std::endl;
   }
   return telux::common::Status::SUCCESS;
}

int LocationMenu::init() {
   std::shared_ptr<ConsoleAppCommand> startDetailedReportsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Start_Detailed_Reports", {},
         std::bind(&LocationMenu::startDetailedReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> startDetailedEngineReportsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Start_Detailed_Engine_Reports", {},
         std::bind(&LocationMenu::startDetailedEngineReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> startBasicReportsCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Start_Basic_Reports", {},
         std::bind(&LocationMenu::startBasicReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> stopReportsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("4", "Stop_Reports", {},
                        std::bind(&LocationMenu::stopReports, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> enableReportLogsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "Filter_notifications", {},
                        std::bind(&LocationMenu::enableReportLogs, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> enableDisableTunc = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("6", "C-TUNC", {},
                        std::bind(&LocationMenu::enableDisableTunc, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListGnssSubMenu
      = {startDetailedReportsCommand, startDetailedEngineReportsCommand, startBasicReportsCommand,
         stopReportsCommand, enableReportLogsCommand, enableDisableTunc};
   addCommands(commandsListGnssSubMenu);
   ConsoleApp::displayMenu();

   telux::common::Status status = telux::common::Status::FAILED;
   int rc = 0;
   status = initLocationManager(locationManager_, posListener_);
   if (status != telux::common::Status::SUCCESS) {
       rc = -1;
   }
   status = initLocationConfigurator(locationConfigurator_);
   if (status != telux::common::Status::SUCCESS) {
       rc = -1;
   }
   return rc;
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

void LocationMenu::startDetailedEngineReports(std::vector<std::string> userInput) {
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
      std::string enginePreference;
      LocReqEngine engineType = DEFAULT_UNKNOWN;
      std::vector<int> options;
      std::cout << " Enter the type of engine reports : \n"
                   " (0 - FUSED\n 1 - SPE\n 2 - PPE) \n\n";
      std::cout << " Enter your engine preference\n"
                   " (For example: enter 0,1 to choose FUSED & SPE engine fixes) : ";
      std::getline(std::cin,enginePreference,delimiter);
      std::stringstream ss(enginePreference);
      int i;
      while(ss >> i) {
        options.push_back(i);
        if(ss.peek() == ',' || ss.peek() == ' ')
          ss.ignore();
      }
      for(auto &opt : options) {
        if(opt >= 0 && opt <= 2) {
          try {
            engineType |= 1UL << opt;
          } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << opt
                         << std::endl;
          }
        } else {
            std::cout << "Engine preference should not be out of range" << std::endl;
        }
      }

      if(opt > 0) {
         myLocCmdResponseCb_
            = std::make_shared<MyLocationCommandCallback>("Detailed engine report request");
         locationManager_->startDetailedEngineReports(
            (uint32_t)opt, engineType, std::bind(&MyLocationCommandCallback::commandResponse,
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

void LocationMenu::enableDisableTunc(std::vector<std::string> userInput) {
   if(locationConfigurator_) {
       char delimiter = '\n';
       std::string option;
       std::cout << "Enter Y to enable or N to disable C-TUNC: ";
       std::getline(std::cin, option, delimiter);
       std::string threshold;
       std::cout << "Enter value for threshold in ms, default is 0.0: ";
       std::getline(std::cin, threshold, delimiter);
       std::string energyBudget;
       std::cout << "Enter value for power in .1 milli watt second, default is 0: ";
       std::getline(std::cin, energyBudget, delimiter);

       bool enable = false;
       if(option == "Y") {
            enable = true;
       } else if(option == "N") {
            enable = false;
       } else {
            std::cout << " BAD input " << std::endl;
       }
       float optThreshold = 0.0;
       if(!threshold.empty()) {
           try {
                optThreshold = std::stof(threshold);
           } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << optThreshold
                          << std::endl;
           }
        } else {
             optThreshold = 0.0;
        }
        int optPower = 0;
        if(!energyBudget.empty()) {
            try {
                optPower = std::stoi(energyBudget);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << optPower
                          << std::endl;
            }
        } else {
             optPower = 0;
        }
        std::cout << " Enable: " << enable << " Threshold: " << optThreshold << " Power: " <<
                optPower << std::endl;

        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Constraint-TUNC");
        telux::common::Status status = locationConfigurator_->configureCTunc(enable,
                std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                        std::placeholders::_1), optThreshold, optPower);
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not Implemented" << std::endl;
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

   while(true) {
     char delimiter = '\n';
     std::string usrInput;
     std::cout << "------------------------------------------------" << std::endl;
     std::cout << "           "
               << "FILTER NOTIFICATION MENU" << std::endl;
     std::cout << "------------------------------------------------" << std::endl << std::endl;
     std::cout << "  1 - Basic_location_notifications" << std::endl;
     std::cout << "  2 - Detailed/Detailed_engine_location_notifications" << std::endl;
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
