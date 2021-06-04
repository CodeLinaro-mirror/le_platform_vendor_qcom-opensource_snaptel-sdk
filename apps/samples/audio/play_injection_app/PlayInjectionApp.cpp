/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * @file       PlayInjectionApp.cpp
 *
 * @brief      This application demonstrates steps needed to inject the audio data over the voice
 *             call uplink node. This application expects that the voice call is in active state
 *             @ref telux::tel::makeCall. It will need to create two streams IAudioVoiceStream
 *             and IAudioPlayStream to inject the audio data. IAudioPlayStream will be required
 *             to be created with @ref DeviceType::DEVICE_TYPE_OUT_VIRTUAL.
 *
 */

#include <future>
#include <iostream>
#include <condition_variable>

#include <telux/audio/AudioFactory.hpp>
#include "PlayInjectionApp.hpp"

using std::promise;
using namespace telux::common;
using namespace telux::audio;

const uint32_t SAMPLE_RATE = 8000;

/* Below constant represents number of buffers allocated to pass Bitstream. Additional number of
 * buffers would provide flexibility in copying Bitsream from source and write to Stream Interface
 * in two parallel threaded operations.
 */
const int TOTAL_BUFFERS = 1;

PlayInjectionApp::PlayInjectionApp() {
}

PlayInjectionApp::~PlayInjectionApp() {
}

Status PlayInjectionApp::init() {
    // Get the AudioFactory and AudioManager instances.
    auto &audioFactory = AudioFactory::getInstance();
    audioManager_ = audioFactory.getAudioManager();

    // Requesting to get audio subsystem state
    bool subSystemStatus = false;
    if (audioManager_) {
        subSystemStatus = audioManager_->isSubsystemReady();
    } else {
        std::cout << "Invalid Audio Manager" << std::endl;
        return Status::FAILED;
    }

    //  Checking state of audio subsystem if it is ready or not, if not ready waiting for it to
    //  get ready.
    if (!subSystemStatus) {
        std::future<bool> f = audioManager_->onSubsystemReady();
        subSystemStatus = f.get();
    }

    if (subSystemStatus) {
        std::cout << "Audio Subsystem is ready." << std::endl;
    } else {
        std::cout << "Audio Subsystem is NOT ready." << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status PlayInjectionApp::createVoiceStream() {
    StreamConfig config;
    config.type = StreamType::VOICE_CALL;
    config.modemSubId = 1;
    config.sampleRate = SAMPLE_RATE;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);

    std::promise<bool> p;
    auto status = audioManager_->createStream(config,
        [&p,this](std::shared_ptr<IAudioStream> &audioStream, ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(audioStream);
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to create stream sent" << std::endl;
    } else {
        std::cout << "Request to create stream failed"  << std::endl;
    }

    if (p.get_future().get()) {
        std::cout<< "Audio voice stream is created" << std::endl;
    } else {
        std::cout<< "Audio voice stream creation Failed" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status PlayInjectionApp::createPlayStream() {
    StreamConfig config;
    config.type = StreamType::PLAY;
    config.sampleRate = SAMPLE_RATE;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_OUT_VIRTUAL);
    std::promise<bool> p;
    auto status = audioManager_->createStream(config,
        [&p,this](std::shared_ptr<IAudioStream> &audioStream, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                audioPlayStream_ = std::dynamic_pointer_cast<IAudioPlayStream>(audioStream);
                p.set_value(true);
            } else {
                p.set_value(false);
            }
        });
    if (status == Status::SUCCESS) {
        std::cout << "Request to create stream sent" << std::endl;
    } else {
        std::cout << "Request to create stream failed"  << std::endl;
    }

    if (p.get_future().get()) {
        std::cout<< "Audio play stream is created" << std::endl;
    } else {
        std::cout<< "Audio play stream creation failed" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status PlayInjectionApp::deletePlayStream() {
    std::promise<bool> p;
    Status status = audioManager_-> deleteStream(audioPlayStream_, [&p,this](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to delete stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
    }
    if (p.get_future().get()) {
        audioPlayStream_= nullptr;
        std::cout << "Audio play stream is deleted" << std::endl;
    } else {
        std::cout << "Failed to delete a stream" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status PlayInjectionApp::deleteVoiceStream() {
    std::promise<bool> p;
    auto status = audioManager_-> deleteStream(audioVoiceStream_, [&p,this](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to delete stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
    }
    if (p.get_future().get()) {
        audioVoiceStream_= nullptr;
        std::cout << "Audio voice stream is deleted" << std::endl;
    } else {
        std::cout << "Failed to delete a stream" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

// Callback to provide response to the write request
void PlayInjectionApp::writeCallback(std::shared_ptr<IStreamBuffer> buffer,
        uint32_t bytes, ErrorCode error) {
    std::cout << "Bytes Written : " << bytes << std::endl;
    if (error != ErrorCode::SUCCESS || buffer->getDataSize() != bytes) {
        std::cout <<
            "Bytes Requested " << buffer->getDataSize() << " Bytes Written " << bytes << std::endl;
    }
    buffer->reset();
    freeBuffers_.push(buffer);
    cv_.notify_all();
    return;
}


void PlayInjectionApp::play() {
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
            streamBuffer->setDataSize(size);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
            return;
        }
        /* Here Bitstream content passed as zero for representation only. Actually valid Bitstream
         * of "size" bytes need to be passed, except during end of operation where "size"
         * represents last leftover Bitstream.
         */
        memset(streamBuffer->getRawBuffer(),0,size);
    }
    if (!freeBuffers_.empty()) {
        streamBuffer = freeBuffers_.front();
        freeBuffers_.pop();
        auto writeCb = std::bind(&PlayInjectionApp::writeCallback, this, std::placeholders::_1,
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
}

void PlayInjectionApp::startVoiceAudio() {
    if(audioVoiceStream_) {
        std::promise<bool> p;
        Status status =
                audioVoiceStream_->startAudio( [&p,this](telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to start audio voice stream" << std::endl;
        }
        });
        if(status == Status::SUCCESS){
            std::cout << "Request to start audio voice stream sent" << std::endl;
        } else {
            std::cout << "Request to start audio voice stream failed" << std::endl;
        }

        if (p.get_future().get()) {
            std::cout << "Audio voice stream is started" << std::endl;
        }
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }
}

void PlayInjectionApp::stopVoiceAudio() {
    if(audioVoiceStream_) {
        std::promise<bool> p;
        Status status = audioVoiceStream_->stopAudio([&p,this](telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to stop audio voice stream" << std::endl;
        }
        });
        if(status == Status::SUCCESS){
            std::cout << "Request to stop audio voice stream sent" << std::endl;
        } else {
            std::cout << "Request to stop audio voice stream failed" << std::endl;
        }

        if (p.get_future().get()) {
            std::cout << "audio voice stream is stopped" << std::endl;
        }
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }
}

int main(int, char **) {
    // Creating an instance of application
    std::shared_ptr<PlayInjectionApp> app = std::make_shared<PlayInjectionApp>();
    // Initialing the object
    auto status = app->init();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }

    /* Creating an audio voice stream
     * this application expects that the voice call is in active state w.r.t telephony.
     * @ref telux::tel::makeCall
     */
    status = app->createVoiceStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }

    app->startVoiceAudio();

    // Creating an audio playback stream
    status = app->createPlayStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }

    // Playing buffer is a continous opeartion.
    app->play();

    // Deleting audio play stream
    status = app->deletePlayStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }

    app->stopVoiceAudio();

    // Deleting audio voice stream
    status = app->deleteVoiceStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
