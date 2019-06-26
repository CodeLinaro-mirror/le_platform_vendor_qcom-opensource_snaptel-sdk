/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

/**
 * This is a sample program to register and receive auto-shutdown mode updates, send commands to
 * change the auto-shutdown mode
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <csignal>
#include <mutex>
#include <condition_variable>

extern "C" {
#include "unistd.h"
}

#include "ThermShutdownMgrTestApp.hpp"

static bool listenerEnabled = false;
static std::mutex mutex;
static std::condition_variable cv;

static void printAutoShutdownMode(AutoShutdownMode mode) {

    if(mode == AutoShutdownMode::ENABLE) {
        PRINT_NOTIFICATION << "Auto shutdown mode : ENABLE" << std::endl;
    } else if(mode == AutoShutdownMode::DISABLE) {
        PRINT_NOTIFICATION << "Auto shutdown mode : DISABLE" << std::endl;
    } else if(mode == AutoShutdownMode::UNKNOWN) {
        PRINT_NOTIFICATION << "Auto shutdown mode : UNKNOWN" << std::endl;
    } else {
        std::cout << APP_NAME << " ERROR: Invalid Auto shutdown mode notified" << std::endl;
    }
}

static void printHelp() {
    std::cout << "-----------------------------------------------" << std::endl;
    std::cout << "./telux_therm_shutdown_test_app <-l> <-e> <-d> <-g> <-c> <-h>" << std::endl;
    std::cout << "   -l : listen to Auto shutdown mode updates" << std::endl;
    std::cout << "   -e : send ENABLE command" << std::endl;
    std::cout << "   -d : send DISABLE command" << std::endl;
    std::cout << "   -g : get Auto shutdown mode" << std::endl;
    std::cout << "   -c : open interactive console" << std::endl;
    std::cout << "   -h : print the help menu" << std::endl;
}

ThermShutdownMgrTestApp::ThermShutdownMgrTestApp()
    : ConsoleApp("Thermal Shutdown-Management Menu", "thrml-shtdwn-mgmt> ")
    , thermShutdownMgr_(nullptr) {
}

ThermShutdownMgrTestApp::~ThermShutdownMgrTestApp() {
}

void ThermShutdownMgrTestApp::onShutdownEnabled() {
    std::cout << std::endl;
    printAutoShutdownMode(AutoShutdownMode::ENABLE);
}

void ThermShutdownMgrTestApp::onShutdownDisabled() {
    std::cout << std::endl;
    printAutoShutdownMode(AutoShutdownMode::DISABLE);
}

void ThermShutdownMgrTestApp::onImminentShutdownEnablement(uint32_t imminentDuration) {
    std::cout << std::endl;
    PRINT_NOTIFICATION << "Auto shutdown will be enabled in " <<
        imminentDuration << " seconds" << std::endl;
}

void ThermShutdownMgrTestApp::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if(status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : UNAVAILABLE" << std::endl;
    } else if(status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : AVAILABLE" << std::endl;
    }
}

static void commandCallback(ErrorCode errorCode) {
    if(errorCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << APP_NAME << " Command initiated successfully " << std::endl;
    } else {
        std::cout << APP_NAME << " Command failed !!!" << std::endl;
    }
    if(!listenerEnabled) {
        std::unique_lock<std::mutex> lock(mutex);// To make sure cv.notify happens after cv.wait
        cv.notify_all();
    }
}

static void getShutdownModeCallback(AutoShutdownMode state) {
    if(state == AutoShutdownMode::ENABLE) {
        std::cout << APP_NAME << " Current auto shutdown mode is: Enable" << std::endl;
    } else if(state == AutoShutdownMode::DISABLE) {
        std::cout << APP_NAME << " Current auto shutdown mode is: Disable" << std::endl;
    }
    else {
        std::cout << APP_NAME <<
            " *** ERROR - Failed to send get auto-shutdown mode " << std::endl;
    }
}

static void signalHandler( int signum ) {
    std::unique_lock<std::mutex> lock(mutex);
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    cv.notify_all();
}

void ThermShutdownMgrTestApp::sendAutoShutdownModeCommand(AutoShutdownMode state) {

    if(state == AutoShutdownMode::ENABLE) {
        std::cout << APP_NAME << " Sending ENABLE command" << std::endl;
    } else if(state == AutoShutdownMode::DISABLE) {
        std::cout << APP_NAME << " Sending DISABLE command" << std::endl;
    }
    telux::common::Status status = thermShutdownMgr_->setAutoShutdownMode(state, &commandCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME <<
            " *** ERROR - Failed to send set auto-shutdown mode command" << std::endl;
    }
}

void ThermShutdownMgrTestApp::getAutoShutdownModeCommand() {

    telux::common::Status status =
        thermShutdownMgr_->getAutoShutdownMode(&getShutdownModeCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME <<
             " *** ERROR - Failed to send get auto-shutdown mode command" << std::endl;
    }
}

int ThermShutdownMgrTestApp::init() {
    // Get thermal factory instance
    auto &thermalFactory = ThermalFactory::getInstance();
    // Get thermal shutdown manager object
    thermShutdownMgr_ = thermalFactory.getThermalShutdownManager();
    if(thermShutdownMgr_ == NULL)
    {
        std::cout << APP_NAME << " *** ERROR - Failed to get manager instance" << std::endl;
        return -1;
    }
    // Check thermal shutdown manager service status
    bool isReady = thermShutdownMgr_->isReady();
    if(!isReady) {
        std::cout << APP_NAME << " Thermal-Shutdown management services are not ready, "
                                 "waiting for it to be ready " << std::endl;
        std::future<bool> f = thermShutdownMgr_->onReady();
        isReady = f.get();
    }

    if(isReady) {
        std::cout << APP_NAME << " Thermal-Shutdown management services are ready !" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR - Unable to initialize Thermal-Shutdown management "
                                 "services" << std::endl;
        return -1;
    }

    return 0;
}

void ThermShutdownMgrTestApp::registerForUpdates() {
    // Registering a listener for auto-shutdown mode updates
    telux::common::Status status = thermShutdownMgr_->registerListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " *** ERROR - Failed to register for auto-shutdown mode events"
                << std::endl;
    } else {
        std::cout << APP_NAME << " Registered Listener for auto-shutdown mode events" << std::endl;
    }
}

void ThermShutdownMgrTestApp::deregisterForUpdates() {
    // De-registering a listener for auto-shutdown mode updates
    telux::common::Status status = thermShutdownMgr_->deregisterListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " *** ERROR - Failed to de-register for auto-shutdown mode events"
                << std::endl;
    } else {
        std::cout << APP_NAME << " De-registered listener" << std::endl;
    }
}

void ThermShutdownMgrTestApp::consoleinit() {
   std::shared_ptr<ConsoleAppCommand> enableAutoShutdownCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Enable auto shutdown mode", {},
         std::bind(&ThermShutdownMgrTestApp::sendAutoShutdownModeCommand, this, AutoShutdownMode::ENABLE)));

   std::shared_ptr<ConsoleAppCommand> disableAutoShutdownCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Disable auto shutdown mode", {},
         std::bind(&ThermShutdownMgrTestApp::sendAutoShutdownModeCommand, this, AutoShutdownMode::DISABLE)));

   std::shared_ptr<ConsoleAppCommand> getAutoShutdownmode
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Get auto shutdown mode", {},
         std::bind(&ThermShutdownMgrTestApp::getAutoShutdownModeCommand, this)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListThermalMenu
      = {enableAutoShutdownCommand, disableAutoShutdownCommand, getAutoShutdownmode};
   ConsoleApp::addCommands(commandsListThermalMenu);
   ConsoleApp::displayMenu();
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {

    bool inputSetCommand = false;
    bool inputGetCommand = false;
    AutoShutdownMode state=AutoShutdownMode::UNKNOWN;

    std::shared_ptr<ThermShutdownMgrTestApp> myThermMgmtTest = std::make_shared<ThermShutdownMgrTestApp>();
    if( 0 != myThermMgmtTest->init()) {
        std::cout << APP_NAME <<
            " Failed to initialize the Thermal-Shutdown management service" << std::endl;
        return -1;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-l") {
            listenerEnabled =true;
        } else if (std::string(argv[i]) == "-e") {
            inputSetCommand=true;
            state=AutoShutdownMode::ENABLE;
        } else if (std::string(argv[i]) == "-d") {
            inputSetCommand=true;
            state=AutoShutdownMode::DISABLE;
        } else if (std::string(argv[i]) == "-g") {
            inputGetCommand=true;
        } else if (std::string(argv[i]) == "-c") {
            myThermMgmtTest->registerForUpdates();
            listenerEnabled =true;
            myThermMgmtTest->consoleinit();
            myThermMgmtTest->mainLoop();
            myThermMgmtTest->deregisterForUpdates();
            return 0;
        } else {
            printHelp();
            return -1;
        }
    }

    if(listenerEnabled) {
        myThermMgmtTest->registerForUpdates();
    }
    signal(SIGINT, signalHandler);
    std::unique_lock<std::mutex> lock(mutex);
    if(inputSetCommand) {
        myThermMgmtTest->sendAutoShutdownModeCommand(state);
    } else if(inputGetCommand) {
        myThermMgmtTest->getAutoShutdownModeCommand();
    }
    std::cout << APP_NAME << " Press CTRL+C to exit" << std::endl;
    cv.wait(lock);
    if(listenerEnabled) {
        myThermMgmtTest->deregisterForUpdates();
    }

    std::cout << "Exiting application..." << std::endl;
    return 0;
}
