/*
 *  Copyright (c) 2020 The Linux Foundation. All rights reserved.
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
 * @file: Cv2xTmApp.cpp
 *
 * @brief: sample app that connects to throttle manager.
 */

#include <iostream>
#include <string>
#include <future>
#include <unistd.h>

#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xThrottleManager.hpp>

#define LOOP_COUNT  10

using namespace telux::cv2x;
static std::promise<telux::common::ErrorCode> gCallbackPromise;
static int load = 2000;      /* intial verification load */
static int filter_rate = 0;  /* initial filter rate */

class Cv2xTmListener : public ICv2xThrottleManagerListener {
    void onFilterRateAdjustment(int filter_rate_adjustment) {
        std::cout << "rate adjustment: " << filter_rate_adjustment << std::endl;
        filter_rate += filter_rate_adjustment;
        std::cout << "verification capability: " << load - filter_rate << std::endl;
    }
};

// Callback function for Cv2xThrottleManager->setVerificationLoad()
static void cv2xsetVerificationLoadCallback(telux::common::ErrorCode error) {
    std::cout << "error=" << static_cast<int>(error) << std::endl;
    gCallbackPromise.set_value(error);
}

int main(int argc, char *argv[]) {
    int loop = 0;
    auto listener = std::make_shared<Cv2xTmListener>();
    std::cout << "Running TM app" << std::endl;

    // Get handle to Cv2xRadioManager
    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xThrottleManager = cv2xFactory.getCv2xThrottleManager();

    // Wait for throttle manager to complete initialization
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=  cv2xThrottleManager->getServiceStatus()) {
        if (!cv2xThrottleManager->onSubsystemReady().get()) {
            std::cout << "Error : C-V2X Throttle Manager initialization failed" << std::endl;
            return EXIT_FAILURE;
        }
    }

    // register listener
    if (cv2xThrottleManager->registerListener(listener) !=
            telux::common::Status::SUCCESS) {
        std::cout << "Failed to register listener" << std::endl;
        return EXIT_FAILURE;
    }

    //simulating the incoming verification load fluctuate by periodically set the verification load
    while(loop < LOOP_COUNT) {
        std::cout << "Setting verification load to: " << load << std::endl;
        cv2xThrottleManager->setVerificationLoad(load, cv2xsetVerificationLoadCallback);
        if (telux::common::ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            std::cout << "Error : failed to set verification load" << std::endl;
            return EXIT_FAILURE;
        } else {
            std::cout << "set verification load success" << std::endl;
        }
        gCallbackPromise = std::promise<telux::common::ErrorCode>();

        load -= 20;
        sleep(5);
        loop++;
    };
    return EXIT_SUCCESS;
}
