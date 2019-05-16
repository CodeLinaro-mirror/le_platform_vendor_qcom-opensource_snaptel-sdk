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
#include <cstdio>
#include <cstring>
#include <future>
#include <iostream>
#include <sstream>
#include <mutex>
#include <condition_variable>
#include <queue>

#include <telux/audio/AudioFactory.hpp>
#include <telux/audio/AudioManager.hpp>

#define PRINT_MAIN_MENU_CMD 0
#define SET_TIMEOUT_CMD 20
#define EXIT_PROGRAM_CMD 21

#define AUDIO_MANAGER_IS_SUBSYSTEM_READY_CMD 1
#define AUDIO_MANAGER_ON_SUBSYSTEM_READY_CMD 2
#define AUDIO_MANAGER_GET_DEVICES_CMD 3
#define AUDIO_MANAGER_GET_STREAM_TYPES_CMD 4
#define AUDIO_MANAGER_CREATE_STREAM_CMD 5
#define AUDIO_MANAGER_DELETE_STREAM_CMD 6

#define AUDIO_STREAM_GET_TYPE_CMD 7
#define AUDIO_STREAM_SET_DEVICE_CMD 8
#define AUDIO_STREAM_GET_DEVICE_CMD 9
#define AUDIO_STREAM_SET_VOLUME_CMD 10
#define AUDIO_STREAM_GET_VOLUME_CMD 11
#define AUDIO_STREAM_SET_MUTE_CMD 12
#define AUDIO_STREAM_GET_MUTE_CMD 13

#define AUDIO_VOICE_STREAM_START_AUDIO_CMD 14
#define AUDIO_VOICE_STREAM_STOP_AUDIO_CMD 15

#define AUDIO_PLAY_STREAM_CMD 16
#define AUDIO_CAPTURE_STREAM_CMD 17

#define BITS_PER_SAMPLE 16
#define CHANNEL_TYPE_BOTH 3

static std::promise<bool> getDevicesResponse;
static std::promise<bool> getStreamTypesResponse;

static std::promise<bool> createStreamResponse;
static std::promise<bool> deleteStreamResponse;

static std::promise<bool> setStreamDeviceResponse;
static std::promise<bool> getStreamDeviceResponse;

static std::promise<bool> setStreamVolumeResponse;
static std::promise<bool> getStreamVolumeResponse;

static std::promise<bool> setStreamMuteResponse;
static std::promise<bool> getStreamMuteResponse;

static std::promise<bool> startAudioResponse;
static std::promise<bool> stopAudioResponse;

static std::condition_variable cv;

static unsigned int timeoutSec = 5;
static bool programExiting = false;
static bool voiceStreamMade = false;
static bool playStreamMade = false;
static bool captureStreamMade = false;
static bool audioStarted = false;
static std::shared_ptr<telux::audio::IAudioManager> audioManager;
static std::shared_ptr<telux::audio::IAudioStream> streamPtr;
static std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream;
static std::shared_ptr<telux::audio::IAudioPlayStream> audioPlayStream;
static std::shared_ptr<telux::audio::IAudioCaptureStream> audioCaptureStream;

static telux::audio::StreamType createStreamReqType = telux::audio::StreamType::NONE;
std::string filePath = "";
FILE * inFile , * outFile;
uint32_t sampleRate;
uint32_t channelMask;
uint32_t totalBufferToRecord = 0;
uint32_t bufferRecordedTillNow = 0;

std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> freeBuffers;

struct wav_header {
     uint32_t riff_id;
     uint32_t riff_sz;
     uint32_t riff_fmt;
     uint32_t fmt_id;
     uint32_t fmt_sz;
     uint16_t audio_format;
     uint16_t num_channels;
     uint32_t sample_rate;
     uint32_t byte_rate;
     uint16_t block_align;
     uint16_t bits_per_sample;
     uint32_t data_id;
     uint32_t data_sz;
};


static void executeIsSubsystemReady()
{
    if (audioManager->isSubsystemReady()) {
        std::cout << "isSubsystemReady(): Subsystem is ready." << std::endl;
    } else {
        std::cout << "isSubsystemReady(): Subsystem is NOT ready." << std::endl;
    }
}

static void executeOnSubsystemReady()
{
    std::future<bool> f = audioManager->onSubsystemReady();

    if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
        std::cout << "onSubsystemReady(): operation timed out." << std::endl;
    } else {
        std::cout << "onSubsystemReady(): Subsystem is ready." << std::endl;
    }
}

