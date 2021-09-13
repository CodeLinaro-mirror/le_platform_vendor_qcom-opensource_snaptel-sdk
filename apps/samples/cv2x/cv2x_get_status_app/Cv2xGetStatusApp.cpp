/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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
 * @file: Cv2xGetStatusApp.cpp
 *
 * @brief: Simple application that queries C-V2X Status and prints
 *         to stdout.
 */

#include <iostream>
#include <future>
#include <map>
#include <mutex>
#include <string>

#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/common/CommonDefines.hpp>

#include "../../../common/utils/Utils.hpp"

using std::cout;
using std::endl;
using std::map;
using std::promise;
using std::string;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusEx;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::Cv2xCauseType;
using telux::cv2x::Cv2xPoolStatus;

static bool gExtStatus = false;
static Cv2xStatusEx gCv2xStatus;
static promise<ErrorCode> gCallbackPromise;
static map<Cv2xStatusType, string> gCv2xStatusToString = {
    {Cv2xStatusType::INACTIVE, "Inactive"},
    {Cv2xStatusType::ACTIVE, "Active"},
    {Cv2xStatusType::SUSPENDED, "SUSPENDED"},
    {Cv2xStatusType::UNKNOWN, "UNKNOWN"},
};
static map<Cv2xCauseType, string> gCv2xCauseToString = {
    {Cv2xCauseType::TIMING, "TIMING"},
    {Cv2xCauseType::CONFIG, "CONFIG"},
    {Cv2xCauseType::UE_MODE, "UE_MODE"},
    {Cv2xCauseType::GEOPOLYGON, "GEOPOLYGON"},
    {Cv2xCauseType::THERMAL, "THERMAL"},
    {Cv2xCauseType::THERMAL_ECALL, "THERMAL_ECALL"},
    {Cv2xCauseType::GEOPOLYGON_SWITCH, "GEOPOLYGON_SWITCH"},
    {Cv2xCauseType::SENSING, "SENSING"},
    {Cv2xCauseType::LPM, "LPM"},
    {Cv2xCauseType::UNKNOWN, "UNKNOWN"},
};

// Callback function for Cv2xRadioManager->requestCv2xStatus(Cv2xStatus)
static void cv2xStatusCallback(Cv2xStatus status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gCv2xStatus.status = status;
    }
    gCallbackPromise.set_value(error);
}

// Callback function for Cv2xRadioManager->requestCv2xStatus(Cv2xStatusEx)
static void cv2xExtStatusCallback(Cv2xStatusEx status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        gCv2xStatus = status;
    }
    gCallbackPromise.set_value(error);
}

static void printCv2xStatus(Cv2xStatusEx eStatus) {
    cout << "C-V2X Status:" << endl;
    cout << "  Overall RX status=" << gCv2xStatusToString[eStatus.status.rxStatus] << endl;
    cout << "  Overall TX status=" << gCv2xStatusToString[eStatus.status.txStatus] << endl;

    // print Tx pool status
    for(uint32_t i = 0; i < eStatus.poolStatus.size(); ++i) {
        if (eStatus.poolStatus[i].status.txStatus != Cv2xStatusType::UNKNOWN) {
            cout << "  Tx pool " << +eStatus.poolStatus[i].poolId;
            cout << ": status=" << gCv2xStatusToString[eStatus.poolStatus[i].status.txStatus];
            cout << ", cause=" << gCv2xCauseToString[eStatus.poolStatus[i].status.txCause] << endl;
        }
    }

    // print Rx pool Status
    for(uint32_t i = 0; i < eStatus.poolStatus.size(); ++i) {
        if (eStatus.poolStatus[i].status.rxStatus != Cv2xStatusType::UNKNOWN) {
            cout << "  Rx pool " << +eStatus.poolStatus[i].poolId;
            cout << ": status=" << gCv2xStatusToString[eStatus.poolStatus[i].status.rxStatus];
            cout << ", cause=" << gCv2xCauseToString[eStatus.poolStatus[i].status.rxCause] << endl;
        }
    }
}

static void printUsage(const char *Opt) {
    cout << "Usage: " << Opt << endl;
    cout << "-e    Get V2X status and per pool status, default is V2X status" << endl;
}

// Parse options
static int parseOpts(int argc, char *argv[]) {
    int rc = 0;
    int c;
    while ((c = getopt(argc, argv, "?he")) != -1) {
        switch (c) {
        case 'e':
            cout << "Get V2X status and per pool status." << endl;
            gExtStatus = true;
            break;
        case '?':
        case 'h':
        default:
            rc = -1;
            printUsage(argv[0]);
            return rc;
        }
    }

    return rc;
}


int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X Get Status APP" << endl;

    std::vector<std::string> groups{"system", "diag", "radio"};
    if (-1 == Utils::setSupplementaryGroups(groups)){
        cout << "Adding supplementary group failed!" << std::endl;
    }

    // Parse parameters, set V2X status type
    if (parseOpts(argc, argv)){
        return EXIT_FAILURE;
    }

    bool cv2xRadioManagerStatusUpdated = false;
    telux::common::ServiceStatus cv2xRadioManagerStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    std::condition_variable cv;
    std::mutex mtx;
    auto statusCb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2xRadioManagerStatusUpdated = true;
        cv2xRadioManagerStatus = status;
        cv.notify_all();
    };
    // Get handle to Cv2xRadioManager
    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioManager = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioManager) {
        cout << "Error: failed to get Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
        cv2xRadioManagerStatus) {
        cout << "Error: failed to initialize Cv2xRadioManager." << endl;
        return EXIT_FAILURE;
    }

    Status ret = Status::SUCCESS;
    if (gExtStatus) {
        // Get C-V2X Ext status
        ret = cv2xRadioManager->requestCv2xStatus(cv2xExtStatusCallback);
    } else {
        // Get C-V2X status
        ret = cv2xRadioManager->requestCv2xStatus(cv2xStatusCallback);
    }

    if (Status::SUCCESS != ret) {
        cout << "Error : request for C-V2X status failed." << endl;
        return EXIT_FAILURE;
    }
    if (ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cout << "Error : failed to retrieve C-V2X status." << endl;
        return EXIT_FAILURE;
    }

    // Print status
    printCv2xStatus(gCv2xStatus);

    return EXIT_SUCCESS;
}
