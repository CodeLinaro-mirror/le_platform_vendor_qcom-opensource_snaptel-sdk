/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

/**
 * @file       ECallManager.cpp
 *
 * @brief      ECallManager class provides methods to initiate an eCall and answer an incoming call
 *             (typically PSAP callback). It manages various subsytems(location, audio, etc.) using
 *             Telematics-SDK, in order to handle the eCall appropriately.
 */

#include <iostream>

#include "ECallManager.hpp"

#define DEFAULT_ECALL_CONFIG_FILE_PATH "/etc"
#define DEFAULT_ECALL_CONFIG_FILE_NAME "eCall.conf"
#define DEFAULT_LOCATION_FIX_INTERVAL_MS 100
#define CLIENT_NAME "ECall-Manager: "

ECallManager::ECallManager()
    : telClient_(nullptr)
    , locClient_(nullptr)
    , audioClient_(nullptr)
    , thermClient_(nullptr)
    , phoneId_(-1)
    , locUpdateIntervalMs_(DEFAULT_LOCATION_FIX_INTERVAL_MS)
    , locFixReceived_(false)
    , audioDevice_(DeviceType::DEVICE_TYPE_SPEAKER)
    , voiceSampleRate_(16000)
    , voiceFormat_(AudioFormat::PCM_16BIT_SIGNED)
    , voiceChannels_(ChannelType::LEFT | ChannelType::RIGHT) {
}

ECallManager::~ECallManager() {
}

/**
 * Initialize necessary Telematics-SDK components like location, audio, etc. and get required
 * parameters from the configuration file
 */
telux::common::Status ECallManager::init() {
    telClient_ = std::make_shared<TelClient>();
    auto status = telClient_->init();
    if(status != telux::common::Status::SUCCESS) {
        return status;
    }

    locClient_ = std::make_shared<LocationClient>();
    locClient_->init();
    audioClient_ = std::make_shared<AudioClient>();
    audioClient_->init();
    thermClient_ = std::make_shared<ThermClient>();
    thermClient_->init();

    // Parse the eCall settings and fetch the static MSD data
    parseAppConfig();

    return telux::common::Status::SUCCESS;
}

/**
 * Function to trigger the standard eCall procedure(eg.112)
 */
telux::common::Status ECallManager::triggerECall(int phoneId, ECallCategory category,
                                                 ECallVariant variant, bool transmitMsd) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    if(telClient_->isECallInProgress()) {
        std::cout << CLIENT_NAME << "An ECall is in progress already " << std::endl;
        return telux::common::Status::FAILED;
    }
    phoneId_ = phoneId;
    setup(phoneId_);
    if(transmitMsd && !isLocationReceived()) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        if(std::cv_status::timeout
                == locUpdateCV_.wait_for(lock, std::chrono::milliseconds(locUpdateIntervalMs_))) {
                std::cout << CLIENT_NAME << "Error: Location fetch timeout! " << std::endl;
        }
    }
    auto status = telClient_->startECall(phoneId, msdData_, category, variant, transmitMsd,
                                         shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to initiate eCall " << std::endl;
        cleanup();
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "ECall initiated " << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Function to trigger a voice eCall procedure to the specified phone number
 */
telux::common::Status ECallManager::triggerECall(int phoneId, ECallCategory category,
                                                const std::string dialNumber, bool transmitMsd) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    if(telClient_->isECallInProgress()) {
        std::cout << CLIENT_NAME << "An ECall is in progress already " << std::endl;
        return telux::common::Status::FAILED;
    }
    phoneId_ = phoneId;
    setup(phoneId_);
    if(transmitMsd && !isLocationReceived()) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        if(std::cv_status::timeout
                == locUpdateCV_.wait_for(lock, std::chrono::milliseconds(locUpdateIntervalMs_))) {
                std::cout << CLIENT_NAME << "Error: Location fetch timeout! " << std::endl;
        }
    }
    auto status = telClient_->startECall(phoneId, msdData_, category, dialNumber, transmitMsd,
                                        shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to initiate Voice eCall " << std::endl;
        cleanup();
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Voice ECall initiated " << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Answer an incoming Call
 */
telux::common::Status ECallManager::answerCall(int phoneId) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << " Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    if(telClient_->isECallInProgress()) {
        std::cout << CLIENT_NAME << " An ECall is in progress already " << std::endl;
        return telux::common::Status::FAILED;
    }
    phoneId_ = phoneId;
    setup(phoneId);
    auto status = telClient_->answer(phoneId_, shared_from_this());
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to answer call " << std::endl;
        cleanup();
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Incoming call answered" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Hang-up an ongoing Call
 */