void getDevicesCallback(std::vector<std::shared_ptr<telux::audio::IAudioDevice>> devices,
                        telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "getDevices() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        getDevicesResponse.set_value(false);
        return;
    }
    int i = 0;
    for (auto device_type : devices) {
        std::cout << "Device [" << i << "] type: "
            << static_cast<unsigned int>(device_type->getType()) << ", direction: "
            << static_cast<unsigned int>(device_type->getDirection()) << std::endl;
        i++;
    }
    getDevicesResponse.set_value(true);
}

static void executeGetDevices()
{
    getDevicesResponse = std::promise<bool>();
    std::future<bool> f = getDevicesResponse.get_future();
    telux::common::Status status = audioManager->getDevices(getDevicesCallback);

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getDevices() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "getDevices() operation timed out." << std::endl;
        }
    }
}

void getStreamTypesCallback(std::vector<telux::audio::StreamType> streams,
                            telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "getStreamTypes() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        getStreamTypesResponse.set_value(false);
        return;
    }

    int i = 0;
    for (auto stream_type : streams) {
        std::cout << "Stream [" << i << "] type: " << static_cast<unsigned int>(stream_type)
            << std::endl;
        i++;
    }
    getStreamTypesResponse.set_value(true);
}

static void executeGetStreamTypes()
{
    getStreamTypesResponse = std::promise<bool>();
    std::future<bool> f = getStreamTypesResponse.get_future();
    telux::common::Status status = audioManager->getStreamTypes(getStreamTypesCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getStreamTypes() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "getStreamTypes() operation timed out." << std::endl;
        }
    }
}

static void takeUserCreateStreamInput(telux::audio::StreamConfig &config)
{
    config.modemSubId = 1;
    config.sampleRate = 48000;
    config.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    std::string userInput = "";
    int command = -1;
    // Take User Input for type of Stream
    while (1) {
        std::cout << std::endl;
        std::cout << "Enter Stream Type (1 for VOICE_CALL, 2 for PLAYBACK, 3 for CAPTURE): ";

        if (std::getline(std::cin, userInput)) {
            command = stoi(userInput);
            if (command == 1 || command == 2 || command == 3) {
                    break;
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
    if(command == 1)
         config.type = telux::audio::StreamType::VOICE_CALL;
    else if (command == 2)
         config.type = telux::audio::StreamType::PLAY;
    else if (command == 3)
         config.type = telux::audio::StreamType::CAPTURE;
    else
        std::cout << "Invalid Stream Type" << std::endl;
    if(command == 1 || command == 3) {
        // if the stream is voiceStream or captureStream then only
        // user defined sampleRate and channelMask required, in case
        // of playStream picked from the file.
        std::cout << std::endl;
        std::cout << "Enter Sample Rate  :" ;
        if (std::getline(std::cin, userInput)) {
            config.sampleRate = stoi(userInput);
            sampleRate = stoi(userInput);
        } else {
            std::cout << "Invalid input!" << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Enter channel mask (1 for left, 2 for right, 3 for both ): ";
        if (std::getline(std::cin, userInput)) {
            channelMask = stoi(userInput);
            config.channelTypeMask = stoi(userInput);
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    } else if( command == 2) {
        struct wav_header* meta = (wav_header*)malloc(sizeof(struct wav_header));
        while(1) {
            std::cout << "Enter File name with path :" ;
            std::getline(std::cin, filePath);
            inFile = fopen(filePath.c_str(),"r");
            if(inFile) {
                int numBytes = fread(meta, 1, sizeof(struct wav_header), inFile);
                if(numBytes != sizeof(struct wav_header)) {
                std::cout << "Wav File Header Read Mismatch " << std::endl;
                } else {
                    break;
                }
            } else {
                std::cout << "Corrupted file" <<std::endl;
            }
        }
        config.sampleRate = meta->sample_rate;
        std::cout << "Sample Rate: " << meta->sample_rate << std::endl;
        config.channelTypeMask = meta->num_channels == 2 ? 3:1;
        std::cout << "Number of Channels: " << meta->num_channels << std::endl;
    }

    // Take user input for device type
    command = -1;
    int numDevices=0;
    std::cout << std::endl;
    std::cout << "Enter no. of devices : " ;
    std::getline(std::cin, userInput);
    numDevices = stoi(userInput);
    std::cout << std::endl;
    while(numDevices) {
        std::cout << "Enter device type (1 for speaker, 257 for microphone): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                config.deviceTypes.emplace_back(
                     static_cast<telux::audio::DeviceType>(command));
                numDevices--;
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
}

void createStreamCallback(std::shared_ptr<telux::audio::IAudioStream> &stream,
                          telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "createStream() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        createStreamResponse.set_value(false);
        return;
    }

    std::cout << "createStream() succeeded." << std::endl;
    streamPtr = stream;
    if(createStreamReqType == telux::audio::StreamType::VOICE_CALL) {
        audioVoiceStream = std::dynamic_pointer_cast<telux::audio::IAudioVoiceStream>(streamPtr);
        voiceStreamMade = true;
    }
    else if (createStreamReqType == telux::audio::StreamType::PLAY) {
        audioPlayStream = std::dynamic_pointer_cast<telux::audio::IAudioPlayStream>(streamPtr);
        playStreamMade = true;
    }
    else if (createStreamReqType == telux::audio::StreamType::CAPTURE) {
       audioCaptureStream = std::dynamic_pointer_cast<telux::audio::IAudioCaptureStream>(streamPtr);
       captureStreamMade = true;
    }
    else {
        std::cout << "Unknown type of stream created" << std::endl;
    }
    createStreamResponse.set_value(true);
}

static void executeCreateStream()
{
    telux::audio::StreamConfig config;
    takeUserCreateStreamInput(config);
    createStreamReqType = config.type;
    createStreamResponse = std::promise<bool>();
    std::future<bool> f = createStreamResponse.get_future();
    telux::common::Status status = audioManager->createStream(config, createStreamCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "createStream() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "createStream() operation timed out." << std::endl;
        }
    }
}

void deleteStreamCallback(telux::common::ErrorCode error) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "deleteStream() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        deleteStreamResponse.set_value(false);
        return;
    }

    std::cout << "deleteStream() succeeded." << std::endl;
    if(voiceStreamMade) {
        voiceStreamMade = false;
        audioVoiceStream.reset();
        streamPtr.reset();
    } else if (playStreamMade) {
        playStreamMade = false;
        audioPlayStream.reset();
        streamPtr.reset();
    } else if (captureStreamMade) {
        captureStreamMade = false;
        audioCaptureStream.reset();
        streamPtr.reset();
    } else {
        std::cout << "Unknown Stream Type deleted" << std::endl;
    }
    deleteStreamResponse.set_value(true);
}

static void executeDeleteStream()
{
    deleteStreamResponse = std::promise<bool>();
    std::future<bool> f = deleteStreamResponse.get_future();
    telux::common::Status status = audioManager->deleteStream(streamPtr, deleteStreamCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "deleteStream() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "deleteStream() operation timed out." << std::endl;
        }
    }
}

