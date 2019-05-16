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

extern int enableDebug;
extern int enableSyslog;

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
        LOGD("V2X radio already started\n");
        return Status::SUCCESS;
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

    LOGI("Stopped V2X radio\n");
    return Status::SUCCESS;
}

Status Cv2xDaemon::runAsDaemon() {

    Status ret = Status::FAILED;

    LOGI("Starting the CV2X Daemon\n");

    // Register for Radio Status, Data Connection, SSR
    ret = cv2xTelux_->registerListeners();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to register listener\n");
        return ret;
    }

    ret = startV2xMode();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to start v2x mode\n");
        return ret;
    }

    // Create Profile and Start Data call
    ret = cv2xTelux_->createProfileAndStartDataCalls();
    if (ret != Status::SUCCESS) {
        LOGE("Failed to create v2x data profile\n");
        return ret;
    }

    return Status::SUCCESS;
}

Status Cv2xDaemon::init() {

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

    return Status::SUCCESS;
}

void terminationHandler(int signum) {

    LOGE("Got signal %d, tearing down all services\n",signum );

    Cv2xDaemon::getInstance().deInit();

    signal(signum, SIG_DFL);
    raise(signum);
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
        << "--start-v2x-mode|-S --stop-v2x-mode|-E --daemon-mode|-D\n"
        << "--debug|-d: Enable debug\n"
        << "--use-syslog|-s: Use syslog\n"
        << "--start-v2x-mode|-S: Start v2x mode\n"
        << "--stop-v2x-mode|-E: Stop v2x mode\n"
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

    ret = cv2xDaemon.parseArguments(argc, argv);
    if (ret != Status::SUCCESS) {
        return -1;
    }

    cv2xDaemon.setupSignalHandler();
    ret = cv2xDaemon.init();
    if (ret != Status::SUCCESS) {
        return -1;
    }

    bool isRunningDaemonMode = false;
    ret = cv2xDaemon.handleArguments(isRunningDaemonMode);
    if (ret != Status::SUCCESS) {
        cv2xDaemon.deInit();
        return -1;
    }

    // The App is running in daemon mode, We wait on Signal to terminate program
    if (isRunningDaemonMode) {
#ifdef WITH_SYSTEMD
        sd_notify(0, "READY=1");
#endif
        std::unique_lock<std::mutex> lock(cv2xDaemon.mutex_);
        LOGD("Press CTRL+C to exit\n");
        cv2xDaemon.cv_.wait(lock);
    } else {
        cv2xDaemon.deInit();
    }

    return 0;
}
