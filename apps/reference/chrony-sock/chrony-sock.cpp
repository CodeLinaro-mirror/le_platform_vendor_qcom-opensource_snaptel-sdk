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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a reference application that is used to feed GNSS time data
 * obtained from the Location APIs to the Chrony NTP server via the SOCK
 * interface. The application also calls chronyc to update the RTC file
 * periodically.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <glib.h>

#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationFactory.hpp>
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/LocationListener.hpp>

#include "../../common/utils/SignalHandler.hpp"

#define SOCK_NAME "/var/run/chrony.sock"
#define SOCK_MAGIC 0x534f434b
#define RTC_TIMER_SEC (60 * 11)

void chronylog(int level, const char *fmt, ...);

#define LOGI(fmt, args...) \
    chronylog(LOG_NOTICE, "[I][%s:%d] " fmt, __func__, __LINE__, ## args)
#define LOGD(fmt, args...) \
    chronylog(LOG_DEBUG, "[D][%s:%d] " fmt, __func__, __LINE__, ## args)
#define LOGE(fmt, args...) \
    chronylog(LOG_ERR, "[E][%s:%d] " fmt, __func__, __LINE__, ## args)

using namespace telux::loc;
using namespace telux::common;

// Chrony SOCK sample
struct TimeSample {
    struct timeval tv;
    double offset;
    int pulse;
    int leap;
    int _pad;
    int magic;
};

static int chronyfd;
static bool gTimeCapability = false;
static bool gExit = false;

bool enableDebug = false;
bool enableSyslog = false;
bool enableWriteRtc = false;

// Used to get the Telux async result
std::mutex mtx;
std::condition_variable cv;
bool cv_done = false;
ErrorCode ec;

int system_call(const char *command)
{
    FILE *stream = NULL;
    int result = -1;
    stream = popen(command, "w");
    if (stream == NULL) {
        LOGE("system call failed popen failed\n");
    } else {
        result = pclose(stream);
        if (WIFEXITED(result)) {
            result = WEXITSTATUS(result);
        }
        LOGD("popen closed with %d status", result);
    }
    return result;
}

void chronylog(int level, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    if (level != LOG_DEBUG || enableDebug) {
        if (!enableSyslog) {
            vprintf(fmt, args);
        } else {
            vsyslog(level, fmt, args);
        }
    }
    va_end(args);
}

void printUsage(char *app_name) {
    printf("Usage: %s -d -s -r\n", app_name);
    printf("\t-d: Enable debug logs\n");
    printf("\t-s: Log to syslog instead of stdout\n");
    printf("\t-r: Enable updating the rtc file\n");
}

static void writeRtcFile(int sig, siginfo_t *si, void *uc) {
    int rc;

    LOGI("Updating rtc file using: chronyc writertc\n");
    rc = system_call("chronyc writertc");
    if (rc) {
        LOGE("Error sending the writertc command\n");
    }
}

int installRtcTimer() {
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its = {0};
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = writeRtcFile;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        LOGE("Error setting rtc signal\n");
        goto error;
    }

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;

    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        LOGE("Error creating the rtc timer\n");
        goto error;
    }

    its.it_value.tv_sec = RTC_TIMER_SEC;
    its.it_interval.tv_sec = its.it_value.tv_sec;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        LOGE("Error starting the rtc timer\n");
        goto error;
    }

    LOGI("Set timer to update rtc file every 11 minutes\n");

    return 0;

error:
    return -EINVAL;
}

class MyLocationListener : public ILocationListener {
public:
    void onBasicLocationUpdate(
        const std::shared_ptr<ILocationInfoBase> &locationInfo) {
        uint64_t utc = locationInfo->getTimeStamp();
        static bool firstFix = true;

        if (firstFix) {
            LOGI("Got first GNSS report\n");
            firstFix = false;
        }

        if (utc % 1000 == 0) {
           LOGD("GNSS report with UTC = %" PRIu64 "\n", utc);
        } else {
           LOGD("GNSS report ignored with UTC = %" PRIu64 "\n", utc);
           return;
        }

        struct TimeSample sample = { 0 };
        struct timeval gps_time, offset_time;

        sample.magic = SOCK_MAGIC;
        gettimeofday(&sample.tv, NULL);
        gps_time.tv_sec = (time_t)(utc / 1000);
        gps_time.tv_usec = (suseconds_t)(utc % 1000);
        timersub(&gps_time, &sample.tv, &offset_time);
        sample.offset = (double)offset_time.tv_sec +
                        ((double)offset_time.tv_usec / 1000000);

        ssize_t bytesSent = send(chronyfd, &sample, sizeof(sample), 0);
        // Checking if the socket was closed
        if (-1 == bytesSent) {
            LOGE("Failed to send sample to chrony, error = %d\n", errno);
            exit(errno);
        } else if (sizeof(sample) != bytesSent) {
            LOGE("Failed to send sample to chrony, bytesSent = %d\n",
                 bytesSent);
            exit(-EIO);
        }
    }

