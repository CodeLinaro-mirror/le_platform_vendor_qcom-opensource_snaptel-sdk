/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
 * @file: Cv2xRxApp.cpp
 *
 * @brief: Simple application that demonstrates Rx in Cv2x
 */

#include <assert.h>
#include <ifaddrs.h>

#include <iostream>
#include <memory>

#include <telux/cv2x/Cv2xRadio.hpp>


using std::array;
using std::cerr;
using std::cout;
using std::endl;
using std::promise;
using std::shared_ptr;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::ICv2xRxSubscription;
using telux::cv2x::TrafficCategory;
using telux::cv2x::TrafficIpType;

static constexpr uint16_t RX_PORT_NUM = 9000u;
static constexpr uint32_t G_BUF_LEN = 3000u;
static constexpr uint32_t NUM_TEST_ITERATIONS = 100u;

static Cv2xStatus gCv2xStatus;
static promise<ErrorCode> gCallbackPromise;
static shared_ptr<ICv2xRxSubscription> gRxSub;
static uint32_t gPacketsReceived = 0u;
static array<char, G_BUF_LEN> gBuf;


// Resets the global callback promise
static inline void resetCallbackPromise(void) {
    gCallbackPromise = promise<ErrorCode>();
}

// Callback function for Cv2xRadioManager->requestCv2xStatus()
static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gCv2xStatus = status;
    }
    gCallbackPromise.set_value(error);
}

// Callback function for Cv2xRadio->createRxSubscription() and Cv2xRadio->closeRxSubscription()
static void rxSubCallback(shared_ptr<ICv2xRxSubscription> rxSub, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gRxSub = rxSub;
    }
    gCallbackPromise.set_value(error);
}

// Function for reading from Rx socket
static void sampleRx(void) {

    int sock = gRxSub->getSock();

    cout << "sampleRx(" << sock << ")" << endl;

    // Attempt to read from socket
    int n = recv(sock, gBuf.data(), gBuf.max_size(), 0);

    if (n < 0) {
        cerr << "Error occurred reading from socket[" << sock << "]" << endl;
    }
    else {
        cout << __FUNCTION__ << ": Received " << n << " bytes" << endl;
        ++gPacketsReceived;
    }
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X RX app" << endl;

    // Get handle to Cv2xRadioManager
    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager();

    // Get C-V2X status and make sure Rx is enabled
    assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    if (Cv2xStatusType::ACTIVE == gCv2xStatus.rxStatus) {
        cout << "C-V2X RX status is active" << endl;
    }
    else {
        cerr << "C-V2X RX is inactive" << endl;
        return EXIT_FAILURE;
    }

    // Get handle to Cv2xRadio
    auto cv2xRadio = cv2xRadioManager->getCv2xRadio(TrafficCategory::SAFETY_TYPE);

    // Wait for radio to complete initialization
    if (not cv2xRadio->isReady()) {
        if (Status::SUCCESS == cv2xRadio->onReady().get()) {
            cout << "C-V2X Radio is ready" << endl;
        }
        else {
            cerr << "C-V2X Radio initialization failed." << endl;
            return EXIT_FAILURE;
        }
    }

    // Create new Rx subscription
    resetCallbackPromise();
    assert(Status::SUCCESS == cv2xRadio->createRxSubscription(TrafficIpType::TRAFFIC_NON_IP,
                                                              RX_PORT_NUM,
                                                              rxSubCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    // Read from the RX socket in a loop
    for (uint32_t i = 0; i < NUM_TEST_ITERATIONS; ++i) {
        sampleRx();
    }

    // Close subscription
    resetCallbackPromise();
    assert(Status::SUCCESS == cv2xRadio->closeRxSubscription(gRxSub,
                                                             rxSubCallback));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    cout << "Done." << endl;

    return EXIT_SUCCESS;
}
