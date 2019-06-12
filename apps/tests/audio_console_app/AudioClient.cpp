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
 * Audio Client class provides functionality in SDK to create Audio Stream,
 * start/stop Audio on the created Stream and delete the Stream.
 */

#include <chrono>
#include <iostream>
#include <stdio.h>

#include <telux/audio/AudioFactory.hpp>

#include "AudioClient.hpp"

AudioClient::AudioClient() {
    sampleRate_ = 0;
    channelType_ = 0;
    filepath_ = "";
    audioManager_ = nullptr;
    stream_ = nullptr;
    audioVoiceStream_ = nullptr;
    audioPlayStream_ = nullptr;
    audioCaptureStream_ = nullptr;
}

AudioClient::~AudioClient() {
}

void AudioClient::init() {
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    //  Get the AudioFactory and AudioManager instances.
    auto &audioFactory = telux::audio::AudioFactory::getInstance();
    audioManager_ = audioFactory.getAudioManager();

    //  Check if audio subsystem is ready
    bool audioSubSystemStatus = audioManager_->isSubsystemReady();

    //  If audio subsystem is not ready, wait for it to be ready
    if(!audioSubSystemStatus) {
        std::cout << "\nAudio subsystem is not ready, Please wait!!!..." << std::endl;
        std::future<bool> f = audioManager_->onSubsystemReady();
        // If we want to wait unconditionally for audio subsystem to be ready
        audioSubSystemStatus = f.get();
    }

    //  Exit the application, if SDK is unable to initialize audio subsystems
    if(audioSubSystemStatus) {
        endTime = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Audio Subsystems to ready : " << elapsedTime.count() << "s"
                << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize audio subsystem" << std::endl;
        return;
    }
}

void AudioClient::resolveStreamType(StreamType streamType) {
    if(streamType == StreamType::VOICE_CALL){
        stream_ = audioVoiceStream_;
    } else if( streamType == StreamType::PLAY) {
        stream_ = audioPlayStream_;
    } else if( streamType == StreamType::CAPTURE) {
        stream_ = audioCaptureStream_;
    } else {
        stream_ = nullptr;
    }
}

void AudioClient::takeUserModemIdInput(int &modemId) {
    std::string userInput = "";
    while(1) {
        std::cout << "Enter Modem Id: ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> modemId) {
                break;
            } else {
                std::cout << "Invalid Input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
}

void AudioClient::takeUserSampleRateInput(uint32_t &userSampleRate) {
    std::string userInput = "";
    while(1) {
        std::cout << "Enter Sample Rate (16000 32000 48000) :" ;
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> userSampleRate) {
                break;
            } else {
                std::cout << "Invalid Input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
}

void AudioClient::takeUserChannelInput(telux::audio::ChannelTypeMask &channelType) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Enter channel mask (1 for left, 2 for right, 3 for both): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> command) {
                if(command == 1 || command == 2 || command == 3){
                    if(command == 1) {
                        channelType = ChannelType::LEFT;
                    } else if(command == 2) {
                        channelType = ChannelType::RIGHT;
                    } else {
                        channelType = (ChannelType::LEFT | ChannelType::RIGHT);
                    }
                    break;
                } else {
                    std::cout << "Invalid Input!" << std::endl;
                }
            } else {
                std::cout << "Invalid Input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
}

void AudioClient::takeUserDeviceInput(std::vector<telux::audio::DeviceType> &devices) {
    std::string userInput = "";
    int command = -1;
    int numDevices=0;
    while(1) {
        std::cout << "Enter no. of devices : " ;
        if(std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> numDevices){
                break;
            } else {
                std::cout << "Invalid Input!" << std::endl;
            }
        }
    }

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

void AudioClient::takeVolumeValueInput(float &vol) {
    std::string userInput = "";
    while(1) {
        std::cout << "Enter Volume :";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> vol) {
                break;
            } else {
                std::cout << "Invalid Input!" << std::endl;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }
    }
}

void AudioClient::takeUserVolumeInput(StreamVolume &streamVolume) {

    ChannelTypeMask channelType;
    ChannelVolume channelVolume;
    float vol;

    takeUserDirectionInput(streamVolume.dir);
    takeUserChannelInput(channelType);

    if(channelType != ChannelType::LEFT){
        std::cout << "For Right Channel " << std::endl;
        takeVolumeValueInput(vol);
        channelVolume.channelType = ChannelType::RIGHT;
        channelVolume.vol = vol;
        streamVolume.volume.emplace_back(channelVolume);
    }

    if(channelType != ChannelType::RIGHT){
        std::cout << "For Left Channel " << std::endl;
        takeVolumeValueInput(vol);
        channelVolume.channelType = ChannelType::LEFT;
        channelVolume.vol = vol;
        streamVolume.volume.emplace_back(channelVolume);
    }
}