static void executeGetType()
{
    int streamType = static_cast<int>(streamPtr->getType());
    std::cout << "Stream type: ";

    switch (streamType) {
        case -1:
            std::cout << "NONE";
            break;
        case 1:
            std::cout << "VOICE_CALL";
            break;
        case 2:
            std::cout << "PLAY";
            break;
        case 3:
            std::cout << "CAPTURE";
            break;
    }
    std::cout << std::endl;
}

static void takeUserSetDeviceInput(std::vector<telux::audio::DeviceType> &devices)
{
    std::string userInput = "";
    int command = -1;
    int numDevices=0;
    std::cout << std::endl;
    std::cout << "Enter no. of devices : " ;
    std::getline(std::cin, userInput);
    numDevices = stoi(userInput);
    std::cout << std::endl;
    while(numDevices) {
        std::cout << "Enter device type (1 for speaker, 257 for microphone): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                devices.emplace_back(static_cast<telux::audio::DeviceType>(command));
                numDevices--;
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
}

void setStreamDeviceCallback(telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "setDevice() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        setStreamDeviceResponse.set_value(false);
        return;
    }

    std::cout << "setDevice() succeeded." << std::endl;
    setStreamDeviceResponse.set_value(true);
}

static void executeSetDevice()
{
    std::vector<telux::audio::DeviceType> devices;
    takeUserSetDeviceInput(devices);

    setStreamDeviceResponse = std::promise<bool>();
    std::future<bool> f = setStreamDeviceResponse.get_future();
    telux::common::Status status = streamPtr->setDevice(devices, setStreamDeviceCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "setDevice() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "setDevice() operation timed out." << std::endl;
        }
    }
}

void getStreamDeviceCallback(std::vector<telux::audio::DeviceType> devices,
                             telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "getDevice() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        getStreamDeviceResponse.set_value(false);
        return;
    }

    int i = 0;
    for (auto device_type : devices) {
        std::cout << "Device [" << i << "] type: " << static_cast<uint32_t>(device_type)
            << std::endl;
        i++;
    }
    getStreamDeviceResponse.set_value(true);
}

