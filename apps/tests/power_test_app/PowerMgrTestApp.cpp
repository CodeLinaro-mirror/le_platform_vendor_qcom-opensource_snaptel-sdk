/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * This is a sample program to register and receive TCU-activity state updates, send commands to
 * change the TCU-activity state
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

#include "PowerMgrTestApp.hpp"
#include "../../common/utils/Utils.hpp"

static bool listenerEnabled = false;
static std::mutex mutex;
static std::condition_variable cv;

static void printTcuActivityState(TcuActivityState state) {

    if(state == TcuActivityState::SUSPEND) {
        PRINT_NOTIFICATION << " TCU-activity State : SUSPEND" << std::endl;
    } else if(state == TcuActivityState::RESUME) {
        PRINT_NOTIFICATION << " TCU-activity State : RESUME" << std::endl;
    } else if(state == TcuActivityState::SHUTDOWN) {
        PRINT_NOTIFICATION << " TCU-activity State : SHUTDOWN" << std::endl;
    } else if(state == TcuActivityState::UNKNOWN) {
        PRINT_NOTIFICATION << " TCU-activity State : UNKNOWN" << std::endl;
    } else {
        std::cout << APP_NAME << " ERROR: Invalid TCU-activity state notified" << std::endl;
    }
}

static void printHelp() {
    std::cout << "-----------------------------------------------" << std::endl;
    std::cout << "./telux_power_test_app <-l> <-s> <-r> <-p> <-c> <-h>" << std::endl;
    std::cout << "   -l : listen to TCU-activity state updates (as SLAVE)" << std::endl;
    std::cout << "   -s : send SUSPEND command (as MASTER)" << std::endl;
    std::cout << "   -r : send RESUME command (as MASTER)" << std::endl;
    std::cout << "   -p : send SHUT-DOWN command (as MASTER)" << std::endl;
    std::cout << "   -c : open interactive console (as MASTER)" << std::endl;
    std::cout << "   -h : print the help menu" << std::endl;
}

PowerMgmtTestApp::PowerMgmtTestApp()
    : ConsoleApp("System Power-Management Menu", "power-mgmt> ")
    , tcuActivityMgr_(nullptr) {
}

PowerMgmtTestApp::~PowerMgmtTestApp() {
}

void PowerMgmtTestApp::onTcuActivityStateUpdate(TcuActivityState tcuState) {
    std::cout << std::endl;
    printTcuActivityState(tcuState);
    if(tcuState == TcuActivityState::SUSPEND) {
        Status ackStatus = tcuActivityMgr_->sendActivityStateAck(TcuActivityStateAck::SUSPEND_ACK);
        if(ackStatus == Status::SUCCESS) {
            std::cout << APP_NAME << " Sent SUSPEND acknowledgement" << std::endl;
        } else {
            std::cout << APP_NAME << " Failed to send SUSPEND acknowledgement !" << std::endl;
        }
    } else if(tcuState == TcuActivityState::SHUTDOWN) {
        Status ackStatus = tcuActivityMgr_->sendActivityStateAck(TcuActivityStateAck::SHUTDOWN_ACK);
        if(ackStatus == Status::SUCCESS) {
            std::cout << APP_NAME << " Sent SHUTDOWN acknowledgement" << std::endl;
        } else {
            std::cout << APP_NAME << " Failed to send SHUTDOWN acknowledgement !" << std::endl;
        }
    }
}

void PowerMgmtTestApp::onSlaveAckStatusUpdate(telux::common::Status status) {
    std::cout << std::endl;
    if(status == telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " Slave applications successfully acknowledged the state" <<
                                 " transition" << std::endl;
    } else if(status == telux::common::Status::EXPIRED) {
        std::cout << APP_NAME << " Timeout occured while waiting for acknowledgements from slave" <<
                                 " applications" << std::endl;
    } else {
        std::cout << APP_NAME << " Failed to receive acknowledgements from slave applications"
                              << std::endl;
    }
}

void PowerMgmtTestApp::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if(status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << " Service Status : UNAVAILABLE" << std::endl;
    } else if(status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << " Service Status : AVAILABLE" << std::endl;
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

static void signalHandler( int signum ) {
    std::unique_lock<std::mutex> lock(mutex);
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    cv.notify_all();
}

void PowerMgmtTestApp::sendActivityStateCommand(TcuActivityState state) {
    if(state == TcuActivityState::SUSPEND) {
        std::cout << APP_NAME << " Sending SUSPEND command" << std::endl;
    } else if(state == TcuActivityState::SHUTDOWN) {
        std::cout << APP_NAME << " Sending SHUTDOWN command" << std::endl;
    } else if((state == TcuActivityState::RESUME)) {
        std::cout << APP_NAME << " Sending RESUME command" << std::endl;
    }
    telux::common::Status status = tcuActivityMgr_->setActivityState(state, &commandCallback);;
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to send TCU-activity state command" << std::endl;
    }
}

TcuActivityState PowerMgmtTestApp::getTcuActivityState() {
    TcuActivityState state = tcuActivityMgr_->getActivityState();
    printTcuActivityState(state);
    return state;
}

