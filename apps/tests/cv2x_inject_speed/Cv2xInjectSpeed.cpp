/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: Cv2xInjectSpeed.cpp
 *
 * @brief: Application that provide vehicle speed to be used when GNSS is not available.
 */

#include <iostream>
#include <future>
#include <string>
#include <mutex>
#include <stdlib.h>
#include "../../common/utils/Utils.hpp"
#include <telux/cv2x/Cv2xRadio.hpp>

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::promise;

using telux::common::ErrorCode;
using telux::common::ServiceStatus;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::TrafficCategory;

int main(int argc, char *argv[]) {
    cout << "Running C-V2X inject speed app" << endl;

    int32_t speed = -1;
    // parse speed
    if ((argc < 2) or ((speed = atoi(argv[1])) < 0)) {
        cout << "Usage: cv2x_inject_speed <speed in kmph>" << endl;
        return -1;
    }

    std::vector<std::string> groups{"system", "diag", "radio", "dlt", "logd"};
    if (-1 == Utils::setSupplementaryGroups(groups)) {
        cout << "Adding supplementary group failed!" << std::endl;
    }

    // Get handle to Cv2xRadioManager
    std::condition_variable cv;
    std::mutex mtx;
    auto &cv2xFactory            = Cv2xFactory::getInstance();
    bool radioMgrStatusUpdated   = false;
    ServiceStatus radioMgrStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    auto statusCb                = [&](ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        radioMgrStatusUpdated = true;
        radioMgrStatus        = status;
        cv.notify_all();
    };
    auto cv2xRadioMgr = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioMgr) {
        cout << "Error: failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return (radioMgrStatusUpdated); });
        if (ServiceStatus::SERVICE_AVAILABLE != radioMgrStatus) {
            cerr << "C-V2X radio Manager initialization failed!" << endl;
            return EXIT_FAILURE;
        }
    }

    bool radioStatusUpdated   = false;
    ServiceStatus radioStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    auto cb                   = [&](ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        radioStatusUpdated = true;
        radioStatus        = status;
        cv.notify_all();
    };
    auto radio = cv2xRadioMgr->getCv2xRadio(TrafficCategory::SAFETY_TYPE, cb);
    if (not radio) {
        cerr << "C-V2X radio creation failed." << endl;
        return EXIT_FAILURE;
    }

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return (radioStatusUpdated); });
        if (radioStatus != ServiceStatus::SERVICE_AVAILABLE) {
            cerr << "C-V2X radio initialization failed!" << endl;
            return EXIT_FAILURE;
        }
    }

    promise<ErrorCode> p;
    auto ret = radio->injectVehicleSpeed((uint32_t)speed, [&p](ErrorCode error) {
        if (ErrorCode::SUCCESS != error) {
            cout << "Inject speed fail, error code " << static_cast<int>(error) << endl;
        }
        p.set_value(error);
    });
    if (telux::common::Status::SUCCESS == ret && ErrorCode::SUCCESS == p.get_future().get()) {
        cout << "Injected speed " << speed << endl;
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}
