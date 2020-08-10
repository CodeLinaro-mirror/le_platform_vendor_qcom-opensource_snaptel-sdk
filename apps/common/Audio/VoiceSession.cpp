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

#include <chrono>
#include <iostream>
#include <sstream>

#include <telux/audio/AudioFactory.hpp>

#include "VoiceSession.hpp"

VoiceSession::VoiceSession()
    : audioStarted_(false) {
}

VoiceSession::~VoiceSession() {
}

Status VoiceSession::startAudio() {
    auto status = Status::FAILED;
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    if (audioVoiceStream_) {
        if (!audioStarted_) {
            std::promise<bool> p;
            status = audioVoiceStream_->startAudio(
                [&p, &status, &audioVoiceStream_, this](ErrorCode error) {
                if (error == ErrorCode::SUCCESS) {
                    p.set_value(true);
                    audioStarted_ = true;
                } else {
                    status = Status::FAILED;
                    p.set_value(false);
                }
            });
            p.get_future().wait();
        } else {
            LOG(ERROR, "Audio already started");
            status = Status::SUCCESS;
        }
    } else {
        LOG(ERROR, "No stream exists");
    }
    return status;
}

Status VoiceSession::stopAudio() {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto status = Status::FAILED;
    if (audioVoiceStream_ && audioStarted_) {
        std::promise<bool> p;
        status = audioVoiceStream_->stopAudio(
            [&p, &status, &audioVoiceStream_, this](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
                audioStarted_ = false;
            } else {
                status = Status::FAILED;
                p.set_value(false);
            }
        });
        p.get_future().wait();
    } else {
        status = Status::SUCCESS;
        LOG(ERROR, "Audio not started yet");
    }
    return status;
}

Status VoiceSession::startDtmf(DtmfTone tone, uint16_t gain, uint32_t duration) {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto status = Status::FAILED;
    if (audioVoiceStream_ && audioStarted_) {
        std::promise<bool> p;
        status = audioVoiceStream_->playDtmfTone(tone, duration, gain,
            [&p, &status, &audioVoiceStream_](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                status = Status::FAILED;
                p.set_value(false);
            }
        });
        p.get_future().wait();
    } else {
        LOG(ERROR, "Audio not started yet");
    }
    return status;
}

Status VoiceSession::stopDtmf() {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto status = Status::FAILED;
    if (audioVoiceStream_) {
        std::promise<bool> p;
        status = audioVoiceStream_->stopDtmfTone(StreamDirection::RX,
            [&p, &status, &audioVoiceStream_](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                status = Status::FAILED;
                p.set_value(false);
            }
            });
        p.get_future().wait();
    } else {
        LOG(ERROR, "No stream exists");
    }
    return status;
}

Status VoiceSession::registerListener(std::weak_ptr<IVoiceListener> listener) {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto status = Status::FAILED;
    if (audioVoiceStream_ && audioStarted_) {
        std::promise<bool> p;
        status = audioVoiceStream_ ->registerListener(
            listener, [&p, &status, &audioVoiceStream_](ErrorCode error) {
            if (error == ErrorCode::SUCCESS) {
                p.set_value(true);

            } else {
                p.set_value(false);
                status = Status::FAILED;
                LOG(ERROR, "Failed to register Listener");
            }
            });
        p.get_future().wait();
    } else {
        LOG(ERROR, "Audio is not started yet");
    }
    return status;
}

Status VoiceSession::deRegisterListener(std::weak_ptr<IVoiceListener> listener) {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(stream_);
    auto status = Status::FAILED;
    if (audioVoiceStream_ && audioStarted_) {
        status = audioVoiceStream_ ->deRegisterListener(listener);
        if (status == Status::SUCCESS) {
            LOG(DEBUG, "Request to deregister DTMF Sent");
        }
    } else {
        LOG(ERROR, "Audio is not started yet");
    }
    return status;
}
