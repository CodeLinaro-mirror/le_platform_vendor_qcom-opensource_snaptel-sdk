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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a reference application that is used to feed GNSS or SLSS time data,
 * obtained from the TelSDK platform APIs, to the Chrony NTP server via the SOCK
 * interface. If GNSS and SLSS time are unavailable when the app starts, the
 * application also calls TelSDK telephony APIs to retrieve the network time and
 * adjust the system time if it's avaialble. Once GNSS or SLSS time becomes available,
 * network time is no longer used.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <glib.h>
#include <linux/rtc.h>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/TimeListener.hpp>
#ifdef FEATURE_NETWORK_TIME_ENABLED
#include <telux/power/PowerFactory.hpp>
#include <telux/tel/PhoneFactory.hpp>
#endif

#include "../../common/utils/SignalHandler.hpp"
#include "../../common/ConfigParser.hpp"

#include "ChronySock.hpp"

using telux::common::Status;
using telux::common::ErrorCode;
using telux::platform::PlatformFactory;
using telux::platform::ITimeManager;
using telux::platform::ITimeListener;
using telux::platform::SupportedTimeType;
using telux::platform::TimeTypeMask;

#ifdef FEATURE_NETWORK_TIME_ENABLED
using telux::power::TcuActivityState;
using telux::power::TcuActivityStateAck;
#endif

// keys in telux config
#define CONFIG_FILE "telux_chrony-sock.conf"
#define HYSTERESIS_THRESHOLD "time.hysteresis.threshold"
#define ENABLE_CV2X_TIME "enable.cv2x.time"
#define ENABLE_NETWORK_TIME "enable.network.time"
#define ENABLE_DELTA_UPDATE "enable.delta.update"
#define NETWORK_TIME_SLOT "network.time.slot"

#define SOCK_NAME "/var/run/chrony.sock"
#define SOCK_MAGIC 0x534f434b
#define RTC_TIMER_SEC (60 * 11)

#define INTERNAL_RTC_DEV_NAME "/dev/rtc0"
#define DELTA_TIME_FILE "/etc/deltatime/dlt_msec"

MyListener::MyListener(std::weak_ptr<ChronySock> instance) {
    chronySock_ = instance;
}

void MyListener::onGnssUtcTimeUpdate(const uint64_t utc) {
    auto sp = chronySock_.lock();
    if (sp) {
        sp->handleTimeUpdate(MyTimeSrc::GNSS, utc);
    }
}

void MyListener::onCv2xUtcTimeUpdate(const uint64_t utc) {
    auto sp = chronySock_.lock();
    if (sp) {
        sp->handleTimeUpdate(MyTimeSrc::CV2X, utc);
    }
}

#ifdef FEATURE_NETWORK_TIME_ENABLED
// handle network time
void MyListener::onNetworkTimeChanged(telux::tel::NetworkTimeInfo info) {
    auto sp = chronySock_.lock();
    if (sp) {
        uint64_t utc;
        if (0 == sp->convertNetworkTime(info, utc)) {
            sp->handleTimeUpdate(MyTimeSrc::NETWORK, utc);
        }
    }
}

// handle TCU state
void MyListener::onTcuActivityStateUpdate(TcuActivityState tcuState) {
    auto sp = chronySock_.lock();
    if (sp) {
        sp->handleTcuStateUpdate(tcuState);
    }
}
#endif

void ChronySock::printUsage(char *app_name) {
    printf("Usage: %s -d -s -r\n", app_name);
    printf("\t-d: Enable debug logs\n");
    printf("\t-s: Log to syslog instead of stdout\n");
    printf("\t-r: Enable updating the rtc file\n");
}

int ChronySock::parseArguments(int& argc, char **argv) {
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
            enableWriteRtc_ = true;
            break;
        case 'h':
        default:
            printUsage(argv[0]);
            return -1;
        }
    }
    return 0;
}

