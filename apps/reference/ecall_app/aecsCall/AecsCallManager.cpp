/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "AecsCallManager.hpp"
#include "Utils.hpp"

#include <iostream>
#include <sstream>
#include <future>

#define DEFAULT_AECS_CONFIG_FILE_PATH "/etc"
#define DEFAULT_AECS_CONFIG_FILE_NAME "eCall.conf"

using telux::common::Status;
using telux::common::ServiceStatus;

AecsCallManager::AecsCallManager()
    : voiceSampleRate_(16000),
      voiceFormat_(AudioFormat::PCM_16BIT_SIGNED),
      voiceChannels_(ChannelType::LEFT | ChannelType::RIGHT),
      ecnrMode_(EcnrMode::ENABLE) {
}

AecsCallManager::~AecsCallManager() {
    audioClient_.reset();
    callMgr_.reset();
}

AecsCallManager &AecsCallManager::getInstance() {
    static AecsCallManager instance;
    return instance;
}

bool AecsCallManager::init() {

    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
    std::promise<telux::common::ServiceStatus> callMgrprom;

    // Get the PhoneFactory and CallManager instances.
    callMgr_ = phoneFactory.getCallManager([&](telux::common::ServiceStatus status) {
        callMgrprom.set_value(status);
    });
    if (!callMgr_) {
        std::cout << "ERROR - Failed to get CallManager instance \n";
        return false;
    }
    std::cout << "CallManager subsystem is not ready "
              << ", Please wait " << std::endl;
    ServiceStatus callMgrStatus = callMgrprom.get_future().get();

    if (callMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "CallManager subsystem is ready \n";
     } else {
        std::cout << "Unable to initialise CallManager subsystem " << std::endl;
        return false;
    }

    // ---------- SMS Manager init ----------
    int noOfSlots = MIN_SIM_SLOT_COUNT;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {
        noOfSlots = MAX_SIM_SLOT_COUNT;
    }

    smsMgrs_.clear();

    for (int index = 1; index <= noOfSlots; ++index) {
        std::promise<telux::common::ServiceStatus> prom;
        auto smsMgr = phoneFactory.getSmsManager(
            index,
            [&](telux::common::ServiceStatus status) { prom.set_value(status); });

        if (!smsMgr) {
            std::cout << "ERROR - Failed to get SMS Manager instance \n";
            return false;
        }

        std::cout << " Waiting for SMS Manager to be ready \n";
        telux::common::ServiceStatus smsMgrStatus = prom.get_future().get();
        if (smsMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "SMS Manager is ready \n";
            smsMgrs_.emplace(index, smsMgr);
        } else {
            std::cout << "ERROR - Unable to initialize SMS Manager \n";
            return false;
        }
    }

    // Init audio
    audioClient_ = std::make_shared<AudioClient>();
    Status audioStatus = audioClient_->init();
    if (audioStatus != Status::SUCCESS) {
        std::cout << "ERROR - AecsCallManager: Audio initialization failed\n";
        return false;
    }

    parseConfig();
    return true;
}

