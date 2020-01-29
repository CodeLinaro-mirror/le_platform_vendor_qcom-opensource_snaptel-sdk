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
 * @file    AudioTranscoder.hpp
 *
 * @brief   Audio Transcoder is a primary interface for audio transcoding operations. It provides
 *          APIs to convert one audio format to another.
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to change
 *          and could break backwards compatibility.
 */

#ifndef AUDIOTRANSCODER_HPP
#define AUDIOTRANSCODER_HPP

#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/audio/AudioDefines.hpp>
#include <telux/audio/AudioListener.hpp>

namespace telux {

namespace audio {
/** @addtogroup telematics_audio
 * @{ */

class IAudioBuffer;

/**
 * This function is called with the response to ITranscoder::read().
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] buffer         Buffer that was used to capture the data from the transcode read
 *                            operation. Applications could call IAudioBuffer::reset() and reuse
 *                            this buffer for subsequent read operations on the same transcoder
 *                            instance. Also buffer.getDataSize() will represent the number of
 *                            bytes contained in a buffer.
 *
 * @param [in] isLastBuffer   represents whether the transcoded buffer is last buffer or not. Once
 *                            the last buffer is received no more further read operations are
 *                            required.
 *
 * @param [in] error  Return code which indicates whether the operation succeeded or not.
 *                    @ref ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to
 *         change and could break backwards compatibility.
 */
using TranscoderReadResponseCb = std::function<void(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t isLastBuffer, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to ITranscoder::write().
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] buffer       Buffer that was used for the write operation for transcoding.
 *                          Application could call IAudioBuffer::reset() and reuse this buffer
 *                          for subsequent write operations on the same transcoder instance.
 *
 * @param [in] bytesWritten Return how many bytes are sent for transcoding.
 *
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not.
 *                          @ref ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to
 *         change and could break backwards compatibility.
 */
using TranscoderWriteResponseCb = std::function<void(std::shared_ptr<IAudioBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error)>;

/**
 * @brief   ITranscoder is used to convert one audio format to another audio format
 *          using the transcoding operation.
 */
class ITranscoder {
public:
    /**
     * Get a buffer to be used for writing samples for transcoding operation.
     *
     * @returns            a buffer or nullptr in case of failure.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual std::shared_ptr<IAudioBuffer> getWriteBuffer() = 0;

    /**
     * Get a buffer to be used for reading samples from transcoding operation.
     *
     * @returns            a buffer or nullptr in case of failure.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual std::shared_ptr<IAudioBuffer> getReadBuffer() = 0;

    /**
     * Write Samples/Frames to transcode stream. First write starts transcoding operation.
     *
     * Write in case of compressed audio format maintains a pipeline, if the callback returns with
     * same number of bytes written as requested and no error occured, user can send next buffer.
     * If the number of bytes returned are not equal to the requested write size, then user needs to
     * resend the buffer again from the leftover offset after waiting for the @onReadyForWrite()
     * event.
     *
     * @param [in] buffer         buffer that needs to be transcoded.
     * @param [in] isLastBuffer   represents whether this buffer is last buffer or not. Once last
     *                            buffer is set no more write operations are required.
     * @param [in] callback       callback to get the response of write.
     *
     * @returns Status of the request i.e. success or suitable status code.
     *
     * @note       Eval: This is a new API and is being evaluated. It is subject to change
     *             and could break backwards compatibility.
     */
    virtual telux::common::Status write(std::shared_ptr<IAudioBuffer> buffer,
            uint32_t isLastBuffer, TranscoderWriteResponseCb callback = nullptr) = 0;

    /**
     * It is mandatory to call this API after the end of a transcode operation or to abort a
     * transcode operation. After this API call the ITranscoder object is no longer usable.
     *
     * @param [in] callback      callback to get the response of tearDown.
     *
     * @returns Status of the request i.e. success or suitable status code.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change
     *         and could break backwards compatibility.
     */
    virtual telux::common::Status tearDown(telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Reads samples/Frames from transcoder during transcoding operation.
     *
     * @param [in] buffer       stream buffer for read.
     * @param [in] bytesToRead  specifying how many bytes to be read from stream.
     * @param [in] callback     callback to get the response of read.
     *
     * @returns Status of the request i.e. success or suitable status code.
     *
     * @note       Eval: This is a new API and is being evaluated. It is subject to change
     *             and could break backwards compatibility.
     */
    virtual telux::common::Status read(std::shared_ptr<IAudioBuffer> buffer, uint32_t bytesToRead,
            TranscoderReadResponseCb callback = nullptr) = 0;

    /**
     * Register a listener to get notified for events of Transcoder.
     *
     * @param [in] listener    Pointer of ITranscodeListener object that processes the notification.
     *
     * @returns Status of registerListener i.e success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated.It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ITranscodeListener> listener) = 0;

    /**
     * Remove a previously registered listener.
     *
     * @param [in] listener Previously registered ITranscodeListener that needs to be removed.
     *
     * @returns Status of deRegisterListener, success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated.It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status
            deRegisterListener(std::weak_ptr<ITranscodeListener> listener) = 0;
};

/** @} */ /* end_addtogroup telematics_audio */
}  // End of namespace audio

}  // End of namespace telux

#endif  // end of AUDIOTRANSCODER_HPP
