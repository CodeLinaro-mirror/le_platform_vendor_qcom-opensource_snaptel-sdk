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
 * @file       Cv2xDaemon.cpp
 *
 * @brief      This is Sample Reference app which uses Telematics SDK API to Start/Stop V2X Mode,
 *             Start/Stop data call, Register and Handle for SSR( Sub-System Restart) and handles
 *             state transition.
 *
 *             It allows one to interactively invoke most of the public APIs in the Telematics SDK.
 *             Works in Foreground and Background Mode.
 *
 */

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <iterator>
#include <sstream>

#include <limits>
#include <type_traits>
#include <net/if.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#ifdef WITH_SYSTEMD
#include <systemd/sd-daemon.h>
#endif

#include "Cv2xDaemon.hpp"
#include "Cv2xLog.hpp"
#include "PowerFactory.hpp"

extern int enableDebug;
extern int enableSyslog;
static bool exiting_ = false;

class SystemStateListener : public telux::power::ITcuActivityListener {

public:
    void onTcuActivityStateUpdate(TcuActivityState tcuState) override {
        Cv2xDaemon & cv2xDaemon = Cv2xDaemon::getInstance();
        LOGD("System state change notification\n");

        std::unique_lock<std::mutex> lock(cv2xDaemon.mutex_);
        cv2xDaemon.setSystemState(tcuState);
        cv2xDaemon.cv_.notify_all();
    }
};

Status Cv2xDaemon::startV2xMode() {

    Status ret = Status::FAILED;
    Cv2xStatus v2xStatus;

    LOGI("Starting V2X mode\n");

    ret = cv2xTelux_->getV2xRadioStatus(v2xStatus);
    if (ret != Status::SUCCESS) {
        LOGE("Failed to get v2x status\n");
        return ret;
    }

    LOGI("Read V2X radio status\n");
    if (v2xStatus.rxStatus != Cv2xStatusType::INACTIVE &&
            v2xStatus.txStatus != Cv2xStatusType::INACTIVE) {
        // Try stopping v2x mode post ssr for EAP
        ret = cv2xTelux_->stopV2xRadio();
        if (ret != Status::SUCCESS) {
            LOGE("Failed to stop v2x mode, applicable post ssr on EAP\n");
        }
    }

    ret = cv2xTelux_->startV2xRadio();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to start v2x mode\n");
        return ret;
    }
    LOGI("V2X mode started\n");

    return Status::SUCCESS;
}

Status Cv2xDaemon::stopV2xMode() {

    Status ret = Status::FAILED;
    Cv2xStatus v2xStatus;

    LOGI("Stopping V2X mode\n");

    ret = cv2xTelux_->getV2xRadioStatus(v2xStatus);
    if (ret != Status::SUCCESS) {
        LOGE("Failed to get v2x status\n");
        return ret;
    }

    if (v2xStatus.rxStatus == Cv2xStatusType::INACTIVE &&
            v2xStatus.txStatus == Cv2xStatusType::INACTIVE) {
        LOGD("V2X radio already stopped\n");
        return Status::SUCCESS;
    }

    ret = cv2xTelux_->stopV2xRadio();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to stop v2x mode\n");
        return ret;
    }

    return Status::SUCCESS;
}

Status Cv2xDaemon::handleSystemStateChange() {
    Status ret = Status::SUCCESS;
    TcuActivityState newState = getSystemState();

    switch (newState) {
        case TcuActivityState::SUSPEND:
            LOGI("System going to suspend, stop CV2X mode\n");
            /* Existing data calls will be auto terminated if v2x mode off */
            stopV2xMode();
            sysStateMgr_->sendActivityStateAck(
                          TcuActivityStateAck::SUSPEND_ACK);
            break;
        case TcuActivityState::RESUME:
            LOGI("System is resuming, start CV2X mode\n");
            startV2xMode();
            break;
        case TcuActivityState::SHUTDOWN:
            LOGI("System shutting down\n");
            sysStateMgr_->sendActivityStateAck(
                             TcuActivityStateAck::SHUTDOWN_ACK);
            break;
        default:
            break;
    }

    setSystemState(TcuActivityState::UNKNOWN);
    return ret;
}

void Cv2xDaemon::setSystemState(TcuActivityState newState) {
    systemState_ = newState;
}

TcuActivityState Cv2xDaemon::getSystemState() {
    return systemState_;
}

