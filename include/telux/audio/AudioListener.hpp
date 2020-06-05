/*
*  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @file    AudioListener.hpp
 *
 * @brief   AudioListener provides callback methods for listening to notifications like DTMF tone
 *          detection. Client need to implement these methods.
 *          The methods in listener can be invoked from multiple threads.So the client needs to
 *          make sure that the implementation is thread-safe.
 */

#ifndef AUDIOLISTENER_HPP
#define AUDIOLISTENER_HPP

#include <telux/audio/AudioDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace audio {

/** @addtogroup telematics_audio
 * @{ */

/**
 * @brief Listener class for getting notifications related to DTMF tone detection. The client needs
 *        to implement these methods as briefly as possible and avoid blocking calls in it.
 *        The methods in this class can be invoked from multiple different threads. Client
 *        needs to make sure that the implementation is thread-safe.
 */
class IVoiceListener {
public:
    /**
     * This function is called when a DTMF tone is detected in the voice stream
     *
     * @param [in] dtmfTone     DTMF tone properties
     */
    virtual void onDtmfToneDetection(DtmfTone dtmfTone) {
    }

    /**
     * Destructor of IVoiceListener
     */
    virtual ~IVoiceListener() {
    }
};

class IPlayListener {
public:
    /**
     * This function is called when pipeline is ready to accept new buffer. It is applicable only
     * for compressed audio format type where a client can write and queue buffers for playback.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual void onReadyForWrite() {}

    /**
     * This function is called when stopAudio() is called with StopType::STOP_AFTER_PLAY. It
     * indicates that all the buffers that were present in the pipeline have been played.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual void onPlayStopped() {}

    /**
     * Destructor of IPlayListener
     */
    virtual ~IPlayListener() {}
};

class ITranscodeListener {
public:
    /**
     * This function is called when pipeline is ready to accept new buffer. It is applicable only
     * for compressed audio format type where a client can write and queue buffers for transcoding.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual void onReadyForWrite() {}

    /**
     * Destructor of ITranscodeListener
     */
    virtual ~ITranscodeListener() {}
};

class IAudioListener : public telux::common::IServiceStatusListener {
public:

    /**
     * Destructor of IAudioListener
     */
    virtual ~IAudioListener() {
    }
};

/** @} */ /* end_addtogroup telematics_audio */

}  // end of namespace audio
}  // end of namespace telux

#endif  // AUDIOLISTENER_HPP
