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

#ifndef AUDIOCLIENT_HPP
#define AUDIOCLIENT_HPP

#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>

using namespace telux::common;
using namespace telux::audio;

/** AudioClient class provides methods to start and stop a voice session */
class AudioClient {
public:
    /**
     * Initialize audio subsystem
     */
    telux::common::Status init();

    /**
     * This function starts a voice session, which enables speech communication during an eCall.
     * This is typically invoked when an eCall is triggered.
     *
     * @param [in] phoneId         Represents phone corresponding to which the operation will be
     *                             performed
     * @param [in] deviceType      Output device type to be used
     * @param [in] sampleRate      Audio sample Rate of voice stream
     * @param [in] voiceFormat     Audio stream data format
     * @param [in] channels        Channels to be used
     *
     * @returns Status of startVoiceSession i.e success or suitable status code.
     *
     */
    telux::common::Status startVoiceSession(int phoneId, DeviceType deviceType, uint32_t sampleRate,
                                AudioFormat voiceFormat, ChannelTypeMask channels);

    /**
     * This function stops the voice session.
     * This is typically invoked when an eCall is cleared down.
     *
     * @returns Status of stopVoiceSession i.e success or suitable status code.
     *
     */
    telux::common::Status stopVoiceSession();

    void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error);
    void deleteStreamCallback(ErrorCode error);
    void startAudioCallback(ErrorCode error);
    void stopAudioCallback(ErrorCode error);

    AudioClient();
    ~AudioClient();

private:
    bool isVoiceEnabled();
    void setVoiceState(bool state);

    /** Represents voice session status */
    bool voiceEnabled_;
    /** Member variables to hold Audio Manager and voice stream objects */
    std::shared_ptr<IAudioManager> audioMgr_;
    std::shared_ptr<IAudioVoiceStream> audioVoiceStream_;
    std::mutex mutex_;

};

#endif  // AUDIOCLIENT_HPP
