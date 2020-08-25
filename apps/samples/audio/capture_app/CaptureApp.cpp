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
#include <chrono>
#include <future>
#include <iostream>
#include <condition_variable>

#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>


#define SAMPLE_RATE 16000
#define TIMEOUT 5

using std::promise;
using namespace telux::common;
using namespace telux::audio;

static std::shared_ptr<IAudioCaptureStream> audioCaptureStream;
static promise<ErrorCode> gCallbackPromise;
static std::condition_variable cv;

// Resets the global callback promise variable
static inline void resetCallbackPromise(void) {
    gCallbackPromise = promise<ErrorCode>();
}

// Callback which provides response to createStream, with pointer to base interface IAudioStream.
// Type Casting Base Stream Pointer to CaptureStream, as requested stream is of type CAPTURE
static void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error)
{
    if (ErrorCode::SUCCESS == error) {
        std::cout << "capture stream created." << std::endl;
        audioCaptureStream = std::dynamic_pointer_cast<IAudioCaptureStream>(stream);
    }
    gCallbackPromise.set_value(error);
    return;
}

// Callback to provide response to the write request
static void readCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer, ErrorCode error)
{
    if (ErrorCode::SUCCESS == error) {
        std::cout << "read from stream succeeded." << std::endl;
    } else {
        std::cout << "read failed with error code " << static_cast<int>(error) << std::endl;
    }
    buffer->reset();
    cv.notify_all();
    return;
}

// Callback which provides response to deleteStream
static void deleteStreamCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        std::cout << "capture stream deleted." << std::endl;
        audioCaptureStream.reset();
    }
    gCallbackPromise.set_value(error);
    return;
}

int main(int, char **) {

    // ### 1. Get the AudioFactory and AudioManager instances.
    auto &audioFactory = AudioFactory::getInstance();
    static std::shared_ptr<IAudioManager> audioManager = audioFactory.getAudioManager();

    // ### 2. Requesting to get audio subsystem state
    bool subSystemsStatus = false;
    if (audioManager) {
        subSystemsStatus = audioManager->isSubsystemReady();
    } else {
        std::cout << "Invalid Audio Manager" << std::endl;
        return EXIT_FAILURE;
    }

    // #### 2.1  Checking state of audio subsystem if it is ready or not ready
    if (subSystemsStatus) {
        std::cout << "Audio Subsystem is ready." << std::endl;
    } else {
        std::cout << "Audio Subsystem is NOT ready." << std::endl;
    }

    // Option # 1 if we want to wait for only timeout period for audio subsystem to get ready
    std::future<bool> f = audioManager->onSubsystemReady();
    if (f.wait_for(std::chrono::seconds(TIMEOUT)) == std::future_status::timeout) {
        std::cout << "operation timed out." << std::endl;
    }

    // Option # 2 if we want to wait unconditionally for audio subsystem to get ready.
    subSystemsStatus = f.get();
    if (subSystemsStatus) {
        std::cout << "onSubsystemReady: Audio Subsystem is ready." << std::endl;
    } else {
        std::cout << "Audio Subsystem is NOT ready." << std::endl;
        return EXIT_FAILURE;
    }

    resetCallbackPromise();

    // ### 3. Create an Audio Capture Stream
    StreamConfig config;
    config.type = StreamType::CAPTURE;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = SAMPLE_RATE;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    // here both channel selected, this can be selected according to requirement
    config.channelTypeMask = (ChannelType::LEFT | ChannelType::RIGHT);
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_MIC);
    auto status = audioManager->createStream(config, createStreamCallback);
    if(status == Status::SUCCESS) {
        std::cout << "Request to create capture stream sent." << std::endl;
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            std::cout << "Error : failed to create capture stream ,error code : " <<
                    static_cast<int>(error) << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        std::cout << "Request to create capture stream failed." << std::endl;
        return EXIT_FAILURE;
    }

    resetCallbackPromise();

    // ### 4. Reading from stream
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    // Pointer variable to stream buffer
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    uint32_t size; // variable to define buffer size

    streamBuffer = audioCaptureStream->getStreamBuffer();
    if(streamBuffer != nullptr) {
        // Setting the bytesToRead (bytes to be readed from stream) as minimum size
        // required by stream. In any case if size returned is 0, using the Maximum Buffer
        // Size, any buffer size between min and max can be used
        size = streamBuffer->getMinSize();
        if(size == 0) {
            size = streamBuffer->getMaxSize();
        }
    } else {
        std::cout << "Failed to get Stream Buffer." << std::endl;
        return EXIT_FAILURE;
    }

    status = audioCaptureStream->read(streamBuffer, size, readCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << "Request to read from stream Failed." << std::endl;
    } else {
        std::cout << "Request to read from stream sent." << std::endl;
        cv.wait(lock);
    }

    // ### 5. Delete an Audio Stream (Capture Session), which was created earlier
    status = audioManager->deleteStream(audioCaptureStream, deleteStreamCallback);
    if(status == Status::SUCCESS) {
        std::cout << "Request to delete capture stream sent." << std::endl;
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            std::cout << "Error : failed to delete audio stream, error code : " <<
                    static_cast<int>(error) << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        std::cout << "Request to delete capture stream failed." << std::endl;
    }

    return EXIT_SUCCESS;
}
