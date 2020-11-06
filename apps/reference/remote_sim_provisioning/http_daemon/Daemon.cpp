/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
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

#include <iostream>
#include <csignal>
#include <future>

extern "C" {
#include <getopt.h>
}

#include <telux/common/CommonDefines.hpp>

#include "Daemon.hpp"
#include "Log.hpp"

HttpDaemon &HttpDaemon::getInstance() {
    static HttpDaemon instance;
    return instance;
}

telux::common::Status HttpDaemon::init() {
    LOGI(" init \n");
    std::promise<telux::common::ServiceStatus> prom;
    //  Get the PhoneFactory instances.
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    httpMgr_ = phoneFactory.getHttpTransactionManager([&](telux::common::ServiceStatus status) {
        if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            prom.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        } else {
            prom.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
        }
    });
    if (!httpMgr_) {
        LOGE(" HttpTransactionManager is null \n");
        return telux::common::Status::FAILED;
    }

    telux::common::ServiceStatus httpMgrStatus = httpMgr_->getServiceStatus();
    if (httpMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("Http Manager subsystem is not ready, Please wait \n");
        httpMgrStatus = prom.get_future().get();
    }
    if (httpMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGD("Http Manager subsystem is ready \n");
        httpListener_ = std::make_shared<HttpTransactionListener>(httpMgr_);
        telux::common::Status status = httpMgr_->registerListener(httpListener_);
        if (status != telux::common::Status::SUCCESS) {
            LOGE(" ERROR - Failed to register listener \n");
            return telux::common::Status::FAILED;
        }
    } else {
        LOGE(" ERROR - Unable to initialize Http Manager subsystem \n");
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

int HttpDaemon::startDaemon(int argc, char **argv) {
    LOGI(" startDaemon \n");
    if (parseArguments(argc, argv) != telux::common::Status::SUCCESS) {
        return EXIT_FAILURE;
    }

    std::signal(SIGHUP, signalHandler);
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    if (init() != telux::common::Status::SUCCESS) {
        return EXIT_FAILURE;
    }

    {
        // block current thread, till we get signal
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [this]{ return exiting_; });

    }
    return EXIT_SUCCESS;
}

void HttpDaemon::stopDaemon() {
    LOGI(" stopDaemon \n");
    std::lock_guard<std::mutex> lock(mtx_);
    exiting_ = true;
    cv_.notify_all();
}

void HttpDaemon::signalHandler(int signum) {
    LOGI("Received signal %d, terminating program.\n", signum);
    HttpDaemon::getInstance().stopDaemon();

    std::signal(signum, SIG_DFL);
    if (std::raise(signum) != 0) {
        LOGE("raise(): error \n");
    }
}

void HttpDaemon::printUsage(char **argv) {
    std::cout << std::endl;
    std::cout << "Usage: " << argv[0] << " [options] \n";
    std::cout << "Options: \n";
    std::cout << "\t -h --help        Print helpful information\n";
    std::cout << "\t -d --debug       Enable debug logs\n";
    std::cout << "\t -s --syslog      Use syslog\n";
    std::cout << "Example: \n";
    std::cout << "   ./rsp_httpd \n";
    std::cout << std::endl;
}

telux::common::Status HttpDaemon::parseArguments(int argc, char **argv) {
    int c;
    struct option long_options[]
            = {{"debug", no_argument, 0, 'd'}, {"syslog", no_argument, 0, 's'},
               {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};
    while (1) {
        int option_index = 0;
        c = getopt_long(argc, argv, "dsh", long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'd':
                LOGD("Enable debug\n");
                ENABLE_DEBUG = 1;
                break;
            case 's':
                LOGD("Enable sys log\n");
                ENABLE_SYSLOG = 1;
                break;
            case 'h':
            default:
                printUsage(argv);
                return telux::common::Status::INVALIDPARAM;
                break;
            }
    }
    return telux::common::Status::SUCCESS;
}
