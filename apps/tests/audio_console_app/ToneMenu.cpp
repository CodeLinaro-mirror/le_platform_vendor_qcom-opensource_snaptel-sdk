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

#include <chrono>
#include <iostream>

#include "ToneMenu.hpp"

ToneMenu::ToneMenu(std::string appName, std::string cursor,
                                            std::shared_ptr<AudioClient> audioClient)
    : ConsoleApp(appName, cursor),
      audioClient_(audioClient) {
        toneStarted_ = false;
        ready_ = false;
}

ToneMenu::~ToneMenu() {
}

void ToneMenu::cleanup() {
    ready_ = false;
    toneStarted_ = false;
    audioToneStream_ = nullptr;
}

void ToneMenu::setSystemReady() {
    ready_ = true;
}


void ToneMenu::init() {
    std::shared_ptr<ConsoleAppCommand> createStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Create Stream",
         {}, std::bind(&ToneMenu::createStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deleteStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Delete Stream",
         {}, std::bind(&ToneMenu::deleteStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get Device",
         {}, std::bind(&ToneMenu::getDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set Device",
         {}, std::bind(&ToneMenu::setDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Get Volume",
         {}, std::bind(&ToneMenu::getVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Set Volume",
         {}, std::bind(&ToneMenu::setVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Get Mute Status",
         {}, std::bind(&ToneMenu::getMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Set Mute",
         {}, std::bind(&ToneMenu::setMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> playToneCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Play Tone",
         {}, std::bind(&ToneMenu::playTone, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopToneCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Stop Tone",
         {}, std::bind(&ToneMenu::stopTone, this, std::placeholders::_1)));

     std::vector<std::shared_ptr<ConsoleAppCommand>> ToneMenuCommandsList
      = {createStreamCommand,
         deleteStreamCommand,
         getDeviceCommand,
         setDeviceCommand,
         getVolumeCommand,
         setVolumeCommand,
         getMuteCommand,
         setMuteCommand,
         playToneCommand,
         stopToneCommand};

    if (audioClient_) {
        ready_ = true;
        audioToneStream_ =std::dynamic_pointer_cast<IAudioToneGeneratorStream>(
           audioClient_->getStream(StreamType::TONE_GENERATOR));
        ConsoleApp::addCommands(ToneMenuCommandsList);
    } else {
       std::cout << "AudioClient not initialized " << std::endl;
    }
}

void ToneMenu::createStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if (ready_) {
        if (!audioToneStream_) {
            status = audioClient_->createStream(telux::audio::StreamType::TONE_GENERATOR);
            if(status == telux::common::Status::SUCCESS) {
                audioToneStream_ = std::dynamic_pointer_cast<IAudioToneGeneratorStream>(
                    audioClient_->getStream(StreamType::TONE_GENERATOR));
            }
        } else {
            std::cout << "Stream exist please delete first" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void ToneMenu::deleteStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if (audioToneStream_) {
       status = audioClient_->deleteStream(StreamType::TONE_GENERATOR);
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }

    if (status == telux::common::Status::SUCCESS) {
        audioToneStream_ = nullptr;
    }
}

void ToneMenu::getDevice(std::vector<std::string> userInput) {
    if (audioToneStream_) {
        audioClient_->getStreamDevice(telux::audio::StreamType::TONE_GENERATOR);
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }
}

void ToneMenu::setDevice(std::vector<std::string> userInput) {
    if (audioToneStream_) {
        audioClient_->setStreamDevice(telux::audio::StreamType::TONE_GENERATOR);
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }
}

void ToneMenu::getVolume(std::vector<std::string> userInput) {
    if (audioToneStream_) {
        audioClient_->getVolume(telux::audio::StreamType::TONE_GENERATOR);
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }
}

void ToneMenu::setVolume(std::vector<std::string> userInput) {
    if (audioToneStream_) {
        audioClient_->setVolume(telux::audio::StreamType::TONE_GENERATOR);
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }

}

void ToneMenu::getMute(std::vector<std::string> userInput) {
    if (audioToneStream_) {
        audioClient_->getMute(telux::audio::StreamType::TONE_GENERATOR);
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }

}

void ToneMenu::setMute(std::vector<std::string> userInput) {
    if (audioToneStream_) {
        audioClient_->setMute(telux::audio::StreamType::TONE_GENERATOR);
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }
}

void ToneMenu::playTone(std::vector<std::string> userInput) {
    if (audioToneStream_) {
        std::promise<bool> p;
        std::string userInput = "";

        std::vector<uint16_t> freq;
        uint16_t tempFreq = 0, gain = 0, numFreq = 0, duration = 0;

        std::cout << "Enter number of frequencies ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(!(inputStream >> numFreq)) {
                std::cout << "Invalid Input" << std::endl;
                return;
            }
        } else {
        std::cout << "Invalid Input" << std::endl;
        }

        for (int i = 0; i<numFreq; i++) {
            std::cout << "Enter Frequency [ " << i << " ] : ";
            if(std::getline(std::cin, userInput)) {
                std::stringstream inputStream(userInput);
                if(!(inputStream >> tempFreq)){
                    std::cout << "Invalid Input" << std::endl;
                    return;
                } else {
                    freq.push_back(tempFreq);
                }
            } else {
                std::cout << "Invlaid Input" << std::endl;
            }
        }

        std::cout << "Enter the Gain : ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(!(inputStream >> gain)) {
                std::cout << "Invalid Input" << std::endl;
                return;
            }
        } else {
        std::cout << "Invalid Input" << std::endl;
        }

        std::cout << "Enter the duration (in ms (0-65534) and 65535 for infinite): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(!(inputStream >> duration)) {
                std::cout << "Invalid Input" << std::endl;
                return;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }

        Status status =
                audioToneStream_->playTone(freq, duration, gain,
                [&p,this](telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to play Tone" << std::endl;
        }
        });
        if (status == Status::SUCCESS){
            std::cout << "Request to play tone sent" << std::endl;
        } else {
            std::cout << "Request to play tone Failed" << std::endl;
        }

        if (p.get_future().get()) {
            toneStarted_ = true;
            std::cout << " Tone has Started" << std::endl;
        }
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }
}

void ToneMenu::stopTone(std::vector<std::string> userInput) {
    if (audioToneStream_) {
        std::promise<bool> p;
        Status status = audioToneStream_->stopTone(
            [&p,this](telux::common::ErrorCode error) {
        if (error == telux::common::ErrorCode::SUCCESS) {
            p.set_value(true);
        } else {
            p.set_value(false);
            std::cout << "Failed to stop tone" << std::endl;
        }
        });
        if (status == Status::SUCCESS){
            std::cout << "Request to stop tone sent" << std::endl;
        } else {
            std::cout << "Request to stop tone Failed" << std::endl;
        }

        if (p.get_future().get()) {
            std::cout << "Tone has Stopped" << std::endl;
            toneStarted_ = false;
        }
    } else {
        std::cout << "No running tone generator session please create one" << std::endl;
    }
}
