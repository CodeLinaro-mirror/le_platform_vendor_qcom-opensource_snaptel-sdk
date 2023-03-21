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
 *  Steps to capture audio samples from an audio source are:
 *
 *  1. Get a AudioFactory instance.
 *  2. Get a IAudioManager instance from AudioFactory.
 *  3. Wait for the audio service to become available.
 *  4. Create a capture stream (IAudioCaptureStream).
 *  5. Start reading audio samples from capture stream.
 *  6. When required samples have been captured, delete the capture stream.
 *
 *  Usage:
 *  # capture_pcm duration /data/captured.pcm
 *
 *  Audio samples are captured for the given duration (in seconds) and saved
 *  in /data/captured.pcm.
 */

#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "CapturePCM.hpp"

/*
 * Initialize application and get an audio service.
 */
telux::common::Status CapturePCM::init() {

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
 * Step - 4, create a capture stream.
 */
telux::common::Status CapturePCM::createCaptureStream() {

    std::promise<bool> p{};
    telux::common::Status status;
    telux::audio::StreamConfig sc;

    sc.type = telux::audio::StreamType::CAPTURE;
    sc.slotId = DEFAULT_SLOT_ID;
    sc.sampleRate = 48000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            audioCaptureStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioCaptureStream>(audioStream);
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create capture stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't create capture stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Step - 6, delete capture stream.
 */
telux::common::Status CapturePCM::deleteCaptureStream() {

    std::promise<bool> p{};
    telux::common::Status status;

    status = audioManager_-> deleteStream(audioCaptureStream_, [&p, this] (
            telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete capture stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't delete capture stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Gets called whenever audio samples are read from the capture stream.
 */
void CapturePCM::readCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {

    uint32_t bytesRead, bytesWrittenToFile;

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read failed, err: " << static_cast<int>(error) << std::endl;
    } else {
        bytesRead = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(), 1, bytesRead, fileToSaveSamples_);
        if (bytesWrittenToFile != bytesRead) {
            std::cout << "can't write to file, " << "written "
            << bytesWrittenToFile << ", read " << bytesRead << std::endl;
        }
    }

    buffer->reset();
    freeBuffers_.push(buffer);
    cv_.notify_all();
}

/*
 *  Step - 5, read samples from the capture stream.
 */
void CapturePCM::capture() {

    uint32_t bytesToRead = 0;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::unique_lock<std::mutex> lock(captureMutex_);

    try {
        captureDurationMs_ = (std::stoul(captureDuration_)) * 1000;
    } catch (const std::exception& e) {
        std::cout << "can't interpret time " << captureDuration_ << std::endl;
        return;
    }

    fileToSaveSamples_ = std::fopen(fileToSaveSamplesPath_, "w");
    if (!fileToSaveSamples_) {
        std::cout << "can't open file " << fileToSaveSamplesPath_ << std::endl;
        return;
    }

    for (int x = 0; x < 2; x++) {
        streamBuffer = audioCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            fclose(fileToSaveSamples_);
            return;
        }

        freeBuffers_.push(streamBuffer);

        bytesToRead = streamBuffer->getMinSize();
        if (!bytesToRead) {
            bytesToRead =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(bytesToRead);
    }

    auto readCb = std::bind(&CapturePCM::readCompletion, this,
        std::placeholders::_1, std::placeholders::_2);

    std::cout << "capture started" << std::endl;

    auto startTime = std::chrono::steady_clock::now();

    while(1) {
        streamBuffer = freeBuffers_.front();
        freeBuffers_.pop();

        status = audioCaptureStream_->read(streamBuffer, bytesToRead, readCb);
        if(status != telux::common::Status::SUCCESS) {
            std::cout << "can't read, err " << static_cast<int>(status) << std::endl;
            break;
        }

        if(freeBuffers_.empty()) {
            cv_.wait(lock);
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime).count();

        if (diff >= captureDurationMs_) {
            /* Let all initiated read complete, buffers saved to file */
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            break;
        }
    }

    fflush(fileToSaveSamples_);
    fclose(fileToSaveSamples_);

    std::cout << "capture finished" << std::endl;
}

int main(int argc, char **argv) {

    telux::common::Status status;
    std::shared_ptr<CapturePCM> app;

    if (argc < 3) {
        std::cout << "need reading time and file path" << std::endl;
        return -EINVAL;
    }

    app = std::make_shared<CapturePCM>();
    status = app->init();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    app->captureDuration_ = argv[1];
    app->fileToSaveSamplesPath_ = argv[2];

    status = app->createCaptureStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    std::thread captureWorker(&CapturePCM::capture, &(*app));
    captureWorker.join();

    status = app->deleteCaptureStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    return 0;
}
