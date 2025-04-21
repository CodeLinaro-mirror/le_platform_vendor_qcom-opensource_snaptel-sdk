/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <future>
#include <iostream>
#include <sstream>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>

#include <telux/loc/LocationFactory.hpp>
#include <telux/common/Version.hpp>
#include "../../common/utils/Utils.hpp"
#include "LocationMenu.hpp"
#include "MyLocationListener.hpp"
#include "LocationUtils.hpp"

const int DEFAULT_UNKNOWN = 0;
#define RECORDING_MODE_SLEEP 60

using namespace telux::common;

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

   if(locConfigListener_) {
      locConfigListener_ = nullptr;
   }
}

telux::common::Status LocationMenu::initLocationManager(std::shared_ptr<ILocationManager>
        &locationManager, std::shared_ptr<MyLocationListener> &posListener) {
    if(locationManager == nullptr) {
      std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
      auto &locationFactory = LocationFactory::getInstance();
      locationManager = locationFactory.getLocationManager([&](ServiceStatus status) {
          if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
            } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
            }
        });
      std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
      startTime = std::chrono::system_clock::now();
      ServiceStatus locMgrStatus = locationManager->getServiceStatus();
      if(locMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
         std::cout << "Location subsystem is not ready, Please wait" << std::endl;
      }
      locMgrStatus = prom.get_future().get();
      if(locMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
          endTime = std::chrono::system_clock::now();
          std::chrono::duration<double> elapsedTime = endTime - startTime;
          std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count()
              << "s\n" << std::endl;
      } else {
          std::cout << "ERROR - Unable to initialize Location subsystem" << std::endl;
          return telux::common::Status::FAILED;
      }

      posListener = std::make_shared<MyLocationListener>();
      posListener->setSvInfoFlag(false);
      posListener->setDetailedLocationReportFlag(false);
      posListener->setBasicLocationReportFlag(false);
      posListener->setDataInfoFlag(false);
      posListener->setNmeaInfoFlag(false);
      posListener->setDetailedEngineLocReportFlag(false);
      posListener->setMeasurementsInfoFlag(false);
      posListener->setDisasterCrisisInfoFlag(false);
      posListener->setEphemerisInfoFlag(false);
      posListener->setLocSystemInfoFlag(false);

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
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        auto &locationFactory = LocationFactory::getInstance();
        locationConfigurator = locationFactory.getLocationConfigurator([&](ServiceStatus status) {
            if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
            } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
            }
        });
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
        startTime = std::chrono::system_clock::now();

        ServiceStatus locCfgStatus = locationConfigurator->getServiceStatus();
        if(locCfgStatus != ServiceStatus::SERVICE_AVAILABLE) {
         std::cout << "Location configuration subsystem is not ready, Please wait" << std::endl;
        }
        locCfgStatus = prom.get_future().get();
        if(locCfgStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            std::cout << "Elapsed Time for configuration subsystems to ready : "
                << elapsedTime.count() << "s\n" << std::endl;
        } else {
            std::cout << "ERROR - Unable to initialize Location configuration subsystem"
                << std::endl;
            return telux::common::Status::FAILED;
        }
        locConfigListener_ = std::make_shared<MyLocationConfigListener>();
    } else {
       std::cout<< "Location configurator is already initialized" << std::endl;
    }
   return telux::common::Status::SUCCESS;
}

