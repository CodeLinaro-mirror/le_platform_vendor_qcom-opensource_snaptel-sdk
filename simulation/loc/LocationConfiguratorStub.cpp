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

#define SVELEVATION_MAXVALUE 90

#include "LocationConfiguratorStub.hpp"
#include "StubHelper.hpp"
#include "Logger/Logger.hpp"

namespace telux {
namespace loc {

LocationConfiguratorStub::LocationConfiguratorStub(telux::common::InitResponseCb callback) {
    Debug(__FILE__,__func__);
    static bool created = false;
    if (created == false) {
        created = true;
        systemStarter_ = std::make_shared<StubSystemStarter>(SubSystemType::LOCATION_CONFIGURATOR,
            callback);
    }

    //default initialization of member variables
    confgMinSVElevation_ = 0;
    confgMinGpsWeek_ = 0;
    confgRobustLocation_.validMask = 1;
    confgRobustLocation_.enabled = false;
    confgRobustLocation_.enabledForE911 = false;
    confgRobustLocation_.version.major = 0;
    confgRobustLocation_.version.minor = 1;
    confgCTuncEnabled_ = false;
    confgCTuncThreshold_ = 0.0f;
    confgCTuncEnergyBudget_ = 0;
    confgPaceEnabled_  = false;
}

std::future<bool> LocationConfiguratorStub::onSubsystemReady() {
    Debug(__FILE__,__func__);
    return(systemStarter_->onSubSystemReady(SubSystemType::LOCATION_CONFIGURATOR));
}

bool LocationConfiguratorStub::isSubsystemReady() {
    Debug(__FILE__,__func__);
    return(systemStarter_->isSubSystemReady(SubSystemType::LOCATION_CONFIGURATOR));
}

telux::common::ServiceStatus LocationConfiguratorStub::getServiceStatus() {
    Debug(__FILE__,__func__);
    return(systemStarter_->getServiceStatus(SubSystemType::LOCATION_CONFIGURATOR));
}

void responseCallback(telux::common::ResponseCallback callback, telux::common::ErrorCode
        retValue, int delay) {
    Debug(__FILE__,__func__);
    if(callback) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        callback(retValue);
    }
}

telux::common::Status LocationConfiguratorStub::configureCTunc(bool enable,
        telux::common::ResponseCallback callback, float timeUncertainty, uint32_t energyBudget) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    confgCTuncEnabled_ = enable;
    confgCTuncThreshold_ = timeUncertainty;
    confgCTuncEnergyBudget_ = energyBudget;
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::configurePACE(bool enable,
        telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    confgPaceEnabled_ = enable;
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::deleteAllAidingData(telux::common::ResponseCallback
        callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::configureLeverArm(const LeverArmConfigInfo& info,
        telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    confgLeverArmInfo_.insert(info.begin(), info.end());
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::configureConstellations(const SvBlackList& list,
        telux::common::ResponseCallback callback,  bool resetToDefault) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    if(!resetToDefault) {
        confgBlackList_.clear();
        //if list is empty then all will be enabled
        for(SvBlackListInfo info : list) {
            //insert only valid svId
            if(info.constellation == GnssConstellationType::GALILEO && info.svId >= 301 &&
                    info.svId <= 336) {
                confgBlackList_.push_back(info);
            } else if(info.constellation == GnssConstellationType::SBAS && ((info.svId >= 120 &&
                    info.svId<= 158) || (info.svId >= 183 && info.svId <= 191))) {
                confgBlackList_.push_back(info);
            } else if(info.constellation == GnssConstellationType::GLONASS && info.svId >= 65
                    && info.svId <= 96) {
                confgBlackList_.push_back(info);
            } else if(info.constellation == GnssConstellationType::BDS && info.svId >= 201
                    && info.svId <= 237) {
                confgBlackList_.push_back(info);
            } else if(info.constellation == GnssConstellationType::QZSS && info.svId >= 193
                    && info.svId <= 197) {
                confgBlackList_.push_back(info);
            }
        }
    } else {
        //By default none will be blacklisted
        confgBlackList_.clear();
    }
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::configureRobustLocation(bool enable,
        bool enableForE911, telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    confgRobustLocation_.enabled = enable;
    confgRobustLocation_.enabledForE911 = enableForE911;
    return (telux::common::Status::SUCCESS);
}

void requestRobustLocationCb(LocationConfiguratorStub::GetRobustLocationCallback cb,
        telux::common::ErrorCode retValue, int delay, RobustLocationConfiguration robustLoc) {
    Debug(__FILE__,__func__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    cb(robustLoc, retValue);
}

telux::common::Status LocationConfiguratorStub::requestRobustLocation(
        GetRobustLocationCallback cb) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(requestRobustLocationCb, cb, telux::common::ErrorCode::SUCCESS, delay,
        confgRobustLocation_);
    t.detach();
    return (telux::common::Status::SUCCESS);
}


telux::common::Status LocationConfiguratorStub::configureMinGpsWeek(uint16_t minGpsWeek,
        telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    confgMinGpsWeek_ = minGpsWeek;
    return (telux::common::Status::SUCCESS);
}

void requestMinGpsWeekCb(LocationConfiguratorStub::GetMinGpsWeekCallback cb,
        telux::common::ErrorCode retValue, int delay, uint16_t minGpsWeek) {
    Debug(__FILE__,__func__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    cb(minGpsWeek, retValue);
}

telux::common::Status LocationConfiguratorStub::requestMinGpsWeek(GetMinGpsWeekCallback cb) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(requestMinGpsWeekCb, cb, telux::common::ErrorCode::SUCCESS
        , delay, confgMinGpsWeek_);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::configureMinSVElevation(uint8_t minSVElevation,
        telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    if (minSVElevation <= SVELEVATION_MAXVALUE) {
        std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
        t.detach();
        confgMinSVElevation_ = minSVElevation;
        return (telux::common::Status::SUCCESS);
    }
    else {
        Error(__func__, "Invalid Parameter");
        return (telux::common::Status::INVALIDPARAM);
    }
}

void requestMinSVElevationCb(LocationConfiguratorStub::GetMinSVElevationCallback callback,
        telux::common::ErrorCode retValue, int delay, uint8_t configValue) {
    Debug(__FILE__,__func__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    callback(configValue, retValue);
}

telux::common::Status LocationConfiguratorStub::requestMinSVElevation(GetMinSVElevationCallback cb) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(requestMinSVElevationCb, cb, telux::common::ErrorCode::SUCCESS, delay,
        confgMinSVElevation_);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::configureSecondaryBand(const ConstellationSet& set,
        telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    configSet_ = set;
    return (telux::common::Status::SUCCESS);
}

void requestSecondaryBandConfigCb(LocationConfiguratorStub::GetSecondaryBandCallback callback,
        telux::common::ErrorCode retValue, int delay, const ConstellationSet& set) {
    Debug(__FILE__,__func__);
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    callback(set, retValue);
}

telux::common::Status LocationConfiguratorStub::requestSecondaryBandConfig(GetSecondaryBandCallback
        cb) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(requestSecondaryBandConfigCb, cb, telux::common::ErrorCode::SUCCESS, delay,
        configSet_);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::deleteAidingData(AidingData aidingDataMask,
        telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

// Currently DREngineConfiguration value passed in this API is not stored as there is request/get for this
telux::common::Status LocationConfiguratorStub::configureDR(const DREngineConfiguration& config,
        telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::configureEngineState(const EngineType engineType,
      const LocationEngineRunState engineState, telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::provideConsentForTerrestrialPositioning(
      bool userConsent, telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

telux::common::Status LocationConfiguratorStub::configureNmeaTypes(
      const NmeaSentenceConfig nmeaType, telux::common::ResponseCallback callback) {
    Debug(__FILE__,__func__);
    int delay;
    auto &s_stubbed =  StubHelper::getInstance();
    delay = s_stubbed.getCallbackDelay();
    std::thread t(responseCallback, callback, telux::common::ErrorCode::SUCCESS, delay);
    t.detach();
    return (telux::common::Status::SUCCESS);
}

LocationConfiguratorStub::~LocationConfiguratorStub() {}

} // namespace loc

} //namespace telux