static void executeGetDevice()
{
    getStreamDeviceResponse = std::promise<bool>();
    std::future<bool> f = getStreamDeviceResponse.get_future();
    telux::common::Status status = streamPtr->getDevice(getStreamDeviceCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getDevice() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "getDevice() operation timed out." << std::endl;
        }
    }
}

static float takeUserChannelVolumeInput(std::string channel)
{
    std::string userInput = "";
    float vol = 0.0;

    while (1) {
        std::cout << std::endl;
        std::cout << "Enter " << channel << " channel between volume 0 to 1 (e.g. 0.5): ";

        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);

            if (inputStream >> vol) {
                break;
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
    return vol;
}

static void takeUserSetVolumeInput(telux::audio::StreamVolume &streamVol)
{
    telux::audio::ChannelVolume channelVol;
    std::string userInput = "";
    int command = -1;
    // Take user input for stream direction
    while (1) {
        std::cout << std::endl;
        std::cout << "Enter stream direction (1 for RX, 2 for TX): ";

        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);

            if (inputStream >> command) {
                if (command == 1 || command == 2) {
                    break;
                } else {
                    std::cout << "Invalid input!" << std::endl;
                }
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
    streamVol.dir = static_cast<telux::audio::StreamDirection>(command);

    command = -1;

    while (1) {
        std::cout << std::endl;
        std::cout <<
          "Enter stream channel need update (1 for left only, 2 for right only, rest for both): ";

        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);

            if (inputStream >> command) {
                    break;
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }

    if (command != 2 ) {
        // Take user input for left channel volume
        channelVol.channelType = telux::audio::ChannelType::LEFT;
        channelVol.vol = takeUserChannelVolumeInput("left");
        streamVol.volume.emplace_back(channelVol);
    }

    if (command != 1 ) {
        // Take user input for right channel volume
        channelVol.channelType = telux::audio::ChannelType::RIGHT;
        channelVol.vol = takeUserChannelVolumeInput("right");
        streamVol.volume.emplace_back(channelVol);
    }
}

void setStreamVolumeCallback(telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "setVolume() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        setStreamVolumeResponse.set_value(false);
        return;
    }

    std::cout << "setVolume() succeeded." << std::endl;
    setStreamVolumeResponse.set_value(true);
}

static void executeSetVolume()
{
    telux::audio::StreamVolume streamVol;
    takeUserSetVolumeInput(streamVol);

    setStreamVolumeResponse = std::promise<bool>();
    std::future<bool> f = setStreamVolumeResponse.get_future();
    telux::common::Status status = streamPtr->setVolume(streamVol, setStreamVolumeCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "setVolume() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "setVolume() operation timed out." << std::endl;
        }
    }
}

void getStreamVolumeCallback(telux::audio::StreamVolume volume,
                             telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "getVolume() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        getStreamVolumeResponse.set_value(false);
        return;
    }

    std::cout << "Volume direction: " << static_cast<uint32_t>(volume.dir) << std::endl;

    int i = 0;
    for (auto channelVolume : volume.volume) {
        std::cout << "ChannelVolume [" << i << "] channel type: "
            << static_cast<uint32_t>(channelVolume.channelType) << ", " << "volume: "
            << channelVolume.vol << std::endl;
    }
    getStreamVolumeResponse.set_value(true);
}

static void executeGetVolume()
{
    getStreamVolumeResponse = std::promise<bool>();
    std::future<bool> f = getStreamVolumeResponse.get_future();
    telux::common::Status status = streamPtr->getVolume(telux::audio::StreamDirection::RX,
                                                               getStreamVolumeCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getVolume() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "getVolume() operation timed out." << std::endl;
        }
    }
}

