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
/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ReportHandler.hpp"
#include "ReportReader.hpp"
#include "../common/Logger.hpp"

namespace telux {

namespace loc {

void ReportHandler::reportThread() {
    LOG(DEBUG, __FUNCTION__, " Creating");
    static int count = 0;
    // Thread exits when Destructor changes atomic variable exitThread to 1
    while (exitThread.load() == 0) {
        {
            std::lock_guard<std::mutex> lk(cv_m);
            auto &myReader = ReportReader::getInstance();
            if(basicNotification_.load() == 1) {
                myReader.getLocationInfoBase(iBase_);
                repSeqNo_.br++;
            }
            if(detailedNotification_.load() == 1 || detailedEngineNotification_.load() == 1) {
                myReader.getLocationInfoEx(iBaseEx_);
                gnssSVInfo_ =  myReader.getGnssSVInfo();
                gnssSignalInfo_ = myReader.getGnssSignalInfo();
                nmeaVals_ = myReader.getNmeaVal();
                // Measurement read every 10 cycles (1 sec)
                if (count++ == 10) {
                    gnssMeasurement_ = myReader.getGnssMeasurements();
                    count = 0;
                }
                repSeqNo_.dr++;
                repSeqNo_.der++;
            }
            if(sysinfoNotification_.load() == 1) {
                locationSystemInfo_ = myReader.getSystemInfoReport();
            }
        }
        cv.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    cv.notify_all();
    // informs destructor it exited
    exited.store(1);
}

ReportHandler::ReportHandler() {
    LOG(DEBUG, __FUNCTION__);
    exitThread.store(0);
    exited.store(0);
    basicNotification_.store(0);
    detailedNotification_.store(0);
    detailedEngineNotification_.store(0);
    sysinfoNotification_.store(0);
    //areListenersEnabled_.store(0);
    std::thread t(&ReportHandler::reportThread, this);
    t.detach();
}

ReportHandler &ReportHandler::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static ReportHandler reportHandler_;
    return (reportHandler_);
}

ReportHandler::~ReportHandler() {
    LOG(DEBUG, __FUNCTION__);
    exitThread.store(1);
    while (exited.load() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

}  // namespace loc
}  // namespace telux
