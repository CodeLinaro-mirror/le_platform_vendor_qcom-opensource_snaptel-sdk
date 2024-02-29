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
 *  Copyright (c) 2021, 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/CommonDefines.hpp>
#include "LocationManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/JsonParser.hpp"
#include "common/CommonUtils.hpp"
#include <chrono>

//Default cb delay.
#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1

// This is used for computing energy consumed based on duration
#define ENERGY_CONSUMED_PER_SECOND 500

// Year of HW used as below
#define YEAR_OF_HW 0

// Identify reports
#define NONE_REPORTS 0
#define BASIC_REPORTS 1
#define DETAILED_REPORTS 2
#define DETAILED_ENG_REPORTS 3

#define RPC_FAIL_SUFFIX " RPC Request failed - "

namespace telux {

namespace loc {

// This denotes system start time and is used to arrive at elapsed duration for energy consumed info
static std::chrono::time_point<std::chrono::steady_clock> time_t0
    = std::chrono::steady_clock::now();

void LocationManagerStub::invokeSystemInfoReport(ReportHandler &rClass_) {
    LOG(DEBUG, __FUNCTION__);
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm *ltm = localtime(&now);
    // add 1 to avoid 00 hour reading
    sysInfoHourTime_ = ltm->tm_hour + 1;
    if (sysInfoHourTime_ != usedSysInfoHourTime_) {
        struct LocationSystemInfo info_ = rClass_.getSystemInfoReport();
        for (auto iter = systemInfoListener_.begin(); iter != systemInfoListener_.end();) {
            auto spt = (*iter).lock();
            if (spt != nullptr) {
                LOG(DEBUG, __FUNCTION__, " Sending System Info");
                spt->onLocationSystemInfo(info_);
                ++iter;
            } else {
                iter = systemInfoListener_.erase(iter);
            }
        }
        usedSysInfoHourTime_ = sysInfoHourTime_;
    }
}

void LocationManagerStub::invokeBasicReport(ReportHandler &rClass_) {
    LOG(DEBUG, __FUNCTION__);
    struct ReportSeqNos seqNo_ = rClass_.getReportSeqNos();
    if (seqNo_.br > (brSeqNo_ + brSeqDelta_)) {
        std::shared_ptr<LocationInfoBase> ibase = rClass_.getLocationInfoBase();
        for (auto iter = listeners_.begin(); iter != listeners_.end();) {
            auto spt = (*iter).lock();
            if (spt != nullptr) {
                spt->onBasicLocationUpdate(ibase);
                ++iter;
            } else {
                iter = listeners_.erase(iter);
            }
        }
        brSeqNo_ = seqNo_.br;
    }
}

void LocationManagerStub::invokeDetailedReport(ReportHandler &rClass_) {
    LOG(DEBUG, __FUNCTION__);
    struct ReportSeqNos seqNo_ = rClass_.getReportSeqNos();
    if (seqNo_.dr > (drSeqNo_+ drSeqDelta_)) {
        for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
            auto spt = (*iter).lock();
            if (spt != nullptr) {
                if (reportTypeMask_ & GnssReportType::LOCATION) {
                    spt->onDetailedLocationUpdate(rClass_.getLocationInfoEx());
                }
                if (reportTypeMask_ & GnssReportType::SATELLITE_VEHICLE) {
                    spt->onGnssSVInfo(rClass_.getGnssSVInfo());
                }
                if (reportTypeMask_ & GnssReportType::DATA) {
                    spt->onGnssSignalInfo(rClass_.getGnssSignalInfo());
                }
                if (reportTypeMask_ & GnssReportType::NMEA) {
                    std::vector<NMEAVals> &nmeaVals_ = rClass_.getNmeaVal();
                    for (auto iterNMEA = nmeaVals_.begin(); iterNMEA != nmeaVals_.end(); iterNMEA++)
                        spt->onGnssNmeaInfo(iterNMEA->nmeaTimestamp, iterNMEA->nmeaString);
                }
                if (reportTypeMask_ & GnssReportType::MEASUREMENT) {
                    spt->onGnssMeasurementsInfo(rClass_.getGnssMeasurements());
                }
                if (reportTypeMask_ & GnssReportType::HIGH_RATE_MEASUREMENT) {
                    spt->onGnssMeasurementsInfo(rClass_.getGnssMeasurements());
                }
                ++iter;
            } else {
                iter = listeners_.erase(iter);
            }
        }
        drSeqNo_ = seqNo_.dr;
    }
}

void LocationManagerStub::invokeDetailedEngineReport(ReportHandler &rClass_) {
    LOG(DEBUG, __FUNCTION__);
    struct ReportSeqNos seqNo_ = rClass_.getReportSeqNos();
    if (seqNo_.der > (derSeqNo_ + derSeqDelta_)) {
        std::vector<std::shared_ptr<ILocationInfoEx>> infoEngineReports;
        std::shared_ptr<LocationInfoEx> infoEx = rClass_.getLocationInfoEx();
        infoEngineReports.push_back(infoEx);
        for (auto iter = listeners_.begin(); iter != listeners_.end();) {
            auto spt = (*iter).lock();
            if (spt != nullptr) {
                if (reportTypeMask_ & GnssReportType::LOCATION) {
                    spt->onDetailedEngineLocationUpdate(infoEngineReports);
                }
                if (reportTypeMask_ & GnssReportType::SATELLITE_VEHICLE) {
                    spt->onGnssSVInfo(rClass_.getGnssSVInfo());
                }
                if (reportTypeMask_ & GnssReportType::DATA) {
                    spt->onGnssSignalInfo(rClass_.getGnssSignalInfo());
                }
                if (reportTypeMask_ & GnssReportType::NMEA) {
                    std::vector<NMEAVals> &nmeaVals_ = rClass_.getNmeaVal();
                    for (auto iterNMEA = nmeaVals_.begin(); iterNMEA != nmeaVals_.end(); iterNMEA++)
                        spt->onGnssNmeaInfo(iterNMEA->nmeaTimestamp, iterNMEA->nmeaString);
                }
                if (reportTypeMask_ & GnssReportType::MEASUREMENT) {
                    spt->onGnssMeasurementsInfo(rClass_.getGnssMeasurements());
                }
                if (reportTypeMask_ & GnssReportType::HIGH_RATE_MEASUREMENT) {
                    spt->onGnssMeasurementsInfo(rClass_.getGnssMeasurements());
                }
                ++iter;
            } else {
                iter = listeners_.erase(iter);
            }
        }
        derSeqNo_ = seqNo_.der;
    }
}

void LocationManagerStub::managerThread() {
    LOG(DEBUG, __FUNCTION__);
    auto &rClass_ = ReportHandler::getInstance();
    while (exitThread_.load() == 0) {
        std::unique_lock<std::mutex> lk(rClass_.cv_m);
        rClass_.cv.wait(lk);
        {
            std::lock_guard<std::mutex> listenerLock(listenerMutex_);
            if (listeners_.size() != 0) {
                if (type_.load() == BASIC_REPORTS) {
                    if(rClass_.basicNotification_.load() == 0) {
                        rClass_.basicNotification_.store(1);
                    }
                    invokeBasicReport(rClass_);
                } else if (type_.load() == DETAILED_REPORTS) {
                    if(rClass_.detailedNotification_.load() == 0) {
                        rClass_.detailedNotification_.store(1);
                    }
                    invokeDetailedReport(rClass_);
                } else if (type_.load() == DETAILED_ENG_REPORTS) {
                    if(rClass_.detailedEngineNotification_.load() == 0) {
                        rClass_.detailedEngineNotification_.store(1);
                    }
                    invokeDetailedEngineReport(rClass_);
                }
           } else {
               //"No Listeners"
           }
           if (systemInfoListener_.size() != 0) {
               if(rClass_.sysinfoNotification_.load() == 0) {
                    rClass_.sysinfoNotification_.store(1);
                }
               invokeSystemInfoReport(rClass_);
           }
        }
    }
    exited_.store(1);
}

LocationManagerStub::LocationManagerStub() {
    LOG(DEBUG, __FUNCTION__, " Creating");
    type_.store(NONE_REPORTS);
    exitThread_.store(0);
    exited_.store(0);
    brInterval_.store(0);
    brSeqDelta_.store(0);
    brSeqNo_.store(0);
    drInterval_.store(0);
    drSeqDelta_.store(0);
    drSeqNo_.store(0);
    derInterval_.store(0);
    derSeqDelta_.store(0);
    derSeqNo_.store(0);
    managerStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
    stub_ = CommonUtils::getGrpcStub<LocationManagerService>();
    std::thread t(&telux::loc::LocationManagerStub::managerThread, this);
    t.detach();
}

std::future<bool> LocationManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async, [&] { return waitForInitialization(); });
    return f;
}

