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
 * This is a sample program to register and receive Thermal
 * Autoshutdown mode updates
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "unistd.h"
}

#include "ThermalShutdownSampleApp.hpp"

static std::promise<AutoShutdownMode> prom;

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

ThermalShutdownSampApp::ThermalShutdownSampApp()
    : thermShutdownMgr_(nullptr) {
}

ThermalShutdownSampApp::~ThermalShutdownSampApp() {
}


void ThermalShutdownSampApp::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if(status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : UNAVAILABLE" << std::endl;
    } else if(status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : AVAILABLE" << std::endl;
    } else {
        std::cout << APP_NAME << " Invalid service status notified " << std::endl;
    }
}

void ThermalShutdownSampApp::onShutdownEnabled(){
    std::cout << std::endl;
    printAutoShutdownMode(AutoShutdownMode::ENABLE);
}

void ThermalShutdownSampApp::onShutdownDisabled(){
    std::cout << std::endl;
    printAutoShutdownMode(AutoShutdownMode::DISABLE);
}

void ThermalShutdownSampApp::onImminentShutdownEnablement(uint32_t imminentDuration) {
    std::cout << std::endl;
    PRINT_NOTIFICATION << "Auto shutdown will be enabled in " <<
        imminentDuration << " seconds" << std::endl;
}

int ThermalShutdownSampApp::init() {
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

void ThermalShutdownSampApp::registerForUpdates() {
    // Registering a listener for auto-shutdown mode updates
    telux::common::Status status = thermShutdownMgr_->registerListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " *** ERROR - Failed to register for auto-shutdown mode events"
                << std::endl;
    } else {
        std::cout << APP_NAME << " Registered Listener for auto-shutdown mode events" << std::endl;
    }
}

void ThermalShutdownSampApp::deregisterForUpdates() {
     // De-registering a listener for auto-shutdown mode updates
    telux::common::Status status = thermShutdownMgr_->deregisterListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " *** ERROR - Failed to de-register for auto-shutdown mode events"
                << std::endl;
    } else {
        std::cout << APP_NAME << " De-registered listener" << std::endl;
    }
}

static void getShutdownModeCallback(AutoShutdownMode state) {
    if(state == AutoShutdownMode::ENABLE) {
        std::cout << APP_NAME << " Current auto shutdown mode is: Enable" << std::endl;
        prom.set_value(AutoShutdownMode::ENABLE);
    } else if(state == AutoShutdownMode::DISABLE) {
        std::cout << APP_NAME << " Current auto shutdown mode is: Disable" << std::endl;
        prom.set_value(AutoShutdownMode::DISABLE);
    }
    else {
        std::cout << APP_NAME <<
            " *** ERROR - Failed to send get auto-shutdown mode " << std::endl;
        prom.set_value(AutoShutdownMode::UNKNOWN);
    }
}

std::future<AutoShutdownMode> ThermalShutdownSampApp::getAutoShutdownMode() {
    telux::common::Status status =
        thermShutdownMgr_->getAutoShutdownMode(&getShutdownModeCallback);
    std::future<AutoShutdownMode> f = prom.get_future();
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME <<
             " *** ERROR - Failed to send get auto-shutdown mode command" << std::endl;
        prom.set_value(AutoShutdownMode::UNKNOWN);
    }
    return f;
}
/**
 * Main routine
 */
int main(int argc, char ** argv) {

    std::shared_ptr<ThermalShutdownSampApp> myThermMgrSamp=std::make_shared<ThermalShutdownSampApp>();
    if( 0 != myThermMgrSamp->init()) {
        std::cout << APP_NAME << " Failed to initialize the Thermal-Shutdown management service"
                << std::endl;
        return -1;
    }
    myThermMgrSamp->registerForUpdates();
    auto f = myThermMgrSamp->getAutoShutdownMode();
    f.wait(); //waiting for get status callback.
    // Exit logic is specific to an application
    std::cout << APP_NAME << " Press enter to exit" << std::endl;
    std::string input;
    std::getline(std::cin, input);
    myThermMgrSamp->deregisterForUpdates();

    std::cout << "Exiting application..." << std::endl;
    return 0;
}