static void takeUserSetMuteInput(telux::audio::StreamMute &mute)
{
    std::string userInput = "";
    int command = -1;

    // Take user input for stream direction
    while (1) {
        std::cout << std::endl;
        std::cout << "Enter stream direction (1 for RX, 2 for TX): ";

        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);

            if (inputStream >> command) {
                if (command == 1 || command == 2) {
                    break;
                } else {
                    std::cout << "Invalid input!" << std::endl;
                }
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
    mute.dir = static_cast<telux::audio::StreamDirection>(command);

    // Take user input for mute enable
    while (1) {
        std::cout << std::endl;
        std::cout << "Enter mute enable (0 to disable, 1 to enable): ";

        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);

            if (inputStream >> command) {
                if (command == 0 || command == 1) {
                    break;
                } else {
                    std::cout << "Invalid input!" << std::endl;
                }
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
    mute.enable = command;
}

void setStreamMuteCallback(telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "setMute() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        setStreamMuteResponse.set_value(false);
        return;
    }

    std::cout << "setMute() succeeded." << std::endl;
    setStreamMuteResponse.set_value(true);
}

static void executeSetMute()
{
    telux::audio::StreamMute mute;
    takeUserSetMuteInput(mute);

    setStreamMuteResponse = std::promise<bool>();
    std::future<bool> f = setStreamMuteResponse.get_future();
    telux::common::Status status = streamPtr->setMute(mute, setStreamMuteCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "setMute() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "setMute() operation timed out." << std::endl;
        }
    }
}

static void takeUserGetMuteInput(telux::audio::StreamDirection &strmDir)
{
    std::string userInput = "";
    int command = -1;

    // Take user input for stream direction
    while (1) {
        std::cout << std::endl;
        std::cout << "Enter stream direction (1 for RX, 2 for TX): ";

        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);

            if (inputStream >> command) {
                if (command == 1 || command == 2) {
                    break;
                } else {
                    std::cout << "Invalid input!" << std::endl;
                }
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
    strmDir = static_cast<telux::audio::StreamDirection>(command);
}

void getStreamMuteCallback(telux::audio::StreamMute mute,
                           telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "getMute() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        getStreamMuteResponse.set_value(false);
        return;
    }

    std::cout << "Mute enable: " << mute.enable << ", direction: "
        << static_cast<uint32_t>(mute.dir) << std::endl;

    getStreamMuteResponse.set_value(true);
}

static void executeGetMute()
{
    telux::audio::StreamDirection strmDir;
    takeUserGetMuteInput(strmDir);

    getStreamMuteResponse = std::promise<bool>();

    std::future<bool> f = getStreamMuteResponse.get_future();
    telux::common::Status status = streamPtr->getMute(strmDir,
                                                             getStreamMuteCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "getMute() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "getMute() operation timed out." << std::endl;
        }
    }
}

void startAudioCallback(telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "startAudio() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        startAudioResponse.set_value(false);
        return;
    }

    std::cout << "startAudio() succeeded." << std::endl;
    audioStarted = true;
    startAudioResponse.set_value(true);
}

static void executeStartAudio()
{
    startAudioResponse = std::promise<bool>();
    std::future<bool> f = startAudioResponse.get_future();
    telux::common::Status status = audioVoiceStream->startAudio(startAudioCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "startAudio() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "startAudio() operation timed out." << std::endl;
        }
    }
}

void stopAudioCallback(telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "stopAudio() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
        stopAudioResponse.set_value(false);
        return;
    }

    std::cout << "stopAudio() succeeded." << std::endl;
    audioStarted = false;
    stopAudioResponse.set_value(true);
}

static void executeStopAudio()
{
    stopAudioResponse = std::promise<bool>();
    std::future<bool> f = stopAudioResponse.get_future();
    telux::common::Status status = audioVoiceStream->stopAudio(stopAudioCallback);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "stopAudio() failed with error " << static_cast<unsigned int>(status)
            << std::endl;
    } else {
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "stopAudio() operation timed out." << std::endl;
        }
    }
}

void writeCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytes,
                telux::common::ErrorCode error)
{
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "write() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
    } else {
        buffer->reset();
    }
    freeBuffers.push(buffer);
    cv.notify_all();
    return;
}

void readCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
           telux::common::ErrorCode error)
{
    uint32_t bytesWrittenToFile = 0;
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
    } else {
        uint32_t size = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(),1,size,outFile);
        if(bytesWrittenToFile != size) {
            std::cout << "Write Size mismatch while writing to file" << std::endl;
        }
        bufferRecordedTillNow = bufferRecordedTillNow + size;
        buffer->reset();
    }
    freeBuffers.push(buffer);
    cv.notify_all();
    return;
}