bool LocationManagerStub::waitForInitialization() {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> cvLock(mutex_);
    cv_.wait(cvLock);
    return isSubsystemReady();
}

bool LocationManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::ServiceStatus LocationManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return managerStatus_;
}

telux::common::Status LocationManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async, [this, callback]() { this->initSync(callback); }).share();
    taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

void LocationManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::locStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    if(reqstatus.ok()) {
        std::lock_guard<std::mutex> lock(mutex_);
        managerStatus_ = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
        std::lock_guard<std::mutex> lock(mutex_);
        managerStatus_ = telux::common::ServiceStatus::SERVICE_FAILED;
    }
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(managerStatus_));

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        callback(managerStatus_);
    }
    cv_.notify_all();
}

telux::common::Status LocationManagerStub::registerListenerEx(std::weak_ptr<ILocationListener>
        listener) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        bool existing = 0;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, " Register Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
        }
    }
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::deRegisterListenerEx(std::weak_ptr<ILocationListener>
        listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " In deRegister Listener : Removing");
                retVal=telux::common::Status::SUCCESS;
                break;
            }
        }
    }
    return (retVal);
}

telux::common::Status LocationManagerStub::startDetailedReports(uint32_t intervalInMs,
    telux::common::ResponseCallback callback, GnssReportTypeMask reportMask) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->StartDetailedReports(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        std::lock_guard<std::mutex> listenerLock(listenerMutex_);
        if (intervalInMs < 100) {
            intervalInMs = 100;
        }
        reportTypeMask_ = reportMask;
        drInterval_.store(intervalInMs);
        drSeqDelta_.store(intervalInMs * .01);
        drSeqNo_.store(0);
        type_.store(DETAILED_REPORTS);
    }
    return status;
}

