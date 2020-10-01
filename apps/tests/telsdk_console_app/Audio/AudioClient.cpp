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

/**
 * @file       AudioClient.cpp
 *
 * @brief      AudioClient class provides methods to start and stop voice session.
 *             It manages the audio subsystem using Telematics-SDK APIs.
 */

#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "AudioClient.hpp"

#define FILE_PATH "/etc"
#define FILE_NAME "telsdk_app.conf"
#define DEFAULT_SAMPLE_RATE 16000
#define DEFAULT_CHANNEL_MASK 1
#define DEFAULT_DEVICE 1
#define DEFAULT_AUDIO_FORMAT 1

AudioClient::AudioClient()
    : audioMgr_(nullptr) {
}

AudioClient::~AudioClient() {
}

AudioClient &AudioClient::getInstance() {
   static AudioClient instance;
   return instance;
}

bool AudioClient::isReady() {
    return ready_;
}

// Initialize the audio subsystem
Status AudioClient::init() {
#ifdef AUDIO_SUPPORTED
    // Get the AudioFactory and AudioManager instances.
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    //  Get the AudioFactory and AudioManager instances.
    auto &audioFactory = AudioFactory::getInstance();
    audioMgr_ = audioFactory.getAudioManager();

    //  Check if audio subsystem is ready
    if (audioMgr_) {
        ready_ = audioMgr_->isSubsystemReady();
    } else {
        std::cout << "Invalid Audio Manager" << std::endl;
        return Status::FAILED;
    }
    //  If audio subsystem is not ready, wait for it to be ready
    if (!ready_) {
        std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
        std::future<bool> f = audioMgr_->onSubsystemReady();
        // If we want to wait unconditionally for audio subsystem to be ready
        ready_ = f.get();
    }

    //  Exit the application, if SDK is unable to initialize audio subsystems
    if (ready_) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Audio Subsystems to ready : " << elapsedTime.count() << "s"
                << std::endl;
        setActiveSession(DEFAULT_SLOT_ID);
        loadConfFileData();
    } else {
        std::cout << " *** ERROR - Unable to initialize audio subsystem" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
#else
    return Status::FAILED;
#endif
}

// Function to start an audio on voice call
void AudioClient::startVoiceSession(SlotId slotId) {
#ifdef AUDIO_SUPPORTED
    if (!audioMgr_) {
        std::cout << "Invalid Audio Manager" << std::endl;
        return;
    }
    setActiveSession(slotId);
    queryInputType();
    config_.slotId = slotId;
    config_.type = StreamType::VOICE_CALL;
    auto status = activeSession_->createStream(config_);
    if (status == Status::SUCCESS) {
        status = activeSession_->startAudio();
    }
    if (status == Status::SUCCESS) {
        std::cout << "Audio is enabled for call on slotId : " << slotId << std::endl;
    } else {
        std::cout << "Error in enabling audio on slotId : " << slotId << std::endl;
    }
    return;
#else
    return;
#endif
}

// Function to stop an active voice session
void AudioClient::stopVoiceSession(SlotId slotId) {
#ifdef AUDIO_SUPPORTED
    setActiveSession(slotId);
    auto status = activeSession_->stopAudio();
    if (status == Status::SUCCESS) {
        status = activeSession_->deleteStream();
    }
    if (status == Status::SUCCESS) {
        std::cout << "Audio is disabled for call on slotId : " << slotId << std::endl;
    } else {
        std::cout << "Error in disabling audio on slotId : " << slotId << std::endl;
    }
    return;
#else
    return;
#endif
}

void AudioClient::setActiveSession(SlotId slotId) {
#ifdef AUDIO_SUPPORTED
    std::lock_guard<std::mutex> lk(mutex_);
    if (!voiceSessions_.count(slotId)) {
        auto session = std::make_shared<VoiceSession>();
        voiceSessions_.insert(std::pair<SlotId, std::shared_ptr<VoiceSession>>(slotId, session));
    }
    activeSession_ = voiceSessions_[slotId];
#else
    return;
#endif
}

