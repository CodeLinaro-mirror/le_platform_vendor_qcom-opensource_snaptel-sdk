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
 *  Steps to record audio samples during an active voice call are:
 *
 *  1. Get a AudioFactory instance.
 *  2. Get a IAudioManager instance from AudioFactory.
 *  3. Wait for the audio service to become available.
 *  4. Create a voice call stream (IAudioVoiceStream).
 *  5. Start the voice call stream.
 *  6. Create a capture stream (IAudioCaptureStream).
 *  7. Start reading audio samples from capture stream.
 *  8. When we want to stop recording, delete the capture stream.
 *  9. Stop voice call stream.
 * 10. Delete voice call stream.
 *
 *  Usage:
 *  # in_call_record_pcm duration /data/incalloutout.pcm
 *
 *  Audio data sent from the remote end is recorded for given duration (in seconds)
 *  and saved in /data/incalloutout.pcm file. Voice call must be active (answered)
 *  between local and far end.
 */

#include <cstdio>
#include <chrono>
#include <thread>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "InCallRecordPCM.hpp"

/*
 * Initialize application and get an audio service.
 */
telux::common::Status InCallRecordPCM::init() {

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
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Step - 4, create a voice call stream.
 */
telux::common::Status InCallRecordPCM::createVoiceStream() {

    std::promise<bool> p{};
    telux::common::Status status;
    telux::audio::StreamConfig config;

    config.type = telux::audio::StreamType::VOICE_CALL;
    config.slotId = DEFAULT_SLOT_ID;
    config.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    config.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;

    status = audioManager_->createStream(config, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            audioVoiceStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioVoiceStream>(audioStream);
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create voice call stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't create voice call stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Step - 10, delete voice call stream.
 */
telux::common::Status InCallRecordPCM::deleteVoiceStream() {

    std::promise<bool> p{};
    telux::common::Status status;

    status = audioManager_-> deleteStream(audioVoiceStream_, [&p, this] (
            telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete voice call stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't delete voice call stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Step - 5, start voice call stream.
 */
telux::common::Status InCallRecordPCM::startVoiceStream() {

    std::promise<bool> p{};
    telux::common::Status status;

    status = audioVoiceStream_->startAudio([&p] (telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request start voice call stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't start voice call stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 * Step - 9, stop voice call stream.
 */
telux::common::Status InCallRecordPCM::stopVoiceStream() {

    std::promise<bool> p{};
    telux::common::Status status;

    status = audioVoiceStream_->stopAudio([&p] (telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request stop voice call stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't stop voice call stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 * Step - 6, create a incall-record stream.
 * Audio device is not specified. Voice downlink is specified.
 */
telux::common::Status InCallRecordPCM::createIncallRecordStream() {

    std::promise<bool> p{};
    telux::common::Status status;
    telux::audio::StreamConfig config;

    config.type = telux::audio::StreamType::CAPTURE;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = 48000;
    config.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;

    /* Direction::RX indicates voice downlink */
    config.voicePaths.emplace_back(telux::audio::Direction::RX);

    status = audioManager_->createStream(config, [&p, this] (
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
        std::cout << "can't request create incall-record stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't create incall-record stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Step - 8, delete capture stream.
 */
telux::common::Status InCallRecordPCM::deleteIncallRecordStream() {

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
        std::cout << "can't request delete incall-record stream"  << std::endl;
        return telux::common::Status::FAILED;
    }

    if (!(p.get_future().get())) {
        std::cout<< "can't delete incall-record stream" << std::endl;
        return telux::common::Status::FAILED;
    }

    return telux::common::Status::SUCCESS;
}

/*
 *  Gets called whenever audio samples are read from the capture stream.
 */
void InCallRecordPCM::readCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {

    uint32_t bytesRead, bytesWrittenToFile;

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read failed, err: " << static_cast<int>(error) << std::endl;
    } else {
        bytesRead = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(), 1, bytesRead, fileToSaveRecording_);
        if (bytesWrittenToFile != bytesRead) {
            std::cout << "can't write to file " << "written to file "
            << bytesWrittenToFile << "bytes read " << bytesRead << std::endl;
        }
    }

    buffer->reset();
    freeBuffers_.push(buffer);
    cv_.notify_all();
}

/*
 *  Step - 7, read samples from the capture stream.
 */
void InCallRecordPCM::record() {

    uint32_t bytesToRead = 0;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::unique_lock<std::mutex> lock(captureMutex_);

    try {
        recordingInterval_ = (std::stoul(recordingTimeLength_)) * 1000;
    } catch (const std::exception& e) {
        std::cout << "can't interpret time " << recordingTimeLength_ << std::endl;
        return;
    }

    fileToSaveRecording_ = std::fopen(fileToSaveRecordingPath_, "w");
    if (!fileToSaveRecording_) {
        std::cout << "can't open file " << fileToSaveRecordingPath_ << std::endl;
        return;
    }

    for (int x = 0; x < 2; x++) {
        streamBuffer = audioCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            fclose(fileToSaveRecording_);
            return;
        }
        freeBuffers_.push(streamBuffer);

        bytesToRead = streamBuffer->getMinSize();
        if (!bytesToRead) {
            bytesToRead =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(bytesToRead);
    }

    auto readCb = std::bind(&InCallRecordPCM::readCompletion, this,
        std::placeholders::_1, std::placeholders::_2);

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
        if (diff >= recordingInterval_) {
            break;
        }
    }

    fflush(fileToSaveRecording_);
    fclose(fileToSaveRecording_);
    std::cout << "audio recorded!" << std::endl;
}

int main(int argc, char **argv) {

    telux::common::Status status;
    std::shared_ptr<InCallRecordPCM> app;

    if (argc < 3) {
        std::cout << "need recording time and file to save recording" << std::endl;
        return -EIO;
    }

    app = std::make_shared<InCallRecordPCM>();
    status = app->init();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    app->recordingTimeLength_ = argv[1];
    app->fileToSaveRecordingPath_ = argv[2];

    status = app->createVoiceStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    status = app->startVoiceStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    status = app->createIncallRecordStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    std::thread recordWorker(&InCallRecordPCM::record, &(*app));
    recordWorker.join();

    status = app->deleteIncallRecordStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    status = app->stopVoiceStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    status = app->deleteVoiceStream();
    if (status != telux::common::Status::SUCCESS) {
        return -EIO;
    }

    return 0;
}
