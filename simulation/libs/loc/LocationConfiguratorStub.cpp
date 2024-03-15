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
 *  Copyright (c) 2021, 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "LocationConfiguratorStub.hpp"
#include "../common/Logger.hpp"
#include "../common/JsonParser.hpp"
#include "../common/CommonUtils.hpp"

namespace telux {
namespace loc {

LocationConfiguratorStub::LocationConfiguratorStub() {
    LOG(DEBUG, __FUNCTION__);
    managerStatus_ = ServiceStatus::SERVICE_UNAVAILABLE;
}

std::future<bool> LocationConfiguratorStub::onSubsystemReady() {
  LOG(DEBUG, __FUNCTION__);
  auto f = std::async(std::launch::async, [&] {
    return waitForInitialization();
  });
  return f;
}

bool LocationConfiguratorStub::waitForInitialization() {
  LOG(DEBUG, __FUNCTION__);
  std::unique_lock<std::mutex> cvLock(mutex_);
  cv_.wait(cvLock);
  return isSubsystemReady();
}

bool LocationConfiguratorStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::ServiceStatus LocationConfiguratorStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lock(mutex_);
    return managerStatus_;
}

telux::common::Status LocationConfiguratorStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    auto f = std::async(std::launch::async,
    [this, callback]() {
        this->initSync(callback);
        }).share();
        taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

void LocationConfiguratorStub::initSync(telux::common::InitResponseCb callback) {
    int cbDelay = 100;
    Json::Value rootNode;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        ErrorCode errorCode = JsonParser::readFromJsonFile(rootNode, "api/loc/ILocationConfigurator.json");
        if(errorCode == ErrorCode::SUCCESS) {
            cbDelay = rootNode["ILocationConfigurator"]["SubSystemReadinessDelay"].asInt();
            managerStatus_ = rootNode["ILocationConfigurator"]["SubSystemInit"].asBool() == true
                ? ServiceStatus::SERVICE_AVAILABLE : ServiceStatus::SERVICE_FAILED;
        } else {
            LOG(ERROR, "Unable to read LocationConfigurator JSON");
        }
    }

    LOG(DEBUG, "Delay: ", cbDelay, " ServiceStatus: ", static_cast<int>(managerStatus_));

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    callback(managerStatus_);
    cv_.notify_all();
}

telux::common::Status LocationConfiguratorStub::configureCTunc(bool enable,
        telux::common::ResponseCallback callback, float timeUncertainty, uint32_t energyBudget) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(timeUncertainty),
                {"ILocationConfigurator", "CTunc", "timeUncertainty"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(energyBudget),
                {"ILocationConfigurator", "CTunc", "energyBudget"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configurePACE(bool enable,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(enable),
                {"ILocationConfigurator", "PACE", "enable"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::deleteAllAidingData(telux::common::ResponseCallback
        callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureLeverArm(const LeverArmConfigInfo& info,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            for(auto itr: info) {
                if(itr.first == LeverArmType::LEVER_ARM_TYPE_GNSS_TO_VRP) {
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.forwardOffset),
                        {"ILocationConfigurator", "LeverArm", "GNSSTOVRPforwardOffset"});
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.sidewaysOffset),
                        {"ILocationConfigurator", "LeverArm", "GNSSTOVRPsidewaysOffset"});
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.upOffset),
                        {"ILocationConfigurator", "LeverArm", "GNSSTOVRPupOffset"});
                }
                if(itr.first == LeverArmType::LEVER_ARM_TYPE_DR_IMU_TO_GNSS) {
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.forwardOffset),
                        {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSforwardOffset"});
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.sidewaysOffset),
                        {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSsidewaysOffset"});
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.upOffset),
                        {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSupOffset"});
                }
                if( (itr.first == LeverArmType::LEVER_ARM_TYPE_VEPP_IMU_TO_GNSS) ||
                    (itr.first == LeverArmType::LEVER_ARM_TYPE_VPE_IMU_TO_GNSS) ) {
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.forwardOffset),
                        {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSforwardOffset"});
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.sidewaysOffset),
                        {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSsidewaysOffset"});
                    CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(itr.second.upOffset),
                        {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSupOffset"});
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureConstellations(const SvBlackList& list,
        telux::common::ResponseCallback callback,  bool resetToDefault) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            std::string blacklist = "";
            for (auto itr : list) {
                blacklist += std::to_string(static_cast<int>(itr.constellation));
                blacklist += " : ";
                blacklist += std::to_string(itr.svId);
                blacklist += ", ";
            }
            if(!blacklist.empty()) {
                blacklist.pop_back();
                blacklist.pop_back();
            }
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", blacklist,
                {"ILocationConfigurator", "configureConstellations", "Blacklist"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureRobustLocation(bool enable,
        bool enableForE911, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(enable),
                {"ILocationConfigurator", "RobustLocation", "enable"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(enableForE911),
                {"ILocationConfigurator", "RobustLocation", "enableForE911"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestRobustLocation(
        GetRobustLocationCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        RobustLocationConfiguration rLConfig;
        if (errorCode == ErrorCode::SUCCESS) {
            rLConfig.enabled = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "RobustLocation", "enable"}));
            rLConfig.enabledForE911 = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "RobustLocation", "enableForE911"}));
            rLConfig.validMask = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "7",
                {"ILocationConfigurator", "RobustLocation", "validity"}));
            rLConfig.version.major = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "1",
                {"ILocationConfigurator", "RobustLocation", "majorversion"}));
            rLConfig.version.minor = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "2",
                {"ILocationConfigurator", "RobustLocation", "minorversion"}));

            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        cb(rLConfig, errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}


