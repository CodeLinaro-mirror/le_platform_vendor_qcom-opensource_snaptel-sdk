/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <string>
#include <iomanip>
#include <ios>
#include <sstream>
#include <thread>
#include <inttypes.h>

#include "NtnClient.hpp"
#include <telux/satcom/SatcomFactory.hpp>
#ifdef TELSDK_FEATURE_LOC_ENABLED
#include <telux/loc/LocationFactory.hpp>
#endif

#define DEFAULT_REPORT_MASK 0
#define DEFAULT_REPORT_INTERVAL 1000

/**
 * @file: NtnClient.cpp
 *
 * @brief: Perform ntn operations.
 */

NtnClient::NtnClient()
   : ntnMgr_(nullptr) {
}

NtnClient::~NtnClient() {
}

telux::common::Status NtnClient::init() {
    LOGFD();
    if (ntnMgr_ == nullptr) {
        if (!initSatcom()) {
            LOGFE("Ntn manager init failed");
            return telux::common::Status::FAILED;
        } else {
            LOGFD("Ntn manager init success");
        }
    }

#ifdef TELSDK_FEATURE_LOC_ENABLED
    if (locationManager_ == nullptr) {
        if (!initLocationManager()) {
            LOGFE("Location manager init failed");
            return telux::common::Status::FAILED;
        } else {
            LOGFD("Location manager init success");
        }
    }
#endif
    return telux::common::Status::SUCCESS;
}

bool NtnClient::initSatcom() {
    if (ntnMgr_ == nullptr) {
        auto &satcomFactory              = telux::satcom::SatcomFactory::getInstance();
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        ntnMgr_                          = satcomFactory.getNtnManager(
            [&](telux::common::ServiceStatus status) { prom.set_value(status); });
        if (ntnMgr_ == nullptr) {
            LOGFE("satcomFactory.getNtnManager returned nullptr");
            return false;
        }
        ServiceStatus ntnMgrStatus = ntnMgr_->getServiceStatus();
        if (ntnMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LOGFD("Ntn subsystem is not ready, Please wait");
        }
        ntnMgrStatus = prom.get_future().get();
        if (ntnMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOGFD("*** Ntn subsystem is ready ***");
        } else {
            LOGFE("*** Ntn subsystem is not ready ***");
            return false;
        }
    }
    return true;
}

#ifdef TELSDK_FEATURE_LOC_ENABLED
bool NtnClient::initLocationManager() {
    bool locSubsystemReady = true;
    if (locationManager_ == nullptr) {
        std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
        auto &locationFactory            = LocationFactory::getInstance();
        locationManager_ = locationFactory.getLocationManager([&](ServiceStatus status) {
            if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
            } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
            }
        });
        std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
        startTime                  = std::chrono::system_clock::now();
        ServiceStatus locMgrStatus = locationManager_->getServiceStatus();
        if (locMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
            LOGFD("Location subsystem is not ready, Please wait");
        }
        locMgrStatus = prom.get_future().get();
        if (locMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
            endTime                                   = std::chrono::system_clock::now();
            std::chrono::duration<double> elapsedTime = endTime - startTime;
            LOGFD("Elapsed Time for Subsystems to ready: %f", elapsedTime.count());
        } else {
            LOGFE("ERROR - Unable to initialize Location subsystem");
            locSubsystemReady = false;
        }

        posListener_ = std::make_shared<NtnClientLocationListener>();
        locationManager_->registerListenerEx(posListener_);
    }
    return locSubsystemReady;
}
#endif

void NtnClient::registerForUpdates() {
    LOGFD();
    Status status = ntnMgr_->registerListener(shared_from_this());
    if (status != Status::SUCCESS) {
        LOGFE("Failed to register for ntn notification");
    } else {
        LOGFD("Registered Listener for ntn notification");
    }
}

void NtnClient::deregisterForUpdates() {
    LOGFD();
    Status status = ntnMgr_->deregisterListener(shared_from_this());
    if (status != Status::SUCCESS) {
        LOGFE("Failed to deregister for ntn notification");
    } else {
        LOGFD("Deregistered Listener");
    }
}

