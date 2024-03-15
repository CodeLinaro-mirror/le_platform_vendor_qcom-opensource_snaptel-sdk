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

#ifndef REPORTHANDLER_HPP
#define REPORTHANDLER_HPP
#include <future>
#include "LocationDefinesStub.hpp"
#include "commonDef.hpp"

namespace telux {

namespace loc {

class ReportHandler {
    ReportHandler();
    ReportHandler(const ReportHandler &) = delete;
    ReportHandler &operator=(const ReportHandler &) = delete;
    ~ReportHandler();

    // Sequence No of Basic Report(Br), DR(Detailed Report), DER(Detailed Engine Report)
    // increases by 1 every 100 msecs
    struct ReportSeqNos repSeqNo_;
    std::shared_ptr<LocationInfoBase> iBase_ = nullptr;
    std::shared_ptr<LocationInfoEx> iBaseEx_ = nullptr;
    std::shared_ptr<GnssSVInfo> gnssSVInfo_ = nullptr;
    std::shared_ptr<GnssSignalInfo> gnssSignalInfo_ = nullptr;
    struct LocationSystemInfo locationSystemInfo_;
    std::vector<NMEAVals> nmeaVals_;
    GnssMeasurements gnssMeasurement_;

    // reportThread gets reports from ReportReader every ~100 msecs
    //(it waits 100 msecs after getting report)
    void reportThread();

 public:
    static ReportHandler &getInstance();
    std::shared_ptr<LocationInfoBase> getLocationInfoBase() {
        return iBase_;
    };
    std::shared_ptr<LocationInfoEx> getLocationInfoEx() {
        return iBaseEx_;
    };
    struct ReportSeqNos getReportSeqNos() {
        return repSeqNo_;
    };
    std::shared_ptr<GnssSVInfo> getGnssSVInfo() {
        return gnssSVInfo_;
    };
    std::shared_ptr<GnssSignalInfo> getGnssSignalInfo() {
        return gnssSignalInfo_;
    };
    std::vector<NMEAVals> &getNmeaVal() {
        return nmeaVals_;
    };
    struct LocationSystemInfo &getSystemInfoReport() {
        return locationSystemInfo_;
    };
    GnssMeasurements &getGnssMeasurements() {
        return gnssMeasurement_;
    };

    // Manager objects can wait on condition_variable cv. All objects are notified every ~100msecs
    std::condition_variable cv;
    std::mutex cv_m;
    std::atomic<int> exitThread;
    std::atomic<int> exited;
    /**
     *  Enable report/sysinfo notifications.
     */
    std::atomic<int> basicNotification_;
    std::atomic<int> detailedNotification_;
    std::atomic<int> detailedEngineNotification_;
    std::atomic<int> sysinfoNotification_;
};

}  // namespace loc
}  // namespace telux
#endif