void AecsCallManager::parseConfig() {
    auto appSettings = std::make_shared<ConfigParser>(
        DEFAULT_AECS_CONFIG_FILE_NAME, DEFAULT_AECS_CONFIG_FILE_PATH);

    std::string param = appSettings->getValue("AUDIO_OUTPUT_DEVICE_TYPE");
    if (!param.empty()) {
        std::stringstream ss(param);
        int i = -1;
        audioDevices_.clear();
        std::cout << "AecsCallManager: Using audio devices: ";
        while (ss >> i) {
            audioDevices_.emplace_back(static_cast<DeviceType>(i));
            std::cout << i << " ";
            if (ss.peek() == ',') {
                ss.ignore();
            }
        }
        std::cout << std::endl;
    } else {
        std::cout << "AecsCallManager: Using default audio devices" << std::endl;
    }

    param = appSettings->getValue("VOICE_SAMPLE_RATE");
    if (!param.empty()) {
        voiceSampleRate_ = atol(param.c_str());
    } else {
        std::cout << "AecsCallManager: Using default sample rate "
                  << voiceSampleRate_ << std::endl;
    }

    param = appSettings->getValue("VOICE_CHANNEL_TYPE");
    if (param == "LEFT") {
        voiceChannels_ = ChannelType::LEFT;
    } else if (param == "RIGHT") {
        voiceChannels_ = ChannelType::RIGHT;
    } else if (param == "STEREO") {
        voiceChannels_ = ChannelType::LEFT | ChannelType::RIGHT;
    } else {
        std::cout << "AecsCallManager: Using default audio channels" << std::endl;
    }

    param = appSettings->getValue("VOICE_STREAM_FORMAT");
    if (param == "PCM_16BIT_SIGNED") {
        voiceFormat_ = AudioFormat::PCM_16BIT_SIGNED;
    } else {
        std::cout << "AecsCallManager: Using default audio format" << std::endl;
    }

    param = appSettings->getValue("ECNR_MODE");
    if (param == "DISABLE") {
        ecnrMode_ = EcnrMode::DISABLE;
    } else if (param == "ENABLE") {
        ecnrMode_ = EcnrMode::ENABLE;
    } else {
        std::cout << "AecsCallManager: Enabling ECNR by default" << std::endl;
    }

    param = appSettings->getValue("AECS_CALL_OR_MSD_FAIL_RETRY_INTERVAL");
    if (!param.empty()) {
        aecsRetryInterval_ = atol(param.c_str());
    } else {
        aecsRetryInterval_ = AECS_CALL_OR_MSD_RETRY_INTERVAL_MAX_S;
        std::cout << "AecsCallManager: Using default interval "
                  << aecsRetryInterval_ << std::endl;
    }

    param = appSettings->getValue("AECS_CALL_OR_MSD_FAIL_RETRY_DURATION");
    if (!param.empty()) {
        aecsRetryDuration_ = atol(param.c_str());
    } else {
        aecsRetryDuration_ = AECS_CALL_OR_MSD_RETRY_DURATION_MIN_S;
        std::cout << "AecsCallManager: Using default duration "
                  << aecsRetryDuration_ << std::endl;
    }

    param = appSettings->getValue("AECS_OEM_CALL_DROP_RETRY_INTERVAL");
    if (!param.empty()) {
        aecsOemRetryInterval_ = atol(param.c_str());
    } else {
        aecsOemRetryInterval_ = AECS_CALL_OR_MSD_RETRY_INTERVAL_MAX_S;
        std::cout << "AecsCallManager: Using default interval "
                  << aecsOemRetryInterval_ << std::endl;
    }

    param = appSettings->getValue("AECS_OEM_CALL_DROP_RETRY_DURATION");
    if (!param.empty()) {
        aecsOemRetryDuration_ = atol(param.c_str());
    } else {
        aecsOemRetryDuration_ = AECS_CALL_OR_MSD_RETRY_DURATION_MIN_S;
        std::cout << "AecsCallManager: Using default duration "
                  << aecsOemRetryDuration_ << std::endl;
    }
}

int AecsCallManager::getAecsRetryInterval() {
   return aecsRetryInterval_;
}

int AecsCallManager::getAecsRetryDuration() {
   return aecsRetryDuration_;
}

int AecsCallManager::getAecsOemRetryInterval() {
   return aecsOemRetryInterval_;
}

int AecsCallManager::getAecsOemRetryDuration() {
   return aecsOemRetryDuration_;
}

std::vector<telux::tel::PduBuffer> AecsCallManager::getRetryRawPdu() {
    return retryRawPdu_;
}

void AecsCallManager::setRetryRawPdu(std::vector<telux::tel::PduBuffer> rawPdus) {
    retryRawPdu_.clear();
    retryRawPdu_ = rawPdus;
}

