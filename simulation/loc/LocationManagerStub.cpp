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

#include <telux/common/CommonDefines.hpp>
#include "LocationManagerStub.hpp"
#include "StubHelper.hpp"
#include "Logger/Logger.hpp"

// This is used for computing energy consumed based on duration
#define ENERGY_CONSUMED_PER_SECOND 500

// Year of HW used as below
#define YEAR_OF_HW 0

namespace telux {

namespace loc {
//This denotes system start time and is used to arrive at elapsed duration for energy consumed info
static std::chrono::time_point<std::chrono::steady_clock> time_t0 =
    std::chrono::steady_clock::now();


void LocationManagerStub::invokeSystemInfoReport(ReportHandler & rClass_) {
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm *ltm = localtime(&now);
    // add 1 to avoid 00 hour reading
    sysInfoHourTime_ = ltm->tm_hour + 1;
    if (sysInfoHourTime_ != usedSysInfoHourTime_)
    {
        struct LocationSystemInfo info_ = rClass_.getSystemInfoReport();
        for (auto iter=systemInfoListener_.begin();iter != systemInfoListener_.end();) {
            auto spt = (*iter).lock();
            if (spt != nullptr) {
                Debug(__func__, "Sending System Info");
                spt->onLocationSystemInfo(info_);
                ++iter;
            } else {
                iter = systemInfoListener_.erase(iter);
            }
        }
        usedSysInfoHourTime_ = sysInfoHourTime_;
    }
}

void LocationManagerStub::invokeBasicReport(ReportHandler & rClass_) {
    struct ReportSeqNos seqNo_ = rClass_.getReportSeqNos();
    if (seqNo_.br > (brSeqNo_+ brSeqDelta_)) {
        std::shared_ptr<LocationInfoBase>ibase = rClass_.getLocationInfoBase();
        for (auto iter=listeners_.begin();iter != listeners_.end();) {
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

void LocationManagerStub::invokeDetailedReport(ReportHandler & rClass_) {
    struct ReportSeqNos seqNo_ = rClass_.getReportSeqNos();
    if (seqNo_.dr > (drSeqNo_+ drSeqDelta_)) {
        std::shared_ptr<LocationInfoEx>infoEx = rClass_.getLocationInfoEx();
        for (auto iter=listeners_.begin(); iter != listeners_.end(); ) {
            auto spt = (*iter).lock();
            if (spt != nullptr) {
                if (reportTypeMask_ & GnssReportType::LOCATION) {
                    spt->onDetailedLocationUpdate(infoEx);
                }
                if (reportTypeMask_ & GnssReportType::SATELLITE_VEHICLE) {
                    spt->onGnssSVInfo(rClass_.getGnssSVInfo());
                }
                if (reportTypeMask_ & GnssReportType::DATA) {
                    spt->onGnssSignalInfo(rClass_.getGnssSignalInfo());
                }
                if (reportTypeMask_ & GnssReportType::NMEA) {
                    std::vector<NMEAVals>& nmeaVals_ = rClass_.getNmeaVal();
                    for (auto iterNMEA = nmeaVals_.begin(); iterNMEA != nmeaVals_.end(); iterNMEA++ )
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

void LocationManagerStub::invokeDetailedEngineReport(ReportHandler & rClass_) {
    struct ReportSeqNos seqNo_ = rClass_.getReportSeqNos();
    if (seqNo_.der > (derSeqNo_+ derSeqDelta_)) {
        std::vector<std::shared_ptr<ILocationInfoEx>> infoEngineReports;
        std::shared_ptr<LocationInfoEx>infoEx = rClass_.getLocationInfoEx();
        infoEngineReports.push_back(infoEx);
        for (auto iter=listeners_.begin();iter != listeners_.end();) {
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
                    std::vector<NMEAVals>& nmeaVals_ = rClass_.getNmeaVal();
                    for (auto iterNMEA = nmeaVals_.begin(); iterNMEA != nmeaVals_.end(); iterNMEA++ )
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
    Debug(__FILE__,__func__);
    auto &rClass_ = ReportHandler::getInstance();
    while (exitThread_.load() == 0) {
        std::unique_lock<std::mutex> lk(rClass_.cv_m);
        rClass_.cv.wait(lk);
        {
            std::lock_guard<std::mutex> listenerLock(mutex_);
            if (listeners_.size() != 0) {
                if (Type_.load()==1) {
                    invokeBasicReport(rClass_);
                } else if (Type_.load()==2) {
                    invokeDetailedReport(rClass_);
                } else if (Type_.load()==3) {
                    invokeDetailedEngineReport(rClass_);
                }
           } else {
               //Debug(__func__, "No Listeners");
           }
           if (systemInfoListener_.size() != 0)
           {
               invokeSystemInfoReport(rClass_);
           }
        }
    }
    Debug(__func__, "Exiting Manager Thread");
    exited_.store(1);
}

LocationManagerStub::LocationManagerStub(telux::common::InitResponseCb
        callback) {
    Debug(__FILE__,__func__);
    systemStarter_ = std::make_shared<StubSystemStarter>(SubSystemType::LOCATION_MANAGER, callback);
    Type_.store(0);
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

    std::thread t(&telux::loc::LocationManagerStub::managerThread, this);
    t.detach();
}

std::future<bool> LocationManagerStub::onSubsystemReady() {
    Debug(__FILE__,__func__);
    return(systemStarter_->onSubSystemReady(SubSystemType::LOCATION_MANAGER));
}

bool LocationManagerStub::isSubsystemReady() {
    Debug(__FILE__,__func__);
    return(systemStarter_->isSubSystemReady(SubSystemType::LOCATION_MANAGER));
}

telux::common::ServiceStatus LocationManagerStub::getServiceStatus() {
    Debug(__FILE__,__func__);
    return(systemStarter_->getServiceStatus(SubSystemType::LOCATION_MANAGER));
}

telux::common::Status LocationManagerStub::registerListenerEx(std::weak_ptr<ILocationListener>
        listener) {
    Debug(__FILE__,__func__);
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        bool existing = 0;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                Debug(__func__, "Register Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            Debug(__func__, "Register Listener : Adding");
        }
    }
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::deRegisterListenerEx(std::weak_ptr<ILocationListener>
        listener) {
    Debug(__FILE__,__func__);
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = listeners_.erase(iter);
                Debug(__func__, "In deRegister Listener : Removing");
                retVal=telux::common::Status::SUCCESS;
                break;
            }
        }
    }
    return (retVal);
}

void locationResponseCallback(telux::common::ResponseCallback callback, telux::common::ErrorCode
        retValue, int delay) {
    Debug(__FILE__,__func__);
    if(callback) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        callback(retValue);
    }
}

telux::common::Status LocationManagerStub::startDetailedReports(uint32_t intervalInMs,
        telux::common::ResponseCallback callback, GnssReportTypeMask reportMask) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    if (intervalInMs <100){
        intervalInMs = 100;
    }
    drInterval_.store(intervalInMs);
    drSeqDelta_.store(intervalInMs * .01);
    drSeqNo_.store(0);
    //Type should be updated last after other variables are set
    Type_.store(2);
    reportTypeMask_ = reportMask;
    std::thread t(locationResponseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return(telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::startDetailedEngineReports(uint32_t intervalInMs,
        LocReqEngine engineType, telux::common::ResponseCallback callback,
            GnssReportTypeMask reportMask) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    if (intervalInMs <100){
        intervalInMs = 100;
    }
    derInterval_.store(intervalInMs);
    derSeqDelta_.store(intervalInMs * .01);
    derSeqNo_.store(0);
    //Type should be updated last after other variables are set
    Type_.store(3);
    reportTypeMask_ = reportMask;
    std::thread t(locationResponseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return(telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::startBasicReports(uint32_t distanceInMeters,
        uint32_t intervalInMs, telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    if (intervalInMs <100){
        intervalInMs = 100;
    }
    brInterval_.store(intervalInMs);
    brSeqDelta_.store(intervalInMs * .01);
    brSeqNo_.store(0);
    //Type should be updated last after other variables are set
    Type_.store(1);
    std::thread t(locationResponseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return(telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::registerForSystemInfoUpdates(
        std::weak_ptr<ILocationSystemInfoListener> listener,
            telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        bool existing = 0;
        for (auto iter=systemInfoListener_.begin(); iter<systemInfoListener_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                Debug(__func__, "System Info Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            systemInfoListener_.emplace_back(listener);
            Debug(__func__, "Registering SystemInfo Listener");
        }
    }
    std::thread t(locationResponseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::deRegisterForSystemInfoUpdates(
        std::weak_ptr<ILocationSystemInfoListener> listener,
            telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=systemInfoListener_.begin(); iter<systemInfoListener_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = systemInfoListener_.erase(iter);
                Debug(__func__, "Removing System Info Listener");
                retVal=telux::common::Status::SUCCESS;
                break;
            }
        }
    }
    std::thread t(locationResponseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (retVal);
}

void LocationManagerStub::requestEnergyConsumedCb(telux::common::ErrorCode retValue, int delay,
        telux::loc::GnssEnergyConsumedInfo energyConsumed) {
    Debug(__FILE__,__func__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    {
        std::lock_guard<std::mutex> lstnerEnergyLk(energyMutex_);
        cbStore_(energyConsumed, retValue);
        cbLock_ = false;
    }
}

telux::common::Status LocationManagerStub::requestEnergyConsumedInfo(GetEnergyConsumedCallback cb) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    {
        std::lock_guard<std::mutex> lstnerEnergyLk(energyMutex_);
        cbStore_ = cb;
        if (cbLock_ == false) {
            cbLock_ = true;
        }
        else
            return (telux::common::Status::SUCCESS);
    }
    auto t1 =  std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = t1 - time_t0;
    struct GnssEnergyConsumedInfo energyConsumed;
    energyConsumed.valid = 1;
    Debug(__func__, "Elapsed Duration : " + std::to_string(diff.count()));
    energyConsumed.energySinceFirstBoot = diff.count() * ENERGY_CONSUMED_PER_SECOND;
    cbStore_ = cb;
    std::thread t(&telux::loc::LocationManagerStub::requestEnergyConsumedCb, this,
        telux::common::ErrorCode::SUCCESS, delay, energyConsumed);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

void LocationManagerStub::getYearofHwCb(telux::common::ErrorCode retValue, int delay,
    uint16_t yearOfHw) {
    Debug(__FILE__,__func__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    cbYearOfHw_(yearOfHw, retValue);
}

telux::common::Status LocationManagerStub::getYearOfHw(GetYearOfHwCallback cb) {
    Debug(__FILE__,__func__);
    int delay;
    uint16_t yearOfHw = YEAR_OF_HW;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    cbYearOfHw_ = cb;
    std::thread t(&telux::loc::LocationManagerStub::getYearofHwCb, this,
        telux::common::ErrorCode::SUCCESS, delay, yearOfHw);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::stopReports(telux::common::ResponseCallback callback) {
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    Type_.store(0);
    std::thread t(locationResponseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

void LocationManagerStub::getTerrestrialPositionCb(int delay) {
    Debug(__FILE__,__func__);
    auto &rClass_ = ReportHandler::getInstance();
    std::shared_ptr<LocationInfoBase>ibase = rClass_.getLocationInfoBase();
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    std::lock_guard<std::mutex> terrestrialPositionLock(mutex_);
    if (isGetTerrestrialRequestActive_ == true) {
        cbTerrestrialPosition_(ibase);
        isGetTerrestrialRequestActive_ = false;
    } else {
        Debug(__func__, "Cancelling terrestrial callback");
    }
}

telux::common::Status LocationManagerStub::getTerrestrialPosition(uint32_t timeoutMsec,
        TerrestrialTechnology techMask, GetTerrestrialInfoCallback cb, telux::common
            ::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    std::lock_guard<std::mutex> terrestrialPositionLock(mutex_);
    isGetTerrestrialRequestActive_ = true;
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(locationResponseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    cbTerrestrialPosition_ = cb;
    // Adding an additional delay of 5000 ms to accommodate possible request from
    // cancelTerrestrialPositionRequest API.
    std::thread t1(&telux::loc::LocationManagerStub::getTerrestrialPositionCb, this, delay + 5000);
    t1.detach();
    return(telux::common::Status::SUCCESS);
}

telux::common::Status LocationManagerStub::cancelTerrestrialPositionRequest(
      telux::common::ResponseCallback callback) {
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::lock_guard<std::mutex> terrestrialPositionLock(mutex_);
    if (isGetTerrestrialRequestActive_ == true) {
        isGetTerrestrialRequestActive_ = false;
        std::thread t(locationResponseCallback, callback,
            telux::common::ErrorCode::SUCCESS, delay);
        t.detach();
    } else {
        std::thread t(locationResponseCallback, callback,
            telux::common::ErrorCode::INVALID_ARGUMENTS, delay);
        t.detach();
    }
    return (telux::common::Status::SUCCESS);
}

LocationManagerStub::~LocationManagerStub() {
    Debug(__FILE__,__func__);
    Debug(__func__, "In Manager Destructor");

    exitThread_.store(1);
    while (exited_.load() == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    Debug(__func__, "Coming out of Manager Destructor");

}

} //namespace loc

} //namespace telux
