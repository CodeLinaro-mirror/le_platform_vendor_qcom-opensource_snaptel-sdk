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

/**
 * @file    AudioDefines.hpp
 *
 * @brief   AudioDefines contains enumerations and variables used for
 *          audio subsystems.
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to change
 *          and could break backwards compatibility.
 */

#ifndef AUDIODEFINES_HPP
#define AUDIODEFINES_HPP

#include <vector>

namespace telux {

namespace audio {

/**
 * Represent type of device like SPEAKER, MIC, etc.
 */
enum DeviceType {
    DEVICE_TYPE_NONE          = -1,
    /* output devices */
    DEVICE_TYPE_SPEAKER       =  1,
    /* input devices */
    DEVICE_TYPE_MIC           =  257,
};

enum class DeviceDirection {
    NONE             = -1,
    RX               =  1,
    TX               =  2,
};

enum class StreamType {
    NONE             = -1,
    VOICE_CALL       =  1,
    PLAY             =  2,
    CAPTURE          =  3,
};

enum class StreamDirection {
    NONE             = -1,
    RX               =  1,
    TX               =  2,
};

enum ChannelType {
    LEFT             =  ( 1 << 0 ), /* left channel*/
    RIGHT            =  ( 1 << 1 ), /* right channel*/
};

using ChannelTypeMask = int;

/**
 * Specifies audio data format
 */
enum class AudioFormat {
    UNKNOWN = -1, /**< Unknown format */
    PCM_16BIT_SIGNED = 1, /* 16 bit signed PCM format */
};

/* common audio stream configuration parameters
 *  ensure forward compatibility
 */
struct StreamConfig {
    StreamType type;
    int modemSubId = 1;
    uint32_t sampleRate;
    ChannelTypeMask channelTypeMask;
    AudioFormat  format;
    std::vector<DeviceType> deviceTypes;
};

struct ChannelVolume {
    ChannelType channelType;
    float vol;
};

struct StreamVolume {
    std::vector<ChannelVolume> volume;
    StreamDirection dir;
};

struct StreamMute {
    bool enable;
    StreamDirection dir;
};

struct StreamBuffer {
    std::vector<uint8_t> buffer; /* Buffer with Size encapsulated */
    size_t offset; /* Actual Buffer Content starting position */
    int64_t timestamp;
};

}  // End of namespace audio

}  // End of namespace telux

#endif //AUDIODEFINES_HPP
