/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CHRONY_SOCK_HPP__
#define CHRONY_SOCK_HPP__

#include <atomic>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/TimeManager.hpp>
#include <telux/platform/TimeListener.hpp>

#ifdef FEATURE_NETWORK_TIME_ENABLED
#include <telux/tel/ServingSystemManager.hpp>
#include <telux/power/TcuActivityManager.hpp>
#endif

#include "MyUtils.hpp"

#define DEFAULT_OFFSET_THRESHOLD (1000)  // default msec offset for adjusting system time

// Chrony SOCK sample
struct TimeSample {
    struct timeval tv;
    double offset;
    int pulse;
    int leap;
    int _pad;
    int magic;
};

enum class MyTimeSrc {
    GNSS,  // GNSS or propagated GNSS
    CV2X,  // CV2X SLSS
    NETWORK  // cellular network
};

class ChronySock;

// general listener for time updates and TCU state updates
class MyListener : public telux::platform::ITimeListener
#ifdef FEATURE_NETWORK_TIME_ENABLED
   ,
                   public telux::tel::IServingSystemListener,
                   public telux::power::ITcuActivityListener
#endif
{
 public:
    explicit MyListener(std::weak_ptr<ChronySock> instance);
    void onGnssUtcTimeUpdate(const uint64_t utc) override;
    void onCv2xUtcTimeUpdate(const uint64_t utc) override;

#ifdef FEATURE_NETWORK_TIME_ENABLED
    void onNetworkTimeChanged(telux::tel::NetworkTimeInfo info) override;
    void onTcuActivityStateUpdate(telux::power::TcuActivityState tcuState) override;
#endif

 private:
    std::weak_ptr<ChronySock> chronySock_;
};

class ChronySock : public std::enable_shared_from_this<ChronySock> {
 public:
    static int systemCall(const char *command);
    static void writeRtcFile(int sig, siginfo_t *si, void *uc);
    int parseArguments(int &argc, char **argv);
    int init();
    void prepareForDeinit();
    void deinit();
    int registerPlatformTime();
    void handleTimeUpdate(MyTimeSrc src, uint64_t utc);

#ifdef FEATURE_NETWORK_TIME_ENABLED
    int registerPwrNotification();
    void deregisterPwrNotification();
    void handleTcuStateUpdate(telux::power::TcuActivityState tcuState);
    int convertNetworkTime(telux::tel::NetworkTimeInfo info, uint64_t &utc);
    void onNetworkTimeResponse(telux::tel::NetworkTimeInfo info, telux::common::ErrorCode error);
    bool registerNwTimeAllowed();
    int registerNetworkTime();
#endif

 private:
    bool enableWriteRtc_      = false;
    bool enableCv2xTime_      = false;
    bool enableNwTime_        = false;
    bool enableDeltaUpdate_   = false;
    uint64_t offsetThreshold_ = DEFAULT_OFFSET_THRESHOLD;
    int slotId_               = DEFAULT_SLOT_ID;
    std::mutex mtx_;
    std::condition_variable cv_;
    int chronyfd_ = -1;
    bool exit_    = false;
    std::mutex timeMtx_;
    bool firstSample_                                         = true;
    bool gnssTimeValid_                                       = false;
    bool cv2xTimeValid_                                       = false;
    std::shared_ptr<AsyncThread> threads_                     = nullptr;
    std::shared_ptr<MyListener> myListener_                   = nullptr;
    std::shared_ptr<telux::platform::ITimeManager> myTimeMgr_ = nullptr;

#ifdef FEATURE_NETWORK_TIME_ENABLED
    std::atomic<bool> nwTimeEnabled_{false};
    std::mutex nwTimeMtx_;  // mutex for protecting network time register/deregister
    std::shared_ptr<telux::tel::IServingSystemManager> srvSysMgr_ = nullptr;
    std::mutex pwrMtx_;  // mutex for protecting pwr register/deregister
    std::shared_ptr<telux::power::ITcuActivityManager> tcuStateMgr_ = nullptr;
    std::atomic<telux::power::TcuActivityState> tcuState_{telux::power::TcuActivityState::UNKNOWN};
#endif

    void printUsage(char *app_name);
    void getTeluxConfig();
    int setupSocket(int &fd);
    int installRtcTimer();
    void deregisterPlatformTime();
    void setSystemTime(uint64_t utc);
    void sendUtcToChronyd(uint64_t utc);
    int getInternalRtcTime(uint64_t &msec);
    void updateDeltaTime(uint64_t utc);
    int readDeltaTimeFromFile(int64_t &deltaMsecs);
    int writeDeltaTimeToFile(int64_t deltaMsecs);

#ifdef FEATURE_NETWORK_TIME_ENABLED
    std::shared_ptr<telux::power::ITcuActivityManager> getTcuMgr();
    void deregisterNetworkTime();
#endif
};
#endif