void ChronySock::getTeluxConfig() {
    auto config = std::make_shared<ConfigParser>(std::string(CONFIG_FILE));
    auto threshold = config->getValue(std::string(HYSTERESIS_THRESHOLD));
    if (not threshold.empty()) {
        offsetThreshold_ = std::stoull(threshold);
    }

    auto enable = config->getValue(std::string(ENABLE_CV2X_TIME));
    enableCv2xTime_ = (enable == "TRUE") ? true : false;

    enable = config->getValue(std::string(ENABLE_NETWORK_TIME));
    enableNwTime_ = (enable == "TRUE") ? true : false;

    enable = config->getValue(std::string(ENABLE_DELTA_UPDATE));
    enableDeltaUpdate_ = (enable == "TRUE") ? true : false;

    if (enableNwTime_) {
        auto slotId = atoi(config->getValue(std::string(NETWORK_TIME_SLOT)).c_str());
        slotId_ = (slotId >= DEFAULT_SLOT_ID and slotId <= MAX_SLOT_ID) ?
                   slotId : DEFAULT_SLOT_ID;
    }
    LOGI("threshold=%d, enableCv2xTime=%d, enableNwTime=%d, enableDeltaUpdate=%d, slotId=%d\n",
         offsetThreshold_, enableCv2xTime_, enableNwTime_, enableDeltaUpdate_, slotId_);
}

int ChronySock::init() {
    // Open chrony UNIX socket
    if (0 != setupSocket(chronyfd_)) {
        return -1;
    }

    // get configuration
    getTeluxConfig();

    if (enableWriteRtc_) {
        installRtcTimer();
    }

    threads_ = std::make_shared<AsyncThread>();

    myListener_ = std::make_shared<MyListener>(shared_from_this());

    return 0;
}

void ChronySock::prepareForDeinit() {
    LOGD("Prepare for deinit\n");
    {
        std::lock_guard<std::mutex> lock(mtx_);
        exit_ = true;
        cv_.notify_all();
    }
}

void ChronySock::deinit() {
    LOGD("Start deinit\n");

    // close unix socket
    if (chronyfd_ >= 0) {
        close(chronyfd_);
        chronyfd_ = -1;
    }

    // wait for threads to end
    if (threads_) {
        threads_->joinAllThreads();
    }

    // deregister from GNSS/CV2X time
    deregisterPlatformTime();

#ifdef FEATURE_NETWORK_TIME_ENABLED
    // deregister from power notification and NW time
    deregisterPwrNotification();
    deregisterNetworkTime();
#endif
}

int ChronySock::setupSocket(int &fd) {
    struct sockaddr_un name;

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) {
        LOGE("Failed to create chrony socket ret=%d\n", errno);
        return errno;
    }

    name.sun_family = AF_UNIX;
    g_strlcpy(name.sun_path, SOCK_NAME, sizeof(name.sun_path));

    if (connect(fd, (struct sockaddr *)&name, sizeof(name))) {
        LOGE("Failed to connect chrony socket ret=%d\n", errno);
        close(fd);
        fd = -1;
        return -1;
    }

    LOGI("Connected to the chronyd socket\n");
    return 0;
}

int ChronySock::systemCall(const char *command) {
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

void ChronySock::writeRtcFile(int sig, siginfo_t *si, void *uc) {
    int rc;

    LOGI("Updating rtc file using: chronyc writertc\n");
    rc = systemCall("chronyc writertc");
    if (rc) {
        LOGE("Error sending the writertc command\n");
    }
}

int ChronySock::installRtcTimer() {
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

// This API is called in the main thread when app starts
int ChronySock::registerPlatformTime() {
    LOGD("Registering platform time\n");

    // Initialize the TelSDK utc info manager
    auto &platformFactory = PlatformFactory::getInstance();
    bool statusUpdated = false;
    auto servicStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx_);
        statusUpdated = true;
        servicStatus = status;
        cv_.notify_all();
    };

    myTimeMgr_ = platformFactory.getTimeManager(statusCb);
    if (myTimeMgr_) {
        // wait for utc manager to be ready
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [&statusUpdated] { return statusUpdated; });
    }

    if (servicStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGI("Time manager is ready\n");
    } else {
        LOGE("Unable to initialize time manager\n");
        return -1;
    }

    TimeTypeMask mask;
    mask.set(SupportedTimeType::GNSS_UTC_TIME);
    if (enableCv2xTime_) {
        mask.set(SupportedTimeType::CV2X_UTC_TIME);
    }
    auto status = myTimeMgr_->registerListener(myListener_, mask);
    if (status != Status::SUCCESS) {
        LOGE("Failed to register utc listener\n");
        myTimeMgr_ = nullptr;
        return -1;
    }

    LOGI("Started providing fixes to chronyd\n");
    return 0;
}