void AudioClient::takeUserCreateStreamInput(telux::audio::StreamConfig &config)
{
    config.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    takeUserModemIdInput(config.modemSubId);

    takeUserSampleRateInput(config.sampleRate);
    takeUserChannelInput(config.channelTypeMask);
    sampleRate_ = config.sampleRate;
    channelType_ = config.channelTypeMask;

    if( config.type == telux::audio::StreamType::PLAY) {
        FILE * file;
        while(1) {
            std::cout << "Enter File name with path :" ;
            std::getline(std::cin, filepath_);
            file = fopen(filepath_.c_str(),"r");
            if(file) {
                fseek(file, 0 , SEEK_SET);
                break;
            } else {
                std::cout << "Corrupted file" <<std::endl;
            }
        }
        fclose(file);
    }
    takeUserDeviceInput(config.deviceTypes);
}

void AudioClient::takeUserDirectionInput(StreamDirection &direction) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Enter direction of stream: (0 for TX, 1 for RX) ";
        if(std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(inputStream >> command){
                if(command == 0 || command == 1) {
                    break;
                } else {
                    std::cout << "Invalid Input!" << std::endl;
                }
            } else {
                std::cout << "Invalid Input!" << std::endl;
            }
        }
    }

    if(command == 0) {
        direction = telux::audio::StreamDirection::TX;
    } else if (command == 1) {
        direction = telux::audio::StreamDirection::RX;
    }
}

std::shared_ptr<IAudioStream> AudioClient::getStream(StreamType streamtype) {
    resolveStreamType(streamtype);
    return stream_;
}

void AudioClient::getCaptureConfig(uint32_t &sampleRate, uint32_t &channelType) {
    sampleRate = sampleRate_;
    channelType = channelType_;
}

std::string AudioClient::getFilePathForPlay() {
    return filepath_;
}

Status AudioClient::createStream(StreamType streamType) {

    std::promise<bool> p;
    StreamConfig streamConfig;
    // Initialising the Configuration of stream
    streamConfig.type = streamType;
    takeUserCreateStreamInput(streamConfig);

    std::shared_ptr<telux::audio::IAudioStream> myAudioStream;
    //Sending a request to create audio stream
    Status audioStatus = audioManager_->createStream(streamConfig,
        [&p,&myAudioStream,this](std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                myAudioStream = audioStream;
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "failed to Create a stream" <<std::endl;
            }
        });
    if(audioStatus == Status::SUCCESS) {
        std::cout << "Request to create stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
    }

    if (p.get_future().get()) {
        std::cout<< "Audio Stream is Created" << std::endl;
        if(myAudioStream->getType() == StreamType::VOICE_CALL) {
            audioVoiceStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioVoiceStream>(myAudioStream);
            std::cout<< "Audio Voice Stream is Created" << std::endl;
        } else if(myAudioStream->getType() == StreamType::PLAY) {
            audioPlayStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioPlayStream>(myAudioStream);
            std::cout<< "Audio Play Stream is Created" << std::endl;
        } else if(myAudioStream->getType() == StreamType::CAPTURE) {
            audioCaptureStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioCaptureStream>(myAudioStream);
            std::cout<< "Audio Capture Stream is Created" << std::endl;
        } else {
            std::cout << "Unknown Stream type is generated" << std::endl;
        }
    }
    return Status::SUCCESS;
}

Status AudioClient::deleteStream(StreamType streamType) {
    resolveStreamType(streamType);
    std::promise<bool> p;
    telux::common::Status deleteStreamStatus = audioManager_-> deleteStream(
    stream_, [&p,this](telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
        p.set_value(true);
        } else {
        p.set_value(false);
        std::cout << "Failed to delete a stream" << std::endl;
        }
    });
    if(deleteStreamStatus == Status::SUCCESS) {
        std::cout << "request to delete stream sent" << std::endl;
    } else {
        std::cout << "Request to delete stream failed"  << std::endl;
    }
    if (p.get_future().get()) {
        if(streamType == StreamType::VOICE_CALL) {
            audioVoiceStream_= nullptr;
        } else if(streamType == StreamType::PLAY) {
            audioPlayStream_= nullptr;
        } else if(streamType == StreamType::CAPTURE) {
            audioCaptureStream_= nullptr;
        } else {
            std::cout << " Unknown Stream Type " << std::endl;
        }
        std::cout << "Audio Stream is Deleted" << std::endl;
    }
    return Status::SUCCESS;
}