int LocationMenu::init() {

    int commandSeqNum = 1; 
    std::shared_ptr<ConsoleAppCommand> startDetailedReportsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
          std::to_string(commandSeqNum++), "Start_Detailed_Reports", {},
          std::bind(&LocationMenu::startDetailedReports, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> startDetailedEngineReportsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
          std::to_string(commandSeqNum++), "Start_Detailed_Engine_Reports", {},
          std::bind(&LocationMenu::startDetailedEngineReports, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> startBasicReportsCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
          std::to_string(commandSeqNum++), "Start_Basic_Reports", {},
          std::bind(&LocationMenu::startBasicReports, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> stopReportsCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand(std::to_string(commandSeqNum++), "Stop_Reports", {},
                        std::bind(&LocationMenu::stopReports, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> enableReportLogsCommand = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand(std::to_string(commandSeqNum++), "Filter_notifications", {},
                        std::bind(&LocationMenu::enableReportLogs, this, std::placeholders::_1)));


    std::shared_ptr<ConsoleAppCommand> enableDisablePace = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand(std::to_string(commandSeqNum++), "Configure PACE", {},
                        std::bind(&LocationMenu::enableDisablePace, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> deleteAllAidingData = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand(std::to_string(commandSeqNum++), "Delete_data", {},
                        std::bind(&LocationMenu::deleteAllAidingData, this, std::placeholders::_1)));


    std::shared_ptr<ConsoleAppCommand> configureConstellation = std::make_shared<ConsoleAppCommand>(
        ConsoleAppCommand(std::to_string(commandSeqNum++), "Configure blacklist constellation or SVs", {}, std::bind(
                        &LocationMenu::configureConstellation, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> registerLocationSystemInfo = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Register Location System Info", {},
                        std::bind(&LocationMenu::registerLocationSystemInfo, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> deRegisterLocationSystemInfo = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Deregister Location System Info", {},
                        std::bind(&LocationMenu::deRegisterLocationSystemInfo, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureMinGpsWeek = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Configure minimum gps week", {},
                        std::bind(&LocationMenu::configureMinGpsWeek, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestMinGpsWeek = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Request minimum gps week", {},
                        std::bind(&LocationMenu::requestMinGpsWeek, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> deleteAidingDataWarm = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Delete aiding data", {}, std::bind(
                        &LocationMenu::deleteAidingDataWarm, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureMinSVElevation = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Configure minimum sv elevation", {},
                        std::bind(&LocationMenu::configureMinSVElevation, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestMinSVElevation = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Request minimum sv elevation", {},
                        std::bind(&LocationMenu::requestMinSVElevation, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureConstellationEmpty = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Configure constellation, enable all", {}, std::bind(
                        &LocationMenu::configureConstellationEmpty, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureConstellationDeviceDefault = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Configure constellation, device default", {}, std::bind(
                        &LocationMenu::configureConstellationDeviceDefault, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureSecondaryBand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Configure secondary band constellation", {}, std::bind(
                        &LocationMenu::configureSecondaryBand, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> enableDefaultSecondaryBand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Enable default secondary band constellation", {}, std::bind(
                        &LocationMenu::enableDefaultSecondaryBand, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestSecondaryBand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand(std::to_string(commandSeqNum++), "Request secondary band constellation", {}, std::bind(
                        &LocationMenu::requestSecondaryBand, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureNmeaSentence =
       std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandSeqNum++),
           "Configure Nmea sentences", {}, std::bind(&LocationMenu::
               configureNmeaSentence, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureAllNmeaSentence =
       std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandSeqNum++),
           "Configure All Nmea sentences", {}, std::bind(&LocationMenu::
               configureAllNmeaSentence, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> getCapabilities =
       std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandSeqNum++),
           "Request capabilities information", {}, std::bind(&LocationMenu::
               getCapabilities, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> configureXtraParams =
       std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandSeqNum++),
           "Configure Xtra Parameters", {}, std::bind(&LocationMenu::
               configureXtraParameters, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> requestXtraStatus =
       std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandSeqNum++),
           "Request Xtra Status", {}, std::bind(&LocationMenu::
               requestXtraStatus, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> registerConfigListener =
       std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandSeqNum++),
           "Register Configuration Listener", {}, std::bind(&LocationMenu::
               registerConfigListener, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> deRegisterConfigListener =
       std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandSeqNum++),
           "De-Register Configuration Listener", {}, std::bind(&LocationMenu::
               deRegisterConfigListener, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListGnssSubMenu
        = {startDetailedReportsCommand, startDetailedEngineReportsCommand, startBasicReportsCommand,
        stopReportsCommand, enableReportLogsCommand, enableDisablePace,
        deleteAllAidingData, configureConstellation,
        registerLocationSystemInfo, deRegisterLocationSystemInfo,
        configureMinGpsWeek, requestMinGpsWeek, deleteAidingDataWarm,
        configureMinSVElevation, requestMinSVElevation,
        configureConstellationEmpty, configureConstellationDeviceDefault,
        configureSecondaryBand, enableDefaultSecondaryBand, requestSecondaryBand,
        configureNmeaSentence,
        configureAllNmeaSentence, getCapabilities, configureXtraParams, requestXtraStatus,
        registerConfigListener, deRegisterConfigListener,
        };

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

      std::string configureSet;
      std::cout << "Press Y to configure the set of reports : " << std::endl;
      std::getline(std::cin, configureSet, delimiter);
      if (configureSet == "Y" || configureSet == "y") {
          std::string reportPreference;
          GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
          std::vector<int> options;
          std::cout << " Enter the type of reports to enable : \n"
                       " (0 - Location\n 1 - SV\n 2 - NMEA\n 3 - DATA\n 4 - Measurement\n "
                       "5 - NHzMeasurement\n 6 - Disaster-Crisis\n 8 - Ephemeris)\n\n";
          std::cout << " Enter your preference\n"
                       " (For example: enter 0,1 to choose Location & SV reports) : ";
          std::getline(std::cin,reportPreference,delimiter);
          std::stringstream ss(reportPreference);
          int i;
          while(ss >> i) {
              options.push_back(i);
              if(ss.peek() == ',' || ss.peek() == ' ')
                  ss.ignore();
          }
          for(auto &option : options) {
              if(option >= 0 && option <= 8 && option != 7) {
                  try {
                      reportMask |= 1UL << option;
                  } catch(const std::exception &e) {
                      std::cout << "ERROR: invalid input, please enter numerical values " << option
                                << std::endl;
                  }
              } else {
                  std::cout << "Report preference should not be out of range" << std::endl;
              }
          }
          if(opt > 0) {
              myLocCmdResponseCb_
                  = std::make_shared<MyLocationCommandCallback>("Detailed report request");
              locationManager_->startDetailedReports(
                  (uint32_t)opt, std::bind(&MyLocationCommandCallback::commandResponse,
                      myLocCmdResponseCb_, std::placeholders::_1), reportMask);
          } else {
              std::cout << " Invalid input \n";
          }
      } else {
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
                   " (0 - FUSED\n 1 - SPE\n 2 - PPE\n 3 - VPE) \n\n";
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
        if(opt >= 0 && opt <= 3) {
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

      std::string configureSet;
      std::cout << "Press Y to configure the set of reports : " << std::endl;
      std::getline(std::cin, configureSet, delimiter);
      if (configureSet == "Y" || configureSet == "y") {
          std::string reportPreference;
          GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
          std::vector<int> options;
          std::cout << " Enter the type of reports to enable : \n"
                       " (0- Location\n 1- SV\n 2- NMEA\n 3- DATA\n 4- Measurement\n "
                       "5- NHzMeasurement\n 6 - DisasterCrisis\n 7- EngineNMEA\n 8- Ephemeris)\n\n";
          std::cout << " Enter your preference\n"
                       " (For example: enter 0,1 to choose Location & SV reports) : ";
          std::getline(std::cin,reportPreference,delimiter);
          std::stringstream ss(reportPreference);
          int i;
          while(ss >> i) {
              options.push_back(i);
              if(ss.peek() == ',' || ss.peek() == ' ')
                  ss.ignore();
          }
          for(auto &option : options) {
              if(option >= 0 && option <= 8) {
                  try {
                      reportMask |= 1UL << option;
                  } catch(const std::exception &e) {
                      std::cout << "ERROR: invalid input, please enter numerical values " << option
                                << std::endl;
                  }
              } else {
                  std::cout << "Report preference should not be out of range" << std::endl;
              }
          }
          if(opt > 0) {
              myLocCmdResponseCb_
                  = std::make_shared<MyLocationCommandCallback>("Detailed engine report request");
              locationManager_->startDetailedEngineReports(
                  (uint32_t)opt, engineType, std::bind(&MyLocationCommandCallback::commandResponse,
                      myLocCmdResponseCb_, std::placeholders::_1), reportMask);
          } else {
              std::cout << " Invalid input \n";
          }
      } else {
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
}

void LocationMenu::startBasicReports(std::vector<std::string> userInput) {
   if(locationManager_) {
      char delimiter = '\n';
      std::string minItervalInput;
      std::cout << "Enter Interval in Milliseconds (default: 1000ms): ";
      std::getline(std::cin, minItervalInput, delimiter);
      int optInterval = -1;
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

      if(optInterval > 0) {
         myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Basic report request");
         locationManager_->startBasicReports((uint32_t)optInterval,
                                             std::bind(&MyLocationCommandCallback::commandResponse,
                                                       myLocCmdResponseCb_, std::placeholders::_1));
      } else {
         std::cout << " Invalid input \n";
      }
   }
}

void LocationMenu::registerLocationSystemInfo(std::vector<std::string> userInput) {
  myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Register Location System Info");
   locationManager_->registerForSystemInfoUpdates(posListener_, std::bind(
       &MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_, std::placeholders::_1));
}

void LocationMenu::deRegisterLocationSystemInfo(std::vector<std::string> userInput) {
  myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Deregister Location System Info");
   locationManager_->deRegisterForSystemInfoUpdates(posListener_, std::bind(
       &MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_, std::placeholders::_1));
}

void LocationMenu::stopReports(std::vector<std::string> userInput) {
   myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Stop request");
   locationManager_->stopReports(std::bind(&MyLocationCommandCallback::commandResponse,
                                           myLocCmdResponseCb_, std::placeholders::_1));
}

void LocationMenu::enableDisablePace(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
       char delimiter = '\n';
       std::string option;
       std::cout << "Enter Y to enable or N to disable PACE: ";
       std::getline(std::cin, option, delimiter);

       bool enable = false;
       if(option == "Y" || option == "y") {
            enable = true;
       } else if(option == "N" || option == "n") {
            enable = false;
       } else {
            std::cout << " BAD input " << std::endl;
       }
        std::cout << " Enable: " << enable << std::endl;

        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Configure-PACE");
        telux::common::Status status = locationConfigurator_->configurePACE(enable,
                std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                        std::placeholders::_1));
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
   }
}

void LocationMenu::deleteAllAidingData(std::vector<std::string> userInput) {
   if(locationConfigurator_) {
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Delete Aiding Data");
        telux::common::Status status = locationConfigurator_->deleteAllAidingData(
                std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                        std::placeholders::_1));
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
   }
}

void LocationMenu::deleteAidingDataWarm(std::vector<std::string> userInput) {
   if(locationConfigurator_) {
      char delimiter = '\n';
      std::string deleteDataPreference;
      AidingData dataType = DEFAULT_UNKNOWN;
      std::vector<int> options;
      std::cout << "Enter the types of data to be deleted : \n"
                   "0 - EPHEMERIS \n"
                   "Enter your delete data preference\n";
      std::getline(std::cin,deleteDataPreference,delimiter);
      std::stringstream ss(deleteDataPreference);
      int i = -1;
      while(ss >> i) {
        options.push_back(i);
        if(ss.peek() == ',' || ss.peek() == ' ')
          ss.ignore();
      }
      for(auto &opt : options) {
        if(opt == 0 || opt == 1) {
          try {
            dataType |= 1UL << opt;
          } catch(const std::exception &e) {
            std::cout << "ERROR: invalid input, please enter numerical values " << opt
                         << std::endl;
          }
        } else {
            std::cout << "Delete data preference should not be out of range" << std::endl;
        }
      }

      myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
          "Delete Aiding Data Warm Start");
      telux::common::Status status = locationConfigurator_->deleteAidingData(dataType,
          std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
              std::placeholders::_1));
      if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
      }
   }
}

void LocationMenu::configureNmeaSentence(std::vector<std::string> userInput) {
    if(locationConfigurator_) {
        char delimiter = '\n';
        std::string nmeaSentencePreference;
        NmeaSentenceConfig nmeaType = DEFAULT_UNKNOWN;
        std::vector<int> options;
        std::cout << "Enter the nmea sentence types to be enabled : \n"
                    "0 - GGA, 1 - RMC, 2 - GSA, 3 - VTG, \n"
                    "4 - GNS, 5 - DTM, 6 - GPGSV, 7 - GLGSV \n"
                    "8 - GAGSV, 9 - GQGSV, 10 - GBGSV, 11 - GIGSV \n"
                    "Enter your nmea type preference\n"
                    "(Example: enter 0,1,3 to enable GGA, RMC and VTG):\n";
        std::getline(std::cin,nmeaSentencePreference,delimiter);
        std::stringstream ss(nmeaSentencePreference);
        int i = -1;
        while(ss >> i) {
            options.push_back(i);
            if(ss.peek() == ',' || ss.peek() == ' ')
            ss.ignore();
        }
        for(auto &opt : options) {
            if(opt >= 0 && opt <= 11) {
                nmeaType |= 1UL << opt;
            } else {
                std::cout << "Nmea types should not be out of range" << std::endl;
            }
        }

        int nmeaDatumPref;
        std::cout << "\nEnter Nmea Datum Type to be used: \n"
                        "0 - WGS_84 \n"
                        "1 - PZ-90 \n";
        std::cin >> nmeaDatumPref;
        Utils::validateInput(nmeaDatumPref);

        std::string enginePreference;
        LocReqEngine engineType = DEFAULT_UNKNOWN;
        std::vector<int> engOptions;
        std::cout << " Enter the type of engine reports : \n"
                    " (0 - FUSED\n 1 - SPE\n 2 - PPE\n 3 - VPE) \n\n";
        std::cout << " Enter your engine preference\n"
                    " (For example: enter 0,1 to choose FUSED & SPE engine fixes) : ";
        std::getline(std::cin,enginePreference,delimiter);
        std::stringstream engss(enginePreference);
        int itr;
        while(engss >> itr) {
            engOptions.push_back(itr);
            if(engss.peek() == ',' || engss.peek() == ' ')
            engss.ignore();
        }
        for(auto &opt : engOptions) {
            if(opt >= 0 && opt <= 3) {
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

        telux::common::Status status;
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
            "Configure Nmea sentence types");

        telux::loc::NmeaConfig nmeaConfigParams;
        nmeaConfigParams.sentenceConfig = nmeaType;
        if(nmeaDatumPref > 1) {
            nmeaDatumPref = 0;
        }
        nmeaConfigParams.datumType = static_cast<telux::loc::GeodeticDatumType>(nmeaDatumPref);
        nmeaConfigParams.engineType = engineType;
        status = locationConfigurator_->configureNmea(nmeaConfigParams,
            std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                std::placeholders::_1));

        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Configure Nmea sentence types failed" << std::endl;
        }
    }
}

void LocationMenu::configureAllNmeaSentence(std::vector<std::string> userInput) {
    if(locationConfigurator_) {
        telux::loc::NmeaSentenceConfig nmeaType = telux::loc::NmeaSentenceType::ALL;

        int nmeaDatumPref;
        std::cout << "\nEnter Nmea Datum Type to be used: \n"
                        "0 - WGS_84 \n"
                        "1 - PZ-90 \n";
        std::cin >> nmeaDatumPref;
        Utils::validateInput(nmeaDatumPref);

        char delimiter = '\n';
        std::string enginePreference;
        LocReqEngine engineType = DEFAULT_UNKNOWN;
        std::vector<int> engOptions;
        std::cout << " Enter the type of engine reports : \n"
                    " (0 - FUSED\n 1 - SPE\n 2 - PPE\n 3 - VPE) \n\n";
        std::cout << " Enter your engine preference\n"
                    " (For example: enter 0,1 to choose FUSED & SPE engine fixes) : ";
        std::getline(std::cin,enginePreference,delimiter);
        std::stringstream engss(enginePreference);
        int itr;
        while(engss >> itr) {
            engOptions.push_back(itr);
            if(engss.peek() == ',' || engss.peek() == ' ')
            engss.ignore();
        }
        for(auto &opt : engOptions) {
            if(opt >= 0 && opt <= 3) {
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

        telux::common::Status status;
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
            "Configure All Nmea sentence types");

        telux::loc::NmeaConfig nmeaConfigParams;
        nmeaConfigParams.sentenceConfig = nmeaType;
        if(nmeaDatumPref > 1) {
            nmeaDatumPref = 0;
        }
        nmeaConfigParams.datumType = static_cast<telux::loc::GeodeticDatumType>(nmeaDatumPref);
        nmeaConfigParams.engineType = engineType;
        status = locationConfigurator_->configureNmea(nmeaConfigParams,
            std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                std::placeholders::_1));

        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Configure All Nmea sentence types failed" << std::endl;
        }
    }
}

void LocationMenu::configureConstellation(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
        typedef std::vector<telux::loc::SvBlackListInfo> SvBlackList;
        SvBlackList svBlackList;
        bool deviceReset = false;
        char delimiter = '\n';
        while(true) {
            telux::loc::SvBlackListInfo blackListInfo;
            std::string constellation;
            std::cout << " Enter the constellation : " << std::endl;
            std::cout << " Enter 1 for GPS, 2 for GALILEO, 3 for SBAS, 5 for GLONASS " << std::endl;
            std::cout << " 6 for BEIDOU, 7 for QZSS, 8 for NAVIC : " << std::endl;
            std::getline(std::cin, constellation, delimiter);
            int constellationOption = 2;
            if(!constellation.empty()) {
                try {
                    constellationOption = std::stof(constellation);
                } catch(const std::exception &e) {
                    std::cout << "ERROR: invalid input, please enter numerical values " <<
                        constellationOption << std::endl;
                }
            } else {
                 constellationOption = 2;
            }
            if(constellationOption < 1 or constellationOption == 4 or  constellationOption > 8) {
                std::cout << "invalid constellation, enter again." << std::endl;
                continue;
            }
            if(constellationOption == 1) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::GPS;
            } else if(constellationOption == 2) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::GALILEO;
            } else if (constellationOption == 3) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::SBAS;
            } else if (constellationOption == 5) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::GLONASS;
            } else if (constellationOption == 6) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::BDS;
            } else if (constellationOption == 7) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::QZSS;
            } else {
                blackListInfo.constellation = telux::loc::GnssConstellationType::NAVIC;
            }
            std::cout << " constellationOption : " << constellationOption << std::endl;
            std::string satId;
            std::cout << " Enter the svId : " << std::endl;
            std::getline(std::cin, satId, delimiter);
            uint32_t satIdOption = 0;
            if(!satId.empty()) {
                try {
                    satIdOption = std::stoi(satId);
                } catch(const std::exception &e) {
                    std::cout << "ERROR: invalid input, please enter numerical values " <<
                        satIdOption << std::endl;
                }
            } else {
                 satIdOption = 0;
            }
            blackListInfo.svId = satIdOption;
            std::cout << " blackListInfo.svId" << blackListInfo.svId << std::endl;
            svBlackList.push_back(blackListInfo);
            std::string option;
            std::cout << "Do you want to insert more, enter Y/N : " << std::endl;
            std::getline(std::cin, option, delimiter);
            if(option == "Y" || option == "y") {
                continue;
            } else {
                break;
            }

        }
        for (auto i : svBlackList) {
            std::cout << " i.constellation : " << std::endl;
            std::cout << " i.svId : " << i.svId << std::endl;
        }

        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Configure constellation");
        telux::common::Status status = locationConfigurator_->configureConstellations(svBlackList,
                std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                    std::placeholders::_1), deviceReset);
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
  }
}

void LocationMenu::configureConstellationEmpty(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
        typedef std::vector<telux::loc::SvBlackListInfo> SvBlackList;
        SvBlackList svBlackList;
        bool deviceReset = false;
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Configure constellation");
        telux::common::Status status = locationConfigurator_->configureConstellations(svBlackList,
                std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                    std::placeholders::_1), deviceReset);
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
  }
}

void LocationMenu::configureConstellationDeviceDefault(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
        typedef std::vector<telux::loc::SvBlackListInfo> SvBlackList;
        SvBlackList svBlackList;
        bool deviceReset = true;

        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Configure constellation");
        telux::common::Status status = locationConfigurator_->configureConstellations(svBlackList,
                std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                        std::placeholders::_1), deviceReset);
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
  }
}