// This API is called in the main thread when the app exits
void ChronySock::deregisterPlatformTime() {
    if (myTimeMgr_) {
        LOGD("Deregistering platform time\n");
        TimeTypeMask mask;
        mask.set(SupportedTimeType::GNSS_UTC_TIME);
        if (enableCv2xTime_) {
            mask.set(SupportedTimeType::CV2X_UTC_TIME);
        }
        myTimeMgr_->deregisterListener(myListener_, mask);
        myTimeMgr_ = nullptr;
    }
}

void ChronySock::handleTimeUpdate(MyTimeSrc src, uint64_t utc) {
    // ignore time update during exit
    if (exit_) {
        return;
    }

    bool utcValid = (utc == 0) ? false : true;
    bool setSysTime = false;
    bool deregNwTime = false;
    switch (src) {
        case MyTimeSrc::GNSS:
            {
                std::lock_guard<std::mutex> lock(timeMtx_);
                if (gnssTimeValid_ != utcValid) {
                    LOGI("GNSS UTC valid:%d\n", utcValid);
                    gnssTimeValid_ = utcValid;
                }
                if (!utcValid) {
                    return;
                }

                // use CV2X time if it's avaialbe, it's more precise than propagated GNSS time
                if (cv2xTimeValid_) {
                    LOGD("GNSS report ignored with UTC = %" PRIu64 " due to CV2X UTC is valid\n",
                        utc);
                    return;
                }

                // update system time if it's the first valid GNSS/CV2X time
                if (firstSample_) {
                    firstSample_ = false;
                    setSysTime = true;
                }
            }

            // forward time to chronyd if it's the first sample or the second boundary
            if (utc % 1000 == 0) {
                sendUtcToChronyd(utc);
                LOGD("GNSS report with UTC = %" PRIu64 "\n", utc);
            } else {
                LOGD("GNSS report ignored with UTC = %" PRIu64 "\n", utc);
            }

#ifdef FEATURE_NETWORK_TIME_ENABLED
            // deregister from NW time if it's registered
            deregNwTime = nwTimeEnabled_;
#endif
            break;
        case MyTimeSrc::CV2X:
            {
                std::lock_guard<std::mutex> lock(timeMtx_);

                if (cv2xTimeValid_ != utcValid) {
                    LOGI("CV2X UTC valid:%d\n", utcValid);
                    cv2xTimeValid_ = utcValid;
                }
                if (!utcValid) {
                    return;
                }

                // update system time if it's the first valid GNSS/CV2X time
                if (firstSample_) {
                    firstSample_ = false;
                    setSysTime = true;
                }
            }

            // forward time to chronyd
            sendUtcToChronyd(utc);
            LOGD("CV2X report with UTC = %" PRIu64 "\n", utc);

#ifdef FEATURE_NETWORK_TIME_ENABLED
            // deregister from NW time if it's registered
            deregNwTime = nwTimeEnabled_;
#endif
            break;
        case MyTimeSrc::NETWORK:
            {
                std::lock_guard<std::mutex> lock(timeMtx_);
                // ignore NW time if GNSS/CV2X time is available
                if (!utcValid || gnssTimeValid_ || cv2xTimeValid_) {
                    LOGD("NW time ignored with UTC = %" PRIu64 "\n", utc);
                    return;
                }
            }
            // update system time according to each received NW time (rarely happen)
            setSysTime = true;
            break;
        default:
            return;
    }

    // process set systemt time and deregister NW time in async thread
    if (!exit_ && (setSysTime || deregNwTime)) {
        auto f = std::async(std::launch::async, [this, utc, setSysTime, deregNwTime] {
            if (setSysTime) {
                setSystemTime(utc);
            }

#ifdef FEATURE_NETWORK_TIME_ENABLED
            if (deregNwTime) {
                deregisterPwrNotification();
                deregisterNetworkTime();
            }
#endif
        }).share();

        if (threads_) {
            threads_->addToQueue(f);
        }

    }
}

