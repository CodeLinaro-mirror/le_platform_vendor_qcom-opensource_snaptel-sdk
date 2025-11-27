/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: ThrottleManager.hpp
 *
 * @brief: Application that handles and abstracts the communication to Throttle Manager
 *
 */
#include <iostream>
#include <string>
#include <future>
#include <unistd.h>

#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xThrottleManager.hpp>

using namespace telux::cv2x;
static std::promise<telux::common::ErrorCode> gCallbackPromise;
static int tmFilterRate = 0;
static void cv2xsetVerificationLoadCallback(telux::common::ErrorCode error);

class Cv2xTmListener : public ICv2xThrottleManagerListener {

    static std::shared_ptr<Cv2xTmListener> instance;
    void onFilterRateAdjustment(int rate);
    int tmVerbosity = 0;

 public:
    /*
     * Default Constructor. Uses default verbosity.
     */
    Cv2xTmListener();

    /**
     * Constructor that creates a Cv2xTmListener Object
     * @param tmVerbosity - Verbosity set for debug prints.
     */
    Cv2xTmListener(int tmVerbosity);

    // Set verification load to throttle manager.
    int setLoad(int load);

    // Get the filter rate from throttle manager.
    int getFilterRate();

    bool cv2xTmStatusUpdated                  = false;
    telux::common::ServiceStatus cv2xTmStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    Cv2xFactory &cv2xFactory = Cv2xFactory::getInstance();
    std::shared_ptr<ICv2xThrottleManager> cv2xThrottleManager;
};