void LocationMenu::configureSecondaryBand(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
      telux::loc::ConstellationSet constellationSet{};
      char delimiter = '\n';
      std::string constellations;
      std::cout << " Enter the constellations whose secondary bands need to be disabled : \n"
                   " 1 - GPS\n"
                   " 2 - GALILEO\n"
                   " 3 - SBAS\n"
                   " 5 - GLONASS\n"
                   " 6 - BDS\n"
                   " 7 - QZSS\n"
                   " 8 - NAVIC\n"
                   " (For example: enter 3,6 to disable secondary band for SBAS and BDS) : \n";
      std::getline(std::cin,constellations,delimiter);
      std::stringstream ss(constellations);
      std::vector<int> options;
      int i = -1;
      while(ss >> i) {
          options.push_back(i);
          if(ss.peek() == ',' || ss.peek() == ' ')
              ss.ignore();
      }
      for(auto &opt : options) {
          if (opt == 1) {
              constellationSet.insert(telux::loc::GnssConstellationType::GPS);
          } else if (opt == 2) {
              constellationSet.insert(telux::loc::GnssConstellationType::GALILEO);
          } else if (opt == 3) {
              constellationSet.insert(telux::loc::GnssConstellationType::SBAS);
          } else if (opt == 5) {
              constellationSet.insert(telux::loc::GnssConstellationType::GLONASS);
          } else if (opt == 6) {
              constellationSet.insert(telux::loc::GnssConstellationType::BDS);
          } else if (opt == 7) {
              constellationSet.insert(telux::loc::GnssConstellationType::QZSS);
          } else if (opt == 8) {
              constellationSet.insert(telux::loc::GnssConstellationType::NAVIC);
          } else {
              std::cout << "Ignoring option as not supported: " << opt << std::endl;
          }
      }
      myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Configure secondary band constellations");
      telux::common::Status status = locationConfigurator_->configureSecondaryBand(
          constellationSet, std::bind(&MyLocationCommandCallback::commandResponse,
              myLocCmdResponseCb_, std::placeholders::_1));
      Utils::printStatus(status);
  }
}