// set sys time according to the time sample if the time diff exceeds the threshold
void ChronySock::setSystemTime(uint64_t utc) {
    struct timeval curTime, newTime;
    newTime.tv_sec = (time_t)(utc / 1000);
    newTime.tv_usec = (suseconds_t)((utc % 1000) * 1000);
    gettimeofday(&curTime, NULL);
    uint64_t curUtc = static_cast<uint64_t>(curTime.tv_sec)*1000
        + static_cast<uint64_t>(curTime.tv_usec)/1000;
    if (utc > curUtc + offsetThreshold_ or curUtc > utc + offsetThreshold_) {
        if (settimeofday(&newTime, NULL) != 0) {
            LOGE("Failed to set sys time, errno:%d\n", errno);
        } else {
            LOGI("System time updated from %lld to %lld\n", curUtc, utc);
        }
    } else {
        LOGD("Ignore utc:%lld, cur utc:%lld\n", utc, curUtc);
    }
    // update delta time if the function is enabled
    if (enableDeltaUpdate_) {
        updateDeltaTime(utc);
    }
}

void ChronySock::sendUtcToChronyd(uint64_t utc) {
    struct TimeSample sample = { 0 };
    struct timeval gps_time, offset_time;

    sample.magic = SOCK_MAGIC;
    gettimeofday(&sample.tv, NULL);
    gps_time.tv_sec = (time_t)(utc / 1000);
    gps_time.tv_usec = (suseconds_t)((utc % 1000) * 1000);
    timersub(&gps_time, &sample.tv, &offset_time);
    sample.offset = (double)offset_time.tv_sec +
                    ((double)offset_time.tv_usec / 1000000);

    ssize_t bytesSent = send(chronyfd_, &sample, sizeof(sample), 0);
    // Checking if the socket was closed
    if (-1 == bytesSent) {
        LOGE("Failed to send sample to chrony, error = %d\n", errno);
    } else if (sizeof(sample) != bytesSent) {
        LOGE("Failed to send sample to chrony, bytesSent = %d\n",
             bytesSent);
    }
}

int ChronySock::getInternalRtcTime(uint64_t& msec) {
    struct tm rtcTime = {0};

    // read internal RTC
    int fd = open(INTERNAL_RTC_DEV_NAME, O_RDONLY);
    if (fd == -1) {
        LOGE("Open file:%s failed:%d\n", INTERNAL_RTC_DEV_NAME, errno);
        return -1;
    }

    if (ioctl(fd, RTC_RD_TIME, &rtcTime) < 0) {
        LOGE("Read rtc time failed:%d\n", errno);
        close(fd);
        return -1;
    }

    close(fd);

    // convert the time to milliseconds
    msec = timegm(&rtcTime) * 1000;

    LOGD("Get RTC Time:%04d %02d %02d %02d:%02d:%02d, msec:%lld\n",
         rtcTime.tm_year + 1900, rtcTime.tm_mon + 1, rtcTime.tm_mday,
         rtcTime.tm_hour, rtcTime.tm_min, rtcTime.tm_sec, msec);

    return 0;
}