// This function is to update the headers for the recorded file
void updateHeaders(uint32_t numChannels, uint32_t sampleRate) {
    int temp = 0;
    fseek(outFile, 0, SEEK_SET);
    fwrite("RIFF", sizeof(uint32_t), 1,outFile);
    // temp stores value of total size of file
    temp = bufferRecordedTillNow + sizeof(struct wav_header) - 8;
    fwrite( &temp, sizeof(uint32_t), 1, outFile);
    fwrite("WAVE", sizeof(uint32_t), 1,outFile);
    fwrite("fmt ", sizeof(uint32_t), 1,outFile);
    const int formatDataSize = 16; // this is constant for wav headers
    fwrite( &formatDataSize, sizeof(uint32_t), 1, outFile);
    const int formatType = 1; // storing the audio format 1 for PCM
    fwrite( &formatType, sizeof(uint16_t), 1, outFile);
    std::cout << "No. of channels: " << numChannels << std::endl;
    fwrite( &numChannels, sizeof(uint16_t), 1, outFile);
    fwrite( &sampleRate, sizeof(uint32_t), 1, outFile);
    temp = (sampleRate* numChannels * BITS_PER_SAMPLE)/8;  // now temp contains byte rate
    fwrite( &temp, sizeof(uint32_t), 1, outFile);
    temp = (numChannels * BITS_PER_SAMPLE)/8 ;
    fwrite( &temp, sizeof(uint16_t), 1, outFile);
    temp = BITS_PER_SAMPLE; // temp now contains bits per sample
    fwrite( &temp, sizeof(uint16_t), 1, outFile);
    fwrite("data", sizeof(uint32_t), 1,outFile);
    temp = bufferRecordedTillNow;// temp here contains data size
    fwrite( &temp, sizeof(uint32_t), 1, outFile);
    std::cout << "create file successful" << std::endl;
    std::cout << "File pointer location: " << ftell(outFile) << std::endl;
    fflush(outFile);
    fclose(outFile);
}

// this function is for clearing buffer queue that stores buffers to read and write
void clearBufferQueue(){
    while(!freeBuffers.empty()) {
            freeBuffers.pop();
        }
}

static void play() {
    clearBufferQueue();
    // Taking the file pointer to point data section after headers
    fseek(inFile, sizeof(struct wav_header), SEEK_SET);
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    uint32_t size = 0;
    std::string userInput ="";
    uint32_t numBytes =0;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    // Here we are creating two buffers to feed data to have ping pong behaviour.
    for(int i = 0; i < 2; i++) {
        streamBuffer = audioPlayStream->getStreamBuffer();
        if(streamBuffer != nullptr) {
             // Setting the size that is to be written to stream as the minimum size
             // required by stream. In any case if size returned is 0, using the Maximum
             // Buffer Size, any buffer size between min and max can be used
             size = streamBuffer->getMinSize();
             if(size == 0) {
                 size =  streamBuffer->getMaxSize();
             }
             streamBuffer->setDataSize(size);
             std::cout << "Buffer no. " << i << " buffer size "<< streamBuffer->getDataSize()
                    << std::endl;
             freeBuffers.push(streamBuffer);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
        }
    }

    while (!feof(inFile))
    {
        if(!freeBuffers.empty()){
            streamBuffer = freeBuffers.front();
            freeBuffers.pop();
            numBytes = fread(streamBuffer->getRawBuffer(),1,size,inFile);
            if(numBytes != size) {
                std::cout << "Readsize mismatch Error or EOF reached = " << numBytes<< std::endl;
            }
            telux::common::Status status = audioPlayStream->write(streamBuffer,writeCallback);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "write() failed with error" << static_cast<unsigned int>(status)
                    <<std::endl;
            }
        } else {
            cv.wait(lock);
        }
    }
    std::cout << "File played SuccessFully" <<std::endl;
}

