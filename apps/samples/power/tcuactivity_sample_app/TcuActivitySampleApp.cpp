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
 * This is a sample program to register and receive TCU-activity state updates
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

extern "C" {
#include "unistd.h"
}

#include "TcuActivitySampleApp.hpp"

static void printTcuActivityState(TcuActivityState tcuState) {

    if(tcuState == TcuActivityState::SUSPEND) {
        PRINT_NOTIFICATION << "TCU-activity State : SUSPEND" << std::endl;
    } else if(tcuState == TcuActivityState::RESUME) {
        PRINT_NOTIFICATION << "TCU-activity State : RESUME" << std::endl;
    } else if(tcuState == TcuActivityState::SHUTDOWN) {
        PRINT_NOTIFICATION << "TCU-activity State : SHUTDOWN" << std::endl;
    } else if(tcuState == TcuActivityState::UNKNOWN) {
        PRINT_NOTIFICATION << "TCU-activity State : UNKNOWN" << std::endl;
    } else {
        std::cout << APP_NAME << " Invalid TCU-activity state provided " << std::endl;
    }
}

TcuActivityTestApp::TcuActivityTestApp()
    : tcuActivityStateMgr_(nullptr) {
}

TcuActivityTestApp::~TcuActivityTestApp() {
}

void TcuActivityTestApp::onTcuActivityStateUpdate(TcuActivityState state) {
    std::cout << std::endl;
    printTcuActivityState(state);
    if(state == TcuActivityState::SUSPEND) {
        Status ackStatus = tcuActivityStateMgr_->sendActivityStateAck(
                                                        TcuActivityStateAck::SUSPEND_ACK);
        if(ackStatus == Status::SUCCESS) {
            std::cout << APP_NAME << " Sent SUSPEND acknowledgement" << std::endl;
        } else {
            std::cout << APP_NAME << " Failed to send SUSPEND acknowledgement !" << std::endl;
        }
    } else if(state == TcuActivityState::SHUTDOWN) {
        Status ackStatus = tcuActivityStateMgr_->sendActivityStateAck(
                                                            TcuActivityStateAck::SHUTDOWN_ACK);
        if(ackStatus == Status::SUCCESS) {
            std::cout << APP_NAME << " Sent SHUTDOWN acknowledgement" << std::endl;
        } else {
            std::cout << APP_NAME << " Failed to send SHUTDOWN acknowledgement !" << std::endl;
        }
    }
}

void TcuActivityTestApp::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if(status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : UNAVAILABLE" << std::endl;
    } else if(status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : AVAILABLE" << std::endl;
    } else {
        std::cout << APP_NAME << " Invalid service status notified " << std::endl;
    }
}

int TcuActivityTestApp::start() {
    // Get power factory instance
    auto &powerFactory = PowerFactory::getInstance();
    // Get TCU-activity manager object
    tcuActivityStateMgr_ = powerFactory.getTcuActivityManager();
    if(tcuActivityStateMgr_ == nullptr)
    {
        std::cout << APP_NAME << " *** ERROR - Failed to get manager instance" << std::endl;
        return -1;
    }
    // Check TCU-activity management service status
    bool isReady = tcuActivityStateMgr_->isReady();
    if(!isReady) {
        std::cout << APP_NAME << " TCU-activity management services are not ready, waiting for"
                " it to be ready " << std::endl;
        std::future<bool> f = tcuActivityStateMgr_->onReady();
        isReady = f.get();
    }

    if(isReady) {
        std::cout << APP_NAME << " TCU-activity management services are ready !" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR -Unable to initialize TCU-activity management services"
                << std::endl;
        return -1;
    }

    return 0;
}

void TcuActivityTestApp::registerForUpdates() {
    // Registering a listener for TCU-activity state updates
    telux::common::Status status = tcuActivityStateMgr_->registerListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " *** ERROR - Failed to register for TCU-activity state events"
                << std::endl;
    } else {
        std::cout << APP_NAME << " Registered Listener for TCU-activity state events" << std::endl;
    }
    // Registering a listener for TCU-activity management service status updates
    status = tcuActivityStateMgr_->registerServiceStateListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to register for Service status updates"
                << std::endl;
    }
}

void TcuActivityTestApp::deregisterForUpdates() {
    // De-registering a listener for TCU-activity state updates
    telux::common::Status status = tcuActivityStateMgr_->deregisterListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " *** ERROR - Failed to de-register for TCU-activity state events"
                << std::endl;
    } else {
        std::cout << APP_NAME << " De-registered listener" << std::endl;
    }
    // De-registering a listener for TCU-activity management service status updates
    status = tcuActivityStateMgr_->deregisterServiceStateListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to de-register for Service status updates"
                << std::endl;
    }
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {

    std::shared_ptr<TcuActivityTestApp> myTcuActivityMgrTest=std::make_shared<TcuActivityTestApp>();
    if( 0 != myTcuActivityMgrTest->start()) {
        std::cout << APP_NAME << " Failed to initialize the TCU-activity management service"
                << std::endl;
        return -1;
    }
    myTcuActivityMgrTest->registerForUpdates();

    // Exit logic is specific to an application
    std::cout << APP_NAME << " Press enter to exit" << std::endl;
    std::string input;
    std::getline(std::cin, input);
    myTcuActivityMgrTest->deregisterForUpdates();

    std::cout << "Exiting application..." << std::endl;
    return 0;
}