telux::common::ErrorCode NtnClient::enableNtn() {
    LOGFD();
    int emergency        = 0;
    std::string iccid    = "";
    ConfigParser *config = ConfigParser::getInstance();
    if (config->getValue("NTN_CONFIGS", "EMERGENCY") == "TRUE") {
        emergency = 1;
    }
    iccid = config->getValue("NTN_CONFIGS", "ICCID");
    return ntnMgr_->enableNtn(true, emergency, iccid);
}

void NtnClient::onNtnStateChange(NtnState state) {
    LOGFD("**** onNtnStateChange = %s", toString(state).c_str());
}

std::string NtnClient::toString(NtnState state) {
    switch (state) {
        case NtnState::DISABLED:
            return "DISABLED";
        case NtnState::OUT_OF_SERVICE:
            return "OUT_OF_SERVICE";
        case NtnState::IN_SERVICE:
            return "IN_SREVICE";
    }
    return "-";
}

void NtnClient::onSignalStrengthChange(SignalStrength newStrength) {
    LOGFD("**** onSignalStrengthChange = %s", toString(newStrength).c_str());
}

std::string NtnClient::toString(SignalStrength ss) {
    switch (ss) {
        case SignalStrength::NONE:
            return "NONE";
        case SignalStrength::POOR:
            return "POOR";
        case SignalStrength::MODERATE:
            return "MODERATE";
        case SignalStrength::GOOD:
            return "GOOD";
        case SignalStrength::GREAT:
            return "GREAT";
    }
    return "-";
}

void NtnClient::onCapabilitiesChange(NtnCapabilities capabilities) {
    LOGFD("**** onCapabilitiesChange maxDataSize = %s", toString(capabilities).c_str());
}

std::string NtnClient::toString(NtnCapabilities cap) {
    return std::to_string(cap.maxDataSize);
}

void NtnClient::onNtnBandUpdate(uint32_t bandValue) {
    LOGFD("**** onNtnBandUpdate BandValue = %" PRIu32, bandValue);
}

void NtnClient::onLocationFixRequest(LocationFixRequestReason reqReason) {
    LOGFD("**** onLocationFixRequest Reason = %s", toString(reqReason).c_str());

    std::thread([this] {
#ifdef TELSDK_FEATURE_LOC_ENABLED
        triggerLocationReports();
#endif
    }).detach();
}

std::string NtnClient::toString(LocationFixRequestReason reqReason) {
    switch (reqReason) {
        case LocationFixRequestReason::NORMAL:
            return "NORMAL";
        case LocationFixRequestReason::VALIDITY_TIMER_EXPIRED:
            return "VALIDITY_TIMER_EXPIRED";
        case LocationFixRequestReason::UNKOWN:
            return "UNKOWN";
    }
    return "-";
}

#ifdef TELSDK_FEATURE_LOC_ENABLED
void NtnClient::triggerLocationReports() {
    LOGFD();
    if (locationManager_ && posListener_) {
        GnssReportTypeMask reportMask = DEFAULT_REPORT_MASK;
        reportMask |= LOCATION;
        auto startStatus
            = locationManager_->startDetailedReports(DEFAULT_REPORT_INTERVAL, nullptr, reportMask);
        if (startStatus != telux::common::Status::SUCCESS) {
            LOGFE("Failed to start location reports");
            return;
        }
        {
            LOGFD("Waiting for location reports");
            std::unique_lock<std::mutex> lck(posListener_->getLocationMutex());
            posListener_->getLocationCV().wait(
                lck, [this] { return posListener_->isReportReceived_; });
        }
        locationManager_->stopReports(nullptr);
        {
            std::unique_lock<std::mutex> lck(posListener_->getLocationMutex());
            posListener_->isReportReceived_ = false;
            posListener_->reportCount_      = 0;
        }
        LOGFD("Stopping reports");
        auto err = ntnMgr_->locationFixResponse(telux::satcom::LocationStatus::SUCCESS, 0);
        LOGFD("locationFixResponse err = %s", Utils::getErrorCodeAsString(err).c_str());

        err = ntnMgr_->setLocationFix(posListener_->locFix_);
        LOGFD("setLocationFix err = %s", Utils::getErrorCodeAsString(err).c_str());
    }
}