void LocationMenu::enableDefaultSecondaryBand(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
      telux::loc::ConstellationSet constellationSet{};

      myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
          ("Configure secondary band empty constellations");
      telux::common::Status status = locationConfigurator_->configureSecondaryBand(
          constellationSet, std::bind(&MyLocationCommandCallback::commandResponse,
              myLocCmdResponseCb_, std::placeholders::_1));
      Utils::printStatus(status);
  }
}

void LocationMenu::requestSecondaryBand(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
      myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Request secondary band constellations");
      auto secondaryBandCb = std::bind(&MyLocationCommandCallback::onSecondaryBandInfo,
          myLocCmdResponseCb_, std::placeholders::_1, std::placeholders::_2);
      telux::common::Status status = locationConfigurator_->requestSecondaryBandConfig(
          secondaryBandCb);
      Utils::printStatus(status);
   }
}

void LocationMenu::getCapabilities(std::vector<std::string> userInput) {
  telux::loc::LocCapability capabilities = locationManager_->getCapabilities();
  LocationUtils::displayCapabilities(capabilities);
}

void LocationMenu::configureMinGpsWeek(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
       char delimiter = '\n';
       std::string option{};
       std::cout << "Enter minimum gps week : ";
       std::getline(std::cin, option, delimiter);
       uint16_t minGpsWeek = 0;
        if(!option.empty()) {
            try {
                minGpsWeek = std::stoi(option);
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " << minGpsWeek
                          << std::endl;
            }
        } else {
             minGpsWeek = 0;
        }
        std::cout << " Entered value is : " << minGpsWeek << std::endl;
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Configure-Minimum Gps Week");
        telux::common::Status status = locationConfigurator_->configureMinGpsWeek(minGpsWeek,
            std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                        std::placeholders::_1));
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
   }
}

