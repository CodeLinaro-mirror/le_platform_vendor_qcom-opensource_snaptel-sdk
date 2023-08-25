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
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef REPORTREADER_HPP
#define REPORTREADER_HPP

#include "LocationDefinesStub.hpp"
#include "commonDef.hpp"
#include "../common/SimulationConfigParser.hpp"
#include "../common/CsvHandler.hpp"

using namespace telux::common;

namespace telux {

namespace loc {

class ReportReader {
    std::shared_ptr<GnssSignalInfo> gnssSignalInfo_;
    std::shared_ptr<GnssSVInfo> gnssSVInfo_;
    std::shared_ptr<LocationInfoBase> iBase_;
    std::shared_ptr<LocationInfoEx> iBaseEx_;
    struct GnssMeasurements gnssMeasurements_;
    struct LocationSystemInfo locationSystemInfo_;
    std::vector<NMEAVals> defaultNmeaVals_;
    csvData csvData_;
    int simulType_;

    // Initalizer
    void reportDataInit();
    // Filling Canned data for REPORT_SIMULATION_TYPE option 0 and also default
    void reportCannedDataInit();
    // Populating Basic Reports from file when REPORT_SIMULATION_TYPE = 1.
    void reportCsvIBase();
    // Populating Detailed PVT Reports from file when REPORT_SIMULATION_TYPE = 1.
    void reportCsvIBaseEx();
    // Populating NMEA Reports from file when REPORT_SIMULATION_TYPE = 1. Planned for Phase 2.
    void reportCsvNmea();
    // Populating SV Reports from file when REPORT_SIMULATION_TYPE = 1. Planned for Phase 2.
    void reportCsvSvInfo();
    // Populating Signal Reports from file when REPORT_SIMULATION_TYPE = 1. Planned for Phase 2.
    void reportCsvSignalInfo();
    // Populating Measurement Reports from file when REPORT_SIMULATION_TYPE = 1. Planned for Phase 2
    void reportCsvMeasurementInfo();
    // Read Csv data
    void readCsvData(std::string configVal, csvData &csvReportData,
        std::shared_ptr<SimulationConfigParser> configParser);

    telux::loc::LocationReliability convertLocationReliability(size_t newLocReliability);
    telux::loc::AltitudeType convertAltitudeType(bool type);

    ReportReader();
    ReportReader(const ReportReader &) = delete;
    ReportReader &operator=(const ReportReader &) = delete;
    ~ReportReader();

 public:
    static ReportReader &getInstance();

    // Gets GnssSignalInfo; Canned values or from CSV
    std::shared_ptr<GnssSignalInfo> getGnssSignalInfo();
    // Gets GnssSVInfo; Canned values or from CSV
    std::shared_ptr<GnssSVInfo> getGnssSVInfo();
    // Gets GnssMeasurements; Canned values or from CSV
    GnssMeasurements &getGnssMeasurements();
    // Gets NMEAVals; Canned values or from CSV
    std::vector<NMEAVals> getNmeaVal();
    // Gets LocationSystemInfo; Canned values or from CSV
    struct LocationSystemInfo &getSystemInfoReport();
    // Gets LocationInfoBase; Canned values or from CSV
    void getLocationInfoBase(std::shared_ptr<LocationInfoBase> &ibase);
    // Gets LocationInfoEx; Canned values or from CSV
    void getLocationInfoEx(std::shared_ptr<LocationInfoEx> &ibaseEx);

};

}  // namespace loc
}  // namespace telux

#endif