int PowerMgmtTestApp::start(ClientType clientType) {
    // Get power factory instance
    auto &powerFactory = PowerFactory::getInstance();
    // Get TCU-activity manager object
    tcuActivityMgr_ = powerFactory.getTcuActivityManager(clientType);
    if(tcuActivityMgr_ == nullptr)
    {
        std::cout << APP_NAME << " ERROR - Failed to get manager instance" << std::endl;
        return -1;
    }
    // Check TCU-activity manager service status
    bool isReady = tcuActivityMgr_->isReady();
    if(!isReady) {
        std::cout << APP_NAME << " TCU-activity management services are not ready, waiting for it "
                "to be ready " << std::endl;
        std::future<bool> f = tcuActivityMgr_->onReady();
        isReady = f.get();
    }

    if(isReady) {
        std::cout << APP_NAME << " TCU-activity management services are ready !" << std::endl;
    } else {
        std::cout << APP_NAME << " ERROR - Unable to initialize TCU-activity management services"
                << std::endl;
        return -1;
    }
    getTcuActivityState();
    return 0;
}

void PowerMgmtTestApp::registerForUpdates() {
    // Registering a listener for TCU-activity state updates
    telux::common::Status status = tcuActivityMgr_->registerListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to register for TCU-activity state updates"
                << std::endl;
    } else {
        std::cout << APP_NAME << " Registered Listener for TCU-activity state updates" << std::endl;
    }
    // Registering a listener for TCU-activity management service status updates
    status = tcuActivityMgr_->registerServiceStateListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to register for Service status updates"
                << std::endl;
    }
}

void PowerMgmtTestApp::deregisterForUpdates() {
    // De-registering a listener for TCU-activity state updates
    telux::common::Status status = tcuActivityMgr_->deregisterListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to de-register for TCU-activity state updates"
                << std::endl;
    } else {
        std::cout << APP_NAME << " De-registered listener" << std::endl;
    }
    // De-registering a listener for TCU-activity management service status updates
    status = tcuActivityMgr_->deregisterServiceStateListener(shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " ERROR - Failed to de-register for Service status updates"
                << std::endl;
    }
}

void PowerMgmtTestApp::consoleinit() {
   std::shared_ptr<ConsoleAppCommand> suspendSytemCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Suspend_System", {},
         std::bind(&PowerMgmtTestApp::sendActivityStateCommand, this, TcuActivityState::SUSPEND)));

   std::shared_ptr<ConsoleAppCommand> resumeSytemCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Resume_System", {},
         std::bind(&PowerMgmtTestApp::sendActivityStateCommand, this, TcuActivityState::RESUME)));

   std::shared_ptr<ConsoleAppCommand> shutdownSytemCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Shutdown_System", {},
         std::bind(&PowerMgmtTestApp::sendActivityStateCommand, this, TcuActivityState::SHUTDOWN)));

   std::shared_ptr<ConsoleAppCommand> getTcuStateCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "4", "Get_System_State", {},
         std::bind(&PowerMgmtTestApp::getTcuActivityState, this)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListPowerMenu
      = {suspendSytemCommand, resumeSytemCommand, shutdownSytemCommand, getTcuStateCommand};
   ConsoleApp::addCommands(commandsListPowerMenu);
   ConsoleApp::displayMenu();
}

std::shared_ptr<PowerMgmtTestApp> init(ClientType clientType) {
    std::shared_ptr<PowerMgmtTestApp> powerMgmtTest = std::make_shared<PowerMgmtTestApp>();
    if (!powerMgmtTest) {
        std::cout << "Failed to instantiate PowerMgmtTestApp" << std::endl;
        return nullptr;
    }
    if( 0 != powerMgmtTest->start(clientType)) {
        std::cout << APP_NAME << " Failed to initialize the TCU-activity management service"
            << std::endl;
        return nullptr;
    }
    return powerMgmtTest;
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {

    bool inputCommand = false;
    TcuActivityState state = TcuActivityState::UNKNOWN;
    ClientType clientType = ClientType::SLAVE;

    if(argc <= 1) {
        printHelp();
        return -1;
    }
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1){
        std::cout << APP_NAME << "Adding supplementary groups failed!" << std::endl;
    }

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "-l") {
            listenerEnabled =true;
        } else if (std::string(argv[i]) == "-s") {
            clientType = ClientType::MASTER;
            inputCommand=true;
            state=TcuActivityState::SUSPEND;
        } else if (std::string(argv[i]) == "-r") {
            clientType = ClientType::MASTER;
            inputCommand=true;
            state=TcuActivityState::RESUME;
        } else if (std::string(argv[i]) == "-p") {
            clientType = ClientType::MASTER;
            inputCommand=true;
            state=TcuActivityState::SHUTDOWN;
        } else if (std::string(argv[i]) == "-c") {
            clientType = ClientType::MASTER;
            std::shared_ptr<PowerMgmtTestApp> myPowerMgmtTest = init(clientType);
            myPowerMgmtTest->registerForUpdates();
            listenerEnabled =true;
            myPowerMgmtTest->consoleinit();
            myPowerMgmtTest->mainLoop();
            myPowerMgmtTest->deregisterForUpdates();
            return 0;
        } else {
            printHelp();
            return -1;
        }
    }
    std::shared_ptr<PowerMgmtTestApp> myPowerMgmtTest = init(clientType);
    if(listenerEnabled) {
        myPowerMgmtTest->registerForUpdates();
    }
    signal(SIGINT, signalHandler);
    std::unique_lock<std::mutex> lock(mutex);
    if(inputCommand) {
        myPowerMgmtTest->sendActivityStateCommand(state);
    }
    std::cout << APP_NAME << " Press CTRL+C to exit" << std::endl;
    cv.wait(lock);
    if(listenerEnabled) {
        myPowerMgmtTest->deregisterForUpdates();
    }

    std::cout << "Exiting application..." << std::endl;
    return 0;
}