void LocationMenu::requestMinGpsWeek(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Request-Minimum Gps Week");
        auto minGpsWeekCb = std::bind(&MyLocationCommandCallback::onMinGpsWeekInfo,
            myLocCmdResponseCb_, std::placeholders::_1, std::placeholders::_2);
        telux::common::Status status = locationConfigurator_->requestMinGpsWeek(minGpsWeekCb);
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
   }
}

void LocationMenu::configureMinSVElevation(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
       char delimiter = '\n';
       std::string option{};
       std::cout << "Enter minimum sv elevation : ";
       std::getline(std::cin, option, delimiter);
       uint8_t minSVElevation = 0;
        if(!option.empty()) {
            try {
                minSVElevation = static_cast<uint8_t>(std::stoi(option));
            } catch(const std::exception &e) {
                std::cout << "ERROR: invalid input, please enter numerical values " <<
                    minSVElevation << std::endl;
            }
        } else {
             minSVElevation = 0;
        }
        std::cout << " Entered value is : " << (uint32_t)minSVElevation << std::endl;
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Configure-Minimum SV Elevation");
        telux::common::Status status = locationConfigurator_->configureMinSVElevation(
            minSVElevation, std::bind(&MyLocationCommandCallback::commandResponse,
                myLocCmdResponseCb_, std::placeholders::_1));
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << __FUNCTION__ << "Not implemented" << std::endl;
        } else if (status != telux::common::Status::SUCCESS) {
          std::cout << __FUNCTION__ << " Command Failed" << std::endl;
        }
   }
}