// ---------------------- Audio ----------------------

void AecsCallManager::startAudio(int phoneId) {
    if (!audioClient_) {
        std::cout << "AecsCallManager: Invalid AudioClient, cannot start audio\n";
        return;
    }
    Status status = audioClient_->startVoiceSession(
        phoneId, audioDevices_[0], voiceSampleRate_,
        voiceFormat_, voiceChannels_, ecnrMode_);
    if (status != Status::SUCCESS) {
        std::cout << "AecsCallManager: audio start is failed\n";
    }
}

void AecsCallManager::stopAudioAll() {
    if (!audioClient_) {
        std::cout << "AecsCallManager: Invalid AudioClient, cannot stop audio\n";
        return;
    }
    Status status = audioClient_->stopVoiceSession();
    if (status != Status::SUCCESS) {
        std::cout << "AecsCallManager: audio stop is failed\n";
    }
}

void AecsCallManager::stopAudioIfNoCalls(int phoneId) {
    if (!callMgr_) {
        std::cout << "AecsCallManager: No CallManager, cannot check calls\n";
        return;
    }
    auto inProgressCalls = callMgr_->getInProgressCalls();
    int numCallsOnSlot = 0;
    for (auto &c : inProgressCalls) {
        if (c->getPhoneId() == phoneId &&
            c->getCallState() != telux::tel::CallState::CALL_ENDED) {
            numCallsOnSlot++;
        }
    }

    if (numCallsOnSlot == 0) {
        stopAudioAll();
    }
}

// ---------------------- Emergency mode ----------------------
// Callback which provides response for set emergency mode
void AecsCallManager::setEmergencyModeResponse(telux::common::ErrorCode error) {
    if (error == telux::common::ErrorCode::NO_EFFECT) {
        std::cout << "Emergency mode already set" << std::endl;
    } else if (error == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Successfully set emergency mode " << std::endl;
    } else {
        std::cout << "Failed to set emergency mode with error code: "
            << Utils::getErrorCodeAsString(error) << std::endl;
        return;
    }
}

Status AecsCallManager::setEmergencyMode(int phoneId, bool emergencyModeEnabled,
    bool antennaSwitchEnabled) {
    if (!callMgr_) {
        std::cout << "AecsCallManager: CallManager not initialized\n";
        return Status::FAILED;
    }

    Status status = callMgr_->setEmergencyMode(
        phoneId, emergencyModeEnabled, antennaSwitchEnabled,
        std::bind(&AecsCallManager::setEmergencyModeResponse, this, std::placeholders::_1));
    if (status == Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(emergencyModeMutex_);
        emergencyMode_[phoneId] = emergencyModeEnabled;
        std::cout << "AecsCallManager: set Emergency mode: " << emergencyModeEnabled
                  << ", antenna switching: "<< antennaSwitchEnabled
                  << " on phoneId " << phoneId << std::endl;
    } else {
        std::cout << "AecsCallManager: Failed to set emergency mode on phoneId "
                  << phoneId << ", status=" << (int)status << std::endl;
    }
    return status;
}

bool AecsCallManager::isEmergencyMode(int phoneId) const {
    std::lock_guard<std::mutex> lock(emergencyModeMutex_);
    auto it = emergencyMode_.find(phoneId);
    if (it == emergencyMode_.end()) {
        return false;
    }
    return it->second;
}

void AecsCallManager::setAecsCallDropStatus(bool aecsCallDrop) {
    aecsCallDrop_ = aecsCallDrop;
}

bool AecsCallManager::getAecsCallDropStatus() {
    return aecsCallDrop_;
}

void AecsCallManager::setAecsCallFailStatus(bool aecsCallFail) {
    aecsCallFail_ = aecsCallFail;
}

bool AecsCallManager::getAecsCallFailStatus() {
    return aecsCallFail_;
}