void NtnClientLocationListener::onDetailedLocationUpdate(
    const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
    if (reportCount_ > 0) {
        return;
    }
    std::cout << " Detailed reports received" << std::endl;
    LOGFD();
    locFix_.lat           = locationInfo->getLatitude();
    locFix_.lon           = locationInfo->getLongitude();
    locFix_.alt           = locationInfo->getAltitude();
    locFix_.uncerCircular = locationInfo->getHorizontalUncertainty();
    locFix_.heading       = locationInfo->getHeading();
    locFix_.headingUncer  = locationInfo->getHeadingUncertainty();
    std::vector<float> velocityEastNorthUp;
    locationInfo->getVelocityEastNorthUp(velocityEastNorthUp);
    size_t itr = 0;
    for (auto vel : velocityEastNorthUp) {
        locFix_.velInfo.enuVel[itr] = vel;
        itr++;
    }
    std::vector<float> velocityUncertaintyEastNorthUp;
    locationInfo->getVelocityUncertaintyEastNorthUp(velocityUncertaintyEastNorthUp);
    itr = 0;
    for (auto vel : velocityUncertaintyEastNorthUp) {
        locFix_.velInfo.enuUncer[itr] = vel;
        itr++;
    }
    telux::loc::LocationInfoValidity validityMask     = locationInfo->getLocationInfoValidity();
    telux::loc::LocationInfoExValidity validityMaskEx = locationInfo->getLocationInfoExValidity();
    if ((validityMask & telux::loc::HAS_HEADING_BIT)) {
        locFix_.isHeadingValid = true;
    } else {
        locFix_.isHeadingValid = false;
    }
    if ((validityMask & telux::loc::HAS_HEADING_ACCURACY_BIT)) {
        locFix_.isHeadingUncerValid = true;
    } else {
        locFix_.isHeadingUncerValid = false;
    }
    if ((validityMask & telux::loc::HAS_HORIZONTAL_ACCURACY_BIT)) {
        locFix_.isConfidenceValid = true;
        locFix_.confidence        = 68;
    } else {
        locFix_.isConfidenceValid = false;
        locFix_.confidence        = 0;
    }
    if ((validityMaskEx & telux::loc::HAS_NORTH_VEL) && (validityMaskEx & telux::loc::HAS_EAST_VEL)
        && (validityMaskEx & telux::loc::HAS_UP_VEL)) {
        locFix_.velInfo.isEnuValueValid = true;
    } else {
        locFix_.velInfo.isEnuValueValid = false;
    }
    if ((validityMaskEx & telux::loc::HAS_NORTH_VEL_UNC)
        && (validityMaskEx & telux::loc::HAS_EAST_VEL_UNC)
        && (validityMaskEx & telux::loc::HAS_UP_VEL_UNC)) {
        locFix_.velInfo.isEnuUncerValid = true;
    } else {
        locFix_.velInfo.isEnuUncerValid = false;
    }
    {
        std::unique_lock<std::mutex> lck(this->getLocationMutex());
        isReportReceived_ = true;
        reportCount_++;
        this->getLocationCV().notify_all();
    }
}
#endif

telux::common::Status NtnClient::sendDataString(std::string text) {
    LOGFD();
    int emergency        = 0;
    ConfigParser *config = ConfigParser::getInstance();
    if (config->getValue("NTN_CONFIGS", "IS_EMERGENCY_DATA") == "TRUE") {
        emergency = 1;
    }
    std::vector<uint8_t> data;
    for (char c : text) {
        data.push_back((uint8_t)c);
    }
    TransactionId tId;
    LOGFD("Data of size: %zu", data.size());
    auto ret = ntnMgr_->sendData(data.data(), text.size(), emergency, tId);
    LOGFD("sendData tId = %" PRIu32, tId);
    return ret;
}

void NtnClient::onDataAck(ErrorCode err, TransactionId id) {
    LOGFD();
    if (err == ErrorCode::SUCCESS) {
        LOGFD("**** onDataAck ack received for id = %" PRIu32, id);
    } else {
        LOGFD("**** onDataAck error = %s id = %" PRIu32, Utils::getErrorCodeAsString(err).c_str(),
            id);
    }
}

void NtnClient::cleanup() {
    deregisterForUpdates();
    LOGFD();
    if (ntnMgr_) {
        ntnMgr_ = nullptr;
    }
#ifdef TELSDK_FEATURE_LOC_ENABLED
    if (locationManager_ && posListener_) {
        locationManager_->deRegisterListenerEx(posListener_);
        posListener_     = nullptr;
        locationManager_ = nullptr;
    }
#endif
}