void LocationMenu::requestMinSVElevation(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Request-Minimum SV Elevation");
        auto minSVElevationCb = std::bind(&MyLocationCommandCallback::onMinSVElevationInfo,
            myLocCmdResponseCb_, std::placeholders::_1, std::placeholders::_2);
        telux::common::Status status =
            locationConfigurator_->requestMinSVElevation(minSVElevationCb);
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << __FUNCTION__ << "Not implemented" << std::endl;
        } else if (status != telux::common::Status::SUCCESS) {
          std::cout << __FUNCTION__ << " Command Failed" << std::endl;
        }
    }
}

void LocationMenu::populateXtraConfigParams(telux::loc::XtraConfig &configParams) {
    char delimiter = '\n';
    uint32_t downloadIntervalMinute;
    std::cout << "Enter Xtra Download Interval Min : ";
    std::cin >> downloadIntervalMinute;
    Utils::validateInput(downloadIntervalMinute);
    configParams.downloadIntervalMinute = downloadIntervalMinute;

    uint32_t downloadTimeoutSec;
    std::cout << "Enter Xtra Download Timeout Sec : ";
    std::cin >> downloadTimeoutSec;
    Utils::validateInput(downloadTimeoutSec);
    configParams.downloadTimeoutSec = downloadTimeoutSec;

    uint32_t downloadRetryIntervalMinute;
    std::cout << "Enter Xtra Download Retry Interval Min : ";
    std::cin >> downloadRetryIntervalMinute;
    Utils::validateInput(downloadRetryIntervalMinute);
    configParams.downloadRetryIntervalMinute = downloadRetryIntervalMinute;

    uint32_t downloadRetryAttempts;
    std::cout << "Enter Xtra Download Retry Attempts : ";
    std::cin >> downloadRetryAttempts;
    Utils::validateInput(downloadRetryAttempts);
    configParams.downloadRetryAttempts = downloadRetryAttempts;

    std::string caPath;
    std::cout << "Enter Xtra CA Path : ";
    std::getline(std::cin, caPath, delimiter);
    configParams.caPath = caPath;

    std::vector<std::string> serverURLs;
    int xtraServerOption;
    std::cout << "Enter Xtra Server URLs count [1-3]: ";
    std::cin >> xtraServerOption;
    if((xtraServerOption >= 1) && (xtraServerOption <= 3)) {
        for(int serverItr = 0; serverItr < xtraServerOption; serverItr++) {
            std::cin.get();
            std::string xtraServerURL;
            std::cout << "Enter Xtra Server URL : ";
            std::getline(std::cin, xtraServerURL, delimiter);
            serverURLs.push_back(xtraServerURL);
        }
    }
    configParams.serverURLs = serverURLs;

    std::vector<std::string> ntpServerURLs;
    int ntpServerOption;
    std::cout << "Enter NTP Server URLs count [1-3]: ";
    std::cin >> ntpServerOption;
    if((ntpServerOption >= 1) && (ntpServerOption <= 3)) {
        for(int serverItr = 0; serverItr < ntpServerOption; serverItr++) {
            std::cin.get();
            std::string ntpServerURL;
            std::cout << "Enter NTP Server URL : ";
            std::getline(std::cin, ntpServerURL, delimiter);
            ntpServerURLs.push_back(ntpServerURL);
        }
    }
    configParams.ntpServerURLs = ntpServerURLs;

    std::cin.get();
    std::string integrityOption;
    std::cout << "Enable Xtra integrity (y/n): ";
    std::getline(std::cin, integrityOption, delimiter);
    if(integrityOption == "Y" || integrityOption == "y") {
        configParams.isIntegrityDownloadEnabled = true;
        uint32_t integrityDownloadIntervalMinute;
        std::cout << "Enter Xtra Integirty Download Interval Min : ";
        std::cin >> integrityDownloadIntervalMinute;
        Utils::validateInput(integrityDownloadIntervalMinute);
        configParams.integrityDownloadIntervalMinute = integrityDownloadIntervalMinute;
    } else {
        configParams.isIntegrityDownloadEnabled = false;
    }

    int daemonDebugLogLevel;
    std::cout << "Enter Xtra Daemon Debug Loglevel [0-5]: ";
    std::cin >> daemonDebugLogLevel;
    Utils::validateInput(daemonDebugLogLevel);
    if((daemonDebugLogLevel < 0) && (daemonDebugLogLevel > 5)) {
        daemonDebugLogLevel = 0;
    }
    configParams.daemonDebugLogLevel =
        static_cast<telux::loc::DebugLogLevel>(daemonDebugLogLevel);

    std::string ntsServer;
    std::cout << "Enter NTS server url: ";
    std::getline(std::cin, ntsServer, delimiter);
    configParams.ntsServerURL = ntsServer;

    std::cin.get();
    std::string diagLogging;
    std::cout << "Enable Xtra Diag logging (y/n): ";
    std::getline(std::cin, diagLogging, delimiter);
    if(diagLogging == "Y" || diagLogging == "y") {
        configParams.isDiagLoggingEnabled = true;
    } else {
        configParams.isDiagLoggingEnabled = false;
    }
}