static void record() {
    bufferRecordedTillNow = 0;
    uint32_t bytesToRead = 0;
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    std::string userInput ="";

    std::cout << "Enter File name with path : "  ;
    std::string filePath = "";
    std::getline(std::cin, filePath);

    std::cout << "How much time you want to record in seconds : " ;
    std::getline(std::cin, userInput);
    int timeInSeconds = stoi(userInput);
    clearBufferQueue();
    // Creating two buffers to fetch the data, to have ping pong behaviour
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    for(int i = 0; i < 2; i++) {
        streamBuffer = audioCaptureStream->getStreamBuffer();
        if(streamBuffer != nullptr) {
            // Setting the bytesToRead (bytes to be readed from stream) as minimum size
            // required by stream. In any case if size returned is 0, using the Maximum Buffer
            // Size, any buffer size between min and max can be used
            bytesToRead = streamBuffer->getMinSize();
            if(bytesToRead == 0) {
                bytesToRead = streamBuffer->getMaxSize();
            }
            freeBuffers.push(streamBuffer);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
        }
    }
    int numChannels = (channelMask == CHANNEL_TYPE_BOTH) ?  2 : 1;  // numChannels here stores num of channels;
    totalBufferToRecord = (timeInSeconds * sampleRate * numChannels * BITS_PER_SAMPLE)/8;
    outFile = fopen(filePath.c_str(),"w");
    if(outFile) {
        fseek(outFile,0,SEEK_SET);
        // Leaving starting bytes a place for headers will be written after file recorded.
        fseek(outFile,sizeof(struct wav_header),SEEK_SET);
    } else {
        std::cout << "Unable to Create File " <<std::endl;
        return;
    }
    while ((totalBufferToRecord - bufferRecordedTillNow) > 0)
    {
        if(!freeBuffers.empty()) {
            streamBuffer = freeBuffers.front();
            freeBuffers.pop();
            telux::common::Status status = audioCaptureStream->read(streamBuffer,
                                                           bytesToRead, readCallback);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "read() failed with error" << static_cast<unsigned int>(status)
                   <<std::endl;
            }
        } else {
            cv.wait(lock);
        }
    }
    cv.wait_for(lock, std::chrono::milliseconds(500));
    updateHeaders(numChannels, sampleRate);
    std::cout << "File Recorded SuccessFully" <<std::endl;
}

static void printMainMenu()
{
    std::cout << std::endl;
    std::cout << "********** Main Menu **********" << std::endl;

    std::cout << std::endl;
    std::cout << "-- AudioManager --" << std::endl;
    std::cout << AUDIO_MANAGER_IS_SUBSYSTEM_READY_CMD << ":\tisSubsystemReady()" << std::endl;
    std::cout << AUDIO_MANAGER_ON_SUBSYSTEM_READY_CMD << ":\tonSubsystemReady()" << std::endl;
    std::cout << AUDIO_MANAGER_GET_DEVICES_CMD << ":\tgetDevices()" << std::endl;
    std::cout << AUDIO_MANAGER_GET_STREAM_TYPES_CMD << ":\tgetStreamTypes()" << std::endl;
    if (!voiceStreamMade && !playStreamMade && !captureStreamMade) {
        std::cout << AUDIO_MANAGER_CREATE_STREAM_CMD << ":\tcreateStream()" << std::endl;
    } else {
        std::cout << AUDIO_MANAGER_DELETE_STREAM_CMD << ":\tdeleteStream()" << std::endl;
    }
    std::cout << std::endl;

    if (voiceStreamMade || playStreamMade || captureStreamMade) {
        std::cout << "-- Stream --" << std::endl;
        std::cout << AUDIO_STREAM_GET_TYPE_CMD << ":\tgetType()" << std::endl;
        std::cout << AUDIO_STREAM_SET_DEVICE_CMD << ":\tsetDevice()" << std::endl;
        std::cout << AUDIO_STREAM_GET_DEVICE_CMD << ":\tgetDevice()" << std::endl;
        std::cout << AUDIO_STREAM_SET_VOLUME_CMD << ":\tsetVolume()" << std::endl;
        std::cout << AUDIO_STREAM_GET_VOLUME_CMD << ":\tgetVolume()" << std::endl;
        std::cout << AUDIO_STREAM_SET_MUTE_CMD << ":\tsetMute()" << std::endl;
        std::cout << AUDIO_STREAM_GET_MUTE_CMD << ":\tgetMute()" << std::endl;
        std::cout << std::endl;

        if(voiceStreamMade) {
            std::cout << "-- AudioVoiceStream --" << std::endl;
            if (!audioStarted) {
                std::cout << AUDIO_VOICE_STREAM_START_AUDIO_CMD << ":\tstartAudio()" << std::endl;
            } else {
                std::cout << AUDIO_VOICE_STREAM_STOP_AUDIO_CMD << ":\tstopAudio()" << std::endl;
            }
            std::cout << std::endl;
        }
        if(playStreamMade) {
            std::cout << "-- AudioPlayStream --" << std::endl;
                std::cout << AUDIO_PLAY_STREAM_CMD << ":\tPlay Audio()" << std::endl;
            std::cout << std::endl;
        }
        if(captureStreamMade) {
            std::cout << "-- AudioCaptureStream --" << std::endl;
                std::cout << AUDIO_CAPTURE_STREAM_CMD<< ":\tStart Record()" << std::endl;
            std::cout << std::endl;
        }
    }


    std::cout << "-- Options --" << std::endl;
    std::cout << PRINT_MAIN_MENU_CMD << ":\tPrint main menu" << std::endl;
    std::cout << SET_TIMEOUT_CMD << ":\tSet custom timeout value for commands" << std::endl;
    std::cout << EXIT_PROGRAM_CMD << ":\tExit" << std::endl;
    std::cout << std::endl << "*******************************" << std::endl;
}