int ChronySock::readDeltaTimeFromFile(int64_t& deltaMsecs) {
    int fd = open(DELTA_TIME_FILE, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        LOGE("Open %s failed:%d\n", DELTA_TIME_FILE, errno);
        return -1;
    }

    if (read(fd, &deltaMsecs, sizeof(uint64_t)) < 0) {
        close(fd);
        LOGE("Read delta from %s failed:%d\n", DELTA_TIME_FILE, errno);
        return -1;
    }
    close(fd);
    return 0;
}

int ChronySock::writeDeltaTimeToFile(int64_t deltaMsecs) {
    int fd = open(DELTA_TIME_FILE, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        LOGE("Open %s failed:%d\n", DELTA_TIME_FILE, errno);
        return -1;
    }

    if (write(fd, &deltaMsecs, sizeof(uint64_t)) < 0) {
        close(fd);
        LOGE("Write delta to %s failed:%d\n", DELTA_TIME_FILE, errno);
        return -1;
    }

    close(fd);
    return 0;
}

void ChronySock::updateDeltaTime(uint64_t utc) {
    int64_t deltaMsecs = 0;
    int64_t newDeltaMsecs = 0;

    // get internal RTC time
    uint64_t rtcMsecs;
    if (getInternalRtcTime(rtcMsecs) < 0) {
        return;
    }

    // calculate new delat - the offset btw RTC time and the update time
    newDeltaMsecs = utc - rtcMsecs;

    // get stored delta
    if (readDeltaTimeFromFile(deltaMsecs) < 0) {
        return;
    }

    // update delta time if the offset exceeds 1 sec (the precision of internal RTC)
    if (newDeltaMsecs > deltaMsecs + 1000 or deltaMsecs > newDeltaMsecs + 1000) {
        if (0 == writeDeltaTimeToFile(newDeltaMsecs)) {
            LOGI("Updated delta from %lld to %lld\n", deltaMsecs, newDeltaMsecs);
        }
    } else {
        LOGD("Not update delta, old:%lld, new:%lld\n", deltaMsecs, newDeltaMsecs);
    }
}

#ifdef FEATURE_NETWORK_TIME_ENABLED
std::shared_ptr<telux::power::ITcuActivityManager> ChronySock::getTcuMgr() {
    std::lock_guard<std::mutex> lock(pwrMtx_);
    return tcuStateMgr_;
}

// This API is called before register NW time
int ChronySock::registerPwrNotification() {
    std::lock_guard<std::mutex> lock(pwrMtx_);

    if (tcuStateMgr_) {
        LOGD("TCU state manager already created\n");
        return 0;
    }
    LOGD("Registering power notification\n");

    telux::common::ProcType procType = telux::common::ProcType::LOCAL_PROC;
#if defined(TELUX_FOR_EXTERNAL_AP) && !defined(TELUX_QTI_EXTERNAL_AP)
    procType = telux::common::ProcType::REMOTE_PROC;
#endif

    bool tcuStatusUpdated = false;
    auto tcuStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&] (telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx_);
        tcuStatusUpdated = true;
        tcuStatus = status;
        cv_.notify_all();
    };

    auto tmpMgr = telux::power::PowerFactory::getInstance().getTcuActivityManager(
        telux::power::ClientType::SLAVE, procType, statusCb);
    if (tmpMgr == nullptr) {
        LOGE("Failed to get TCU state manager\n");
        return -1;
    }

    {
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [&] { return (tcuStatusUpdated || exit_); });
    }

    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != tcuStatus) {
        LOGE("TcuActivityManager initialization failed\n");
        return -1;
    }

    tcuStateMgr_ = tmpMgr;
    // register for TCU state updates
    if (telux::common::Status::SUCCESS != tcuStateMgr_->registerListener(myListener_)) {
        LOGE("Register for TCU updates failed\n");
        tcuStateMgr_ = nullptr;
        return -1;
    }

    // get initial TCU state
    tcuState_ = tcuStateMgr_->getActivityState();
    LOGD("Initial tcu state:%d\n", static_cast<int>(tcuState_.load()));

    return 0;
}

