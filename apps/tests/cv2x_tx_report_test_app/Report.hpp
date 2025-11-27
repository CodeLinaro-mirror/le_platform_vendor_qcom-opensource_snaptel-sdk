/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef REPORT_HPP
#define REPORT_HPP

#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/cv2x/Cv2xTxStatusReportListener.hpp>

using telux::cv2x::ICv2xTxStatusReportListener;
using telux::cv2x::RFTxStatus;
using telux::cv2x::SegmentType;
using telux::cv2x::TxStatusReport;
using telux::cv2x::TxType;

#define DEFAULT_LOG_FILE ("/var/log/tx_report.csv")

class Cv2xTxStatusReportListener : public ICv2xTxStatusReportListener {
 public:
    Cv2xTxStatusReportListener(std::string fileName, uint16_t port, int &ret);

    void onTxStatusReport(const TxStatusReport &info);

    ~Cv2xTxStatusReportListener();

 private:
    std::string txType2String(TxType in);

    std::string rfStatus2String(RFTxStatus in);

    std::string segType2String(SegmentType in);

    void writeReportToFile(const TxStatusReport &info);

    void checkPerPktStatus(const TxStatusReport &info);

    void checkTxChainStatus(const TxStatusReport &info);

    void checkSpsTiming(const TxStatusReport &info);

    FILE *file_           = nullptr;  // user-specified file for logging reports
    uint32_t pktCount_    = 0;  // received report number of newTx with ONLY_ONE or First segment
    uint32_t newTxCount_  = 0;  // received newTx report number
    uint32_t reTxCount_   = 0;  // received reTx report number
    uint32_t slssTxCount_ = 0;  // received SLSS Tx report number
    uint16_t port_        = 0;  // user specified listening port
};

#endif  // REPORT_HPP