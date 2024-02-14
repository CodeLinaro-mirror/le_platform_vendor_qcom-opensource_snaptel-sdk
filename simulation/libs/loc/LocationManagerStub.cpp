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
 *  Copyright (c) 2021, 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/CommonDefines.hpp>
#include "LocationManagerStub.hpp"
#include "LocationDefinesStub.hpp"
#include "common/Logger.hpp"
#include "common/JsonParser.hpp"
#include "common/CommonUtils.hpp"

#include <chrono>
#include <string>

//Default cb delay.
#define DEFAULT_CALLBACK_DELAY 100
#define SKIP_CALLBACK -1
// Year of HW used as below
#define YEAR_OF_HW 0
#define RPC_FAIL_SUFFIX " RPC Request failed - "

namespace telux {

namespace loc {

void LocationManagerStub::invokeSystemInfoReport(struct LocationSystemInfo &info) {
    LOG(DEBUG, __FUNCTION__);
    for (auto iter = systemInfoListener_.begin(); iter != systemInfoListener_.end();) {
        auto spt = (*iter).lock();
        if (spt != nullptr) {
            LOG(DEBUG, __FUNCTION__, " Sending System Info");
            spt->onLocationSystemInfo(info);
            ++iter;
        } else {
            iter = systemInfoListener_.erase(iter);
        }
    }
}

LocationManagerStub::LocationManagerStub() {
    LOG(DEBUG, __FUNCTION__, " Creating");
    managerStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
    stub_ = CommonUtils::getGrpcStub<LocationManagerService>();
    filter_ = nullptr;
    sysInfoRequestCount_.store(0);
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
        bool enableFiltering = false;
        std::shared_ptr<SimulationConfigParser> configParser =
            std::make_shared<SimulationConfigParser>();
        std::string locFiltering = configParser->getValue("ENABLE_LOCATION_FILTERING");
        if (!locFiltering.empty()) {
            enableFiltering = (locFiltering == "TRUE") ? true : false;
        }
        if (enableFiltering) {
            try {
                filter_ = std::make_shared<LocationReportFilter>();
            } catch (std::bad_alloc & e) {
                LOG(ERROR, __FUNCTION__, " LocationReportFilter: ", e.what());
            }
        }
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

void LocationManagerStub::adjustTimeInterval(uint32_t &interval) {
    //If multiple position sessions are started in a process, requesting for different time
    //intervals, the filtering logic usually passes the client requested interval to the lower
    //layers and will filter excess reports accordingly. But, in one of the corner cases where there
    //are two clients requesting 200ms and 500ms, the filtering logic will filter all the reports
    //for the 500ms client. This issue arises as 500 is not a multiple of 200. The other supported
    //intervals do not have this problem. Hence to avoid this case, changing the time interval from
    //200ms to 100ms before passing to LCA client.
    if (interval == 200) {
        LOG(DEBUG, __FUNCTION__);
        interval = 100;
    }
    return;
}

telux::common::Status LocationManagerStub::startDetailedReports(uint32_t interval,
    telux::common::ResponseCallback callback, GnssReportTypeMask reportMask) {
    LOG(DEBUG, __FUNCTION__);
    if(filter_ != nullptr) {
        adjustTimeInterval(interval);
    }
    interval_ = interval;
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
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        if (filter_ != nullptr) {
            telux::common::Status rc = filter_->startReportFilter(interval, ReportType::DETAILED);
            if (rc != telux::common::Status::SUCCESS) {
                LOG(WARNING, __FUNCTION__, " Starting detailed report filter Failed");
            }
        }
        sessionMask_ = telux::loc::DETAILED;
        reportMask_ = reportMask;
    }
    return status;
}

telux::common::Status LocationManagerStub::startDetailedEngineReports(uint32_t interval,
    LocReqEngine engineType, telux::common::ResponseCallback callback,
    GnssReportTypeMask reportMask) {
    LOG(DEBUG, __FUNCTION__);
    if(filter_ != nullptr) {
        adjustTimeInterval(interval);
    }
    interval_ = interval;
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
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        if (filter_ != nullptr) {
            telux::common::Status rc =
                filter_->startReportFilter(interval, ReportType::DETAILED_ENG);
            if (rc != telux::common::Status::SUCCESS) {
                LOG(WARNING, __FUNCTION__, " Starting detailed engine report filter Failed");
            }
        }
        sessionMask_ = telux::loc::DETAILED_ENGINE;
        reportMask_ = reportMask;
    }
    return status;
}

telux::common::Status LocationManagerStub::startBasicReports(
    uint32_t distanceInMeters, uint32_t interval, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if (filter_ != nullptr) {
        adjustTimeInterval(interval);
    }
    interval_ = interval;
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
        if (filter_ != nullptr) {
            telux::common::Status rc = filter_->startReportFilter(interval, ReportType::BASIC);
            if (rc != telux::common::Status::SUCCESS) {
                LOG(WARNING, __FUNCTION__, " Starting basic report filter Failed");
            }
        }
        sessionMask_ = telux::loc::BASIC;
    }
    return status;
}