    void onCapabilitiesInfo(const telux::loc::LocCapability capabilityMask) override {
        if (capabilityMask & telux::loc::TIME_BASED_TRACKING) {
            std::unique_lock<std::mutex> lck(mtx);
            LOGI("Time based tracking session is supported\n");
            if (!gTimeCapability) {
                gTimeCapability = true;
                cv.notify_all();
            }
        }
    }
};

int setupSocket(int *fd) {
    struct sockaddr_un name;

    *fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (*fd < 0) {
        LOGE("Failed to create chrony socket ret=%d\n", errno);
        return errno;
    }

    name.sun_family = AF_UNIX;
    g_strlcpy(name.sun_path, SOCK_NAME, sizeof(name.sun_path));

    if (connect(*fd, (struct sockaddr *)&name, sizeof(name))) {
        LOGE("Failed to connect chrony socket ret=%d\n", errno);
        return errno;
    }

    LOGI("Connected to the chronyd socket\n");

    return 0;
}

// Response to async Telux calls
void responseCallback(ErrorCode error) {
    ec = error;
    cv_done = true;
    cv.notify_all();
}

void parseArguments(int& argc, char **argv) {
    int opt;

    while ((opt = getopt(argc, argv, "dsrh")) != -1) {
        switch (opt) {
        case 'd':
            enableDebug = true;
            break;
        case 's':
            enableSyslog = true;
            break;
        case 'r':
            enableWriteRtc = true;
            break;
        case 'h':
        default:
            printUsage(argv[0]);
            exit(-EINVAL);
        }
    }
}

int main(int argc, char *argv[]) {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [](int sig) {
        std::unique_lock<std::mutex> lck(mtx);
        gExit = true;
        cv.notify_all();
    };

    SignalHandler::registerSignalHandler(sigset, cb);

    int ret = 0;

    // Exits if invalid arguments passed
    parseArguments(argc, argv);

    // Open chrony UNIX socket
    if ((ret = setupSocket(&chronyfd))) {
        return ret;
    }

    if (enableWriteRtc) {
        installRtcTimer();
    }

    // Initialize the TelSDK Location library
    std::shared_ptr<ILocationListener> myLocationListener
        = std::make_shared<MyLocationListener>();
    std::shared_ptr<ILocationManager> locationManager;

    auto &locationFactory = LocationFactory::getInstance();

    bool locMgrStatusUpdated = false;
    auto locMgrStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&locMgrStatusUpdated,
                     &locMgrStatus](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        locMgrStatusUpdated = true;
        locMgrStatus = status;
        cv.notify_all();
    };

    locationManager = locationFactory.getLocationManager(statusCb);
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&locMgrStatusUpdated] { return locMgrStatusUpdated || gExit; });
    }

    if (locMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGI("Location subsystem is ready\n");
    } else {
        LOGE("Unable to initialize the Location subsystem\n");
        return -EINVAL;
    }

    auto status = locationManager->registerListenerEx(myLocationListener);
    if (status != Status::SUCCESS) {
        LOGE("Failed to register location listener\n");
        return -EINVAL;
    }

    auto capabilities = locationManager->getCapabilities();
    if (not (capabilities & telux::loc::TIME_BASED_TRACKING)) {
        LOGI("Wait for time based tracking capability\n");
        std::unique_lock<std::mutex> lck(mtx);
        while (!gTimeCapability && !gExit) {
            cv.wait(lck);
        }

        if (gExit) {
            return 0;
        }
    } else {
        std::unique_lock<std::mutex> lck(mtx);
        LOGI("Time based tracking capability is supported\n");
        gTimeCapability = true;
    }

    status = locationManager->startBasicReports(0, 100, responseCallback);
    if (status != Status::SUCCESS) {
        LOGE("Failed to start basic location reports\n");
        return -EINVAL;
    }

    {
        // Wait for responseCallback to be called
        std::unique_lock<std::mutex> lck(mtx);
        while (!cv_done && !gExit) {
            cv.wait(lck);
        }
    }

    if (gExit) {
        return 0;
    }

    if (ec != ErrorCode::SUCCESS) {
        LOGE("Failed to start basic location reports\n");
        return -EINVAL;
    }

    LOGI("Started providing fixes to chronyd\n");

    {
        std::unique_lock<std::mutex> lck(mtx);
        while (!gExit) {
            cv.wait(lck);
        }
    }

    locationManager->deRegisterListenerEx(myLocationListener);

    return 0;
}