void AudioClient::getStreamDevice(StreamType streamType) {
    resolveStreamType(streamType);
    std::promise<bool> p;
    std::vector<telux::audio::DeviceType> devices_;
    if(stream_) {
        telux::common::Status status = stream_->getDevice(
            [&p, &devices_, this](std::vector<telux::audio::DeviceType> devices,
                     telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
                devices_ = devices;
            } else {
                p.set_value(false);
                std::cout << "Failed to get stream device" << std::endl;
            }
        });
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to get device sent" << std::endl;
        } else {
            std::cout << "Request to get device failed" << std::endl;
        }

        if (p.get_future().get()) {
            for (auto deviceType : devices_) {
                std::string deviceName;
                std::cout << "Device Type"  << (static_cast<uint32_t>(deviceType)) << std::endl;
            }
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
    }
}

void AudioClient::setStreamDevice(StreamType streamType) {
    resolveStreamType(streamType);
    std::promise<bool> p;
    if(stream_) {
        std::vector<telux::audio::DeviceType> devices;
        takeUserDeviceInput(devices);
        telux::common::Status status = stream_->setDevice(devices,
           [&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to set stream device" << std::endl;
            }
        });
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to set device sent" << std::endl;
        } else {
            std::cout << "Request to set device failed" << std::endl;
        }
        if (p.get_future().get()) {
             std::cout << "set stream device succeeded." << std::endl;
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
    }
}

void AudioClient::setVolume(StreamType streamType) {
    resolveStreamType(streamType);
    std::promise<bool> p;
    if(stream_) {
        telux::audio::StreamVolume streamVol;
        takeUserVolumeInput(streamVol);
        telux::common::Status status = stream_->setVolume(streamVol,
              [&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to set stream volume" << std::endl;
            }
        });
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to set volume sent" << std::endl;
        } else {
            std::cout << "Request to set volume failed" << std::endl;
        }
        if (p.get_future().get()) {
            std::cout << "setStreamVolume() succeeded." << std::endl;
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
    }
}

void AudioClient::getVolume(StreamType streamType) {
    resolveStreamType(streamType);
    std::promise<bool> p;
    telux::audio::StreamVolume vol;
    if(stream_) {
        telux::audio::StreamDirection dir;
        takeUserDirectionInput(dir);
        telux::common::Status status = stream_->getVolume(
           dir,  [&p,&vol,this](telux::audio::StreamVolume volume, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
                vol = volume;
            } else {
                p.set_value(false);
                std::cout << "Failed to set stream device" << std::endl;
            }
        });
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to get volume sent" << std::endl;
        } else {
            std::cout << "Request to get volume failed" << std::endl;
        }

        if (p.get_future().get()) {
            for (auto channelVolume : vol.volume) {
                std::cout << "volume: "<< channelVolume.vol << std::endl;
            }
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
    }
}

void AudioClient::setMute(StreamType streamType) {
    resolveStreamType(streamType);
    StreamMute mute;
    std::promise<bool> p;
    if(stream_) {
        takeUserDirectionInput(mute.dir);

        std::string userInput = "";
        int muteStatus;
        while(1) {
            std::cout << " Enter 0 to Unmute and 1 to Mute" ;
            if(std::getline(std::cin, userInput)) {
                std::stringstream inputStream(userInput);
                if(inputStream >> muteStatus) {
                    if(muteStatus == 0 || muteStatus == 1) {
                        break;
                    } else {
                        std::cout << "Invalid Input!" << std::endl;
                    }
                } else {
                    std::cout << "Invalid Input!" << std::endl;
                }
            } else {
                std::cout << "Invalid Input!" << std::endl;
            }
        }

        if(muteStatus == 0) {
            mute.enable = false;
        } else if (muteStatus == 1) {
            mute.enable = true;
        }

        telux::common::Status status = stream_->setMute(mute,
               [&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to set mute" << std::endl;
            }
        });
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to set mute sent " << std::endl;
        } else {
            std::cout << "Request to set mute failed" << std::endl;
        }
        if (p.get_future().get()) {
            std::cout << "set mute succeeded." << std::endl;
        }

    } else {
        std::cout << " No stream running for this type " << std::endl;
    }
}

void AudioClient::getMute(StreamType streamType) {
    resolveStreamType(streamType);
    std::promise<bool> p;
    telux::audio::StreamMute mute_;
    if (stream_) {
        telux::audio::StreamDirection dir;
        std::string input;
        takeUserDirectionInput(dir);
        telux::common::Status status = stream_->getMute(
               dir,  [&p,&mute_,this](telux::audio::StreamMute mute,
               telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
                mute_ = mute;
            } else {
                p.set_value(false);
                std::cout << "Failed to get mute" << std::endl;
            }
        });
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to get mute sent" << std::endl;
        } else {
            std::cout << "Request to get mute failed" << std::endl;
        }
        if (p.get_future().get()) {
            std::string muteStatus;
            if(mute_.enable) {
                muteStatus = "Muted";
            } else {
                muteStatus = "Unmuted";
            }
            std::cout << "Mute Status: " << muteStatus << std::endl;
        }
    } else {
        std::cout << " No stream running for this type " << std::endl;
    }
}
