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
 *  Steps to play audio samples on top of a voice call stream are:
 *  1. Get AudioFactory instance.
 *  2. Get IAudioManager instance from AudioFactory.
 *  3. Wait for the audio service to become available.
 *  4. Create voice call stream (IAudioVoiceStream).
 *  5. Start voice call stream.
 *  6. Create a capture stream (IAudioCaptureStream).
 *  7. Create playback stream (IAudioPlayStream).
 *  8. Start reading audio samples from hpcm capture stream.
 *  9. Start writing audio samples on hpcm playback stream.
 *  10. When the recording/playback is complete, delete the capture and playback streams.
 *  11. Stop voice call stream.
 *  12. Delete voice stream.
 *
 * Usage:
 * # hpcm duration(in seconds) sample_rate(in kHz)
 */

#include <errno.h>
#include <chrono>
#include <thread>
#include <future>
#include <iostream>
#include <condition_variable>

#include <telux/audio/AudioFactory.hpp>

#include "Hpcm.hpp"

Hpcm::Hpcm() {
    exit_ = true;
}

/*
 * Initialize application and get audio service.
 */
int Hpcm::init() {

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
        return -ENOMEM;
    }

    /* Step - 3 */
    serviceStatus = audioManager_->getServiceStatus();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "audio service not ready, waiting..." << std::endl;
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "audio service unavailable" << std::endl;
            return -EIO;
        }
        std::cout << "audio service ready" << std::endl;
    }

    return 0;
}

/*
 *  Step - 4, create a voice call stream with hpcm enabled.
 */
int Hpcm::createVoiceStream() {

    telux::common::ErrorCode ec;
    telux::common::Status status;
    telux::audio::StreamConfig sc{};
    std::promise<telux::common::ErrorCode> p{};

    sc.type = telux::audio::StreamType::VOICE_CALL;
    sc.slotId = DEFAULT_SLOT_ID;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;
    sc.enableHpcm = true;

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            audioVoiceStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioVoiceStream>(audioStream);
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

    return 0;
}

/*
 *  Step - 12, delete voice call stream.
 */
int Hpcm::deleteVoiceStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

    status = audioManager_-> deleteStream(audioVoiceStream_, [&p, this] (
            telux::common::ErrorCode result) {
            p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

/*
 *  Step - 5, start voice call stream.
 */
int Hpcm::startVoiceStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

    status = audioVoiceStream_->startAudio([&p] (telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't start voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed start voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    exit_ = false;
    return 0;
}

/*
 * Step - 11, stop voice call stream.
 */
int Hpcm::stopVoiceStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

    status = audioVoiceStream_->stopAudio([&p] (telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't stop voice stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed stop voice stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    exit_ = true;
    captureCv_.notify_all();
    bufferReadyCv_.notify_all();
    return 0;
}

/*
 * Step - 9, create a hpcm playback stream.
 */
int Hpcm::createHpcmPlayStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    telux::audio::StreamConfig sc{};
    std::promise<telux::common::ErrorCode> p{};

    sc.type = telux::audio::StreamType::PLAY;
    sc.slotId = DEFAULT_SLOT_ID;
    sc.sampleRate = sampleRate_;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
    /* Direction::TX indicates voice uplink playback */
    sc.voicePaths.emplace_back(telux::audio::Direction::TX);
    sc.enableHpcm = true;

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            audioPlayStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioPlayStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't create hpcm playback stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout<< "failed create hpcm playback stream,err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

/*
 * Step - 6, create a HPCM record stream.
 */
int Hpcm::createHpcmRecordStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    telux::audio::StreamConfig sc{};
    std::promise<telux::common::ErrorCode> p{};

    sc.type = telux::audio::StreamType::CAPTURE;
    sc.slotId = DEFAULT_SLOT_ID;
    sc.sampleRate = sampleRate_;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT | telux::audio::ChannelType::RIGHT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
    /* Direction::TX indicates voice uplink capture */
    sc.voicePaths.emplace_back(telux::audio::Direction::TX);
    sc.enableHpcm = true;

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            audioCaptureStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioCaptureStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't create capture stream, err " << static_cast<int>(status) << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout<< "failed create hpcm capture stream, err " << static_cast<int>(ec) <<
                    std::endl;
        return -EIO;
    }

    return 0;
}

/*
 *  Step - 10, delete hpcm playback stream.
 */
int Hpcm::deleteHpcmPlayStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

    status = audioManager_-> deleteStream(audioPlayStream_, [&p, this] (
            telux::common::ErrorCode result) {
            p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete hpcm playback stream, err " << static_cast<int>(status)
                  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete hpcm playback stream, err " << static_cast<int>(ec)
                  << std::endl;
        return -EIO;
    }

    return 0;
}

/*
 *  Step - 9, delete hpcm capture stream.
 */
int Hpcm::deleteHpcmRecordStream() {

    telux::common::Status status;
    telux::common::ErrorCode ec;
    std::promise<telux::common::ErrorCode> p{};

    status = audioManager_-> deleteStream(audioCaptureStream_, [&p, this] (
            telux::common::ErrorCode result) {
            p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't delete hpcm playback stream, err " << static_cast<int>(status)
                  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout<< "failed delete hpcm capture stream, err " << static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

/*
 *  Gets called whenever audio samples are read from the hpcm capture stream. The captured buffer is
 *  then passed to hpcm playback stream.
 */
void Hpcm::readCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {
    uint32_t bytesRead;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer = audioPlayStream_->getStreamBuffer();

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read failed, err: " << static_cast<int>(error) << std::endl;
        readErrorOccurred_ = true;
    } else {
        bytesRead = buffer->getDataSize();
        streamBuffer->setDataSize(bytesRead);
        std::cout << "bytes read: " << bytesRead << std::endl;
        memcpy(streamBuffer->getRawBuffer(), buffer->getRawBuffer(), bytesRead);
        freePlayBuffers_.push(streamBuffer);
        bufferReadyCv_.notify_all();
    }
    buffer->reset();
    freeCaptureBuffers_.push(buffer);
    captureCv_.notify_all();
}

/*
 *  Step - 7, read samples from the hpcm capture stream.
 */
void Hpcm::record() {

    uint32_t bytesToRead = 0;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::unique_lock<std::mutex> lock(captureMutex_);
    readErrorOccurred_ = false;

    try {
        recordingDurationMs_ = (std::stoul(recordingDuration_)) * 1000;
    } catch (...) {
        std::cout << "can't interpret time " << recordingDuration_ << std::endl;
        return;
    }

    for (int x = 0; x < 1; x++) {
        streamBuffer = audioCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            return;
        }
        freeCaptureBuffers_.push(streamBuffer);

        bytesToRead = streamBuffer->getMinSize();
        if (!bytesToRead) {
            bytesToRead =  streamBuffer->getMaxSize();
        }
        streamBuffer->setDataSize(bytesToRead);
    }

    auto readCb = std::bind(&Hpcm::readCompletion, this,
        std::placeholders::_1, std::placeholders::_2);

    std::cout << "HPCM recording started" << std::endl;

    auto startTime = std::chrono::steady_clock::now();

    while(1) {
        streamBuffer = freeCaptureBuffers_.front();
        if (!freeCaptureBuffers_.empty()) {
            freeCaptureBuffers_.pop();
        }

        if (streamBuffer) {
            status = audioCaptureStream_->read(streamBuffer, bytesToRead, readCb);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "can't read, err " << static_cast<int>(status) << std::endl;
                readErrorOccurred_ = true;
                break;
            }
        }

        if(freeCaptureBuffers_.empty()) {
            captureCv_.wait(lock);
        }

        auto currentTime = std::chrono::steady_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - startTime).count();

        if (diff >= recordingDurationMs_) {
            /* Let all initiated read complete, buffers saved to file */
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            break;
        }

        if (readErrorOccurred_ || exit_) {
            /* error occurred during recording, terminate the thread */
            break;
        }
    }

    exit_ = true;
    if (readErrorOccurred_) {
        std::cout << "recording finished with error" << std::endl;
    } else {
        std::cout << "recording finished" << std::endl;
    }
    /*If read operation returns with error then record thread will exit but play thread
    will be waiting for buffer. To avoid that notify play thread to exit. */
    bufferReadyCv_.notify_all();
}

/*
 *  Gets called to confirm how many bytes were actually written to stream.
 */
void Hpcm::writeCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    std::cout << "bytes played: " << bytesWritten << std::endl;
    if (!buffer) { std::cout << "Invalid buffer" << std::endl;  return; }

    if ((error != telux::common::ErrorCode::SUCCESS) ||
            (buffer->getDataSize() != bytesWritten)) {
        std::cout << "error in writting" << std::endl;
        writeErrorOccurred_ = true;
    }
}

