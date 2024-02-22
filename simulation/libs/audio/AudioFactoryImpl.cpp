/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"

#include "AudioFactoryImpl.hpp"

namespace telux {
namespace audio {

AudioFactoryImpl::AudioFactoryImpl() {
}

AudioFactoryImpl::~AudioFactoryImpl() {
}

AudioFactory::AudioFactory() {

}

AudioFactory::~AudioFactory() {
}

AudioFactory &AudioFactoryImpl::getInstance() {
    static AudioFactoryImpl instance;
    return instance;
}

AudioFactory &AudioFactory::getInstance() {
    return AudioFactoryImpl::getInstance();
}

/*
 * Gives an instance of the AudioManagerImpl to the application.
 */
std::shared_ptr<IAudioManager> AudioFactoryImpl::getAudioManager(
        telux::common::InitResponseCb initResultListener) {

    std::thread appCallback;
    telux::common::Status status;
    telux::common::ServiceStatus serviceCurrentStatus;
    std::shared_ptr<AudioManagerImpl> audioMgr;

//TELSDK_FEATURE_AUDIO_ENABLED compiler flag used to control enabling of feature on target.
    std::lock_guard<std::mutex> lock(audioFactoryGuard_);

    audioMgr = audioMgr_.lock();
    if (audioMgr) {
        /* Audio manager already exist, respond based on current service status */
        serviceCurrentStatus = audioMgr->getServiceStatus();

        switch (serviceCurrentStatus) {
            case telux::common::ServiceStatus::SERVICE_FAILED:
                audioMgr = nullptr;
                break;
            case telux::common::ServiceStatus::SERVICE_AVAILABLE:
                appCallback = std::thread{[this] {
                    initCompleteNotifier(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                }};
                appCallback.detach();
                break;
            case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
                if (initResultListener) {
                    initCompleteCallbacks_.push_back(initResultListener);
                }
                break;
            default:
                LOG(ERROR, __FUNCTION__, " invalid service status");
                audioMgr = nullptr;
        };

        audioMgr_ = audioMgr;
        return audioMgr;
    }

    /* Audio manager doesn't exist, create and initialize one */

    auto initCb = [this](telux::common::ServiceStatus serviceStatus) {
        if (serviceStatus == telux::common::ServiceStatus::SERVICE_FAILED) {
            std::lock_guard<std::mutex> lock(audioFactoryGuard_);
            audioMgr_.reset();
        }
        this->initCompleteNotifier(serviceStatus);
    };

    try {
        audioMgr = std::make_shared<AudioManagerImpl>();
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create AudioManager");
        return nullptr;
    }

    status = audioMgr->init(initCb);
    if (status != telux::common::Status::SUCCESS) {
        return nullptr;
    }

    audioMgr_ = audioMgr;

    if (initResultListener) {
        initCompleteCallbacks_.push_back(initResultListener);
    }

    return audioMgr;
}

void AudioFactoryImpl::initCompleteNotifier(
        telux::common::ServiceStatus serviceNewStatus) {

    std::vector<telux::common::InitResponseCb> appCallbacks;

    {
        std::lock_guard<std::mutex> lock(audioFactoryGuard_);
        appCallbacks = initCompleteCallbacks_;
        initCompleteCallbacks_.clear();
    }

    for (const auto &callback : appCallbacks) {
        callback(serviceNewStatus);
    }
}

void AudioFactoryImpl::managerInitResult(telux::common::ServiceStatus status) {
   {
    std::lock_guard<std::mutex> initLock(managerInitGuard_);

    currentServiceStatus_ = status;
    serviceStatusReady_ = true;
    serviceStatusAvailable_.notify_all();
   }
}

/*
 * Provides AudioPlayerImpl instance to the application.
 */
telux::common::ErrorCode AudioFactoryImpl::getAudioPlayer(
        std::shared_ptr<IAudioPlayer>& audioPlayer) {

    bool waitResult = false;
    std::shared_ptr<IAudioPlayer> player;
    std::shared_ptr<IAudioManager> audioManager;
    telux::common::ServiceStatus serviceStatus;

    auto initCb = std::bind(&AudioFactoryImpl::managerInitResult,
        this, std::placeholders::_1);

    serviceStatusReady_ = false;
    audioManager = getAudioManager(initCb);
    if (!audioManager) {
        LOG(ERROR, __FUNCTION__, " can't get IAudioManager");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    serviceStatus = audioManager->getServiceStatus();

    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       {
        std::unique_lock<std::mutex> initLock(managerInitGuard_);

        /*
         * If the audio service doesn't become available within INIT_WAIT_TIME
         * seconds, timeout and let the application retry.
         */
        waitResult = serviceStatusAvailable_.wait_for(initLock,
            std::chrono::seconds(INIT_WAIT_TIME), [=]{return serviceStatusReady_;});
        if ((!waitResult) ||
             (currentServiceStatus_ != telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
            LOG(ERROR, __FUNCTION__, " audio service timedout/unavailable");
            return telux::common::ErrorCode::OPERATION_TIMEOUT;
        }
       }
    }

    try {
        player = std::make_shared<AudioPlayerImpl>(audioManager);
        audioPlayer = player;
    } catch (const std::exception& e) {
        LOG(ERROR, __FUNCTION__, " can't create AudioPlayerImpl");
        return telux::common::ErrorCode::NO_MEMORY;
    }

    return telux::common::ErrorCode::SUCCESS;
}

}  // end namespace audio
}  // end namespace telux