telux::common::Status LocationManagerStub::stopReports(telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;
    ClientContext context;
    ::grpc::Status reqstatus = stub_->StopReports(&context, request, &response);
    if(reqstatus.ok()) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_CALLBACK_DELAY));
                callback(telux::common::ErrorCode::SUCCESS);
            }
        }).share();
        taskQ_.add(f);
        if (filter_ != nullptr) {
            filter_->resetAllFilters();
        }
        sessionMask_ = 0;
        reportMask_ = 0;
        interval_ = 0;
    } else {
        LOG(ERROR, RPC_FAIL_SUFFIX, reqstatus.error_code());
    }
    return (telux::common::Status::SUCCESS);
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
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        if(sysInfoRequestCount_ == 0) {
            //Sending canned data on first invocation.
            struct LocationSystemInfo info;
            info.valid = 1;
            info.info.valid = 0;
            info.info.current = (uint8_t)0;
            invokeSystemInfoReport(info);
            sysInfoRequestCount_++;
        }
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
    if (status == telux::common::Status::SUCCESS) {
        auto f = std::async(std::launch::async, [=]() {
            if (callback && (cbDelay != SKIP_CALLBACK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(errorCode);
            }
        }).share();
        taskQ_.add(f);
        sysInfoRequestCount_--;
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
    if (status == telux::common::Status::SUCCESS) {
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
    if (status == telux::common::Status::SUCCESS) {
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
    if (status == telux::common::Status::SUCCESS) {
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
    if (status == telux::common::Status::SUCCESS) {
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
}

void LocationManagerStub::cleanup() {
}

void LocationManagerStub::parseRequest(std::string msg) {
    LOG(DEBUG, __FUNCTION__);
    std::stringstream ss(msg);
    std::vector<std::string> message;
    while(ss.good()) {
        std::string str;
        getline(ss, str, ',');
        message.push_back(str);
    }
    uint32_t opt = std::stoul(message[1]);
    switch(opt) {
        case telux::loc::GnssReportType::LOCATION :
        if(sessionMask_ & telux::loc::BASIC)
        {
            //1. Check TBF w.r.t UTC field and reject if outside the window.
            if (filter_ != nullptr) {
                uint64_t timestamp = telux::loc::UNKNOWN_TIMESTAMP;
                telux::loc::LocationInfoValidity validity = std::stoul(message[13]);
                if(validity & telux::loc::HAS_TIMESTAMP_BIT) {
                    timestamp = std::stoull(message[2]);
                }
                if (filter_->isReportIgnored(timestamp, ReportType::BASIC)) {
                    LOG(DEBUG, __FUNCTION__, " Report is filtered, hence not sending");
                    return;
                }
                //2. Update the timestamp
                uint64_t utcTimestamp;
                if(interval_ % 1000 == 0) {
                    utcTimestamp =
                        (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 1000) * 1000 ;
                } else {
                    utcTimestamp =
                        (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                }
                message[2] = std::to_string(utcTimestamp);
            } else {
                //2. Update the timestamp
                uint64_t utcTimestamp;
                utcTimestamp =
                        (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                message[2] = std::to_string(utcTimestamp);
            }

            //3. Parse.
            size_t itr = 2;
            std::shared_ptr<LocationInfoBase> loc = std::make_shared<LocationInfoBase>();
            loc->setUtcFixTime(std::stoull(message[itr++]));
            loc->setLocationTechnology(std::stoul(message[itr++]));
            loc->setLatitude(std::stod(message[itr++]));
            loc->setLongitude(std::stod(message[itr++]));
            loc->setAltitude(std::stod(message[itr++]));
            loc->setHeading(std::stof(message[itr++]));
            loc->setSpeed(std::stof(message[itr++]));
            loc->setHeadingUncertainty(std::stof(message[itr++]));
            loc->setSpeedUncertainty(std::stof(message[itr++]));
            loc->setHorizontalUncertainty(std::stof(message[itr++]));
            loc->setVerticalUncertainty(std::stof(message[itr++]));
            loc->setLocationInfoValidity(std::stoul(message[itr++]));
            loc->setElapsedRealTime(std::stoull(message[itr++]));
            loc->setElapsedRealTimeUncertainty(std::stoull(message[itr++]));
            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onBasicLocationUpdate(loc);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        } else if ((reportMask_ & telux::loc::GnssReportType::LOCATION) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            //1. Check TBF w.r.t UTC field and reject if outside the window.
            if (filter_ != nullptr) {
                uint64_t timestamp = telux::loc::UNKNOWN_TIMESTAMP;
                telux::loc::LocationInfoValidity validity = std::stoul(message[13]);
                if(validity & telux::loc::HAS_TIMESTAMP_BIT) {
                    timestamp = std::stoull(message[2]);
                }
                if(sessionMask_ & telux::loc::DETAILED) {
                    if (filter_->isReportIgnored(timestamp, ReportType::DETAILED)) {
                        LOG(DEBUG, __FUNCTION__, " Report is filtered, hence not sending");
                        return;
                    }
                } else {
                    if (filter_->isReportIgnored(timestamp, ReportType::DETAILED_ENG)) {
                        LOG(DEBUG, __FUNCTION__, " Report is filtered, hence not sending");
                        return;
                    }
                }
                //2. Update the timestamp
                uint64_t utcTimestamp;
                if(interval_ % 1000 == 0) {
                    utcTimestamp =
                        (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 1000) * 1000 ;
                } else {
                    utcTimestamp =
                        (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                }
                message[2] = std::to_string(utcTimestamp);
            } else {
                //2. Update the timestamp
                uint64_t utcTimestamp;
                utcTimestamp =
                        (((std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000) / 100) * 100 ;
                message[2] = std::to_string(utcTimestamp);
            }

            //Parse.
            size_t itr = 2;
            std::shared_ptr<LocationInfoEx> loc = std::make_shared<LocationInfoEx>();
            loc->setUtcFixTime(std::stoull(message[itr++]));
            loc->setLocationTechnology(std::stoul(message[itr++]));
            loc->setLatitude(std::stod(message[itr++]));
            loc->setLongitude(std::stod(message[itr++]));
            loc->setAltitude(std::stod(message[itr++]));
            loc->setHeading(std::stof(message[itr++]));
            loc->setSpeed(std::stof(message[itr++]));
            loc->setHeadingUncertainty(std::stof(message[itr++]));
            loc->setSpeedUncertainty(std::stof(message[itr++]));
            loc->setHorizontalUncertainty(std::stof(message[itr++]));
            loc->setVerticalUncertainty(std::stof(message[itr++]));
            loc->setLocationInfoValidity(std::stoul(message[itr++]));
            loc->setElapsedRealTime(std::stoull(message[itr++]));
            loc->setElapsedRealTimeUncertainty(std::stoull(message[itr++]));
            loc->setLocationInfoExValidity(std::stoull(message[itr++]));
            loc->setAltitudeMeanSeaLevel(std::stof(message[itr++]));
            loc->setPositionDop(std::stof(message[itr++]));
            loc->setHorizontalDop(std::stof(message[itr++]));
            loc->setVerticalDop(std::stof(message[itr++]));
            loc->setGeometricDop(std::stof(message[itr++]));
            loc->setTimeDop(std::stof(message[itr++]));
            loc->setMagneticDeviation(std::stof(message[itr++]));
            loc->setHorizontalReliability(
                static_cast<telux::loc::LocationReliability>(std::stoi(message[itr++])));
            loc->setVerticalReliability(
                static_cast<telux::loc::LocationReliability>(std::stoi(message[itr++])));
            loc->setHorizontalUncertaintySemiMajor(std::stof(message[itr++]));
            loc->setHorizontalUncertaintySemiMinor(std::stof(message[itr++]));
            loc->setHorizontalUncertaintyAzimuth(std::stof(message[itr++]));
            loc->setEastStandardDeviation(std::stof(message[itr++]));
            loc->setNorthStandardDeviation(std::stof(message[itr++]));
            loc->setNumSvUsed(std::stoul(message[itr++]));
            telux::loc::SvUsedInPosition svUsedInPosition;
            svUsedInPosition.gps = std::stoull(message[itr++]);
            svUsedInPosition.glo = std::stoull(message[itr++]);
            svUsedInPosition.gal = std::stoull(message[itr++]);
            svUsedInPosition.bds = std::stoull(message[itr++]);
            svUsedInPosition.qzss = std::stoull(message[itr++]);
            svUsedInPosition.navic = std::stoull(message[itr++]);
            loc->setSvUsedInPosition(svUsedInPosition);
            std::bitset<SBAS_COUNT> sbas = std::stoull(message[itr++]);
            loc->setSbasCorrection(sbas);
            loc->setPositionTechnology(std::stoul(message[itr++]));
            telux::loc::GnssKinematicsData bodyFrameData;
            bodyFrameData.latAccel = std::stof(message[itr++]);
            bodyFrameData.longAccel = std::stof(message[itr++]);
            bodyFrameData.vertAccel = std::stof(message[itr++]);
            bodyFrameData.yawRate = std::stof(message[itr++]);
            bodyFrameData.pitch = std::stof(message[itr++]);
            bodyFrameData.latAccelUnc = std::stof(message[itr++]);
            bodyFrameData.longAccelUnc = std::stof(message[itr++]);
            bodyFrameData.vertAccelUnc = std::stof(message[itr++]);
            bodyFrameData.yawRateUnc = std::stof(message[itr++]);
            bodyFrameData.pitchUnc = std::stof(message[itr++]);
            bodyFrameData.pitchRate = std::stof(message[itr++]);
            bodyFrameData.pitchRateUnc = std::stof(message[itr++]);
            bodyFrameData.roll = std::stof(message[itr++]);
            bodyFrameData.rollUnc = std::stof(message[itr++]);
            bodyFrameData.rollRate = std::stof(message[itr++]);
            bodyFrameData.rollRateUnc = std::stof(message[itr++]);
            bodyFrameData.yaw = std::stof(message[itr++]);
            bodyFrameData.yawUnc = std::stof(message[itr++]);
            bodyFrameData.bodyFrameDataMask = std::stoul(message[itr++]);
            loc->setBodyFrameData(bodyFrameData);
            loc->setTimeUncMs(std::stof(message[itr++]));
            loc->setLeapSeconds(std::stoul(message[itr++]));
            loc->setCalibrationConfidencePercent(std::stoul(message[itr++]));
            loc->setCalibrationStatus(std::stoul(message[itr++]));
            loc->setConformityIndex(std::stof(message[itr++]));
            telux::loc::LLAInfo llaVRPInfo = {0};
            llaVRPInfo.latitude = std::stod(message[itr++]);
            llaVRPInfo.longitude = std::stod(message[itr++]);
            llaVRPInfo.altitude = std::stod(message[itr++]);
            loc->setVRPBasedLLA(llaVRPInfo);
            std::vector<float> enuVelocity(3);
            enuVelocity[0] = std::stof(message[itr++]);
            enuVelocity[1] = std::stof(message[itr++]);
            enuVelocity[2] = std::stof(message[itr++]);
            loc->setVRPBasedENUVelocity(enuVelocity);
            loc->setAltitudeType(static_cast<telux::loc::AltitudeType>(std::stoi(message[itr++])));
            loc->setReportStatus(static_cast<telux::loc::ReportStatus>(std::stoi(message[itr++])));
            loc->setIntegrityRiskUsed(std::stoul(message[itr++]));
            loc->setProtectionLevelAlongTrack(std::stof(message[itr++]));
            loc->setProtectionLevelCrossTrack(std::stof(message[itr++]));
            loc->setProtectionLevelVertical(std::stof(message[itr++]));
            loc->setSolutionStatus(std::stoul(message[itr++]));
            size_t measInfoSize = std::stoi(message[itr++]);
            std::vector<GnssMeasurementInfo> measInfo;
            itr = 77;
            for(size_t i = 0; i < measInfoSize; i++) {
                telux::loc::GnssMeasurementInfo temp;
                temp.gnssSignalType = std::stoul(message[itr++]);
                temp.gnssConstellation =
                    static_cast<telux::loc::GnssSystem>(std::stoi(message[itr++]));
                temp.gnssSvId = std::stoul(message[itr++]);
                measInfo.push_back(temp);
            }
            loc->setMeasUsageInfo(measInfo);
            size_t enuVelocitySize = std::stoi(message[itr++]);
            std::vector<float> velocityEastNorthUp;
            for(size_t i = 0; i < enuVelocitySize; i++) {
                velocityEastNorthUp.push_back(std::stof(message[itr++]));
            }
            loc->setVelocityEastNorthUp(velocityEastNorthUp);
            size_t enuVelocityUncertainitySize = std::stoi(message[itr++]);
            std::vector<float> setVelocityEastNorthUpUnc;
            for(size_t i = 0; i < enuVelocityUncertainitySize; i++) {
                setVelocityEastNorthUpUnc.push_back(std::stof(message[itr++]));
            }
            loc->setVelocityUncertaintyEastNorthUp(setVelocityEastNorthUpUnc);
            size_t usedSVsize = std::stoi(message[itr++]);
            std::vector<uint16_t> usedSvs;
            for(size_t i = 0; i < usedSVsize; i++) {
                usedSvs.push_back(std::stoul(message[itr++]));
            }
            loc->setUsedSVsIds(usedSvs);
            telux::loc::GnssSystem system =
                static_cast<telux::loc::GnssSystem>(std::stoi(message[itr++]));
            if (system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS) {
                telux::loc::SystemTime time;
                time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GLONASS;
                time.time.glo.validityMask = std::stoul(message[itr++]);
                time.time.glo.gloDays = std::stoul(message[itr++]);
                time.time.glo.gloMsec = std::stoul(message[itr++]);
                time.time.glo.gloClkTimeBias = std::stof(message[itr++]);
                time.time.glo.gloClkTimeUncMs = std::stof(message[itr++]);
                time.time.glo.refFCount = std::stoul(message[itr++]);
                time.time.glo.numClockResets = std::stoul(message[itr++]);
                time.time.glo.gloFourYear = std::stoul(message[itr++]);
                loc->setGnssSystemTime(time);
            } else if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_SBAS) {
                telux::loc::SystemTime time;
                time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_SBAS;
                loc->setGnssSystemTime(time);
            } else {
                telux::loc::SystemTime time;
                if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS) {
                    time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GPS;
                }
                if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GALILEO) {
                    time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_GALILEO;
                }
                if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_BDS) {
                    time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_BDS;
                }
                if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_QZSS) {
                    time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_QZSS;
                }
                if(system == telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_NAVIC) {
                    time.gnssSystemTimeSrc = telux::loc::GnssSystem::GNSS_LOC_SV_SYSTEM_NAVIC;
                }
                time.time.gps.validityMask = std::stoul(message[itr++]);
                time.time.gps.numClockResets = std::stoul(message[itr++]);
                time.time.gps.refFCount = std::stoul(message[itr++]);
                time.time.gps.systemClkTimeUncMs = std::stof(message[itr++]);
                time.time.gps.systemClkTimeBias = std::stof(message[itr++]);
                time.time.gps.systemMsec = std::stoul(message[itr++]);
                time.time.gps.systemWeek = std::stoul(message[itr++]);
                loc->setGnssSystemTime(time);
            }


            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    if (sessionMask_ & telux::loc::DETAILED) {
                        spt->onDetailedLocationUpdate(loc);
                    } else {
                        std::vector<std::shared_ptr<ILocationInfoEx>> infoEngineReports;
                        infoEngineReports.push_back(loc);
                        spt->onDetailedEngineLocationUpdate(infoEngineReports);
                    }
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;

        case telux::loc::GnssReportType::SATELLITE_VEHICLE :
        if( (reportMask_ & telux::loc::GnssReportType::SATELLITE_VEHICLE) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            //Parse.
            std::shared_ptr<telux::loc::GnssSVInfo> gSV =
                std::make_shared<telux::loc::GnssSVInfo>();
            std::vector<std::shared_ptr<telux::loc::ISVInfo> > gnssSvList;
            size_t rowItr = 2;
            while ((rowItr + 11) < message.size()) {
                std::shared_ptr<telux::loc::SVInfo> svInfo = std::make_shared<SVInfo>();
                svInfo->setId(std::stoul(message[rowItr++]));
                svInfo->setConstellation(static_cast<telux::loc::GnssConstellationType>(
                    std::stoi(message[rowItr++])));
                svInfo->setHasEphemeris(static_cast<telux::loc::SVInfoAvailability>(
                    std::stoi(message[rowItr++])));
                svInfo->setHasAlmanac(static_cast<telux::loc::SVInfoAvailability>(
                    std::stoi(message[rowItr++])));
                svInfo->setHasFix(static_cast<telux::loc::SVInfoAvailability>(
                    std::stoi(message[rowItr++])));
                svInfo->setElevation(std::stof(message[rowItr++]));
                svInfo->setAzimuth(std::stof(message[rowItr++]));
                svInfo->setSnr(std::stof(message[rowItr++]));
                svInfo->setCarrierFrequency(std::stof(message[rowItr++]));
                svInfo->setSignalType(static_cast<telux::loc::GnssSignalType>(
                    std::stoull(message[rowItr++])));
                svInfo->setGlonassFcn(std::stoul(message[rowItr++]));
                svInfo->setBasebandCnr(std::stod(message[rowItr++]));

                if (svInfo != nullptr) {
                    gnssSvList.push_back(svInfo);
                } else {
                    LOG(ERROR, __FUNCTION__, " memory allocation failure for satVehicle");
                    return;
                }
            }
            //Send data to clients.
            if (gSV) {
                gSV->setAltitudeType(telux::loc::AltitudeType::UNKNOWN);
                gSV->setSVInfoList(gnssSvList);
                for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                    auto spt = (*iter).lock();
                    if (spt != nullptr) {
                        spt->onGnssSVInfo(gSV);
                        ++iter;
                    } else {
                        iter = listeners_.erase(iter);
                    }
                }
            } else {
                LOG(ERROR, __FUNCTION__, "memory allocation failure for gSV");
                return;
            }
        }
        break;

        case telux::loc::GnssReportType::NMEA :
        if( (reportMask_ & telux::loc::GnssReportType::NMEA) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            uint64_t timestamp =
                (std::chrono::high_resolution_clock::now().time_since_epoch().count()) / 1000000;
            std::string nmea = "";
            bool calcChecksum = false;
            // update the timestamp, format hhmmss.sss
            // nmea starts with $, end with *checksum
            //   1701338412905,4,1701338412903,$GNGSA,A,3,15,21,27,,,,,,,,,,2.0,1.7,0.9,3*3C
            // for NMEA ID of GNGGA, GNRMC, GNGNS, need to recalculate the checksum.
            // A checksum field is required and shall be transmitted in all sentences. The checksum
            // field is the last field in a sentence and follows the checksum delimiter "*".
            // The checksum is the 8-bit exclusive OR of all characters in the sentence,
            // including "," and "^" delimiters,
            // between but not including the "$" or "!" and "*" delimiters.
            if (0 == message[3].compare("$GNGGA") ||
                0 == message[3].compare("$GNRMC") ||
                0 == message[3].compare("$GNGNS")) {
                message[4] = CommonUtils::getCurrentTimeHHMMSS();
                calcChecksum = true;
                // erase the first '$'
                message[3].erase(message[3].begin());
                // erase the checksum at the end
                while (message.back().back() != '*') {
                    message.back().pop_back();
                }

                if (message.back().back() == '*') {
                    message.back().pop_back();
                } else {
                    // it is not a corrct format, error
                }
            }

            for(size_t itr = 3; itr < message.size(); itr++) {
                nmea += message[itr] + ", ";
            }
            //Remove the last ", ".
            nmea.pop_back();
            nmea.pop_back();

            if (calcChecksum) {
                nmea = "$" + nmea + "*" + std::to_string(CommonUtils::bitwiseXOR(nmea));
            }

            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onGnssNmeaInfo(timestamp, nmea);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;

        case telux::loc::GnssReportType::DATA :
        if( (reportMask_ & telux::loc::GnssReportType::DATA) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            //Parse.
            std::shared_ptr<GnssSignalInfo> gSI =
                std::make_shared<GnssSignalInfo>();
            telux::loc::GnssData gnssData = {};
            int rowItr = 2;
            for(auto i = 0;
                i < telux::loc::GnssDataSignalTypes::GNSS_DATA_MAX_NUMBER_OF_SIGNAL_TYPES; i++) {
                gnssData.gnssDataMask[i] = stoul(message[rowItr]);
                gnssData.jammerInd[i] = stod(message[rowItr + 1]);
                gnssData.agc[i] = stod(message[rowItr + 2]);
                rowItr = rowItr + 3;
            }
            if (gSI != nullptr) {
                gSI->setGnssData(gnssData);
            } else {
                LOG(ERROR, __FUNCTION__, " memory allocation failure for gSI");
                return;
            }
            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onGnssSignalInfo(gSI);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;

        case telux::loc::GnssReportType::MEASUREMENT :
        if( (reportMask_ & telux::loc::GnssReportType::MEASUREMENT) &&
            ((sessionMask_ & telux::loc::DETAILED) || (sessionMask_ & telux::loc::DETAILED_ENGINE)))
        {
            //Parse.
            telux::loc::GnssMeasurements gnssMeas;
            size_t rowItr = 2;
            gnssMeas.clock.valid = std::stoul(message[rowItr++]);
            gnssMeas.clock.leapSecond = std::stoul(message[rowItr++]);
            gnssMeas.clock.timeNs = std::stoull(message[rowItr++]);
            gnssMeas.clock.timeUncertaintyNs = std::stod(message[rowItr++]);
            gnssMeas.clock.fullBiasNs = std::stoull(message[rowItr++]);
            gnssMeas.clock.biasNs = std::stod(message[rowItr++]);
            gnssMeas.clock.biasUncertaintyNs = std::stod(message[rowItr++]);
            gnssMeas.clock.driftNsps = std::stod(message[rowItr++]);
            gnssMeas.clock.driftUncertaintyNsps = std::stod(message[rowItr++]);
            gnssMeas.clock.hwClockDiscontinuityCount = std::stoul(message[rowItr++]);
            while( (rowItr + 24) < message.size() - 1) {
                telux::loc::GnssMeasurementsData data;
                data.valid = std::stoul(message[rowItr++]);
                data.svId = std::stoul(message[rowItr++]);
                data.svType = static_cast<telux::loc::GnssConstellationType>(
                    std::stoi(message[rowItr++]));
                data.timeOffsetNs = std::stod(message[rowItr++]);
                data.stateMask = std::stoul(message[rowItr++]);
                data.receivedSvTimeNs = std::stoull(message[rowItr++]);
                data.receivedSvTimeSubNs = std::stof(message[rowItr++]);
                data.receivedSvTimeUncertaintyNs = std::stoull(message[rowItr++]);
                data.carrierToNoiseDbHz = std::stod(message[rowItr++]);
                data.pseudorangeRateMps = std::stod(message[rowItr++]);
                data.pseudorangeRateUncertaintyMps = std::stod(message[rowItr++]);
                data.adrStateMask = std::stoul(message[rowItr++]);
                data.adrMeters = std::stod(message[rowItr++]);
                data.adrUncertaintyMeters = std::stod(message[rowItr++]);
                data.carrierFrequencyHz = std::stof(message[rowItr++]);
                data.carrierCycles = std::stoull(message[rowItr++]);
                data.carrierPhase = std::stod(message[rowItr++]);
                data.carrierPhaseUncertainty = std::stod(message[rowItr++]);
                data.multipathIndicator = static_cast<
                  telux::loc::GnssMeasurementsMultipathIndicator>(std::stoi(message[rowItr++]));
                data.signalToNoiseRatioDb = std::stod(message[rowItr++]);
                data.agcLevelDb = std::stod(message[rowItr++]);
                data.gnssSignalType = std::stoul(message[rowItr++]);
                data.basebandCarrierToNoise = std::stod(message[rowItr++]);
                data.fullInterSignalBias = std::stod(message[rowItr++]);
                data.fullInterSignalBiasUncertainty = std::stod(message[rowItr++]);
                gnssMeas.measurements.push_back(data);
            }
            gnssMeas.isNHz = std::stoi(message[rowItr]);
            //Send data to clients.
            for (auto iter = listeners_.begin(); iter != listeners_.end();) {
                auto spt = (*iter).lock();
                if (spt != nullptr) {
                    spt->onGnssMeasurementsInfo(gnssMeas);
                    ++iter;
                } else {
                    iter = listeners_.erase(iter);
                }
            }
        }
        break;
        default :
            LOG(ERROR, __FUNCTION__, " No such report type supported");
            break;
    }
}

}  // namespace loc

}  // namespace telux
