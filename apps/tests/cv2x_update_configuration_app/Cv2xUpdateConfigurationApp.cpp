/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 * @file: Cv2xUpdateConfigurationApp.cpp
 *
 * @brief: Application that queries C-V2X Status, stops v2x mode if Active, and
 *         attempts to update configuration file.
 */

#include <iostream>
#include <future>
#include <map>
#include <string>
#include <mutex>
#include <memory>
#include <thread>
#include <chrono>

#include <telux/cv2x/Cv2xConfig.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>

using std::cout;
using std::endl;
using std::cin;
using std::map;
using std::promise;
using std::string;
using std::mutex;
using std::lock_guard;
using std::make_shared;
using std::shared_ptr;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::ICv2xListener;

static Cv2xStatus gCv2xStatus;
static mutex gCv2xStatusMutex;
static promise<ErrorCode> gCallbackPromise;

class Listener : public ICv2xListener {
public:
    void onStatusChanged(Cv2xStatus status) override {
        lock_guard<mutex> lock(gCv2xStatusMutex);
        gCv2xStatus = status;
        if (not promiseSet_) {
            statusPromise_.set_value(status);
            promiseSet_ = true;
        }
    }

    std::promise<Cv2xStatus> & waitForStatusChange() {
        lock_guard<mutex> lock(gCv2xStatusMutex);
        statusPromise_ = std::promise<Cv2xStatus>();
        promiseSet_ = false;
        return statusPromise_;
    }

private:
    promise<Cv2xStatus> statusPromise_;
    bool promiseSet_ = false;

};

// Callback function for Cv2xRadioManager->requestCv2xSatus()
static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        lock_guard<mutex> lock(gCv2xStatusMutex);
        gCv2xStatus = status;
    }
    gCallbackPromise.set_value(error);
}

static void cv2xUpdateConfigurationCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS != error) {
        cout << "Config file update failed with code: " << static_cast<int>(error) << endl;
    }
    gCallbackPromise.set_value(error);
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X Update Configuration app" << endl;

    // Get handle to Cv2xRadioManager
    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager();

    // Wait for radio manager to complete initialization
    if (not cv2xRadioManager->isReady()) {
        if (!cv2xRadioManager->onReady().get()) {
            cout << "Error : C-V2X Radio Manager initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }

    auto cv2xListener = make_shared<Listener>();
    if (Status::SUCCESS != cv2xRadioManager->registerListener(cv2xListener)) {
        cout << "Error : register C-V2X listener failed.";
        return EXIT_FAILURE;
    }

    // Get C-V2X status
    if (Status::SUCCESS != cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback)) {
        cout << "Error : request for C-V2X status failed." << endl;
        return EXIT_FAILURE;
    }
    if (ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cout << "Error : failed to retrieve C-V2X status." << endl;
        return EXIT_FAILURE;
    }

    string configFilePath = "";
    gCallbackPromise = std::promise<ErrorCode>();

    if (argc > 1) {
        configFilePath = string(argv[1]);
    } else {
        cout << "Enter config file path: ";
        cin >> configFilePath;
    }

    bool stopV2xMode = false, v2xModeStopped = false;
    {
        lock_guard<mutex> lock(gCv2xStatusMutex);
        if (gCv2xStatus.rxStatus == Cv2xStatusType::UNKNOWN or
            gCv2xStatus.txStatus == Cv2xStatusType::UNKNOWN) {
            cout << "Error : C-V2X status is unknown." << endl;
            return EXIT_FAILURE;
        } else if (Cv2xStatusType::INACTIVE != gCv2xStatus.rxStatus or
            Cv2xStatusType::INACTIVE != gCv2xStatus.txStatus) {
            cout << "Stopping C-V2X mode." << endl;
            stopV2xMode = true;
        }
    }

    /* V2X mode must be stopped before sending config file update */
    if (stopV2xMode) {
        promise<ErrorCode> prom;
        if (Status::SUCCESS != cv2xRadioManager->stopCv2x(
            [&prom](ErrorCode error) { prom.set_value(error); })) {
            cout << "Error : Unable to stop C-V2X mode." << endl;
            return EXIT_FAILURE;
        }

        auto res = prom.get_future().get();
        if (ErrorCode::SUCCESS != res) {
            cout << "Error : Stop C-V2X mode failed with error: "
                << static_cast<int>(res) << endl;
            return EXIT_FAILURE;
        }

        /* Wait for V2X status to report inactive before proceeding with config update */
        auto tempStatus = cv2xListener->waitForStatusChange().get_future().get();

        if (Cv2xStatusType::INACTIVE == tempStatus.rxStatus and
            Cv2xStatusType::INACTIVE == tempStatus.txStatus) {
            v2xModeStopped = true;
        } else {
            cout << "Error : Unable to stop C-V2X mode." << endl;
            return EXIT_FAILURE;
        }
    }

    auto cv2xConfig = cv2xFactory.getCv2xConfig();

    // Wait for radio manager to complete initialization
    if (not cv2xConfig->isReady()) {
        if (!cv2xConfig->onReady().get()) {
            cout << "Error : C-V2X Radio Manager initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }

    /* Attempt config file update */
    cout << "Updating configuration with file: " << configFilePath << endl;
    if (Status::SUCCESS !=
        cv2xConfig->updateConfiguration(configFilePath, cv2xUpdateConfigurationCallback)) {
        cout << "Error : Config file update failed." << endl;
        return EXIT_FAILURE;
    }

    auto res = gCallbackPromise.get_future().get();
    if (ErrorCode::SUCCESS != res) {
        cout << "Error : Config file update failed with code: "
            << static_cast<int>(res) << "." << endl;
        return EXIT_FAILURE;
    }

    if (stopV2xMode) {
        /* Cv2x Mode was started before attempting update.
           Restart V2X mode here to return system to that state. */
        cout << "Starting C-V2X mode." << endl;
        promise<ErrorCode> prom;
        if (Status::SUCCESS != cv2xRadioManager->startCv2x(
            [&prom](ErrorCode error) { prom.set_value(error); })) {
            cout << "Error : Unable to start C-V2X mode." << endl;
            return EXIT_FAILURE;
        }
        auto res = prom.get_future().get();
        if (ErrorCode::SUCCESS != res) {
            cout << "Error : Start C-V2X mode failed with error code: "
                << static_cast<int>(res) << "." << endl;
            return EXIT_FAILURE;
        }
    }

    cout << "Configuration update successful." << endl;
    return EXIT_SUCCESS;
}
