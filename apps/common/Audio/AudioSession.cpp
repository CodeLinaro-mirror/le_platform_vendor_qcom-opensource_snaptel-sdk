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
 * @file       AudioSession.cpp
 *
 * @brief      AudioSession class provides methods to create & delete the various types of streams.
 *             Also this class provides the methods for device switch, volume and mute control
 *             which are applicable to various streams.
 */

#include <iostream>
#include <sstream>

#include <telux/audio/AudioFactory.hpp>

#include "AudioSession.hpp"

AudioSession::AudioSession()
    : stream_(nullptr) {
}

AudioSession::~AudioSession() {
}

Status AudioSession::createStream(StreamConfig config) {
    Status status = Status::FAILED;
    if (!stream_) {
        std::promise<bool> p;
        auto &audioFactory = AudioFactory::getInstance();
        auto audioManager = audioFactory.getAudioManager();
        // Sending request to create audio stream
        status = audioManager->createStream(config,
            [&p, &status, this](std::shared_ptr<IAudioStream> &audioStream,
                ErrorCode error) {
                if (error == ErrorCode::SUCCESS) {
                    stream_ = audioStream;
                    p.set_value(true);
                } else {
                    status = Status::FAILED;
                    p.set_value(false);
                }
            });
        p.get_future().wait();
    } else {
        LOG(DEBUG, "Stream already exist");
        status = Status::SUCCESS;
    }
    return status;
}

Status AudioSession::deleteStream() {
    Status status = Status::FAILED;
    if (stream_) {
        std::promise<bool> p;
        auto &audioFactory = AudioFactory::getInstance();
        auto audioManager = audioFactory.getAudioManager();
        status = audioManager-> deleteStream(stream_, [&p, &status, this](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                stream_ = nullptr;
                p.set_value(true);
            } else {
                status = Status::FAILED;
                p.set_value(false);
            }
        });
        p.get_future().wait();
    } else {
        status = Status::SUCCESS;
        LOG(ERROR, "No stream exists");
    }
    return status;
}

Status AudioSession::getStreamDevice(std::vector<DeviceType> &devices) {
    Status status = Status::FAILED;
    if (stream_) {
        std::promise<bool> p;
        status = stream_->getDevice(
            [&p, &status, &devices, this](std::vector<DeviceType> myDevices, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
                devices = myDevices;
            } else {
                status = Status::FAILED;
                p.set_value(false);
            }
        });
        p.get_future().wait();
    } else {
        status = Status::FAILED;
        LOG(ERROR, "No stream exists");
    }
    return status;
}

Status AudioSession::setStreamDevice(std::vector<DeviceType> devices) {
    Status status = Status::FAILED;
    if (stream_) {
        std::promise<bool> p;
        status = stream_->setDevice(devices, [&p, &status, this](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                status = Status::FAILED;
            }
        });
        p.get_future().wait();
    } else {
        status = Status::FAILED;
        LOG(ERROR, "No stream exists");
    }
    return status;
}

Status AudioSession::setVolume(StreamVolume streamVol) {
    Status status = Status::FAILED;
    if (stream_) {
        std::promise<bool> p;
        status = stream_->setVolume(streamVol, [&p, &status, this](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                status = Status::FAILED;
            }
        });
        p.get_future().wait();
    } else {
        status = Status::FAILED;
        LOG(ERROR, "No stream exists");
    }
    return status;
}

Status AudioSession::getVolume(StreamVolume &volume) {
    Status status = Status::FAILED;
    if (stream_) {
        std::promise<bool> p;
        status = stream_->getVolume(
            volume.dir,  [&p, &status, &volume, this](StreamVolume vol, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
                volume = vol;
            } else {
                status = Status::FAILED;
                p.set_value(false);
            }
        });
        p.get_future().wait();
    } else {
        status = Status::FAILED;
        LOG(ERROR, "No stream exists");
    }
    return status;
}

Status AudioSession::setMute(StreamMute mute) {
    Status status = Status::FAILED;
    if (stream_) {
        std::promise<bool> p;
        status = stream_->setMute(mute, [&p, &status, this](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                status = Status::FAILED;
            }
        });
        p.get_future().wait();
    } else {
        status = Status::FAILED;
        LOG(ERROR, "No stream exists");
    }
    return status;
}

Status AudioSession::getMute(StreamMute &muteStatus) {
    Status status = Status::FAILED;
    if (stream_) {
        std::promise<bool> p;
        status = stream_->getMute(muteStatus.dir,
        [&p, &status, &muteStatus, this](StreamMute mute, ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
                muteStatus = mute;
            } else {
                status = Status::FAILED;
                p.set_value(false);
            }
        });
        p.get_future().wait();
    } else {
        status = Status::FAILED;
        LOG(ERROR, "No stream exists");
    }
    return status;
}