Status Cv2xDaemon::enableSysPowerNotification() {
    telux::common::Status regStatus;
    sysStateMgr_ =
        telux::power::PowerFactory::getInstance().getTcuActivityManager();

    if (sysStateMgr_ == nullptr){
        LOGE("Failed to get sysStateMgr_\n");
        return Status::FAILED;
    }
    if (sysStateMgr_->isReady() == false) {
        std::future<bool> future = sysStateMgr_->onReady();
        future.get();
    }

    sysStateListener_ = std::make_shared<SystemStateListener>();
    regStatus = sysStateMgr_->registerListener(sysStateListener_);
    if (regStatus != telux::common::Status::SUCCESS) {
        LOGE("Error sysStateMgr_ register listener\n");
        return Status::FAILED;
    }

    LOGD("enableSysPowerNotification success\n");
    return Status::SUCCESS;
}

Status Cv2xDaemon::runAsDaemon() {

    Status ret = Status::FAILED;

    LOGI("Starting the CV2X Daemon\n");
    // This is a good place to initialize data subsytem, as in daemon mode we are supposed to
    // run the complete cv2x include v2x radio and data calls.
    // Initializing data subsytem in async task as this is a blocking call and can be started
    // independant of cv2x radio.
    // TODO: make sure the future is release in case the daemon needs to exit soon enough.
    auto f = std::async(std::launch::async , [this]()
    {
        Status ret = Status::FAILED;
        bootkpilog("cv2x-daemon: Initializing Data subsytem");
        ret = cv2xTelux_->initDataLibrary();
        if (ret != Status::SUCCESS) {
            LOGE("Failed to initialize data library\n");
            bootkpilog("cv2x-daemon: Failed to initialize data library");
            return Status::FAILED;
        } else {
            // Register for Data Connection, SSR
            ret = cv2xTelux_->registerDataListeners();
            if (ret != Status::SUCCESS) {
                LOGE("Failed to register data listener\n");
                bootkpilog("cv2x-daemon: Failed to register data listener");
            } else{
                bootkpilog("cv2x-daemon: Data subsytem ready");
            }
        }
        return Status::SUCCESS;
    });

    // Register for Radio Status, Data Connection, SSR
    ret = cv2xTelux_->registerListeners();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to register listener\n");
        // We dont have to wait for f.get() for async task as the function will be blocked until
        // the async task is completed, before returing from this function.
        // f.get();
        return ret;
    }

    ret = startV2xMode();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to start v2x mode\n");
        // We dont have to wait for f.get() for async task as the function will be blocked until
        // the async task is completed, before returing from this function.
        // f.get();
        return ret;
    }

    Status retData = f.get();
    if (retData != Status::SUCCESS) {
        LOGE("Failed to intialize data subsystem\n");
        return Status::FAILED;
    }

    // Find Profiles and Start Data calls
    ret = cv2xTelux_->findProfilesAndStartDataCalls();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to find v2x profiles and start data calls\n");
        return ret;
    }

    return Status::SUCCESS;
}

