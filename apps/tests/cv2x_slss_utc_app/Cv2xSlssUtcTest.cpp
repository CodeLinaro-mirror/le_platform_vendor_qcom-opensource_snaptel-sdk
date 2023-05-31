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
#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

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
using telux::cv2x::ICv2xListener;
using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::UtcTimeInfo;

static bool gExit = false;
static std::mutex mtx;
static std::condition_variable cv;
static bool gInjectUtcValid = false;
static uint64_t gInjectUtc = 0;
static bool gEnableUtcReport = false;
static shared_ptr<ICv2xRadioManager> gCv2xRadioMgr = nullptr;
static shared_ptr<ICv2xListener> gCv2xListener = nullptr;

class UtcListener : public ICv2xListener {
public:
    void onUtcUpdateFromSlss(const UtcTimeInfo& utcInfo) override {
        cout << "------sys time(ms):" << Utils::getCurrentTimestamp()/1000;
        cout << "------" << endl;
        cout << "utcTime:" << utcInfo.utcTime << endl;
        cout << "tunc:" << utcInfo.tunc << endl;
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
                gInjectUtcValid = true;
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
        gCv2xListener = std::make_shared<UtcListener>();
    } catch (std::bad_alloc& e) {
        cerr << "Error CV2X UTC Listener allocation" << endl;
        return EXIT_FAILURE;
    }

    if (Status::SUCCESS != gCv2xRadioMgr->registerListener(gCv2xListener)) {
        cerr << "Failed to register CV2X UTC listener" << endl;
        gCv2xListener = nullptr;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

static int deregisterUtcReport() {
    if (gCv2xListener and
        Status::SUCCESS != gCv2xRadioMgr->deregisterListener(gCv2xListener)) {
        cerr << "Failed to deregister CV2X listener" << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running CV2X SLSS UTC Test APP" << endl;

    std::vector<std::string> groups{"system", "diag", "radio"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << endl;
    }

    installSignalHandler();

    if (parseOpts(argc, argv) or initCv2x()){
        return EXIT_FAILURE;
    }

    int ret = EXIT_SUCCESS;
    if (gInjectUtcValid) {
        ret = injectUtc();
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