void LocationMenu::configureXtraParameters(std::vector<std::string> userInput) {
    if(locationConfigurator_) {
        telux::loc::XtraConfig configParams = {};
        char delimiter = '\n';
        std::string option;
        std::cout << "Enable Xtra feature (y/n): ";
        std::getline(std::cin, option, delimiter);
        bool enable = false;
        if(option == "Y" || option == "y") {
            enable = true;
            populateXtraConfigParams(configParams);
        } else if(option == "N" || option == "n") {
            enable = false;
        } else {
            std::cout << " BAD input " << std::endl;
        }

        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
            "Configure Xtra Parameters");
        telux::common::Status status = locationConfigurator_->configureXtraParams(
            enable, configParams, std::bind(&MyLocationCommandCallback::commandResponse,
                myLocCmdResponseCb_, std::placeholders::_1));
        if (status != telux::common::Status::SUCCESS) {
            std::cout << __FUNCTION__ << " Command Failed" << std::endl;
        }
    }
}

void LocationMenu::requestXtraStatus(std::vector<std::string> userInput) {
    if(locationConfigurator_) {
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Request Xtra Status");
        auto getXtraStatusCb = std::bind(&MyLocationCommandCallback::onXtraStatusInfo,
            myLocCmdResponseCb_, std::placeholders::_1, std::placeholders::_2);
        telux::common::Status status =
            locationConfigurator_->requestXtraStatus(getXtraStatusCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << __FUNCTION__ << " Command Failed" << std::endl;
        }
    }
}

void LocationMenu::registerConfigListener(std::vector<std::string> userInput) {
    if(locationConfigurator_ && locConfigListener_) {
        std::bitset<32> indicationsList;
        char delimiter = '\n';
        while(true) {
            int listenerIndication;
            std::cout << "Enter the indication to register for Location Configurator [0-32]: ";
            std::cin >> listenerIndication;
            Utils::validateInput(listenerIndication);
            indicationsList.set(listenerIndication);
            std::string option;
            std::cout << "Do you want to insert more (y/n) : ";
            std::getline(std::cin, option, delimiter);
            if(option == "Y" || option == "y") {
                continue;
            } else {
                break;
            }
        }
        telux::common::Status status =
            locationConfigurator_->registerListener(indicationsList, locConfigListener_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << __FUNCTION__ << " Register Listener Failed" << std::endl;
        } else {
            std::cout << __FUNCTION__ << " Register Listener Success" << std::endl;
        }
    }
}

