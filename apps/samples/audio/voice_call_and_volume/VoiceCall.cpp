/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * Steps to create a voice call audio stream and set the volume are:
 *
 * 1. Get an AudioFactory instance.
 * 2. Get an IAudioManager instance from the AudioFactory.
 * 3. Wait for the audio service to become available.
 * 4. Create a voice call stream (IAudioVoiceStream).
 * 5. Start voice call stream.
 * 6. Set volume of the playback stream.
 * 7. When the use-case is complete, stop the voice call stream.
 * 8. Delete voice call stream.
 *
 * Usage:
 * # voice_call_volume
 *
 * A voice call is established and volume of the playback stream (local speaker) is set.
 *
 * For establishing cellular RF path for voice call, telephony APIs should be used.
 */

#include <errno.h>

#include <cstdio>
#include <iostream>
#include <thread>

#include <telux/audio/AudioFactory.hpp>

#include "VoiceCall.hpp"

/*
 * Initialize application and get an audio service.
 */
int VoiceCall::init() {

    std::promise<telux::common::ServiceStatus> p{};
    telux::common::ServiceStatus serviceStatus;

    /* Step - 1 */
    auto &audioFactory = telux::audio::AudioFactory::getInstance();

    /* Step - 2 */
    audioManager_ = audioFactory.getAudioManager(
        [&p](telux::common::ServiceStatus srvStatus) { p.set_value(srvStatus); });

    if (!audioManager_) {
        std::cout << "Can't get IAudioManager" << std::endl;
        return -ENOMEM;
    }

    /* Step - 3 */
    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "audio service unavailable" << std::endl;
        return -EIO;
    }

    std::cout << "Initialization finished" << std::endl;
    return 0;
}

/*
 *  Step - 4, create a voice call stream.
 */
int VoiceCall::createVoiceStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type            = telux::audio::StreamType::VOICE_CALL;
    sc.slotId          = DEFAULT_SLOT_ID;
    sc.format          = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;

    /* For voice-call both sink and source device are required.
     * First device should be sink (speaker) and second should be source (mic). */
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);

    status = audioManager_->createStream(
        sc, [&p, this](std::shared_ptr<telux::audio::IAudioStream> &audioStream,
                telux::common::ErrorCode result) {
            if (result == telux::common::ErrorCode::SUCCESS) {
                audioVoiceStream_
                    = std::dynamic_pointer_cast<telux::audio::IAudioVoiceStream>(audioStream);
            }
            p.set_value(result);
        });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't create voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream created" << std::endl;
    return 0;
}

/*
 *  Step - 8, delete voice call stream.
 */
int VoiceCall::deleteVoiceStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_->deleteStream(
        audioVoiceStream_, [&p, this](telux::common::ErrorCode result) { p.set_value(result); });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream deleted" << std::endl;
    return 0;
}

/*
 *  Step - 5, start voice call stream.
 */
int VoiceCall::startVoiceStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioVoiceStream_->startAudio(
        [&p](telux::common::ErrorCode result) { p.set_value(result); });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't start voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed start voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream started" << std::endl;
    return 0;
}

/*
 * Step - 7, stop voice call stream.
 */
int VoiceCall::stopVoiceStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioVoiceStream_->stopAudio(
        [&p](telux::common::ErrorCode result) { p.set_value(result); });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't stop voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed stop voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Stream stopped" << std::endl;
    return 0;
}

/*
 *  Step - 6, set volume of the speaker.
 */
int VoiceCall::setSpeakerVolume() {

    telux::common::ErrorCode ec;
    telux::common::Status status;
    telux::audio::ChannelVolume channelVolume{};
    telux::audio::StreamVolume streamVol{};
    std::promise<telux::common::ErrorCode> p{};

    channelVolume.vol         = 0.6;
    channelVolume.channelType = telux::audio::ChannelType::LEFT;
    streamVol.volume.emplace_back(channelVolume);

    channelVolume.vol         = 0.6;
    channelVolume.channelType = telux::audio::ChannelType::RIGHT;
    streamVol.volume.emplace_back(channelVolume);

    status = audioVoiceStream_->setVolume(
        streamVol, [&p](telux::common::ErrorCode result) { p.set_value(result); });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't set volume, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed to set volume, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    std::cout << "Volume set" << std::endl;
    return 0;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<VoiceCall> app;

    try {
        app = std::make_shared<VoiceCall>();
    } catch (const std::exception &e) {
        std::cout << "can't allocate VoiceCall" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->createVoiceStream();
    if (ret < 0) {
        return ret;
    }

    ret = app->startVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->setSpeakerVolume();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    /* Application's business logic goes here. We are sleeping here just as an example */
    std::this_thread::sleep_for(std::chrono::minutes(1));

    ret = app->stopVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->deleteVoiceStream();
    if (ret < 0) {
        return ret;
    }

    std::cout << "Application exiting" << std::endl;
    return 0;
}