// This API is called when:
// 1. register for NW time failed
// 2. deregister NW time due to valid GNSS/CV2X time is received
// 3. app exit
void ChronySock::deregisterPwrNotification() {
    std::lock_guard<std::mutex> lock(pwrMtx_);
    if (!tcuStateMgr_) {
        return;
    }

    LOGD("Deregistering power notification\n");

    // deregister listener
    if (myListener_) {
        tcuStateMgr_->deregisterListener(myListener_);
    }

    // discard TcuActivityManager
    tcuStateMgr_ = nullptr;
}

void ChronySock::handleTcuStateUpdate(TcuActivityState tcuState) {
    // update tcu state
    tcuState_ = tcuState;

    // process TCU state in async thread
    auto f = std::async(std::launch::async, [this, tcuState] {
        switch (tcuState) {
            case TcuActivityState::SUSPEND:
                {
                    LOGI("TCU SUSPEND\n");
                    deregisterNetworkTime();
                    auto tcuMgr = getTcuMgr();
                    if (tcuMgr && tcuMgr->sendActivityStateAck(TcuActivityStateAck::SUSPEND_ACK)
                        != Status::SUCCESS) {
                        LOGE("Failed to send suspend ack\n");
                    }
                }
                break;
            case TcuActivityState::RESUME:
                LOGI("TCU RESUME\n");
                if (registerNetworkTime()) {
                    // deregister from power notification if fail to register for NW time
                    deregisterPwrNotification();
                }
                break;
            case TcuActivityState::SHUTDOWN:
                {
                    LOGI("TCU SHUTDOWN\n");
                    deregisterNetworkTime();
                    auto tcuMgr = getTcuMgr();
                    if (tcuMgr && tcuMgr->sendActivityStateAck(TcuActivityStateAck::SHUTDOWN_ACK)
                        != Status::SUCCESS) {
                        LOGE("Failed to send shutdown ack\n");
                    }
                }
                break;
            case TcuActivityState::UNKNOWN:
            default:
                break;
        }
    }).share();
    if (threads_) {
        threads_->addToQueue(f);
    }
}

// convert NW time info to utc in milliseconds
int ChronySock::convertNetworkTime(telux::tel::NetworkTimeInfo info, uint64_t& utc) {
    struct tm time;
    time.tm_year = info.year - 1900; // number of year since 1900
    time.tm_mon = info.month - 1; // month in a zero-based index
    time.tm_mday = info.day;
    time.tm_hour = info.hour;
    time.tm_min = info.minute;
    time.tm_sec = info.second;

    time_t secs = timegm(&time);
    if (secs < 0) {
        LOGE("Convert network time:%s failed\n", info.nitzTime.c_str());
        return -1;
    }

    // convert to msecs
    utc = secs * 1000;
    LOGD("Convert network time:%s to msecs:%llu, %llu\n", info.nitzTime.c_str(), utc);

    return 0;
}

// handle getting NW time response
void ChronySock::onNetworkTimeResponse(telux::tel::NetworkTimeInfo info,
    telux::common::ErrorCode error) {
    LOGD("Receive network time response:%d\n", static_cast<int>(error));
    uint64_t utc;
    if (ErrorCode::SUCCESS == error && (0 == convertNetworkTime(info, utc))) {
        handleTimeUpdate(MyTimeSrc::NETWORK, utc);
    }
}

// allow registering for NW time if:
// 1. NW time is enabled in telux configuration file
// 2. no valid GNSS/CV2X time
// 3. not during exit
bool ChronySock::registerNwTimeAllowed() {
    std::lock_guard<std::mutex> lock(timeMtx_);
    if (enableNwTime_ && !exit_ &&
        !gnssTimeValid_ && !cv2xTimeValid_) {
        return true;
    }
    return false;
}

