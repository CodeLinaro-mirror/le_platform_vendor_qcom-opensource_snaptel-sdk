/*
 *  Copyright (c) 2019-2021 The Linux Foundation. All rights reserved.
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
/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <chrono>
#include <future>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>

using std::promise;
using namespace telux::common;
using namespace telux::audio;

#define TIMEOUT 5

static std::shared_ptr<IAudioManager> audioManager;
static std::shared_ptr<IAudioVoiceStream> audioVoiceStream;
static promise<ServiceStatus> initCallbackPromise;
static promise<ErrorCode> gCallbackPromise;
bool audioStarted = false;

// Resets the global callback promise variable
static inline void resetCallbackPromise(void) {
    gCallbackPromise = promise<ErrorCode>();
}

// Callback which provides response to createStream, with pointer to base interface IAudioStream.
// Type Casting Base Stream Pointer to VoiceStream, as requested stream is of type VOICE CALL
// This stream works only when we have an active voice call
// see tel::CallManager API to see how to originate a call
static void createStreamCallback(std::shared_ptr<IAudioStream> &stream, ErrorCode error)
{
    if (ErrorCode::SUCCESS == error) {
        std::cout << "voice stream created." << std::endl;
        audioVoiceStream = std::dynamic_pointer_cast<IAudioVoiceStream>(stream);
    }
    gCallbackPromise.set_value(error);
    return;
}

// Callback which provides response to startAudio.
static void startAudioCallback(ErrorCode error)
{
    if (ErrorCode::SUCCESS == error) {
        audioStarted = true;
        std::cout << "audio started successfully." << std::endl;
    }
    gCallbackPromise.set_value(error);
    return;
}

// Callback which provides response to playDtmfTone
static void playDtmfCallback(ErrorCode error)
{
    if (ErrorCode::SUCCESS == error) {
        std::cout << "Dtmf tone played !!" << std::endl;
    }
    gCallbackPromise.set_value(error);
    return;
}

// Callback which provides response to setDevice.
static void setStreamDeviceCallback(ErrorCode error)
{
    if (ErrorCode::SUCCESS == error) {
        std::cout << "device set successfully." << std::endl;
    }
    gCallbackPromise.set_value(error);
    return;
}

// Callback which provides response to getDevice.
static void getStreamDeviceCallback(std::vector<DeviceType> devices, ErrorCode error)
{
    if (ErrorCode::SUCCESS == error) {
        int i = 0;
        for (auto deviceType : devices) {
            std::cout << "Device [" << i << "] type: " << static_cast<uint32_t>(deviceType)
                << std::endl;
            i++;
        }
    }
    gCallbackPromise.set_value(error);
    return;
}

// Callback which provides response to stopAudio.
static void stopAudioCallback(ErrorCode error)
{
    if (ErrorCode::SUCCESS == error) {
        audioStarted = false;
        std::cout << "audio stopped successfully" << std::endl;
    }
    gCallbackPromise.set_value(error);
    return;
}

// Callback which provides response to deleteStream
static void deleteStreamCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS == error) {
        std::cout << "deleteStream() succeeded." << std::endl;
        audioVoiceStream.reset();
    }
    gCallbackPromise.set_value(error);
    return;
}

// Callback which provides response for audio Manager initialization
static void initAppCallback(ServiceStatus status) {
    if (status == ServiceStatus::SERVICE_AVAILABLE) {
        initCallbackPromise.set_value(ServiceStatus::SERVICE_AVAILABLE);
    } else {
        initCallbackPromise.set_value(ServiceStatus::SERVICE_FAILED);
    }
    return;
}

int main(int argc, char ** argv) {

    // ### 1. Get the AudioFactory and AudioManager instances.
    auto &audioFactory = AudioFactory::getInstance();
    static std::shared_ptr<IAudioManager> audioManager =
        audioFactory.getAudioManager(initAppCallback);

    // ### 2. Requesting to get audio subsystem state
    ServiceStatus subSystemStatus = ServiceStatus::SERVICE_FAILED;
    if (audioManager) {
        subSystemStatus = audioManager->getServiceStatus();
        if (subSystemStatus != ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nAudio subsystem is not ready, Please wait ..." << std::endl;
            subSystemStatus = initCallbackPromise.get_future().get();
        }
    } else {
        std::cout << "Invalid Audio Manager" << std::endl;
        return EXIT_FAILURE;
    }

    if (subSystemStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Audio Subsystem is ready." << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize audio subsystem" << std::endl;
        return EXIT_FAILURE;
    }

    resetCallbackPromise();

    // ### 3. Create an Audio Stream (Voice Call Session)
    StreamConfig config;
    config.type = StreamType::VOICE_CALL;
    config.slotId = DEFAULT_SLOT_ID;
    config.sampleRate = 16000;
    config.format = AudioFormat::PCM_16BIT_SIGNED;
    config.channelTypeMask = ChannelType::LEFT;
    /*For StreamType::VOICE_CALL, sink and source device are required to be passed.
      First device should be sink (speaker) and then source (mic) should be passed for stream
      creation.*/
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER);
    config.deviceTypes.emplace_back(DeviceType::DEVICE_TYPE_MIC);
    auto status = audioManager->createStream(config, createStreamCallback);
    if (status == Status::SUCCESS) {
        std::cout << "Request to create voice stream sent." << std::endl;
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            std::cout << "Error : failed to create voice stream, error code" <<
                    static_cast<int>(error) << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        std::cout << "Request to create voice stream failed" << std::endl;
        return EXIT_FAILURE;
    }

    resetCallbackPromise();

    // ### 4. Start an Audio Stream (Voice Call Session)
    if (!audioStarted) {
        status = audioVoiceStream->startAudio(startAudioCallback);
        if (status == Status::SUCCESS) {
        std::cout << "Request to start voice stream sent." << std::endl;
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            std::cout << "Error : failed to start voice stream, error code" <<
                    static_cast<int>(error) << std::endl;
        }
        } else {
            std::cout << "Request to start voice stream failed." << std::endl;
        }
    } else {
        std::cout << "Audio started already" << std::endl;
    }

    resetCallbackPromise();

    // ### 5. Generate Dtmf Tone on Voice Stream
    if (audioStarted) {
        DtmfTone dtmfTone;
        dtmfTone.direction = StreamDirection::RX; // Using direction as RX
        dtmfTone.lowFreq = DtmfLowFreq::FREQ_697; // Taking a sample frequency
        dtmfTone.highFreq = DtmfHighFreq::FREQ_1209;  // Taking a sample frequency
        uint16_t sampleDuration = 1000; // 1000 milliseconds
        uint16_t sampleGain = 10000;
        status = audioVoiceStream->playDtmfTone(
                                        dtmfTone, sampleDuration, sampleGain, playDtmfCallback);
        if(status == Status::SUCCESS) {
            std::cout << "Request to play Dtmf Tone sent succesfully" << std::endl;
            ErrorCode error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS != error) {
                std::cout << "Error : failed to play Dtmf Tone, error code" <<
                        static_cast<int>(error) << std::endl;
            }
        } else {
            std::cout << "Request to play Dtmf Tone failed" << std::endl;
        }
    } else {
        std::cout << "Please start audio first." << std::endl;
    }

    resetCallbackPromise();

    // ### 6. Set Device for an Audio Stream (Voice Call Session)
    std::vector<DeviceType> devices;
    /*For StreamType::VOICE_CALL, sink and source device are required to be passed.
      First device should be sink (speaker) and then source (mic) should be passed for stream
      creation.*/
    devices.emplace_back(DeviceType::DEVICE_TYPE_SPEAKER); //Set new device type
    devices.emplace_back(DeviceType::DEVICE_TYPE_MIC); //Set new device type
    status = audioVoiceStream->setDevice(devices, setStreamDeviceCallback);
    if(status == Status::SUCCESS) {
        std::cout << "Request to set device sent." << std::endl;
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            std::cout << "Error : failed to set device, error code " <<
                    static_cast<int>(error) << std::endl;
        }
    } else {
        std::cout << "Request to set device failed." << std::endl;
    }

    resetCallbackPromise();

    // ### 7. Get Device details of an Audio Stream (Voice Call Session)
    status = audioVoiceStream->getDevice(getStreamDeviceCallback);
    if(status == Status::SUCCESS) {
        std::cout << "Request to get device sent." << std::endl;
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            std::cout << "Error : failed to get device, error code" <<
                    static_cast<int>(error) << std::endl;
        }
    } else {
        std::cout << "Request to get device failed." << std::endl;
    }

    resetCallbackPromise();

    // ### 8.Stop an Audio Stream (Voice Call Session)
    if (audioStarted) {
        status = audioVoiceStream->stopAudio(stopAudioCallback);
        if(status == Status::SUCCESS) {
            std::cout << "Request to stop audio sent." << std::endl;
            ErrorCode error = gCallbackPromise.get_future().get();
            if (ErrorCode::SUCCESS != error) {
                std::cout << "Error : failed to stop audio, error code " <<
                        static_cast<int>(error) << std::endl;
            }
        } else {
            std::cout << "Request to stop audio failed." << std::endl;
        }
    } else {
        std::cout << "Audio not started." << std::endl;
    }

    resetCallbackPromise();

    // ### 9. Delete an Audio Stream (Voice Call Session), which was created earlier
    status = audioManager->deleteStream(audioVoiceStream, deleteStreamCallback);
    if(status == Status::SUCCESS) {
        std::cout << "Request to delete audio stream sent." << std::endl;
        ErrorCode error = gCallbackPromise.get_future().get();
        if (ErrorCode::SUCCESS != error) {
            std::cout << "Error : failed to delete audio stream, error code " <<
                    static_cast<int>(error) << std::endl;
            return EXIT_FAILURE;
        }
    } else {
        std::cout << "Request to delete audio stream failed." << std::endl;
    }

    return EXIT_SUCCESS;
}
