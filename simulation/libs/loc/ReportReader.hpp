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

#ifndef REPORTREADER_HPP
#define REPORTREADER_HPP

#include "commonDef.hpp"
#include "LocationDefinesStub.hpp"
namespace telux {

namespace loc {

class ReportReader{
    // file handler
    std::ifstream in;
    std::shared_ptr<GnssSignalInfo> gnssSignalInfo_;
    std::shared_ptr<GnssSVInfo> gnssSVInfo_;
    std::shared_ptr<LocationInfoBase> iBase_;
    std::shared_ptr<LocationInfoEx> iBaseEx_;
    struct GnssMeasurements gnssMeasurements_;
    struct LocationSystemInfo locationSystemInfo_;
    std::vector<NMEAVals> defaultNmeaVals_;

    //Initalizer
    void reportDataInit();
    // Filling Canned data for REPORT_SIMULATION_TYPE option 0 and also default
    void reportCannedDataInit();
    // File reading and populating objects with read data for REPORT_SIMULATION_TYPE option 1
    void readInfoBaseExData();

    ReportReader();
    ReportReader(const ReportReader &) = delete;
    ReportReader &operator=(const ReportReader &) = delete;
    ~ReportReader();

public:
    static  ReportReader &getInstance();

    // Gets GnssSignalInfo; Currently either Default or canned values available (Not from File)
    std::shared_ptr<GnssSignalInfo> getGnssSignalInfo();
    // Gets GnssSVInfo; Currently either Default or canned values available (Not from File)
    std::shared_ptr<GnssSVInfo> getGnssSVInfo();
    // Gets GnssMeasurements; Currently either Default or canned values available (Not from File)
    GnssMeasurements& getGnssMeasurements();
    // Gets NMEAVals; Currently either Default or canned values available (Not from File)
    std::vector<NMEAVals>& getNmeaVal();
    // Gets LocationSystemInfo; Currently either Default or canned values available (Not from File)
    struct LocationSystemInfo& getSystemInfoReport();
    // Gets LocationInfoBase and LocationInfoEx; Currently either Default or
    // canned values or from File
    void getLocationInfoBase(std::shared_ptr<LocationInfoBase> & ibase,
        std::shared_ptr<LocationInfoEx> & ibaseEx);


};

}
}

#endif
