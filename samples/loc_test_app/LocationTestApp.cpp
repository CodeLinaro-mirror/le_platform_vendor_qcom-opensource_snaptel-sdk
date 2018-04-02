/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
#include <iostream>
#include <future>
#include <memory>

#include <telux/loc/LocationFactory.hpp>

#include "LocationTestApp.hpp"
#include "MyLocationListener.hpp"

using namespace telux::loc;

LocationTestApp::~LocationTestApp() {
}

void LocationTestApp::init() {
   std::shared_ptr<ConsoleAppCommand> addLocationListener = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", "add_listener", {}, std::bind(&LocationTestApp::addLocationListener,
                                                           this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> removeLocationListener
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "remove_listener", {},
         std::bind(&LocationTestApp::removeLocationListener, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> finalReportMinInterval
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "set_final_report_min_interval",
         {"min_interval - units: Milli seconds, default:1000ms"},
         std::bind(&LocationTestApp::finalReportMinInterval, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> positionReportTimeout
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "set_position_report_timeout",
         {"time_out - units: Milli seconds, range: 1000ms-255000ms"},
         std::bind(&LocationTestApp::positionReportTimeout, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> horizontalAccuracyLevel
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "5", "horizontal_accuracy_level", {"1 - LOW / 2 - MEDIUM / 3 - HIGH"},
         std::bind(&LocationTestApp::horizontalAccuracyLevel, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListGnssSubMenu
      = {addLocationListener, removeLocationListener, finalReportMinInterval, positionReportTimeout,
         horizontalAccuracyLevel};
   addCommands(commandsListGnssSubMenu);
   ConsoleApp::displayMenu();
}

LocationTestApp::LocationTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   posListener_ = std::make_shared<MyLocationListener>();
   if(locationManager_ == nullptr) {
      auto &locationFactory = LocationFactory::getInstance();
      locationManager_ = locationFactory.getLocationManager();

      std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
      startTime = std::chrono::system_clock::now();
      bool subSystemsStatus = locationManager_->isSubsystemReady();
      if(!subSystemsStatus) {
         std::cout << "Location subsystem is not ready, wait for it to be ready " << std::endl;
         std::future<bool> f = locationManager_->onSubsystemReady();
         subSystemsStatus = f.get();
      }

      if(subSystemsStatus) {
         endTime = std::chrono::system_clock::now();
         std::chrono::duration<double> elapsedTime = endTime - startTime;
         std::cout << "\nElapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                   << std::endl;
      } else {
         std::cout << " *** ERROR - Unable to initialize Location subsystem" << std::endl;
      }
   }
}

void LocationTestApp::addLocationListener(std::vector<std::string> userInput) {
   locationManager_->registerListener(posListener_);
}

void LocationTestApp::removeLocationListener(std::vector<std::string> userInput) {
   locationManager_->removeListener(posListener_);
}

void LocationTestApp::positionReportTimeout(std::vector<std::string> userInput) {
   if(!userInput[1].empty()) {
      int opt = std::stoi(userInput[1]);
      myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
                                                   "Postion report timeout request");
      locationManager_->setPositionReportTimeout((uint32_t)opt, myLocCmdResponseCb_);
   }
}

void LocationTestApp::finalReportMinInterval(std::vector<std::string> userInput) {
   if(!userInput[1].empty()) {
      int opt = std::stoi(userInput[1]);
      myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
                                                   "Final report min interval request");
      locationManager_->setMinIntervalForReports((uint32_t)opt, myLocCmdResponseCb_);
   }
}

void LocationTestApp::horizontalAccuracyLevel(std::vector<std::string> userInput) {
   if(!userInput[1].empty()) {
      int opt = std::stoi(userInput[1]);
      myLocCmdResponseCb_ = std::make_shared<MyLocationCommandCallback>(
                                                   "Horizontal accuracy level request");
      locationManager_->setHorizontalAccuracyLevel((HorizontalAccuracyLevel)opt,
                                                   myLocCmdResponseCb_);
   } else {
      std::cout << "use command 'help' " << std::endl;
   }
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
   LocationTestApp locTestApp("Location Test App", "Loc> ");

   locTestApp.init();  // initialize commands and display

   return locTestApp.mainLoop();  // Main loop to continuously read and execute commands
}