void AudioClient::loadConfFileData() {
    std::string input = "";
    ConfigParser parser(FILE_NAME, FILE_PATH);
    std::cout << "----- Default Parameters -----" << std::endl;
    try {
        input = parser.getValue("SAMPLE_RATE");
        config_.sampleRate = static_cast<uint32_t>(std::stoi(input));
        input = parser.getValue("AUDIO_FORMAT");
        config_.format= static_cast<AudioFormat>(std::stoi(input));
        input = parser.getValue("DEVICE_TYPE");
        DeviceType device = static_cast<DeviceType>(std::stoi(input));
        config_.deviceTypes.emplace_back(device);
        input = parser.getValue("CHANNEL_MASK");
        config_.channelTypeMask = static_cast<ChannelTypeMask>(std::stoi(input));
    } catch (const std::exception &e) {
        std::cout << "ERROR: "<< "Unable to read from file" << std::endl;
        std::cout << "Using default parameters" << std::endl;
        config_.sampleRate = DEFAULT_SAMPLE_RATE;
        config_.format = static_cast<AudioFormat>(DEFAULT_AUDIO_FORMAT);
        config_.deviceTypes.emplace_back(static_cast<DeviceType>(DEFAULT_DEVICE));
        config_.channelTypeMask = static_cast<ChannelTypeMask>(DEFAULT_CHANNEL_MASK);
    }
    std::cout << "The sample rate is " << config_.sampleRate << std::endl;
    std::cout << "The audio format is " << static_cast<int>(config_.format) << std::endl;
    std::cout << "The device is " << static_cast<int>(config_.deviceTypes[0]) << std::endl;
    std::cout << "Channel mask is " << static_cast<int>(config_.channelTypeMask) << std::endl;
    return;
}

void AudioClient::setMuteStatus(SlotId slotId, bool muteStatus) {
#ifdef AUDIO_SUPPORTED
    setActiveSession(slotId);
    std::string operationName = "";
    if (muteStatus) {
        operationName = "Mute";
    } else {
        operationName = "Unmute";
    }
    StreamMute mute{};
    mute.enable = muteStatus;
    mute.dir = StreamDirection::RX;
    auto status = activeSession_->setMute(mute);
    if (status == Status::SUCCESS) {
        mute.dir = StreamDirection::TX;
        status = activeSession_->setMute(mute);
    } else {
        std::cout << operationName << " failed on RX path on slotId " << slotId << std::endl;
    }
    if (status == Status::SUCCESS) {
        std::cout << operationName << " operation Succeded on slotId " << slotId << std::endl;
    } else {
        std::cout << operationName << " failed on TX path on slotId " << slotId << std::endl;
    }
#else
    return;
#endif
}

void AudioClient::queryInputType() {
#ifdef AUDIO_SUPPORTED
    std::string inputSelection;
    char delimiter = '\n';
    int consoleFlag = 0;
    std::cout << "Enter 0 to specify audio parameters, press 1 to use default: ";
    std::getline(std::cin, inputSelection, delimiter);
    if (!inputSelection.empty()) {
        try {
            consoleFlag = std::stoi(inputSelection);
            if (consoleFlag < 0 || consoleFlag > 1) {
                std::cout << "ERROR: Invalid selection" << std::endl;
                return;
            }
        } catch (const std::exception &e) {
            std::cout << "ERROR: "<< e.what() << std::endl;
            return;
        }
    } else {
        std::cout << "Empty input, enter correct choice" << std::endl;
        return;
    }
    if (!consoleFlag) {
        AudioHelper::getUserSampleRateInput(config_.sampleRate);
        AudioHelper::getUserChannelInput(config_.channelTypeMask);
        AudioHelper::getAudioFormatInput(config_.format);
        AudioHelper::getUserDeviceInput(config_.deviceTypes);
    }
    return;
#else
    return;
#endif
}
