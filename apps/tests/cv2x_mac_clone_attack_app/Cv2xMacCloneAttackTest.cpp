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
 * @file: Cv2xMacCloneAttackTest.cpp
 *
 * @brief: Simple application that demonstrates how to register and deregister
 * a listener for MAC cloning attack indication.
 */

#include <iostream>
#include <future>
#include <mutex>
#include <string>
#include <signal.h>

#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioListener.hpp>

#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::make_shared;
using std::shared_ptr;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::ICv2xRadioListener;

static bool gExit = false;
static std::mutex mtx;
static std::condition_variable cv;
static shared_ptr<ICv2xRadio> gCv2xRadio = nullptr;
static shared_ptr<ICv2xRadioListener> gCv2xListener = nullptr;

class MacCloneAttackListener : public ICv2xRadioListener {
public:
    void onMacAddressCloneAttack(const bool detected) override {
        cout << "------sys time:" << Utils::getCurrentTimeString();
        cout << "------" << endl;
        cout << "mac cloning attack detect:" << detected << endl;
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

static int initCv2x() {
    bool statusUpdate = false;
    telux::common::ServiceStatus cv2xStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        statusUpdate = true;
        cv2xStatus = status;
        cv.notify_all();
    };

    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioMgr) {
        cerr << "Failed to get cv2x radio manager" << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return (gExit || statusUpdate); });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE != cv2xStatus) {
            cerr << "CV2X radio Manager initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }

    // init cv2x radio
    statusUpdate = false;
    cv2xStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    gCv2xRadio = cv2xRadioMgr->getCv2xRadio(telux::cv2x::TrafficCategory::SAFETY_TYPE, statusCb);
    if (!gCv2xRadio) {
        cerr << "Failed to get cv2x radio" << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return (gExit || statusUpdate); });
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE != cv2xStatus) {
            cerr << "CV2X radio initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }

    // register listener for mac cloning attack indications
    try {
        gCv2xListener = std::make_shared<MacCloneAttackListener>();
    } catch (std::bad_alloc& e) {
        cerr << "Error cv2x listener allocation" << endl;
        return EXIT_FAILURE;
    }

    if (Status::SUCCESS != gCv2xRadio->registerListener(gCv2xListener)) {
        cerr << "Failed to register cv2x listener" << endl;
        gCv2xListener = nullptr;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    cout << "Running CV2X Mac Clone Attack Test APP" << endl;

    std::vector<std::string> groups{"system", "diag", "radio"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << endl;
    }

    installSignalHandler();

    if (initCv2x()) {
        return EXIT_FAILURE;
    }

    cout << "Start listening to mac cloning attack indications, press CTRL+C to exit." << endl;

    // wait for exit
    std::unique_lock<std::mutex> lck(mtx);
    while (!gExit) {
        cv.wait(lck);
    }

    if (gCv2xRadio and gCv2xListener) {
        gCv2xRadio->deregisterListener(gCv2xListener);
    }

    return EXIT_SUCCESS;
}
