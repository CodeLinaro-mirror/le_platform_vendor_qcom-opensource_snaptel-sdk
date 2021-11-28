/*
 *  Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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


#include <future>
#include <iostream>
#include <condition_variable>

#include <telux/audio/AudioFactory.hpp>
#include "CompressedFmtPlaybackApp.hpp"

using std::promise;
using namespace telux::common;
using namespace telux::audio;


const uint32_t SAMPLE_RATE = 48000;
// Below constant represents number of buffers allocated to pass Bitstream. Additional number of
// buffers would provide flexibility in copying Bitsream from source and write to Stream Interface
// in two parallel threaded operations.
const int TOTAL_BUFFERS = 1;

AmrPlaybackApp::AmrPlaybackApp() {
    pipeLineEmpty_ = true;
}

AmrPlaybackApp::~AmrPlaybackApp() {
}

Status AmrPlaybackApp::init() {
    // Get the AudioFactory and AudioManager instances.
    std::promise<ServiceStatus> prom{};
    auto &audioFactory = AudioFactory::getInstance();
    audioManager_ = audioFactory.getAudioManager([&prom](ServiceStatus status) {
        if (status == ServiceStatus::SERVICE_AVAILABLE) {
            prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
        } else {
            prom.set_value(ServiceStatus::SERVICE_FAILED);
        }
    });
    if (!audioManager_) {
        std::cout << "Failed to get AudioManager object" << std::endl;
        return Status::FAILED;
    }
    //  Check if audio subsystem is ready
    //  If audio subsystem is not ready, wait for it to be ready
    ServiceStatus managerStatus = audioManager_->getServiceStatus();
    if (managerStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
        managerStatus = prom.get_future().get();
    }

    if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Subsystem is ready." << std::endl;
    } else {
        std::cout << "Audio Subsystem is NOT ready." << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status AmrPlaybackApp::createStream() {
    StreamConfig config;
    config.type = StreamType::PLAY;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = SAMPLE_RATE;
    config.format = AudioFormat::AMRWB_PLUS;
    // here both channel selected, this can be selected according to requirement
    config.channelTypeMask = (ChannelType::LEFT | ChannelType::RIGHT);
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    // Passing Decoder Specific Configuration, refer header file for more details.
    AmrwbpParams amrParams{};
    if (config.format == AudioFormat::AMRWB_PLUS) {
        amrParams.bitWidth = 16;
        amrParams.frameFormat = AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
        config.formatParams = &amrParams;
    } else {
        config.formatParams = nullptr;
    }

    std::promise<bool> p;
    auto status = audioManager_->createStream(config,
        [&p,this](std::shared_ptr<IAudioStream> &audioStream, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                audioPlayStream_ = std::dynamic_pointer_cast<IAudioPlayStream>(audioStream);
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "failed to Create a stream" <<std::endl;
            }
        });
    if (status == Status::SUCCESS) {
        std::cout << "Request to create stream sent" << std::endl;
    } else {
        std::cout << "Request to create stream failed"  << std::endl;
    }

    if (p.get_future().get()) {
        std::cout<< "Audio Stream is Created" << std::endl;
    } else {
        std::cout<< "Audio Stream Creation Failed !!" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status AmrPlaybackApp::deleteStream() {
    std::promise<bool> p;
    Status status = audioManager_-> deleteStream(
    audioPlayStream_, [&p,this](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to delete a stream" << std::endl;
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "request to delete stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
    }
    if (p.get_future().get()) {
        audioPlayStream_= nullptr;
        std::cout << "Audio Stream is Deleted" << std::endl;
    } else {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

// Callback to provide response to the write request
void AmrPlaybackApp::writeCallback(std::shared_ptr<IStreamBuffer> buffer,
        uint32_t bytes, ErrorCode error) {
    std::cout << "Bytes Written : " << bytes << std::endl;
    if (error != ErrorCode::SUCCESS || buffer->getDataSize() != bytes) {
        // Application needs to resend the Bitstream buffer from leftover position if bytes
        // consumed are not equal to requested number of bytes to be written.
        pipeLineEmpty_ = false;
    }
    buffer->reset();
    freeBuffers_.push(buffer);
    cv_.notify_all();
    return;
}

// This event is received in case of compressed audio format playback, it is received when the
// buffer pipeline is ready to accept new buffers.
void AmrPlaybackApp::onReadyForWrite() {
    pipeLineEmpty_ = true;
    cv_.notify_all();
}

// This event is received when audio playback is stopped with stopAudio() with
// StopType::STOP_AFTER_PLAY and all the buffers in pipeline are played successfully.
void AmrPlaybackApp::onPlayStopped() {
    std::cout << "Playback Stopped after playing pending buffers in pipeline" << std::endl;
}

void AmrPlaybackApp::play() {
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    // Pointer variable to stream buffer
    std::shared_ptr<IStreamBuffer> streamBuffer;
    uint32_t size; // variable to define buffer size
    for (int i = 0; i < TOTAL_BUFFERS; i++) {
        streamBuffer = audioPlayStream_->getStreamBuffer();
        if (streamBuffer != nullptr) {
            freeBuffers_.push(streamBuffer);
            size = streamBuffer->getMinSize();
            if (size == 0) {
                size =  streamBuffer->getMaxSize();
            }
            streamBuffer->setDataSize(size);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
            return;
        }
        // Here Bitstream content passed as zero for representation only. Actually valid Bitstream
        // of "size" bytes need to be passed, except during end of operation where "size"
        // represents last leftover Bitstream.
        memset(streamBuffer->getRawBuffer(),0,size);
    }
    if (!freeBuffers_.empty() && pipeLineEmpty_) {
        streamBuffer = freeBuffers_.front();
        freeBuffers_.pop();
        auto writeCb = std::bind(&AmrPlaybackApp::writeCallback, this, std::placeholders::_1,
                        std::placeholders::_2, std::placeholders::_3);
        auto status = audioPlayStream_->write(streamBuffer,writeCb);
        if (status != Status::SUCCESS) {
            std::cout << "Request to write to stream failed." << std::endl;
        } else {
            std::cout << "Request to write to stream sent." << std::endl;
        }
    } else {
        cv_.wait(lock);
    }
    // Calling stopAudio() with StopType::STOP_AFTER_PLAY as if last buffer is not the complete
    // buffer we need to call it to play remaining buffer successfully.
    std::promise<bool> p;
    auto status = audioPlayStream_->stopAudio(StopType::STOP_AFTER_PLAY, [&p](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to stop after playing buffers" << std::endl;
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to stop playback after pending buffers Sent" << std::endl;
    } else {
        std::cout << "Request to stop playback after pending buffers failed" << std::endl;
    }
    if (p.get_future().get()) {
        std::cout << "Pending buffers played successful !!" << std::endl;
    }
}

int main(int argc, char ** argv) {
    // Creating an instance of application
    std::shared_ptr<AmrPlaybackApp> app = std::make_shared<AmrPlaybackApp>();
    // Initialing the object
    auto status = app->init();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }
    // Creating an audio playback stream
    status = app->createStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }
    // Playing buffer
    app->play();
    // Deleting audio playback stream
    status = app->deleteStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
