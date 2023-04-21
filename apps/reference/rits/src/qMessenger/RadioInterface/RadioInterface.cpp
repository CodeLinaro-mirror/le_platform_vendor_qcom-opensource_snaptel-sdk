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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


 /**
  * @file: RadioInterface.cpp
  *
  * @brief: Implementation of RadioInterface.h
  *
  */

#include <algorithm>
#include <vector>
#include <iterator>
#include "RadioInterface.h"

class Cv2xRadioListener : public ICv2xRadioListener {
public:
    virtual void onL2AddrChanged(uint32_t newL2Addr) {
        std::vector<v2x_src_l2_addr_update> l2Cbs;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            l2Cbs = l2Cbs_;
        }

        if (newL2Addr > 0 and l2Cbs.size() > 0) {
            for (auto cb : l2Cbs) {
                if (cb) {
                    cb(newL2Addr);
                }
            }
        }
    }

    void addL2AddrCallback(v2x_src_l2_addr_update cb) {
        std::unique_lock<std::mutex> lock(mtx_);
        if (std::find(l2Cbs_.begin(), l2Cbs_.end(), cb) == l2Cbs_.end()) {
            l2Cbs_.emplace_back(cb);
        }
    }

    void deleteL2AddrCallback(v2x_src_l2_addr_update cb) {
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = std::find(l2Cbs_.begin(), l2Cbs_.end(), cb);
        if (it != l2Cbs_.end()) {
            l2Cbs_.erase(it);
        }
    }

private:
    std::mutex mtx_;
    std::vector<v2x_src_l2_addr_update> l2Cbs_;
};

class Cv2xStatusListener : public telux::cv2x::ICv2xListener {
public:

    Cv2xStatusListener(telux::cv2x::Cv2xStatus status, int rVerbosity) {
        cv2xStatus_ = status;
        radioVerbosity = rVerbosity;
    };

    telux::cv2x::Cv2xStatus getCurrentStatus() {
        std::lock_guard<std::mutex> lock(cv2xStatusMutex_);
        return cv2xStatus_;
    }

    bool waitForCv2xStatus(telux::cv2x::Cv2xStatusType status, bool &haltRx) {
        bool closeAllFlow = false;
        // get the inital status
        telux::cv2x::Cv2xStatus tmpStatus;
        {
            std::lock_guard<std::mutex> lock(cv2xStatusMutex_);
            tmpStatus = cv2xStatus_;
        }

        while (tmpStatus.rxStatus != status or tmpStatus.txStatus != status) {
            // the initial status or the received status is not as expected,
            // wait for the next status change
            statusPromise_ = promise<telux::cv2x::Cv2xStatus>();
            promiseSet_ = false;
            //In the case where both tx and rx are enabled, flag to cease rx when cv2x status
            //becomes inactive/suspended.
            {
                std::lock_guard<std::mutex> lock(cv2xStatusMutex_);
                haltRx = true;
            }
            tmpStatus = statusPromise_.get_future().get();
            if(tmpStatus.rxStatus == status or tmpStatus.txStatus == status){
                std::lock_guard<std::mutex> lock(cv2xStatusMutex_);
                //check if we need close the exisiting flows and setup again
                closeAllFlow = true;
            }
        }
        return closeAllFlow;
    }

    void onStatusChanged(telux::cv2x::Cv2xStatus status) override {
        {
            std::lock_guard<std::mutex> lock(cv2xStatusMutex_);

             if (0 != memcmp(&status, &cv2xStatus_, sizeof(telux::cv2x::Cv2xStatus))) {
                if (radioVerbosity &&
                    (status.rxStatus != cv2xStatus_.rxStatus or
                    status.txStatus != cv2xStatus_.txStatus)) {
                    cout << "Cv2x status updated, rxStatus:" << static_cast<int>(status.rxStatus);
                    cout << ", txStatus:" << static_cast<int>(status.txStatus) << endl;
                }
                cv2xStatus_ = status;
            } else {
                // no need set promise if status is not changed
                return;
            }
        }

        if (not promiseSet_) {
            promiseSet_ = true;
            statusPromise_.set_value(status);
        }
    }

private:
    promise<telux::cv2x::Cv2xStatus> statusPromise_;
    std::atomic<bool> promiseSet_{false};
    std::mutex cv2xStatusMutex_;
    telux::cv2x::Cv2xStatus cv2xStatus_;
    int radioVerbosity = 0;
};


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

void RadioInterface::updateSrcL2InfoCallback(ErrorCode error){
    if(ErrorCode::SUCCESS == error) {
        //successful l2 src address update
    }else{
        printf("Error in l2 src address update\n");
    }
    this->gCallbackPromise.set_value(error);
};

