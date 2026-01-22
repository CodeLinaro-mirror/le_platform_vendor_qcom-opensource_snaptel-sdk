/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: ThrottleManager.cpp
 *
 * @brief: Implementation of ThrottleManager
 *
 */

#include "ThrottleManager.h"

std::shared_ptr<Cv2xTmListener> Cv2xTmListener::instance = nullptr;

Cv2xTmListener::Cv2xTmListener() {
}

Cv2xTmListener::Cv2xTmListener(int tmVerbosity) {
    if (!Cv2xTmListener::instance) {
        Cv2xTmListener::instance = std::make_shared<Cv2xTmListener>();
    }
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xTmStatusUpdated = true;
        cv2xTmStatus        = status;
        cv.notify_all();
    };

    cv2xThrottleManager = cv2xFactory.getCv2xThrottleManager(statusCb);
    if (!cv2xThrottleManager) {
        std::cout << "Error: failed to get Cv2xThrottleManager." << std::endl;
    }
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return cv2xTmStatusUpdated; });
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != cv2xTmStatus) {
        std::cout << "Error: failed to initialize Cv2xThrottleManager." << std::endl;
    }

    // register listener
    if (cv2xThrottleManager->registerListener(instance) != telux::common::Status::SUCCESS) {
        std::cout << "Failed to register listener" << std::endl;
    }
}

// Callback function for Cv2xThrottleManager->setVerificationLoad()
void cv2xsetVerificationLoadCallback(telux::common::ErrorCode error) {
    gCallbackPromise.set_value(error);
}

int Cv2xTmListener::setLoad(int load) {
    if (tmVerbosity > 5)
        std::cout << "Setting verification load to: " << load << std::endl;
    cv2xThrottleManager->setVerificationLoad(load, cv2xsetVerificationLoadCallback);
    if (telux::common::ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        std::cout << "Error : failed to set verification load" << std::endl;
        return -1;
    } else {
        if (tmVerbosity > 5)
            std::cout << "set verification load success" << std::endl;
    }
    gCallbackPromise = std::promise<telux::common::ErrorCode>();
    return 0;
}

void Cv2xTmListener::onFilterRateAdjustment(int rate) {
    if (tmVerbosity > 5) {
        std::cout << "Filter rate from throttle manager is " << rate << std::endl;
    }
    tmFilterRate = rate;
}

int Cv2xTmListener::getFilterRate() {
    std::lock_guard<std::mutex> lck(mtx);
    return tmFilterRate;
}