static int takeUserMenuInput()
{
    std::string userInput = "";
    int command = -1;

    while (1) {
        std::cout << std::endl;
        std::cout << "Enter a command number: ";

        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);

            if (inputStream >> command) {
                break;
            } else {
                std::cout << "Invalid input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }

    if ((!(voiceStreamMade || playStreamMade || captureStreamMade ) &&
        command > AUDIO_MANAGER_CREATE_STREAM_CMD && command != SET_TIMEOUT_CMD &&
            command != PRINT_MAIN_MENU_CMD && command != EXIT_PROGRAM_CMD) ||
        (voiceStreamMade && command == AUDIO_MANAGER_CREATE_STREAM_CMD) ||
        (!audioStarted && command == AUDIO_VOICE_STREAM_STOP_AUDIO_CMD) ||
        (audioStarted && command == AUDIO_VOICE_STREAM_START_AUDIO_CMD) ||
        (playStreamMade && command == AUDIO_MANAGER_CREATE_STREAM_CMD) ||
        (captureStreamMade && command == AUDIO_MANAGER_CREATE_STREAM_CMD)
        ) {
        command = -1;
    }

    return command;
}

static void setTimeout()
{
    std::string userInput = "";
    unsigned int oldTimeoutSec = timeoutSec;

    while (1) {
        std::cout << "Enter timeout value in seconds (current: " << timeoutSec << "): ";

        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);

            if (inputStream >> timeoutSec) {
                break;
            } else {
                std::cout << "Invalid input!" << std::endl;
                timeoutSec = oldTimeoutSec;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
}

static void executeCommand(int command)
{
    switch (command) {
        case AUDIO_MANAGER_IS_SUBSYSTEM_READY_CMD:
            executeIsSubsystemReady();
            break;
        case AUDIO_MANAGER_ON_SUBSYSTEM_READY_CMD:
            executeOnSubsystemReady();
            break;
        case AUDIO_MANAGER_GET_DEVICES_CMD:
            executeGetDevices();
            break;
        case AUDIO_MANAGER_GET_STREAM_TYPES_CMD:
            executeGetStreamTypes();
            break;
        case AUDIO_MANAGER_CREATE_STREAM_CMD:
            executeCreateStream();
            printMainMenu();
            break;
        case AUDIO_MANAGER_DELETE_STREAM_CMD:
            executeDeleteStream();
            printMainMenu();
            break;
        case AUDIO_STREAM_GET_TYPE_CMD:
            executeGetType();
            break;
        case AUDIO_STREAM_SET_DEVICE_CMD:
            executeSetDevice();
            break;
        case AUDIO_STREAM_GET_DEVICE_CMD:
            executeGetDevice();
            break;
        case AUDIO_STREAM_SET_VOLUME_CMD:
            executeSetVolume();
            break;
        case AUDIO_STREAM_GET_VOLUME_CMD:
            executeGetVolume();
            break;
        case AUDIO_STREAM_SET_MUTE_CMD:
            executeSetMute();
            break;
        case AUDIO_STREAM_GET_MUTE_CMD:
            executeGetMute();
            break;
        case AUDIO_VOICE_STREAM_START_AUDIO_CMD:
            executeStartAudio();
            printMainMenu();
            break;
        case AUDIO_VOICE_STREAM_STOP_AUDIO_CMD:
            executeStopAudio();
            printMainMenu();
            break;
        case AUDIO_PLAY_STREAM_CMD:
            play();
            printMainMenu();
            break;
        case AUDIO_CAPTURE_STREAM_CMD:
            record();
            printMainMenu();
            break;
        case PRINT_MAIN_MENU_CMD:
            printMainMenu();
            break;
        case SET_TIMEOUT_CMD:
            setTimeout();
            break;
        case EXIT_PROGRAM_CMD:
            programExiting = true;
            break;
        default:
            std::cout << "Invalid input!" << std::endl;
            break;
    }
}

int main(int argc, char **argv)
{
    auto &audioFactory = telux::audio::AudioFactory::getInstance();
    audioManager = audioFactory.getAudioManager();

    printMainMenu();

    while (!programExiting) {
        int command = takeUserMenuInput();
        executeCommand(command);
    }

    return EXIT_SUCCESS;
}
