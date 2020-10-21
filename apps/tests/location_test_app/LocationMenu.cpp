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
#include <memory>
#include <sstream>

#include <telux/loc/LocationFactory.hpp>
#include <telux/common/Version.hpp>
#include "../../common/utils/Utils.hpp"
#include "LocationMenu.hpp"
#include "MyLocationListener.hpp"
#include "DgnssMenu.hpp"

const int DEFAULT_UNKNOWN = 0;
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
}

telux::common::Status LocationMenu::initLocationManager(std::shared_ptr<ILocationManager>
        &locationManager, std::shared_ptr<MyLocationListener> &posListener) {
    if(locationManager == nullptr) {
      std::promise<ServiceStatus> prom{};
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
        std::promise<ServiceStatus> prom{};
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

   std::shared_ptr<ConsoleAppCommand> enableDisablePace = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("7", "Configure PACE", {},
                        std::bind(&LocationMenu::enableDisablePace, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> deleteAllAidingData = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("8", "Delete_data", {},
                        std::bind(&LocationMenu::deleteAllAidingData, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureLeverArm = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("9", "Lever_arm", {},
                        std::bind(&LocationMenu::configureLeverArm, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureConstellation = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("10", "Configure blacklist constellation or SVs ", {}, std::bind(
                        &LocationMenu::configureConstellation, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureRobustLocation = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("11", "Configure robust location", {}, std::bind(
                        &LocationMenu::configureRobustLocation, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> registerLocationSystemInfo = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("12", "Register Location System Info", {},
                        std::bind(&LocationMenu::registerLocationSystemInfo, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> deRegisterLocationSystemInfo = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("13", "Deregister Location System Info", {},
                        std::bind(&LocationMenu::deRegisterLocationSystemInfo, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestEnergyConsumedInfo = std::make_shared<
       ConsoleAppCommand>(ConsoleAppCommand("14", "Request for energy consumed Info", {},
           std::bind(&LocationMenu::requestEnergyConsumedInfo, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> dgnssInjectCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("15", "Dgnss_Correction_Injection", {},
                        std::bind(&LocationMenu::dgnssInject, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureMinGpsWeek = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("16", "Configure minimum gps week", {},
                        std::bind(&LocationMenu::configureMinGpsWeek, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestMinGpsWeek = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("17", "Request minimum gps week", {},
                        std::bind(&LocationMenu::requestMinGpsWeek, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> deleteAidingDataWarm = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("18", "Delete aiding data", {}, std::bind(
                        &LocationMenu::deleteAidingDataWarm, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureMinSVElevation = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("19", "Configure minimum sv elevation", {},
                        std::bind(&LocationMenu::configureMinSVElevation, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestMinSVElevation = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("20", "Request minimum sv elevation", {},
                        std::bind(&LocationMenu::requestMinSVElevation, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestRobustLocation = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("21", "Request robust Location", {},
                        std::bind(&LocationMenu::requestRobustLocation, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureConstellationEmpty = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("22", "Configure constellation, enable all", {}, std::bind(
                        &LocationMenu::configureConstellationEmpty, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureConstellationDeviceDefault = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("23", "Configure constellation, device default", {}, std::bind(
                        &LocationMenu::configureConstellationDeviceDefault, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureDR = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("24", "Configure dead reckoning engine", {}, std::bind(
                        &LocationMenu::configureDR, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> configureSecondaryBand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("25", "Configure secondary band constellation", {}, std::bind(
                        &LocationMenu::configureSecondaryBand, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> enableDefaultSecondaryBand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("26", "Enable default secondary band constellation", {}, std::bind(
                        &LocationMenu::enableDefaultSecondaryBand, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestSecondaryBand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("27", "Request secondary band constellation", {}, std::bind(
                        &LocationMenu::requestSecondaryBand, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> getYearOfHw = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("28", "Request year of hardware information", {}, std::bind(
                        &LocationMenu::getYearOfHw, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListGnssSubMenu
      = {startDetailedReportsCommand, startDetailedEngineReportsCommand, startBasicReportsCommand,
         stopReportsCommand, enableReportLogsCommand, enableDisableTunc, enableDisablePace,
         deleteAllAidingData, configureLeverArm, configureConstellation, configureRobustLocation,
         registerLocationSystemInfo, deRegisterLocationSystemInfo, requestEnergyConsumedInfo,
         dgnssInjectCommand, configureMinGpsWeek, requestMinGpsWeek, deleteAidingDataWarm,
         configureMinSVElevation, requestMinSVElevation, requestRobustLocation,
         configureConstellationEmpty, configureConstellationDeviceDefault, configureDR,
         configureSecondaryBand, enableDefaultSecondaryBand, requestSecondaryBand, getYearOfHw};

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
       if(option == "Y" || option == "y") {
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
          std::cout << "Not implemented" << std::endl;
        }
   }
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
                   "1 - DR_SENSOR_CALIBRATION \n"
                   "Enter your delete data preference\n"
                   "(Example: enter 0,1 to choose both EPHEMERIS and DR_SENSOR_CALIBRATION):\n";
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

//TODO : split configureLeverArm into smaller sub functions
void LocationMenu::configureLeverArm(std::vector<std::string> userInput) {
   if(locationConfigurator_) {
        typedef std::unordered_map<telux::loc::LeverArmType, telux::loc::LeverArmParams>
            LeverArmConfigInfo;
        LeverArmConfigInfo configInfo;
        char delimiter = '\n';
        while(true) {
            telux::loc::LeverArmType leverArmType;
            telux::loc::LeverArmParams leverArmParams;
            std::string type;
            std::cout << "Enter the LeverArmType : " << std::endl;
            std::cout << " Enter 1 for GNSS_TO_VRP or 2 for DR_IMU_TO_GNSS" << std::endl;
            std::cout << "  or 3 for VEPP_IMU_TO_GNSS " << std::endl;
            std::getline(std::cin, type, delimiter);
            int leverArmTypeOption = 1;
            if(!type.empty()) {
                try {
                    leverArmTypeOption = std::stoi(type);
                } catch(const std::exception &e) {
                    std::cout << "ERROR: invalid input, please enter numerical values " <<
                        leverArmTypeOption << std::endl;
                }
            } else {
                 leverArmTypeOption = 1;
            }
            if(leverArmTypeOption < 1 or leverArmTypeOption > 3) {
                std::cout << "invalid LeverArmType, enter again." << std::endl;
                continue;
            }
            if(leverArmTypeOption == 1) {
                leverArmType = LEVER_ARM_TYPE_GNSS_TO_VRP;
            } else if(leverArmTypeOption == 2) {
                leverArmType = LEVER_ARM_TYPE_DR_IMU_TO_GNSS;
            }else {
                leverArmType = LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS;
            }
            std::cout << "leverArmTypeOption : " << leverArmTypeOption << std::endl;
            std::cout << "leverArmType : " << leverArmType << std::endl;
            std::string forwardOffset;
            float optForwardOffset = 0.0;
            std::cout << " Enter the LeverArm Parameters : " << std::endl;
            std::cout << " Enter forward offset : " << std::endl;
            if (std::getline(std::cin, forwardOffset)) {
                std::stringstream inputStream(forwardOffset);
                if(!(inputStream >> optForwardOffset)) {
                    std::cout << "Invalid Input" << std::endl;
                    return;
                }
             } else {
                 std::cout << "Invalid Input" << std::endl;
             }

            leverArmParams.forwardOffset = optForwardOffset;
            std::cout << " leverArmParams.forwardOffset" << leverArmParams.forwardOffset
                << std::endl;

            std::string sidewaysOffset;
            float optSidewaysOffset = 0.0;
            std::cout << " Enter sideways offset : " << std::endl;
            if (std::getline(std::cin, sidewaysOffset)) {
                std::stringstream inputStream(sidewaysOffset);
                if(!(inputStream >> optSidewaysOffset)) {
                    std::cout << "Invalid Input" << std::endl;
                    return;
                }
             } else {
                 std::cout << "Invalid Input" << std::endl;
             }
            leverArmParams.sidewaysOffset = optSidewaysOffset;
            std::cout << " leverArmParams.sidewaysOffset" << leverArmParams.sidewaysOffset <<
                std::endl;

            std::string upOffset;
            float optUpOffset = 0.0;
            std::cout << " Enter up offset : " << std::endl;
            if (std::getline(std::cin, upOffset)) {
                std::stringstream inputStream(upOffset);
                if(!(inputStream >> optUpOffset)) {
                    std::cout << "Invalid Input" << std::endl;
                    return;
                }
             } else {
                 std::cout << "Invalid Input" << std::endl;
             }
            leverArmParams.upOffset = optUpOffset;
            std::cout << " leverArmParams.upOffset" << leverArmParams.upOffset;

            configInfo.insert({leverArmType, leverArmParams});
            std::string option;
            std::cout << "Do you want to insert more : " << std::endl;
            std::cout << "enter Y/N : " << std::endl;
            std::getline(std::cin, option, delimiter);
            if(option == "Y" || option == "y") {
                continue;
            } else {
                break;
            }

        }
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>("Configure lever arm");
        telux::common::Status status = locationConfigurator_->configureLeverArm(configInfo,
                std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                        std::placeholders::_1));
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
   }
}

void LocationMenu::bodyToSensorUtility(telux::loc::DREngineConfiguration& drConfig) {
    char delimiter = '\n';
    std::string option;
    std::cout << "Is body to sensor mount parameters valid ?" << std::endl;
    std::cout << "Enter Y/N" << std::endl;
    std::getline(std::cin, option, delimiter);
    if(option == "Y" || option == "y") {
        drConfig.validMask |= telux::loc::DRConfigValidityType::
            BODY_TO_SENSOR_MOUNT_PARAMS_VALID;
        std::cout << "Enter Body to sensor parameters" << std::endl;
        std::string rollOffset, yawOffset, pitchOffset, offsetUnc;
        std::cout << "Enter rollOffset :" << std::endl;
        std::getline(std::cin, rollOffset, delimiter);
        drConfig.mountParam.rollOffset = std::stof(rollOffset);
        std::cout << "Enter yawOffset :" << std::endl;
        std::getline(std::cin, yawOffset, delimiter);
        drConfig.mountParam.yawOffset = std::stof(yawOffset);
        std::cout << "Enter pitchOffset :" << std::endl;
        std::getline(std::cin, pitchOffset, delimiter);
        drConfig.mountParam.pitchOffset = std::stof(pitchOffset);
        std::cout << "Enter offsetUnc :" << std::endl;
        std::getline(std::cin, offsetUnc, delimiter);
        drConfig.mountParam.offsetUnc = std::stof(offsetUnc);
    } else {
        std::cout << "Body to sensor mount parameters is invalid " << std::endl;
    }
}

void LocationMenu::speedScaleUtility(telux::loc::DREngineConfiguration& drConfig) {
    char delimiter = '\n';
    std::string option;
    std::cout << "Is vehicle speed scale factor valid ?" << std::endl;
    std::cout << "Enter Y/N" << std::endl;
    std::getline(std::cin, option, delimiter);
    if(option == "Y" || option == "y") {
        drConfig.validMask |= telux::loc::DRConfigValidityType::
            VEHICLE_SPEED_SCALE_FACTOR_VALID;
        std::string speedFactor;
        std::cout << "Enter speedFactor :" << std::endl;
        std::getline(std::cin, speedFactor, delimiter);
        drConfig.speedFactor = std::stof(speedFactor);
    } else {
        std::cout << "Vehicle speed scale factor is invalid " << std::endl;
    }
    std::cout << "Is vehicle speed scale factor uncertainty valid ?" << std::endl;
    std::cout << "Enter Y/N" << std::endl;
    std::getline(std::cin, option, delimiter);
    if(option == "Y" || option == "y") {
        drConfig.validMask |= telux::loc::DRConfigValidityType::
            VEHICLE_SPEED_SCALE_FACTOR_UNC_VALID;
        std::string speedFactorUnc;
        std::cout << "Enter speedFactorUnc :" << std::endl;
        std::getline(std::cin, speedFactorUnc, delimiter);
        drConfig.speedFactorUnc = std::stof(speedFactorUnc);
    } else {
        std::cout << "Vehicle speed scale factor uncertainty is invalid " << std::endl;
    }
}

void LocationMenu::gyroScaleUtility(telux::loc::DREngineConfiguration& drConfig) {
    char delimiter = '\n';
    std::string option;
    std::cout << "Is gyro scale factor valid ?" << std::endl;
    std::cout << "Enter Y/N" << std::endl;
    std::getline(std::cin, option, delimiter);
    if(option == "Y" || option == "y") {
        drConfig.validMask |= telux::loc::DRConfigValidityType::
            GYRO_SCALE_FACTOR_VALID;
        std::string gyroFactor;
        std::cout << "Enter gyroFactor :" << std::endl;
        std::getline(std::cin, gyroFactor, delimiter);
        drConfig.gyroFactor = std::stof(gyroFactor);
    } else {
        std::cout << "Gyro scale factor is invalid " << std::endl;
    }
    std::cout << "Is gyro scale factor uncertainty valid ?" << std::endl;
    std::cout << "Enter Y/N" << std::endl;
    std::getline(std::cin, option, delimiter);
    if(option == "Y" || option == "y") {
        drConfig.validMask |= telux::loc::DRConfigValidityType::
            GYRO_SCALE_FACTOR_UNC_VALID;
        std::string gyroFactorUnc;
        std::cout << "Enter gyroFactorUnc :" << std::endl;
        std::getline(std::cin, gyroFactorUnc, delimiter);
        drConfig.gyroFactorUnc = std::stof(gyroFactorUnc);
    } else {
        std::cout << "Gyro scale factor uncertainty is invalid " << std::endl;
    }
}

void LocationMenu::configureDR(std::vector<std::string> userInput) {
    if(locationConfigurator_) {
        telux::loc::DREngineConfiguration drConfig;
        drConfig.validMask = static_cast<telux::loc::DRConfigValidity>(0);
        bodyToSensorUtility(drConfig);
        speedScaleUtility(drConfig);
        gyroScaleUtility(drConfig);

        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
            "Configure DREngineParameters");
        telux::common::Status status = locationConfigurator_->configureDR(
            drConfig, std::bind(&MyLocationCommandCallback::commandResponse,
                myLocCmdResponseCb_, std::placeholders::_1));
        if (status == telux::common::Status::FAILED) {
          std::cout << "Failed" << std::endl;
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
            std::cout << " Enter 2 for GALILEO, 3 for SBAS, 5 for GLONASS " << std::endl;
            std::cout << " 6 for BEIDOU, 7 for QZSS : " << std::endl;
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
            if(constellationOption < 2 or constellationOption > 7 or constellationOption == 4) {
                std::cout << "invalid constellation, enter again." << std::endl;
                continue;
            }
            if(constellationOption == 2) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::GALILEO;
            } else if (constellationOption == 3) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::SBAS;
            } else if (constellationOption == 5) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::GLONASS;
            } else if (constellationOption == 6) {
                blackListInfo.constellation = telux::loc::GnssConstellationType::BDS;
            } else {
                blackListInfo.constellation = telux::loc::GnssConstellationType::QZSS;
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

void LocationMenu::configureRobustLocation(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
       char delimiter = '\n';
       std::string option;
       std::cout << "Enter Y to enable or N to disable Robust Location: ";
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

       std::string optionE911;
       std::cout << "Enter Y to enable or N to disable Robust Location E911 session: ";
       std::getline(std::cin, optionE911, delimiter);

       bool enableE911 = false;
       if(optionE911 == "Y" || optionE911 == "y") {
            enableE911 = true;
       } else if(optionE911 == "N" || optionE911 == "n") {
            enableE911 = false;
       } else {
            std::cout << " BAD input " << std::endl;
       }
        std::cout << " EnableE911: " << enableE911 << std::endl;

        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Configure-Robust Location");
        telux::common::Status status = locationConfigurator_->configureRobustLocation(enable,
            enableE911, std::bind(&MyLocationCommandCallback::commandResponse, myLocCmdResponseCb_,
                        std::placeholders::_1));
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
   }
}

void LocationMenu::requestRobustLocation(std::vector<std::string> userInput) {
  if(locationConfigurator_) {
        myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>
            ("Request-Robust Location");
        auto robustLocationCb = std::bind(&MyLocationCommandCallback::onRobustLocationInfo,
            myLocCmdResponseCb_, std::placeholders::_1, std::placeholders::_2);
        telux::common::Status status = locationConfigurator_->requestRobustLocation(robustLocationCb);
        if (status == telux::common::Status::NOTIMPLEMENTED) {
          std::cout << "Not implemented" << std::endl;
        }
   }
}

void LocationMenu::requestEnergyConsumedInfo(std::vector<std::string> userInput) {
  myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
      "Request GNSS Energy Consumed Info");
  auto gnssEnergyConsumedCb = std::bind(
      &MyLocationCommandCallback::onGnssEnergyConsumedInfo, myLocCmdResponseCb_,
          std::placeholders::_1, std::placeholders::_2);
   locationManager_->requestEnergyConsumedInfo(gnssEnergyConsumedCb);
}

void LocationMenu::getYearOfHw(std::vector<std::string> userInput) {
  myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
      "Request year of hardware info");
  auto getYearOfHwCb = std::bind(
      &MyLocationCommandCallback::onGetYearOfHwInfo, myLocCmdResponseCb_,
          std::placeholders::_1, std::placeholders::_2);
  locationManager_->getYearOfHw(getYearOfHwCb);
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
     std::cout << "  8 - Location_system_information " << std::endl << std::endl << std::endl;
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
void LocationMenu::dgnssInject(std::vector<std::string> userInput) {
   auto dgnssMenu = std::make_shared<DgnssMenu>("Dgnss Menu", "location> ");
   if (dgnssMenu->init() == -1) {
       std::cout << "ERROR - Subsystem not ready, Exiting !!!" << std::endl;
       return;
   }
   dgnssMenu->mainLoop();
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

void LocationMenu::enableLocationSystemInfoLogs() {
  int opt = enableReportLogsUtility();
  if((opt == 0) || (opt == 1)) {
    posListener_->setLocSystemInfoFlag(opt);
  } else {
    std::cout << "ERROR: invalid input, please enter 0 or 1\n";
  }
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string appName = "Location Menu - SDK v" + std::to_string(sdkVersion.major) + "."
        + std::to_string(sdkVersion.minor) + "." + std::to_string(sdkVersion.patch);
    LocationMenu locationMenu(appName, "location> ");
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "locclient"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    if( locationMenu.init() == -1) {
        std::cout << "ERROR - Subsystem not ready, Exiting !!!" << std::endl;
        return -1;
    }
    locationMenu.mainLoop();
    return 0;
}