/*
 *  Step - 8, This function waits for buffer to be read from hpcm capture stream and write samples
 *  on playback stream.
 */
void Hpcm::play() {

    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    writeErrorOccurred_ = false;

    auto writeCb = std::bind(&Hpcm::writeCompletion, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::cout << "HPCM playback started" << std::endl;

    while(1) {
        //waiting for hpcm read buffer to be ready
        std::unique_lock<std::mutex> lck(bufferReadyMutex_);
        bufferReadyCv_.wait(lck);
        if(exit_ || readErrorOccurred_){
            break;
        }

        streamBuffer = freePlayBuffers_.front();
        if (!freePlayBuffers_.empty()) {
            freePlayBuffers_.pop();
        }

        if (streamBuffer) {
            status = audioPlayStream_->write(streamBuffer, writeCb);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "can't write, err " << static_cast<unsigned int>(status) << std::endl;
                writeErrorOccurred_ = true;
                break;
            }
        }
    }

    if (writeErrorOccurred_) {
        std::cout << "Playback finished with error" << std::endl;
    } else if (readErrorOccurred_) {
        std::cout << "Capture finished with error, unable to play " << std::endl;
    } else {
        std::cout << "Playback finished" << std::endl;
    }
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<Hpcm> app;

    if (argc < 3) {
        std::cout << "Need time duration and sample rate to enable HPCM" << std::endl;
        return -EINVAL;
    }

    try {
        app = std::make_shared<Hpcm>();
    } catch (...) {
        std::cout << "can't allocate Hpcm" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    app->recordingDuration_ = argv[1];
    app->sampleRate_ = atoi(argv[2]);

    ret = app->createVoiceStream();
    if (ret < 0) {
        return ret;
    }

    ret = app->startVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->createHpcmRecordStream();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->createHpcmPlayStream();
    if (ret < 0) {
        app->deleteHpcmRecordStream();
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    std::thread recordWorker(&Hpcm::record, &(*app));
    std::thread playWorker(&Hpcm::play, &(*app));
    recordWorker.join();
    playWorker.join();

    ret = app->deleteHpcmRecordStream();
    if (ret < 0) {
        app->deleteHpcmPlayStream();
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->deleteHpcmPlayStream();
    if (ret < 0) {
        app->stopVoiceStream();
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->stopVoiceStream();
    if (ret < 0) {
        app->deleteVoiceStream();
        return ret;
    }

    ret = app->deleteVoiceStream();
    if (ret < 0) {
        return ret;
    }

    return 0;
}
