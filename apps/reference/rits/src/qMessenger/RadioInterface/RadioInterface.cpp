/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
  * @file: RadioInterface.cpp
  *
  * @brief: Implementation of RadioInterface.h
  *
  */

#include "RadioInterface.h"

void RadioInterface::set_radio_verbosity(int value) {
    if(value)
        printf("Radio flow verbosity will be set to: %d\n", value);
    rVerbosity = value;
}

map<Cv2xStatusType, string> RadioInterface:: gCv2xStatusToString = {
    {Cv2xStatusType::INACTIVE, "INACTIVE"},
    {Cv2xStatusType::ACTIVE, "ACTIVE"},
    {Cv2xStatusType::SUSPENDED, "SUSPENDED"},
    {Cv2xStatusType::UNKNOWN, "UNKNOWN"},
};

void RadioInterface::resetCallbackPromise() {
    this->gCallbackPromise = promise<ErrorCode>();
};

void RadioInterface::cv2xStatusCallback(Cv2xStatusEx status, ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        this->gCv2xStatus = status;
    }
    this->gCallbackPromise.set_value(error);
};

Cv2xStatusType RadioInterface::statusCheck(RadioType type) {

    Cv2xStatusType status;
    auto respCb = [&](Cv2xStatusEx status,
        ErrorCode error) {
            cv2xStatusCallback(status, error);
    };

    assert(Status::SUCCESS == cv2xRadioManager->requestCv2xStatus(respCb));
    assert(ErrorCode::SUCCESS == gCallbackPromise.get_future().get());

    if (RadioType::RX == type) {
        cout << "C-V2X RX is ";
        status = gCv2xStatus.status.rxStatus;
    }
    else {
        cout << "C-V2X TX is ";
        status = gCv2xStatus.status.txStatus;
    }

    if (Cv2xStatusType::ACTIVE != status) {
        cerr << "C-V2X RX or TX status " << gCv2xStatusToString[status] << endl;
    }

    cout << gCv2xStatusToString[status] << endl;

    this->resetCallbackPromise();
    return status;
}

bool RadioInterface::ready(TrafficCategory category, RadioType type) {
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

    auto &cv2xFactory = Cv2xFactory::getInstance();
    cv2xRadioManager = cv2xFactory.getCv2xRadioManager(statusCb);
    if (!cv2xRadioManager) {
        std::cout << "Fail to get cv2xRadioMgr" << std::endl;
        return false;
    }
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
    /* Check that V2X radio is initialized */
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
        cv2xRadioManagerStatus) {
        std::cout << "V2X cv2xRadioMgr initialization failed" << std::endl;
        return false;
    }
    // Get C-V2X status and make sure requested radio(Tx or Rx) is enabled
    if (statusCheck(type) != Cv2xStatusType::ACTIVE) {
        return false;
    }

    // Wait for radio to complete initialization
    auto cv2xRadio = cv2xRadioManager->getCv2xRadio(category);
    if (not cv2xRadio->isReady()) {
        if (Status::SUCCESS == cv2xRadio->onReady().get()) {
            cout << "C-V2X Radio is ready" << endl;
        }
        else {
            cerr << "C-V2X Radio initialization failed." << endl;
            return false;
        }
    }

    return true;
}
