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

#include <chrono>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <map>

#include <telux/audio/AudioDefines.hpp>
#include <telux/audio/AudioManager.hpp>
#include <telux/audio/AudioListener.hpp>
#include <telux/common/CommonDefines.hpp>

using namespace telux::audio;
using namespace telux::common;

class AudioClient {
public:
    AudioClient();
    ~AudioClient();

    void init();

    // AudioClient creates stream for any type of stream and any SubMenu can request for stream
    std::shared_ptr<IAudioStream> getStream(StreamType streamtype);

    // since file of path is taken during stream creation it is stored for future use
    std::string getFilePathForPlay();

    // Since sample Rate and Channel Type is asked while opening stream we keep them because it
    // is required during the time of writting to file.
    void getCaptureConfig(uint32_t &sampleRate, uint32_t &channelType);

    Status createStream(StreamType streamType);
    Status deleteStream(StreamType streamType);

    void getStreamDevice(StreamType streamtype);
    void setStreamDevice(StreamType streamtype);
    void getVolume(StreamType streamtype);
    void setVolume(StreamType streamtype);
    void getMute(StreamType streamtype);
    void setMute(StreamType streamtype);

private:
    // Since all functions need streamType so a common stream resolver
    void resolveStreamType(StreamType streamType);

    // Input functions for different cases
    void takeUserModemIdInput(int &modemId);
    void takeUserSampleRateInput(uint32_t &userSampleRate);
    void takeUserChannelInput(telux::audio::ChannelTypeMask &channelType);
    void takeUserDeviceInput(std::vector<telux::audio::DeviceType> &devices);
    void takeUserCreateStreamInput(telux::audio::StreamConfig &config);
    void takeUserDirectionInput(StreamDirection &direction);
    void takeUserVolumeInput(StreamVolume &streamVolume);
    void takeVolumeValueInput(float &vol);

    // Variables for filePath  Used only for play
    std::string filepath_;

    // Variable for sample rate and channel type. Used only for Capture
    uint32_t sampleRate_;
    uint32_t channelType_;

    // Varibles to store Manager and Streams
    std::shared_ptr<IAudioManager> audioManager_;
    std::shared_ptr<IAudioStream> stream_;
    std::shared_ptr<IAudioVoiceStream> audioVoiceStream_;
    std::shared_ptr<IAudioPlayStream> audioPlayStream_;
    std::shared_ptr<IAudioCaptureStream> audioCaptureStream_;
    std::shared_ptr<IAudioLoopbackStream> audioLoopbackStream_;
    std::shared_ptr<IAudioToneGeneratorStream> audioToneStream_;
};

#endif //AUDIOCLIENT_HPP
