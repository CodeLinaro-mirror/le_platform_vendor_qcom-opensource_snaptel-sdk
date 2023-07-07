/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file: Cv2xSlssUtcTest.cpp
 *
 * @brief: Simple application that demonstrates how to inject coarse UTC when UE is
 * sync to remote UE through SLSS and get precise UTC in turn.
 */

#include <iostream>
#include <future>
#include <mutex>
#include <string>
#include <signal.h>

#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/platform/PlatformFactory.hpp>
#include <telux/platform/TimeManager.hpp>
#include <telux/platform/TimeListener.hpp>

#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::promise;
using std::string;
using std::make_shared;
using std::shared_ptr;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;
using telux::platform::PlatformFactory;
using telux::platform::ITimeManager;
using telux::platform::ITimeListener;
using telux::platform::SupportedTimeType;
using telux::platform::TimeTypeMask;

static bool gExit = false;
static std::mutex mtx;
static std::condition_variable cv;
static uint64_t gInjectUtc = 0;
static bool gEnableUtcReport = false;
static shared_ptr<ICv2xRadioManager> gCv2xRadioMgr = nullptr;
static shared_ptr<ITimeManager> gTimeMgr = nullptr;
static shared_ptr<ITimeListener> gTimeListener = nullptr;

class UtcListener : public ITimeListener {
public:
    void onCv2xUtcTimeUpdate(const uint64_t utcInMs) override {
        cout << "------sys time(ms):" << Utils::getCurrentTimestamp()/1000;
        cout << "------" << endl;
        cout << "utcTime:" << utcInMs << endl;
    }
};

static void installSignalHandler() {
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
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << endl;
    cout << " -i <utc> - Inject coarse UTC in units of millisecond" << endl;
    cout << " -l - Listen to UTC reports until exit using CTRL+C" << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int c;
    while ((c = getopt(argc, argv, "?hi:l")) != -1) {
        switch (c) {
        case 'i':
            if (optarg) {
                gInjectUtc = atoll(optarg);
            }
            break;
        case 'l':
            gEnableUtcReport = true;
            break;
        case 'h':
        case '?':
        default:
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int initCv2x() {
    bool statusUpdate = false;
    telux::common::ServiceStatus cv2xRadioMgrStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        statusUpdate = true;
        cv2xRadioMgrStatus = status;
        cv.notify_all();
    };

    auto & cv2xFactory = Cv2xFactory::getInstance();
    gCv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!gCv2xRadioMgr) {
        cerr << "Failed to get Cv2xRadioManager" << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return (gExit || statusUpdate); });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioMgrStatus) {
            cerr << "CV2X Radio Manager initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static int injectUtc() {
    bool getResponse = false;
    auto response = ErrorCode::GENERIC_FAILURE;
    auto injectUtcCb = [&](telux::common::ErrorCode errorCode) {
        std::lock_guard<std::mutex> lock(mtx);
        getResponse = true;
        response = errorCode;
        cv.notify_all();
    };
    if (Status::SUCCESS == gCv2xRadioMgr->injectCoarseUtcTime(gInjectUtc, injectUtcCb)) {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return (gExit || getResponse); });
    }

    if (ErrorCode::SUCCESS != response) {
        cerr << "Failed to inject UTC" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int registerUtcReport() {
    try {
        gTimeListener = std::make_shared<UtcListener>();
    } catch (std::bad_alloc& e) {
        cerr << "Error CV2X UTC Listener allocation" << endl;
        return EXIT_FAILURE;
    }

    bool statusUpdated = false;
    auto servicStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    TimeTypeMask capabilities;
    auto statusCb = [&statusUpdated, &servicStatus](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        statusUpdated = true;
        servicStatus = status;
        cv.notify_all();
    };
    gTimeMgr = PlatformFactory::getInstance().getTimeManager(statusCb);
    if (gTimeMgr) {
        // wait for utc manager to be ready
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&statusUpdated] { return (statusUpdated || gExit); });
    }

    if (gExit) {
        return EXIT_FAILURE;
    }

    if (servicStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        cout << "Time manager is ready" << endl;
    } else {
        cerr << "Unable to initialize time manager" << endl;
        return EXIT_FAILURE;
    }

    TimeTypeMask mask;
    mask.set(SupportedTimeType::CV2X_UTC_TIME);
    if (Status::SUCCESS != gTimeMgr->registerListener(gTimeListener, mask)) {
        cerr << "Failed to register time listener" << endl;
        gTimeListener = nullptr;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int deregisterUtcReport() {
    TimeTypeMask mask;
    mask.set(SupportedTimeType::CV2X_UTC_TIME);
    if (gTimeListener and
        Status::SUCCESS != gTimeMgr->deregisterListener(gTimeListener, mask)) {
        cerr << "Failed to deregister CV2X UTC listener" << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running CV2X SLSS UTC Test APP" << endl;

    std::vector<std::string> groups{"system", "diag", "radio", "locclient"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << endl;
    }

    installSignalHandler();

    if (parseOpts(argc, argv)){
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;
    if (gInjectUtc > 0) {
        if (initCv2x() or injectUtc()) {
            ret = EXIT_FAILURE;
        }
    }

    if (gEnableUtcReport) {
        if (registerUtcReport()) {
            return EXIT_FAILURE;
        }

        cout << "Start listening to CV2X UTC reports, press CTRL+C to exit." << endl;

        // wait for exit
        std::unique_lock<std::mutex> lck(mtx);
        while (!gExit) {
            cv.wait(lck);
        }

        ret = deregisterUtcReport();
    }

    return ret;
}