Status Cv2xDaemon::initV2X() {

    Status ret = Status::FAILED;

    ret = cv2xTelux_->initV2xLibrary();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to initialize v2x library\n");
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status Cv2xDaemon::deInit() {

    Status ret = Status::FAILED;

    if (daemonMode_) {
        ret = stopV2xMode();
        if (ret != Status::SUCCESS) {
            LOGE("Failed to stop v2x mode\n");
            return Status::FAILED;
        }

        ret = cv2xTelux_->deinitV2xLibrary();
        if (ret != Status::SUCCESS) {
            LOGE("Failed to de-initialize v2x library\n");
            return Status::FAILED;
        }
    }

    return Status::SUCCESS;
}

void terminationHandler(int signum) {

    LOGE("Got signal %d, tearing down all services\n",signum );

    Cv2xDaemon::getInstance().deInit();

    signal(signum, SIG_DFL);
    raise(signum);
    exiting_ = true;
    Cv2xDaemon::getInstance().cv_.notify_all();

}

void Cv2xDaemon::setupSignalHandler() {

    // TODO Use C++ signal handler instead of C
    struct sigaction sig_action;

    sig_action.sa_handler = terminationHandler;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;

    sigaction(SIGINT, &sig_action, NULL);
    sigaction(SIGHUP, &sig_action, NULL);
    sigaction(SIGTERM, &sig_action, NULL);
}

void Cv2xDaemon::printUsage(std::string appName) {
    std::cout << "Usage: " << appName << " --debug|-d --use-syslog|-s "
        << "--daemon-mode|-D\n"
        << "--debug|-d: Enable debug\n"
        << "--use-syslog|-s: Use syslog\n"
        << "--daemon-mode|-D: Start v2x and run in daemon mode\n";
}

Status Cv2xDaemon::handleArguments(bool &isRunningDaemonMode) {

    Status ret = Status::FAILED;
    isRunningDaemonMode = false;

    if (startV2x_) {
        ret = startV2xMode();
        if (ret != Status::SUCCESS) {
            LOGE("Failed to start v2x mode\n");
            return ret;
        }
    }

    if (stopV2x_) {
        ret = stopV2xMode();
        if (ret != Status::SUCCESS) {
            LOGE("Failed to start v2x mode\n");
            return ret;
        }
    }

    if (stopV2x_ == 0 and startV2x_ == 0) {
        // If no other action specified, start daemon mode
        daemonMode_ = 1;
    }

    if (daemonMode_) {
        ret = runAsDaemon();
        if (ret != Status::SUCCESS) {
            LOGE("Failed to start in daemon mode\n");
            return ret;
        }
        isRunningDaemonMode = true;
    }

    return Status::SUCCESS;
}

Status Cv2xDaemon::parseArguments(int argc, char **argv) {
    int c;

    while (1) {
        static struct option long_options[] = {
            {"debug",           no_argument, 0, 'd'},
            {"use-syslog",      no_argument, 0, 's'},
            {"help",            no_argument, 0, 'h'},
            {"start-v2x-mode",  no_argument, 0, 'S'},
            {"stop-v2x-mode",   no_argument, 0, 'E'},
            {"daemon-mode",     no_argument, 0, 'D'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        c = getopt_long(argc, argv, "dhsSED", long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'd':
                LOGD("Enable debug\n");
                enableDebug = 1;
                break;
            case 's':
                LOGD("Enable syslog\n");
                enableSyslog = 1;
                break;
            case 'S':
                startV2x_ = 1;
                break;
            case 'E':
                stopV2x_ = 1;
                break;
            case 'D':
                LOGD("Starting in daemon mode\n");
                daemonMode_ = 1;
                break;
            case 'h':
                printUsage(argv[0]);
                return Status::INVALIDPARAM;
                break;
        }
    }

    return Status::SUCCESS;
}

Cv2xDaemon::Cv2xDaemon()
: daemonMode_(0), startV2x_(0), stopV2x_(0) {
    cv2xTelux_ = std::make_shared<Cv2xTelux>();
}

Cv2xDaemon::~Cv2xDaemon() {
}

Cv2xDaemon & Cv2xDaemon::getInstance() {
    static Cv2xDaemon instance;
    return instance;
}

int main(int argc, char **argv) {
    Status ret = Status::FAILED;
    auto &cv2xDaemon = Cv2xDaemon::getInstance();

    bootkpilog("cv2x-daemon: Starting CV2X Daemon Service");

    ret = cv2xDaemon.parseArguments(argc, argv);
    if (ret != Status::SUCCESS) {
        return -1;
    }

    cv2xDaemon.setupSignalHandler();
    ret = cv2xDaemon.initV2X();
    if (ret != Status::SUCCESS) {
        return -1;
    }

    bool isRunningDaemonMode = false;
    ret = cv2xDaemon.handleArguments(isRunningDaemonMode);
    if (ret != Status::SUCCESS) {
        cv2xDaemon.deInit();
        return -1;
    }

    if (isRunningDaemonMode) {
#ifdef WITH_SYSTEMD
        sd_notify(0, "READY=1");
#endif

        if (cv2xDaemon.enableSysPowerNotification() != Status::SUCCESS) {
            cv2xDaemon.deInit();
            return -1;
        }

        while (1) {
            std::unique_lock<std::mutex> lock(cv2xDaemon.mutex_);
            cv2xDaemon.cv_.wait(lock);

            if (true == exiting_) {
                break;
            }

            cv2xDaemon.handleSystemStateChange();
        }
    } else {
        cv2xDaemon.deInit();
    }
    return 0;
}