// The API is called when:
// 1. no valid GNSS/SLSS time has been received after the app starts;
// 2. TCU suspend and then resume after NW time is registered
int ChronySock::registerNetworkTime() {
    // check if NW time is allowed
    if (!registerNwTimeAllowed()) {
        return -1;
    }

    // register for NW time only on TCU resume state
    if (TcuActivityState::RESUME != tcuState_) {
        LOGE("not register NW time due to tcuState:%d\n", static_cast<int>(tcuState_.load()));
        return -1;
    }

    LOGI("Registering network time\n");

    std::lock_guard<std::mutex> lock(nwTimeMtx_);

    if (srvSysMgr_) {
        LOGD("Network time already registered\n");
        return 0;
    }

    // create instance of ServingSystemManager and wait for readiness
    bool srvStatusUpdated = false;
    auto srvStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx_);
        srvStatusUpdated = true;
        srvStatus = status;
        cv_.notify_all();
    };

    auto tmpMgr = telux::tel::PhoneFactory::getInstance().getServingSystemManager(
        slotId_, statusCb);
    if (!tmpMgr) {
        LOGE("Failed to get ServingSystemManager for slot:%d\n", slotId_);
        return -1;
    }

    {
        std::unique_lock<std::mutex> lck(mtx_);
        cv_.wait(lck, [&] { return (srvStatusUpdated || exit_); });
    }

    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != srvStatus or exit_) {
        LOGE("ServingSystemManager initialization failed\n");
        return -1;
    }

    // request current NW time, it might fail due to NW time is not yet available
    tmpMgr->requestNetworkTime([this](
        telux::tel::NetworkTimeInfo info, telux::common::ErrorCode error) {
        this->onNetworkTimeResponse(info, error);
    });

    // register for NW time updates
    if (Status::SUCCESS != tmpMgr->registerListener(myListener_)) {
        LOGE("Register NW time listener failed\n");
        return -1;
    }

    srvSysMgr_ = tmpMgr;
    nwTimeEnabled_ = true;
    return 0;
}

// The API is called when:
// 1. valid GNSS/SLSS time is received after NW time is registered
// 2. TCU suspend after NW time is registered
// 3. app exit
void ChronySock::deregisterNetworkTime() {
    std::lock_guard<std::mutex> lock(nwTimeMtx_);
    if (!srvSysMgr_) {
        return;
    }
    LOGD("Deregistering network time\n");
    nwTimeEnabled_ = false;

    // deregister NW time listener
    if (myListener_) {
        srvSysMgr_->deregisterListener(myListener_);
    }

    // discard ServingSystemManager
    srvSysMgr_ = nullptr;
}
#endif

int main(int argc, char *argv[]) {
    bool exit = false;
    std::mutex exitMtx;
    std::condition_variable exitCv;
    int ret = 0;

    std::shared_ptr<ChronySock> instance = std::make_shared<ChronySock>();
    if (!instance) {
        LOGE("Failed to create ChronySock\n");
        return -1;
    }

    // Exits if invalid arguments passed or initialization failed
    if (instance->parseArguments(argc, argv) || instance->init()) {
        return -1;
    }

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [&](int sig) {
        std::unique_lock<std::mutex> lck(exitMtx);
        exit = true;
        if (instance) {
            instance->prepareForDeinit();
        }
        exitCv.notify_all();
    };

    SignalHandler::registerSignalHandler(sigset, cb);

    // register for platform time (GNSS/CV2X)
    if (exit || (ret = instance->registerPlatformTime())) {
        return ret;
    }

#ifdef FEATURE_NETWORK_TIME_ENABLED
    if (!exit && instance->registerNwTimeAllowed()) {
        // register for power notification before register for NW time
        if (0 == instance->registerPwrNotification()) {
            // register for NW time
            if (instance->registerNetworkTime()) {
                // deregister power notification if fail to register for NW time
                instance->deregisterPwrNotification();
            }
        }
    }
#endif

    {
        std::unique_lock<std::mutex> lck(exitMtx);
        while (!exit) {
            exitCv.wait(lck);
        }
    }

    instance->deinit();
    return 0;
}