telux::common::Status LocationManagerStub::startDetailedEngineReports(uint32_t intervalInMs,
    LocReqEngine engineType, telux::common::ResponseCallback callback,
    GnssReportTypeMask reportMask) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->StartDetailedEngineReports(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        std::lock_guard<std::mutex> listenerLock(listenerMutex_);
        if (intervalInMs < 100) {
            intervalInMs = 100;
        }
        reportTypeMask_ = reportMask;
        derInterval_.store(intervalInMs);
        derSeqDelta_.store(intervalInMs * .01);
        derSeqNo_.store(0);
        type_.store(DETAILED_ENG_REPORTS);
    }
    return status;
}

telux::common::Status LocationManagerStub::startBasicReports(
    uint32_t distanceInMeters, uint32_t intervalInMs, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->StartBasicReports(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        std::lock_guard<std::mutex> listenerLock(listenerMutex_);
        if (intervalInMs < 100) {
            intervalInMs = 100;
        }
        brInterval_.store(intervalInMs);
        brSeqDelta_.store(intervalInMs * .01);
        brSeqNo_.store(0);
        type_.store(BASIC_REPORTS);
    }
    return status;
}

telux::common::Status LocationManagerStub::registerForSystemInfoUpdates(
    std::weak_ptr<ILocationSystemInfoListener> listener, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        bool existing = 0;
        for (auto iter=systemInfoListener_.begin(); iter<systemInfoListener_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, " System Info Listener : Existing");
                return telux::common::Status::ALREADY;
            }
        }
        if (existing == 0) {
            systemInfoListener_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Registering SystemInfo Listener");
        }
    }
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->RegisterLocationSystemInfo(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::common::Status LocationManagerStub::deRegisterForSystemInfoUpdates(
    std::weak_ptr<ILocationSystemInfoListener> listener, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=systemInfoListener_.begin(); iter<systemInfoListener_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = systemInfoListener_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " Removing System Info Listener");
                auto &rClass_ = ReportHandler::getInstance();
                rClass_.sysinfoNotification_.store(0);
                break;
            }
        }
    }
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->DeregisterLocationSystemInfo(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::common::Status LocationManagerStub::requestEnergyConsumedInfo(GetEnergyConsumedCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::RequestEnergyConsumedInfoReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->RequestEnergyConsumedInfo(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == Status::SUCCESS) {
        telux::loc::GnssEnergyConsumedInfo energyConsumed = {};
        energyConsumed.valid = static_cast<int>(response.validity());
        energyConsumed.energySinceFirstBoot = static_cast<int>(response.energy_consumed());
        auto f = std::async(std::launch::async, [=]() {
            if (cb && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                cb(energyConsumed, errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::common::Status LocationManagerStub::getYearOfHw(GetYearOfHwCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::GetYearOfHwReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->GetYearOfHw(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == Status::SUCCESS) {
        uint16_t yearOfHw = static_cast<int>(response.year_of_hw());
        auto f = std::async(std::launch::async, [=]() {
            if (cb && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                cb(yearOfHw, errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::loc::LocCapability LocationManagerStub::getCapabilities() {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::GetCapabilitiesReply response;
    ClientContext context;
    ::grpc::Status reqstatus = stub_->GetCapabilities(&context, request, &response);
    uint32_t capabilities = 0;
    if(reqstatus.ok()) {
        capabilities = static_cast<int>(response.loc_capability());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    return capabilities;
}

telux::common::Status LocationManagerStub::stopReports(telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    type_.store(NONE_REPORTS);
    auto &rClass_ = ReportHandler::getInstance();
    rClass_.basicNotification_.store(0);
    rClass_.detailedNotification_.store(0);
    rClass_.detailedEngineNotification_.store(0);
    auto f = std::async(std::launch::async, [=]() {
        if (callback) {
            std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_CALLBACK_DELAY));
            callback(telux::common::ErrorCode::SUCCESS);
        }
    }).share();
    taskQ_.add(f);
    return (telux::common::Status::SUCCESS);
}

std::shared_ptr<LocationInfoBase> LocationManagerStub::getLastLocation(bool defaultLocInfo) {
    std::shared_ptr<LocationInfoBase> locInfo = std::make_shared<LocationInfoBase>();
    if (defaultLocInfo) {
        locInfo->setLatitude(0);
        locInfo->setLongitude(0);
        locInfo->setLocationInfoValidity(0);
    } else {
        auto &myReader = ReportReader::getInstance();
        myReader.getLocationInfoBase(locInfo);
    }
    return locInfo;
}

telux::common::Status LocationManagerStub::getTerrestrialPosition(uint32_t timeoutMsec,
    TerrestrialTechnology techMask, GetTerrestrialInfoCallback cb,
    telux::common ::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay = DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->GetTerrestrialPosition(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            uint32_t delay = cbDelay;
            std::shared_ptr<LocationInfoBase> locInfo;
            LOG(DEBUG, "Timeout: ", timeoutMsec, ", delay: ", delay);
            if (timeoutMsec <= delay) {
                LOG(INFO, "timeout shorter, will send default location");
                delay = timeoutMsec;
                LOG(DEBUG, "Timeout: ", timeoutMsec, ", delay: ", delay);
                locInfo = getLastLocation(true);
            } else {
                LOG(INFO, "timeout lengthier, will send last received location unless cancelled");
                locInfo = getLastLocation();
            }
            LOG(DEBUG, "Timeout: ", timeoutMsec, ", delay: ", delay);
            std::unique_lock<std::mutex> lk(terrestrialPositionMutex_);
            if (cvTerrestrialPosition_.wait_for(lk, std::chrono::milliseconds(delay))
                == std::cv_status::timeout) {
                    LOG(DEBUG, "Timed out, sending GTP callback");
                    cb(locInfo);
            } else {
                LOG(DEBUG, "GTP callback cancelled");
            }
            if (callback) {
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

telux::common::Status LocationManagerStub::cancelTerrestrialPositionRequest(
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::locStub::LocManagerCommandReply response;
    ClientContext context;
    telux::common::Status status = telux::common::Status::FAILED;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::GENERIC_FAILURE;
    int cbDelay =DEFAULT_CALLBACK_DELAY;
    ::grpc::Status reqstatus = stub_->CancelTerrestrialPosition(&context, request, &response);
    if(reqstatus.ok()) {
        status = static_cast<telux::common::Status>(response.status());
        errorCode = static_cast<telux::common::ErrorCode>(response.error());
        cbDelay = static_cast<int>(response.delay());
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    if (status == Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (errorCode == ErrorCode::SUCCESS) {
                std::lock_guard<std::mutex> lk(terrestrialPositionMutex_);
                cvTerrestrialPosition_.notify_all();
            }
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
    }
    return status;
}

LocationManagerStub::~LocationManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    exitThread_.store(1);
    while (exited_.load() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void LocationManagerStub::cleanup() {
}

}  // namespace loc

}  // namespace telux
