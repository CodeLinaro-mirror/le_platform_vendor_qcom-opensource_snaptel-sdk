/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

#include <future>
#include <iostream>
#include <condition_variable>

#include <telux/audio/AudioFactory.hpp>
#include "InCallAudioApp.hpp"

using std::promise;
using namespace telux::common;
using namespace telux::audio;


const uint32_t SAMPLE_RATE = 48000;
// Below constant represents number of buffers allocated to pass Bitstream. Additional number of
// buffers would provide flexibility in copying Bitsream from source and write to Stream Interface
// in two parallel threaded operations.
const int TOTAL_BUFFERS = 1;

InCallAudioApp::InCallAudioApp() {
    pipeLineEmpty_ = true;
}

InCallAudioApp::~InCallAudioApp() {
}

Status InCallAudioApp::init() {
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

Status InCallAudioApp::createVoiceStream() {
    StreamConfig config;
    config.type = StreamType::VOICE_CALL;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = 16000;
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
            std::cout << "failed to Create a stream" <<std::endl;
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to create stream sent" << std::endl;
    } else {
        std::cout << "Request to create stream failed"  << std::endl;
    }

    if (p.get_future().get()) {
        std::cout<< "Audio Voice Stream is Created" << std::endl;
    } else {
        std::cout<< "Audio Voice Stream Creation Failed !!" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status InCallAudioApp::createPlayStream() {
    StreamConfig config;
    config.type = StreamType::PLAY;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = SAMPLE_RATE;
    config.format = AudioFormat::AMRWB_PLUS;
    // here both channel selected, this can be selected according to requirement
    config.channelTypeMask = (ChannelType::LEFT | ChannelType::RIGHT);
    // Direction::TX indicates Voice Uplink Playback
    config.voicePaths.emplace_back(Direction::TX);
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
        std::cout<< "Audio Play Stream is Created" << std::endl;
    } else {
        std::cout<< "Audio Play Stream Creation Failed !!" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status InCallAudioApp::deletePlayStream() {
    std::promise<bool> p;
    Status status = audioManager_-> deleteStream(audioPlayStream_, [&p,this](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to delete a stream" << std::endl;
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to delete stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
    }
    if (p.get_future().get()) {
        audioPlayStream_= nullptr;
        std::cout << "Audio Play Stream is Deleted" << std::endl;
    } else {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status InCallAudioApp::deleteVoiceStream() {
    std::promise<bool> p;
    auto status = audioManager_-> deleteStream(audioVoiceStream_, [&p,this](ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to delete a stream" << std::endl;
        }
    });
    if (status == Status::SUCCESS) {
        std::cout << "Request to delete stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
    }
    if (p.get_future().get()) {
        audioVoiceStream_= nullptr;
        std::cout << "Audio Voice Stream is Deleted" << std::endl;
    } else {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

// Callback to provide response to the write request
void InCallAudioApp::writeCallback(std::shared_ptr<IStreamBuffer> buffer,
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
void InCallAudioApp::onReadyForWrite() {
    pipeLineEmpty_ = true;
    cv_.notify_all();
}

// This event is received when audio playback is stopped with stopAudio() with
// StopType::STOP_AFTER_PLAY and all the buffers in pipeline are played successfully.
void InCallAudioApp::onPlayStopped() {
    std::cout << "Playback Stopped after playing pending buffers in pipeline" << std::endl;
}

void InCallAudioApp::play() {
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
        auto writeCb = std::bind(&InCallAudioApp::writeCallback, this, std::placeholders::_1,
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

int main(int, char **) {
    // Creating an instance of application
    std::shared_ptr<InCallAudioApp> app = std::make_shared<InCallAudioApp>();
    // Initialing the object
    auto status = app->init();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }

    // Creating an audio voice stream
    status = app->createVoiceStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }

    // Creating an audio playback stream
    status = app->createPlayStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }

    // Playing buffer
    app->play();

    // Deleting audio play stream
    status = app->deletePlayStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }

    // Deleting audio voice stream
    status = app->deleteVoiceStream();
    if (Status::SUCCESS != status) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