void LocationMenu::deRegisterConfigListener(std::vector<std::string> userInput) {
    if(locationConfigurator_ && locConfigListener_) {
        std::bitset<32> indicationsList;
        char delimiter = '\n';
        while(true) {
            int listenerIndication;
            std::cout << "Enter the indication to deregister from Location Configurator : ";
            std::cin >> listenerIndication;
            Utils::validateInput(listenerIndication);
            indicationsList.set(listenerIndication);
            std::string option;
            std::cout << "Do you want to insert more (y/n) : ";
            std::getline(std::cin, option, delimiter);
            if(option == "Y" || option == "y") {
                continue;
            } else {
                break;
            }
        }
        telux::common::Status status =
            locationConfigurator_->deRegisterListener(indicationsList, locConfigListener_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << __FUNCTION__ << " De-Register Listener Failed" << std::endl;
        } else {
            std::cout << __FUNCTION__ << " De-Register Listener Success" << std::endl;
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

void LocationMenu::enableDetailedEngineLocReportLogs() {
   int opt = enableReportLogsUtility();
   if((opt == 0) || (opt == 1)) {
      posListener_->setDetailedEngineLocReportFlag(opt);
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
     std::cout << "  4 - Data_info_notifications" << std::endl;
     std::cout << "  5 - Detailed_Engine_location_notifications" << std::endl;
     std::cout << "  6 - Nmea_info_notifications" << std::endl;
     std::cout << "  7 - Measurements_info_notifications" << std::endl;
     std::cout << "  8 - Location_system_information " << std::endl;
     std::cout << "  9 - Disaster_Crisis_info_notifications" << std::endl;
     std::cout << "  10 - Engine_NMEA_info_notifications" << std::endl;
     std::cout << "  11 - Ephemeris_info_notifications" << std::endl << std::endl << std::endl;
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
     } else if(usrInput == "5") {
         LocationMenu::enableDetailedEngineLocReportLogs();
     } else if(usrInput == "6") {
         LocationMenu::enableNmeaInfoLogs();
     } else if(usrInput == "7") {
         LocationMenu::enableMeasurementsInfoLogs();
     } else if(usrInput == "8") {
         LocationMenu::enableLocationSystemInfoLogs();
     } else if(usrInput == "9") {
         LocationMenu::enableDisasterCrisisInfoLogs();
     }  else if(usrInput == "10") {
         LocationMenu::enableEngineNmeaInfoLogs();
     }  else if(usrInput == "11") {
         LocationMenu::enableEphemerisInfoLogs();
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

void LocationMenu::enableNmeaInfoLogs() {
  int opt = enableReportLogsUtility();
  if((opt == 0) || (opt == 1)) {
    posListener_->setNmeaInfoFlag(opt);
  } else {
    std::cout << "ERROR: invalid input, please enter 0 or 1\n";
  }

}

void LocationMenu::enableMeasurementsInfoLogs() {
  int opt = enableReportLogsUtility();
  if((opt == 0) || (opt == 1)) {
    posListener_->setMeasurementsInfoFlag(opt);
  } else {
    std::cout << "ERROR: invalid input, please enter 0 or 1\n";
  }
}

void LocationMenu::enableDisasterCrisisInfoLogs() {
    int opt = enableReportLogsUtility();
    if((opt == 0) || (opt == 1)) {
        posListener_->setDisasterCrisisInfoFlag(opt);
    } else {
        std::cout << "ERROR: invalid input, please enter 0 or 1\n";
    }
}

void LocationMenu::enableEphemerisInfoLogs() {
    int opt = enableReportLogsUtility();
    if((opt == 0) || (opt == 1)) {
        posListener_->setEphemerisInfoFlag(opt);
    } else {
        std::cout << "ERROR: invalid input, please enter 0 or 1\n";
    }
}

void LocationMenu::enableEngineNmeaInfoLogs() {
    int opt = enableReportLogsUtility();
    if((opt == 0) || (opt == 1)) {
        posListener_->setEngineNmeaInfoFlag(opt);
    } else {
        std::cout << "ERROR: invalid input, please enter 0 or 1\n";
    }
}

void LocationMenu::enableLocationSystemInfoLogs() {
  int opt = enableReportLogsUtility();
  if((opt == 0) || (opt == 1)) {
    posListener_->setLocSystemInfoFlag(opt);
  } else {
    std::cout << "ERROR: invalid input, please enter 0 or 1\n";
  }
}

telux::common::Status LocationMenu::launchAsRecordingUtility(LocReqEngine engineType) {
    std::cout << "Launching location test app as a recording utility \n";
    std::shared_ptr<MyLocationListener> posListener = std::make_shared<MyLocationListener>();
    std::shared_ptr<ILocationManager> locationManager = nullptr;
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    auto &locationFactory = LocationFactory::getInstance();
    locationManager = locationFactory.getLocationManager([&](ServiceStatus status) {
          if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
            } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
            }
        });
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    ServiceStatus locMgrStatus = locationManager->getServiceStatus();
    if(locMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Location subsystem is not ready, Please wait" << std::endl;
    }
    locMgrStatus = prom.get_future().get();
    if(locMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count()
            << "s\n" << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize Location subsystem" << std::endl;
        return telux::common::Status::FAILED;
    }

    posListener->setDetailedLocationReportFlag(true);
    posListener->setSvInfoFlag(true);
    posListener->setNmeaInfoFlag(true);
    posListener->setMeasurementsInfoFlag(true);
    posListener->setDataInfoFlag(true);
    posListener->setDetailedEngineLocReportFlag(true);

    posListener->setRecordingFlag(true);
    //Registering listener for fixes
    locationManager->registerListenerEx(posListener);

    GnssReportTypeMask reportMask = DEFAULT_UNKNOWN;
    reportMask |= LOCATION;
    reportMask |= SATELLITE_VEHICLE;
    reportMask |= NMEA;
    reportMask |= DATA;
    reportMask |= MEASUREMENT;

    std::shared_ptr<MyLocationCommandCallback> myLocCmdResponseCb =
        std::make_shared<MyLocationCommandCallback>("Detailed report request");
    if (DEFAULT_UNKNOWN == engineType) {
        locationManager->startDetailedReports(
            1000, std::bind(&MyLocationCommandCallback::commandResponse,
            myLocCmdResponseCb, std::placeholders::_1), reportMask);
    } else {
        locationManager->startDetailedEngineReports(
            1000, engineType,
            std::bind(&MyLocationCommandCallback::commandResponse,
                      myLocCmdResponseCb, std::placeholders::_1), reportMask);
    }

    while(1) {
        //Infinite polling to keep retrieving position reports.
        std::this_thread::sleep_for(std::chrono::seconds(RECORDING_MODE_SLEEP));
    }
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string sdkReleaseName = telux::common::Version::getReleaseName();
    std::string appName = "Location Menu - SDK v" + std::to_string(sdkVersion.major) + "."
        + std::to_string(sdkVersion.minor) + "." + std::to_string(sdkVersion.patch) +"\n" +
        "Release name: " + sdkReleaseName;
    LocationMenu locationMenu(appName, "location> ");
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "locclient", "logd",
        "dlt", "leprop"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    if((argc > 1) && (strcmp(argv[1], "-r") == 0)) {

        LocReqEngine engineType = DEFAULT_UNKNOWN;

        if (argc > 2) {
            // possible engine types: FUSED,SPE,PPE,VPE
            std::stringstream ss{argv[2]};
            std::string s;

            while (getline(ss, s, ',')) {
                if (0 == s.compare("FUSED")) {
                    engineType |= LOC_REQ_ENGINE_FUSED_BIT;
                } else if (0 == s.compare("SPE")) {
                    engineType |= LOC_REQ_ENGINE_SPE_BIT;
                } else if (0 == s.compare("PPE")) {
                    engineType |= LOC_REQ_ENGINE_PPE_BIT;
                } else if (0 == s.compare("VPE")) {
                    engineType |= LOC_REQ_ENGINE_VPE_BIT;
                } else {
                    std::cout << "Invalid engine type: " << s << std::endl;
                    std::cout << "FUSED,SPE,PPE,VPE are engine types supported." << std::endl;
                    std::cout << "Please specify one or any comninations of the engine names."
                              << std::endl;

                    return -1;
                }
            }
        }

        std::cout << "engineType : " << engineType << std::endl;
        telux::common::Status status = locationMenu.launchAsRecordingUtility(engineType);
        if(status != telux::common::Status::SUCCESS) {
            std::cout << "Exiting \n";
        }
    } else {
        if( locationMenu.init() == -1) {
            std::cout << "ERROR - Subsystem not ready, Exiting !!!" << std::endl;
            return -1;
        }
        locationMenu.mainLoop();
    }
    return 0;
}