telux::common::Status ECallManager::hangupCall(int phoneId, int callIndex) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->hangup(phoneId, callIndex);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to hangup the call" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Call hang-up successful" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Dump the list of calls in progress
 */
telux::common::Status ECallManager::getCalls() {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->getCurrentCalls();
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to get current calls" << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Request status of various eCall HLAP timers
 */
telux::common::Status ECallManager::requestHlapTimerStatus(int phoneId) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->requestECallHlapTimerStatus(phoneId);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to send request for HLAP timers status" << std::endl;
        return telux::common::Status::FAILED;
    } else {
        std::cout << CLIENT_NAME << "Sent request for HLAP timers status" << std::endl;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Function to enable necessary functionalities in various subsystems(location, audio, etc.),
 * that are required for an eCall
 */
void ECallManager::setup(int phoneId) {
    // Start voice session
    if(!audioClient_) {
        std::cout << CLIENT_NAME << "Invalid Audio Client, cannot establish voice conversation"
                << std::endl;
    } else {
        audioClient_->startVoiceSession(phoneId, audioDevice_, voiceSampleRate_, voiceFormat_,
                                voiceChannels_);
    }
    // Get the location updates
    setLocationReceived(false);
    if(!locClient_) {
        std::cout << CLIENT_NAME << "Invalid Location Client, cannot provide current location"
                << std::endl;
    } else {
        locClient_->startLocUpdates(locUpdateIntervalMs_, shared_from_this());
    }
    // Disable Thermal auto-shutdown
    if(!thermClient_) {
        std::cout << CLIENT_NAME << "Invalid Thermal Client, cannot disable thermal auto-shutdown"
                << std::endl;
    } else {
        thermClient_->disableAutoShutdown();
    }
}

/**
 * Function to disable the functionalities in various subsystems(location, audio, etc.). Typically
 * performed when an eCall ends.
 */
void ECallManager::cleanup() {
    // Stop voice session
    if(!audioClient_) {
        std::cout << CLIENT_NAME << "Invalid Audio Client, cannot disable voice conversation"
            << std::endl;
    } else {
        audioClient_->stopVoiceSession();
    }
    // Get the location updates
    if(!locClient_) {
        std::cout << CLIENT_NAME << "Invalid Location Client, cannot stop location updates"
            << std::endl;
    } else {
        locClient_->stopLocUpdates();
    }
    // Enable Thermal auto-shutdown
    if(!thermClient_) {
        std::cout << CLIENT_NAME << "Invalid Thermal Client, cannot enable thermal auto-shutdown"
            << std::endl;
    } else {
        thermClient_->enableAutoShutdown();
    }
    phoneId_ = -1;
}

/**
 * Function to update the cached MSD data stored in Modem
 */
telux::common::Status ECallManager::updateMSD(int phoneId) {
    if(!telClient_) {
        std::cout << CLIENT_NAME << "Invalid Telephony Client" << std::endl;
        return telux::common::Status::FAILED;
    }
    auto status = telClient_->updateECallMSD(phoneId, msdData_);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << CLIENT_NAME << "Failed to update MSD " << std::endl;
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Function to indicate if atleast one location fix is received after the eCall is triggered.
 */
bool ECallManager::isLocationReceived() {
    std::unique_lock<std::mutex> lock(mutex_);
    return locFixReceived_;
}

void ECallManager::setLocationReceived(bool state) {
    std::unique_lock<std::mutex> lock(mutex_);
    locFixReceived_ = state;
}

/**
 * This function will be invoked whenever a new location-fix is received from the location client.
 */
void ECallManager::onLocationUpdate(ECallLocationInfo locInfo) {
    msdData_.vehicleLocation.positionLatitude = locInfo.latitude;
    msdData_.vehicleLocation.positionLongitude = locInfo.longitude;
    msdData_.timestamp = locInfo.timestamp;
    msdData_.vehicleDirection = locInfo.direction;
    if(telClient_->isECallInProgress()) {
        updateMSD(phoneId_);
    } else {
        setLocationReceived(true);
        locUpdateCV_.notify_all();
    }
}

/**
 * Function to parse the settings from the eCall configuration file and fetch the static MSD Data
 */
void ECallManager::parseAppConfig() {
    std::shared_ptr<ConfigParser> appSettings = std::make_shared<ConfigParser>(
                                    DEFAULT_ECALL_CONFIG_FILE_NAME, DEFAULT_ECALL_CONFIG_FILE_PATH);
    // Get the location of MSD data file and fetch the static MSD data
    std::string param = appSettings->getValue("MSD_FILE_NAME");
    if(!param.empty()) {
        MsdProvider msdSettings;
        std::string filePath = appSettings->getValue("MSD_FILE_PATH");
        msdSettings.init(param, filePath);
        msdData_ = msdSettings.getMsd();
    } else {
        std::cout << CLIENT_NAME << "MSD data file not found! " << std::endl;
    }
    // Get the periodic interval for which location updates needs to be received
    param = appSettings->getValue("LOCATION_UPDATE_INTERVAL_MS");
    if(!param.empty()) {
        locUpdateIntervalMs_ = atol(param.c_str());
    } else {
        std::cout << CLIENT_NAME << "Using default location update interval(in ms): " <<
            locUpdateIntervalMs_ << std::endl;
    }
    // Get the configured output audio device
    param = appSettings->getValue("AUDIO_OUTPUT_DEVICE_TYPE");
    if(param.compare("SPEAKER") == 0) {
        audioDevice_ = DeviceType::DEVICE_TYPE_SPEAKER;
    } else {
        std::cout << CLIENT_NAME << "Using default audio output device" << std::endl;
    }
    // Get the configured audio sample rate
    param = appSettings->getValue("VOICE_SAMPLE_RATE");
    if(!param.empty()) {
        voiceSampleRate_ = atol(param.c_str());
    } else {
        std::cout << CLIENT_NAME << "Using default audio sample rate: " << voiceSampleRate_
            << std::endl;
    }
    // Get the configured audio channels
    param = appSettings->getValue("VOICE_CHANNEL_TYPE");
    if(param.compare("LEFT") == 0) {
        voiceChannels_ = ChannelType::LEFT;
    } else if(param.compare("RIGHT") == 0) {
        voiceChannels_ = ChannelType::RIGHT;
    } else if(param.compare("STEREO") == 0) {
        voiceChannels_ = ChannelType::LEFT | ChannelType::RIGHT;
    } else {
        std::cout << CLIENT_NAME << "Using default audio channels: " << voiceChannels_ << std::endl;
    }
    // Get the configured audio sream format
    param = appSettings->getValue("VOICE_STREAM_FORMAT");
    if(param.compare("PCM_16BIT_SIGNED") == 0) {
        voiceFormat_ = AudioFormat::PCM_16BIT_SIGNED;
    } else {
        std::cout << CLIENT_NAME << "Using default audio stream format" << std::endl;
    }
}

/**
 * This function will be invoked when an eCall is failed to establish or an eCall is disconnected
 */
void ECallManager::onCallDisconnect() {
    cleanup();
}
