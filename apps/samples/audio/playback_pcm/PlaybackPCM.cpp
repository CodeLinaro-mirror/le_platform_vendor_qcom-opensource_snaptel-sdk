/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  Steps to play PCM audio samples on a audio sink device are:
 *
 *  1. Get a AudioFactory instance.
 *  2. Get a IAudioManager instance from AudioFactory.
 *  3. Wait for the audio service to become available.
 *  4. Create a playback stream (IAudioPlayStream).
 *  5. Start writing audio samples on the playback stream.
 *  6. When the playback is over, delete the playback stream.
 *
 * Usage:
 * # playback_pcm /data/musicfile.pcm
 *
 * Contents of /data/musicfile.pcm file are played on the speaker.
 */

#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "PlaybackPCM.hpp"

/*
 * Initialize application and get an audio service.
 */
telux::common::Status PlaybackPCM::init() {

    std::promise<telux::common::ServiceStatus> p{};
    telux::common::ServiceStatus serviceStatus;

    /* Step - 1 */
    auto &audioFactory = telux::audio::AudioFactory::getInstance();

    /* Step - 2 */
    audioManager_ = audioFactory.getAudioManager(
            [&p](telux::common::ServiceStatus status) {
        if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            p.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        } else {
            p.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
        }
    });

    if (!audioManager_) {
        std::cout << "Can't get IAudioManager" << std::endl;
        return telux::common::Status::FAILED;
    }

    /* Step - 3 */
    serviceStatus = audioManager_->getServiceStatus();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "audio service not ready, waiting..." << std::endl;
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "audio service unavailable" << std::endl;
            return telux::common::Status::FAILED;
        }
        std::cout << "audio service ready" << std::endl;
    }

    return telux::common::Status::SUCCESS;
}

/*
 * Step - 4, create a playback stream.
 */
telux::common::Status PlaybackPCM::createPlayStream() {

    std::promise<bool> p{};
    telux::common::Status status;
    telux::audio::StreamConfig sc;

    sc.type = telux::audio::StreamType::PLAY;
    sc.slotId = DEFAULT_SLOT_ID;
    sc.sampleRate = 48000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            audioPlayStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioPlayStream>(audioStream);
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create playback stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't create playback stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Step - 6, delete playback stream.
 */
telux::common::Status PlaybackPCM::deletePlayStream() {

    std::promise<bool> p{};
    telux::common::Status status;

    status = audioManager_-> deleteStream(audioPlayStream_, [&p, this] (
            telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete playback stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't delete playback stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Gets called to confirm how many bytes were actually written to the playback stream.
 */
void PlaybackPCM::writeCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    long offset;

    if ((error != telux::common::ErrorCode::SUCCESS) ||
            (buffer->getDataSize() != bytesWritten)) {
        offset = (-1) * (static_cast<long>((buffer->getDataSize() - bytesWritten)));
        fseek(fileToPlay_, offset, SEEK_CUR);
    }

    buffer->reset();
    freeBuffers_.push(buffer);
    cv_.notify_all();
}

/*
 *  Step - 5, write samples on the playback stream.
 */
void PlaybackPCM::play() {

    uint32_t size = 0;
    uint32_t numBytes = 0;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::unique_lock<std::mutex> lock(playMutex_);

    fileToPlay_ = std::fopen(fileToPlayPath_, "r");
    if (!fileToPlay_) {
        std::cout << "can't open file " << fileToPlayPath_ << std::endl;
        return;
    }

    for (int x = 0; x < 2; x++) {
        streamBuffer = audioPlayStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            fclose(fileToPlay_);
            return;
        }
        freeBuffers_.push(streamBuffer);

        size = streamBuffer->getMinSize();
        if (!size) {
            size =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(size);
    }

    auto writeCb = std::bind(&PlaybackPCM::writeCompletion, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::cout << "playback started" << std::endl;

    while(1) {
        streamBuffer = freeBuffers_.front();
        freeBuffers_.pop();

        numBytes = fread(streamBuffer->getRawBuffer(), 1, size, fileToPlay_);
        if (numBytes == 0 && feof(fileToPlay_)) {
            break;
        }
        if(numBytes != size && !feof(fileToPlay_)) {
            std::cout << "can't read required bytes, read " << numBytes << std::endl;
            break;
        }

        streamBuffer->setDataSize(numBytes);

        status = audioPlayStream_->write(streamBuffer, writeCb);
        if(status != telux::common::Status::SUCCESS) {
            std::cout << "can't write, err " << static_cast<unsigned int>(status) << std::endl;
            break;
        }

        if(freeBuffers_.empty()) {
            cv_.wait(lock);
        }
    }

    fclose(fileToPlay_);
    std::cout << "playback finished" << std::endl;
}

int main(int argc, char **argv) {

    telux::common::Status status;
    std::shared_ptr<PlaybackPCM> app;

    if (argc < 2) {
        std::cout << "need audio file absolute path" << std::endl;
        return -EINVAL;
    }

    app = std::make_shared<PlaybackPCM>();
    status = app->init();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    app->fileToPlayPath_ = argv[1];

    status = app->createPlayStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    std::thread playWorker(&PlaybackPCM::play, &(*app));
    playWorker.join();

    status = app->deletePlayStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    return 0;
}