telux::common::Status LocationConfiguratorStub::configureMinGpsWeek(uint16_t minGpsWeek,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(minGpsWeek),
                {"ILocationConfigurator", "MinGpsWeek", "mingpsweek"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestMinGpsWeek(GetMinGpsWeekCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        uint16_t minGpsWeek = 0;
        if (errorCode == ErrorCode::SUCCESS) {
            minGpsWeek = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "MinGpsWeek", "mingpsweek"}));
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        cb(minGpsWeek, errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureMinSVElevation(uint8_t minSVElevation,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator",
                std::to_string(minSVElevation),
                    {"ILocationConfigurator", "MinSvElevation", "minSVElevation"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestMinSVElevation(GetMinSVElevationCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        uint8_t minSVElevation = 0;
        if (errorCode == ErrorCode::SUCCESS) {
            minSVElevation = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "MinSvElevation", "minSVElevation"}));
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        cb(minSVElevation, errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureSecondaryBand(const ConstellationSet& set,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            std::string secBandSet = "";
            for (auto itr : set) {
                int id = static_cast<int>(itr);
                secBandSet += std::to_string(id);
                secBandSet += ", ";
            }
            if(!secBandSet.empty()) {
                secBandSet.pop_back();
                secBandSet.pop_back();
            }
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", secBandSet,
                {"ILocationConfigurator", "SecondaryBand", "Set"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestSecondaryBandConfig(GetSecondaryBandCallback
        cb) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        ConstellationSet set;
        if (errorCode == ErrorCode::SUCCESS) {
            std::string str = CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "8",
                {"ILocationConfigurator", "SecondaryBand", "Set"});
            for(size_t itr = 0; itr < str.size(); itr++) {
                if(str[itr] >= '0' && str[itr] <= '8') {
                    int constel = (str[itr] - 48);
                    GnssConstellationType constelType;
                    switch(constel) {
                        case 1: constelType = GnssConstellationType::GPS;
                                break;
                        case 2: constelType = GnssConstellationType::GALILEO;
                                break;
                        case 3: constelType = GnssConstellationType::SBAS;
                                break;
                        case 5: constelType = GnssConstellationType::GLONASS;
                                break;
                        case 6: constelType = GnssConstellationType::BDS;
                                break;
                        case 7: constelType = GnssConstellationType::QZSS;
                                break;
                        case 8: constelType = GnssConstellationType::NAVIC;
                                break;
                    }
                    set.insert(constelType);
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        cb(set, errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::deleteAidingData(AidingData aidingDataMask,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(aidingDataMask),
                {"ILocationConfigurator", "DeleteAidingData", "aidingDataMask"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureDR(const DREngineConfiguration& config,
        telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.speedFactor),
                {"ILocationConfigurator", "configureDR", "speedFactor"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.speedFactorUnc),
                {"ILocationConfigurator", "configureDR", "speedFactorUnc"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.gyroFactor),
                {"ILocationConfigurator", "configureDR", "gyroFactor"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.gyroFactorUnc),
                {"ILocationConfigurator", "configureDR", "gyroFactorUnc"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.mountParam.rollOffset),
                {"ILocationConfigurator", "configureDR", "rollOffset"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.mountParam.yawOffset),
                {"ILocationConfigurator", "configureDR", "yawOffset"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.mountParam.pitchOffset),
                {"ILocationConfigurator", "configureDR", "pitchOffset"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.mountParam.offsetUnc),
                {"ILocationConfigurator", "configureDR", "offsetUnc"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(config.validMask),
                {"ILocationConfigurator", "configureDR", "validity"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureEngineState(const EngineType engineType,
      const LocationEngineRunState engineState, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(static_cast<int>(engineType)),
                {"ILocationConfigurator", "EngineState", "engineType"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(static_cast<int>(engineState)),
                {"ILocationConfigurator", "EngineState", "engineState"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::provideConsentForTerrestrialPositioning(
      bool ConsentForTerrestrialPositioning, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(ConsentForTerrestrialPositioning),
                {"ILocationConfigurator", "ConsentForTerrestrialPositioning", "Consent"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureNmeaTypes(
      const NmeaSentenceConfig nmeaType, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(nmeaType),
                {"ILocationConfigurator", "configureNmeaTypes", "sentenceConfig"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureNmea(const NmeaConfig configParams,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(static_cast<int>(configParams.sentenceConfig)),
                {"ILocationConfigurator", "configureNmeaTypes", "sentenceConfig"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(static_cast<int>(configParams.datumType)),
                {"ILocationConfigurator", "configureNmeaTypes", "datumType"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureEngineIntegrityRisk(
      const EngineType engineType, uint32_t integrityRisk,
          telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(static_cast<int>(engineType)),
                {"ILocationConfigurator", "configureEngineIntegrityRisk", "engineType"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(integrityRisk),
                {"ILocationConfigurator", "configureEngineIntegrityRisk", "integrityRisk"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureXtraParams(bool enable,
    const XtraConfig configParams, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(static_cast<int>(enable)),
                {"ILocationConfigurator", "XtraParams", "enable"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(configParams.downloadIntervalMinute),
                {"ILocationConfigurator", "XtraParams", "downloadIntervalMinute"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(configParams.downloadTimeoutSec),
                {"ILocationConfigurator", "XtraParams", "downloadTimeoutSec"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(configParams.downloadRetryIntervalMinute),
                {"ILocationConfigurator", "XtraParams", "downloadRetryIntervalMinute"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(configParams.downloadRetryAttempts),
                {"ILocationConfigurator", "XtraParams", "downloadRetryAttempts"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", configParams.caPath,
                {"ILocationConfigurator", "XtraParams", "caPath"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(configParams.isIntegrityDownloadEnabled),
                {"ILocationConfigurator", "XtraParams", "isIntegrityDownloadEnabled"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(configParams.integrityDownloadIntervalMinute),
                {"ILocationConfigurator", "XtraParams", "integrityDownloadIntervalMinute"});
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(static_cast<int>(configParams.daemonDebugLogLevel)),
                {"ILocationConfigurator", "XtraParams", "daemonDebugLogLevel"});
            std::string urls = "";
            for(auto url: configParams.serverURLs) {
                urls += url + ", ";
            }
            if(!urls.empty()) {
                urls.pop_back();
                urls.pop_back();
            }
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", urls,
                {"ILocationConfigurator", "XtraParams", "serverURLs"});
            std::string ntpurls = "";
            for(auto url: configParams.ntpServerURLs) {
                ntpurls += url + ", ";
            }
            if(!ntpurls.empty()) {
                ntpurls.pop_back();
                ntpurls.pop_back();
            }
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", ntpurls,
                {"ILocationConfigurator", "XtraParams", "ntpServerURLs"});

            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
        if(xtraEnabled_ != enable) {
            uint32_t indication =
                static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS);
            if( (registrationMask_ & (1 << indication)) ) {
                invokeXtraStatusUpdate();
            }
            xtraEnabled_ = enable;
        }
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::requestXtraStatus(GetXtraStatusCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        XtraStatus xtraStatus;
        if (errorCode == ErrorCode::SUCCESS) {
            xtraStatus.featureEnabled = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "XtraParams", "enable"}));
            int dataStatus = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "XtraParams", "xtraDataStatus"}));
            switch(dataStatus) {
                case 0: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_UNKNOWN;
                        break;
                case 1: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_NOT_AVAIL;
                        break;
                case 2: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_NOT_VALID;
                        break;
                case 3: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_VALID;
                        break;
            }
            xtraStatus.xtraValidForHours = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "XtraParams", "xtraValidForHours"}));
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        cb(xtraStatus, errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::registerListener(
    LocConfigIndications indicationList, std::weak_ptr<ILocationConfigListener> listener) {
    auto sp = listener.lock();
    if(sp == nullptr) {
        return telux::common::Status::INVALIDPARAM;
    }
    for(size_t itr = 0; itr < indicationList.size(); itr++) {
        if(indicationList.test(itr)) {
            registrationMap_[itr].insert(sp);
        }
    }
    if(indicationList.test(
        static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS))) {
        uint32_t indication =
            static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS);
        if( !(registrationMask_ & (1 << indication)) ) {
            invokeXtraStatusUpdate();
            //Updating the mask after the first registration.
            registrationMask_ |= (1 << indication);
        }
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status LocationConfiguratorStub::deRegisterListener(
    LocConfigIndications indicationList, std::weak_ptr<ILocationConfigListener> listener) {
    auto sp = listener.lock();
    if(sp == nullptr) {
        return telux::common::Status::INVALIDPARAM;
    }
    bool listenerExisted = false;
    for(size_t itr = 0; itr < indicationList.size(); itr++) {
        if(indicationList.test(itr)) {
            if(registrationMap_.find(itr) != registrationMap_.end()) {
                if(registrationMap_[itr].erase(sp)) {
                    listenerExisted = true;
                }
            }
        }
    }
    if(listenerExisted) {
        if(indicationList.test(
            static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS))) {
            uint32_t indication =
                static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS);
            std::vector<std::weak_ptr<ILocationConfigListener>> retList {};
            getAvailableListeners(indication, retList);
            if(retList.empty()) {
                //Resetting the indication to get the update for the next first registration.
                registrationMask_ ^= (1 << indication);
            }
        }
        return telux::common::Status::SUCCESS;
    } else {
        return telux::common::Status::NOSUCH;
    }
}

void LocationConfiguratorStub::getAvailableListeners(uint32_t indication,
    std::vector<std::weak_ptr<ILocationConfigListener>> &vec) {
    if(registrationMap_.find(indication) != registrationMap_.end()) {
        vec.assign(registrationMap_[indication].begin(), registrationMap_[indication].end());
    }
}

void LocationConfiguratorStub::invokeXtraStatusUpdate() {
    uint32_t indication =
            static_cast<uint32_t>(LocConfigIndicationsType::LOC_CONF_IND_XTRA_STATUS);
    XtraStatus xtraStatus;
    xtraStatus.featureEnabled = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
        {"ILocationConfigurator", "XtraParams", "enable"}));
    int dataStatus = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
        {"ILocationConfigurator", "XtraParams", "xtraDataStatus"}));
    switch(dataStatus) {
        case 0: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_UNKNOWN;
                break;
        case 1: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_NOT_AVAIL;
                break;
        case 2: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_NOT_VALID;
                break;
        case 3: xtraStatus.xtraDataStatus = XtraDataStatus::STATUS_VALID;
                break;
    }
    xtraStatus.xtraValidForHours = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
        {"ILocationConfigurator", "XtraParams", "xtraValidForHours"}));
    std::vector<std::weak_ptr<ILocationConfigListener>> retList {};
    getAvailableListeners(indication, retList);
    if(!retList.empty()) {
        for (auto listener : retList) {
            auto l = listener.lock();
            // Prevent accessing a dangling listener reference.
            if(l != nullptr) {
                l->onXtraStatusUpdate(xtraStatus);
            }
        }
    }
}

telux::common::Status LocationConfiguratorStub::injectMerkleTreeInformation(
    std::string merkleTreeInfo, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

telux::common::Status LocationConfiguratorStub::configureOsnma(bool enable,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    handleApiResponseForMethod("loc", "ILocationConfigurator");

    auto f = std::async(std::launch::async, [=]() {
        if (errorCode == ErrorCode::SUCCESS) {
            CommonUtils::writeSystemDataValue("loc/ILocationConfigurator", std::to_string(enable),
                {"ILocationConfigurator", "configureOsnma", "enable"});
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        }
        callback(errorCode);
    }).share();
    taskQ_.add(f);
    return status;
}

void LocationConfiguratorStub::cleanup() {

}

LocationConfiguratorStub::~LocationConfiguratorStub() {}

} // namespace loc

} //namespace telux