bool RadioInterface::updateSrcL2(){
    bool success = false;
    auto respCb = [&](ErrorCode error) {
            updateSrcL2InfoCallback(error);
    };

    if(Status::SUCCESS == cv2xRadio->updateSrcL2Info(respCb)){
        if(ErrorCode::SUCCESS == gCallbackPromise.get_future().get()){
            success = true;
        }
    }

    // successful l2 address change if reach here
    this->resetCallbackPromise();
    return success;
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

    if (Status::SUCCESS != this->cv2xRadioManager->requestCv2xStatus(respCb)) {
        cerr << "Error : request for C-V2X status failed." << endl;
        gCv2xStatus.status.rxStatus = Cv2xStatusType::UNKNOWN;
        gCv2xStatus.status.txStatus = Cv2xStatusType::UNKNOWN;
    }

    if (ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
        cerr << "Error : failed to retrieve C-V2X status." << endl;
        gCv2xStatus.status.rxStatus = Cv2xStatusType::UNKNOWN;
        gCv2xStatus.status.txStatus = Cv2xStatusType::UNKNOWN;
    }

    if (RadioType::RX == type) {
        if (rVerbosity) {
            cout << "C-V2X RX is ";
        }
        status = gCv2xStatus.status.rxStatus;
    }
    else {
        if (rVerbosity) {
            cout << "C-V2X TX is ";
        }
        status = gCv2xStatus.status.txStatus;
    }

    if (rVerbosity) {
        cout << gCv2xStatusToString[status] << endl;
    }

    if (Cv2xStatusType::ACTIVE != status) {
        cerr << "C-V2X RX or TX status " << gCv2xStatusToString[status] << endl;
    }

    this->resetCallbackPromise();
    return status;
}

telux::cv2x::Cv2xStatus RadioInterface::getCurrentStatus() {
    return cv2xStatusListener_->getCurrentStatus();
}

void RadioInterface::waitForCv2xToActivate(bool &haltRx) {
    restartFlow = cv2xStatusListener_->waitForCv2xStatus(Cv2xStatusType::ACTIVE, haltRx);
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

    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait(lck, [&] { return cv2xRadioManagerStatusUpdated; });
        /* Check that V2X radio is initialized */
        if (telux::common::ServiceStatus::SERVICE_AVAILABLE !=
            cv2xRadioManagerStatus) {
            std::cout << "V2X cv2xRadioMgr initialization failed" << std::endl;
            return false;
        }
    }
    // Get C-V2X status and make sure requested radio(Tx or Rx) is enabled
    if (statusCheck(type) != Cv2xStatusType::ACTIVE) {
        return false;
    }

    // register listener for cv2x status change
    cv2xStatusListener_ = std::make_shared<Cv2xStatusListener>(gCv2xStatus.status,rVerbosity);
    if (Status::SUCCESS != cv2xRadioManager->registerListener(cv2xStatusListener_)) {
        cerr << "Error : register Cv2x status listener failed!" << endl;
        return false;
    }

    // Get handle to Cv2xRadio
    bool cv2x_radio_status_updated = false;
    telux::common::ServiceStatus cv2xRadioStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;

    auto cb = [&](telux::common::ServiceStatus status) {
        std::lock_guard<std::mutex> lock(mtx);
        cv2x_radio_status_updated = true;
        cv2xRadioStatus = status;
        cv.notify_all();
    };

    // Wait for radio to complete initialization
    cv2xRadio = cv2xRadioManager->getCv2xRadio(category);
    if (not cv2xRadio) {
        cerr << "C-V2X Radio creation failed." << endl;
        return false;
    }

    {
        std::unique_lock<std::mutex> lc(mtx);
        cv.wait(lc, [&cv2x_radio_status_updated]() { return cv2x_radio_status_updated; });

        if (cv2xRadioStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            cerr << "C-V2X Radio initialization failed." << endl;
            return false;
        }
    }

    // register listener for src L2 addr updates
    radioListener_ = std::make_shared<Cv2xRadioListener>();
    if (Status::SUCCESS != cv2xRadio->registerListener(radioListener_)) {
        cerr << "Error : register Cv2x radio listener failed!" << endl;
        return false;
    }

    return true;
}

int RadioInterface::registerL2AddrCallback(v2x_src_l2_addr_update cb) {
    if (!radioListener_) {
        cerr << "Radio listener not ready!" << endl;
        return -1;
    }

    auto sp = std::dynamic_pointer_cast<Cv2xRadioListener>(radioListener_);
    if (sp) {
        sp->addL2AddrCallback(cb);
        return 0;
    }

    return -1;
}

int RadioInterface::deregisterL2AddrCallback(v2x_src_l2_addr_update cb) {
    if (!radioListener_) {
        cerr << "Radio listener not ready!" << endl;
        return -1;
    }

    auto sp = std::dynamic_pointer_cast<Cv2xRadioListener>(radioListener_);
    if (sp) {
        sp->deleteL2AddrCallback(cb);
        return 0;
    }

    return -1;
}

int RadioInterface::getV2xIfaceName(TrafficIpType type, string& ifName) {
    if (!cv2xRadio or not cv2xRadio->isReady()) {
        cerr << "CV2X Radio not ready!" << endl;
        return -1;
    }

    ifName = cv2xRadio->getIfaceNameFromIpType(type);

    if (rVerbosity > 3) {
        cout << "Get V2X-Iface Name:" << ifName << endl;
    }
    return 0;
}

uint8_t RadioInterface::getCBRValue() {
    uint8_t cbr = 255;
    telux::cv2x::Cv2xStatus status;
    if (cv2xStatusListener_) {
        status = cv2xStatusListener_->getCurrentStatus();
        if (status.cbrValueValid) {
            cbr = status.cbrValue;
        }
    }
    return cbr;
}

uint64_t RadioInterface::latestTxRxTimeMonotonic() {
    return 0;
}

void RadioInterface::enableCsvLog(bool enable) {
    enableCsvLog_ = enable;
}
