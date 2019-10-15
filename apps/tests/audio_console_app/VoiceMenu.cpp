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
#include <iostream>

#include "VoiceMenu.hpp"

VoiceMenu::VoiceMenu(std::string appName, std::string cursor,
                                            std::shared_ptr<AudioClient> audioClient)
    : ConsoleApp(appName, cursor),
      audioClient_(audioClient) {
        audioStarted_ = false;
}

VoiceMenu::~VoiceMenu() {
    audioClient_ = nullptr;
}

void VoiceMenu::init() {
    std::shared_ptr<ConsoleAppCommand> createStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Create Stream",
         {}, std::bind(&VoiceMenu::createStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deleteStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Delete Stream",
         {}, std::bind(&VoiceMenu::deleteStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get Device",
         {}, std::bind(&VoiceMenu::getDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set Device",
         {}, std::bind(&VoiceMenu::setDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Get Volume",
         {}, std::bind(&VoiceMenu::getVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Set Volume",
         {}, std::bind(&VoiceMenu::setVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Get Mute Status",
         {}, std::bind(&VoiceMenu::getMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Set Mute",
         {}, std::bind(&VoiceMenu::setMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startAudioCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Start Audio",
         {}, std::bind(&VoiceMenu::startAudio, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopAudioCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Stop Audio",
         {}, std::bind(&VoiceMenu::stopAudio, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startDtmfCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("11", "Start Dtmf Tone",
         {}, std::bind(&VoiceMenu::startDtmf, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopDtmfCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("12", "Stop Dtmf Tone",
         {}, std::bind(&VoiceMenu::stopDtmf, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> regListenerCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("13", "Register Listener",
         {}, std::bind(&VoiceMenu::registerListener, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deregListenerCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("14", "Deregister Listener",
         {}, std::bind(&VoiceMenu::deRegisterListener, this, std::placeholders::_1)));

     std::vector<std::shared_ptr<ConsoleAppCommand>> voiceMenuCommandsList
      = {createStreamCommand,
         deleteStreamCommand,
         getDeviceCommand,
         setDeviceCommand,
         getVolumeCommand,
         setVolumeCommand,
         getMuteCommand,
         setMuteCommand,
         startAudioCommand,
         stopAudioCommand,
         startDtmfCommand,
         stopDtmfCommand,
         regListenerCmd,
         deregListenerCmd};
   if(audioClient_){
        audioVoiceStream_ =std::dynamic_pointer_cast<IAudioVoiceStream>(
           audioClient_->getStream(StreamType::VOICE_CALL));
        ConsoleApp::addCommands(voiceMenuCommandsList);
   } else {
       std::cout << "AudioClient not initialized " << std::endl;
   }
}

void VoiceMenu::createStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if(audioClient_){
        if(!audioVoiceStream_) {
            status = audioClient_->createStream(telux::audio::StreamType::VOICE_CALL);
            if(status == telux::common::Status::SUCCESS) {
                audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(
                    audioClient_->getStream(StreamType::VOICE_CALL));
            }
        } else {
            std::cout << "Stream exist please delete first" << std::endl;
        }
   } else {
       std::cout << "AudioClient not initialized " << std::endl;
   }
}

void VoiceMenu::deleteStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if(audioVoiceStream_) {
       status = audioClient_->deleteStream(StreamType::VOICE_CALL);
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }

    if(status == telux::common::Status::SUCCESS) {
        audioVoiceStream_ = nullptr;
        audioStarted_ = false;
    }
}

void VoiceMenu::getDevice(std::vector<std::string> userInput) {
    if(audioVoiceStream_) {
        audioClient_->getStreamDevice(telux::audio::StreamType::VOICE_CALL);
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }
}

void VoiceMenu::setDevice(std::vector<std::string> userInput) {
    if(audioVoiceStream_) {
        audioClient_->setStreamDevice(telux::audio::StreamType::VOICE_CALL);
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }
}

void VoiceMenu::getVolume(std::vector<std::string> userInput) {
    if(audioVoiceStream_) {
        audioClient_->getVolume(telux::audio::StreamType::VOICE_CALL);
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }
}

void VoiceMenu::setVolume(std::vector<std::string> userInput) {
    if(audioVoiceStream_) {
        audioClient_->setVolume(telux::audio::StreamType::VOICE_CALL);
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }

}

void VoiceMenu::getMute(std::vector<std::string> userInput) {
    if(audioVoiceStream_) {
        audioClient_->getMute(telux::audio::StreamType::VOICE_CALL);
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }

}

void VoiceMenu::setMute(std::vector<std::string> userInput) {
    if(audioVoiceStream_) {
        audioClient_->setMute(telux::audio::StreamType::VOICE_CALL);
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }
}

void VoiceMenu::startAudio(std::vector<std::string> userInput) {
    if(audioVoiceStream_) {
        if(!audioStarted_) {
            std::promise<bool> p;
            Status status =
                    audioVoiceStream_->startAudio( [&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to start audio" << std::endl;
            }
            });
            if(status == Status::SUCCESS){
                std::cout << "Request to start Audio sent" << std::endl;
            } else {
                std::cout << "Request to start Audio Failed" << std::endl;
            }

            if (p.get_future().get()) {
                audioStarted_ = true;
                std::cout << "Audio Stream is Started" << std::endl;
                // Registering for the Dtmf Detection, not required if detection is not required
            }
        } else {
            std::cout << "Audio already started" << std::endl;
        }
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }
}

void VoiceMenu::stopAudio(std::vector<std::string> userInput) {
    if(audioVoiceStream_) {
        if(audioStarted_) {
            std::promise<bool> p;
            Status status = audioVoiceStream_->stopAudio([&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to stop audio" << std::endl;
            }
            });
            if(status == Status::SUCCESS){
                std::cout << "Request to stop Audio sent" << std::endl;
            } else {
                std::cout << "Request to stop Audio Failed" << std::endl;
            }

            if (p.get_future().get()) {
                std::cout << "Audio Stream is Stopped" << std::endl;
                audioStarted_ = false;
            }
        } else {
            std::cout << "Audio not started yet" << std::endl;
        }
    } else {
        std::cout << "No running voice session please create one" << std::endl;
    }
}


void VoiceMenu::startDtmf(std::vector<std::string> userInput) {
    if(audioStarted_) {
        std::promise<bool> p;
        std::string userInput = "";
        // Start Means we are enabling DTMF and  direction is RX

        uint16_t gain = 0;
        std::cout << "Enter the Gain : ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(!(inputStream >> gain)) {
                std::cout << "Invalid Input!" << std::endl;
                return;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }

        uint32_t lowFreq = 0;
        std::cout << "Enter the Low Frequency (697, 770, 852, 941) : ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(!(inputStream >> lowFreq)) {
                std::cout << "Invalid Input!" << std::endl;
                return;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }

        uint32_t highFreq = 0;
        std::cout << "Enter the High Frequency (1209 1336 1477 1633) : ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(!(inputStream >> highFreq)) {
                std::cout << "Invalid Input!" << std::endl;
                return;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }

        uint32_t duration = 0;
        std::cout << "Enter the duration (in ms (0-65534) and 65535 for infinite): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(!(inputStream >> duration)) {
                std::cout << "Invalid Input!" << std::endl;
                return;
            }
        } else {
            std::cout << "Invalid input!" << std::endl;
        }

        telux::audio::DtmfTone dtmfTone;
        telux::common::Status lowFreqValid = lowFrequencyHelper(lowFreq, dtmfTone.lowFreq);
        telux::common::Status highFreqValid = highFrequencyHelper(highFreq, dtmfTone.highFreq);
        dtmfTone.direction = telux::audio::StreamDirection::RX;

        if(lowFreqValid == telux::common::Status::SUCCESS &&
                    highFreqValid == telux::common::Status::SUCCESS ) {
            telux::common::Status status = audioVoiceStream_ ->playDtmfTone(dtmfTone, duration,
                    gain,[&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to start Dtmf Tone" << std::endl;
            }
            });
            if(status == telux::common::Status::SUCCESS){
                std::cout << "Request to start Dtmf Sent" << std::endl;
            } else {
                std::cout << "Request to start Dtmf Failed" << std::endl;
            }

            if (p.get_future().get()) {
                std::cout << "Dtmf Tone Started" << std::endl;
            }
        }
    } else{
        std::cout << "Audio is not started yet" << std::endl;
    }
}

void VoiceMenu::stopDtmf(std::vector<std::string> userInput) {
    std::promise<bool> p;
    if(audioStarted_) {
        telux::common::Status status = audioVoiceStream_ ->stopDtmfTone(
                telux::audio::StreamDirection::RX,[&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to stop Dtmf" << std::endl;
            }
            });
        if(status == telux::common::Status::SUCCESS){
            std::cout << "Request to stopDtmf Sent" << std::endl;
        } else {
            std::cout << "Request to stopDtmf failed" << std::endl;
        }
        if (p.get_future().get()) {
                std::cout << "Dtmf Tone Stopped" << std::endl;
        }
    } else {
         std::cout << "Audio is not started yet" << std::endl;
    }
}

void VoiceMenu::registerListener(std::vector<std::string> userInput) {
        std::promise<bool> p;
        telux::common::Status status = audioVoiceStream_ ->registerListener(shared_from_this(),
                                     [&p,this](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to Register Listener" << std::endl;
            }
            });
        if(status == telux::common::Status::SUCCESS) {
            std::cout << "Request to Register Listener sent" << std::endl;
        }
        if (p.get_future().get()) {
                std::cout << "Listener Registered Successfully" << std::endl;
        }
}

void VoiceMenu::deRegisterListener(std::vector<std::string> userInput) {
    telux::common::Status status = audioVoiceStream_ ->deRegisterListener(
        shared_from_this());
    if(status == telux::common::Status::SUCCESS){
        std::cout << "Request to deregister Dtmf Sent" << std::endl;
    }
}

telux::common::Status VoiceMenu::lowFrequencyHelper(uint32_t lowFreq,
                             telux::audio::DtmfLowFreq &lowFrequency) {
    switch(lowFreq) {
        case 697:
        lowFrequency = telux::audio::DtmfLowFreq::FREQ_697;
        return telux::common::Status::SUCCESS;
        case 770:
        lowFrequency = telux::audio::DtmfLowFreq::FREQ_770;
        return telux::common::Status::SUCCESS;
        case 852:
        lowFrequency = telux::audio::DtmfLowFreq::FREQ_852;
        return telux::common::Status::SUCCESS;
        case 941:
        lowFrequency = telux::audio::DtmfLowFreq::FREQ_941;
        return telux::common::Status::SUCCESS;
        default:
        std::cout << "unsupported Dtmf Frequency " << std::endl;
        return telux::common::Status::FAILED;
    }
}

telux::common::Status VoiceMenu::highFrequencyHelper(uint32_t highFreq,
                             telux::audio::DtmfHighFreq &highFrequency) {
    switch(highFreq) {
        case 1209:
        highFrequency = telux::audio::DtmfHighFreq::FREQ_1209;
        return telux::common::Status::SUCCESS;
        case 1336:
        highFrequency = telux::audio::DtmfHighFreq::FREQ_1336;
        return telux::common::Status::SUCCESS;
        case 1477:
        highFrequency = telux::audio::DtmfHighFreq::FREQ_1477;
        return telux::common::Status::SUCCESS;
        case 1633:
        highFrequency = telux::audio::DtmfHighFreq::FREQ_1633;
        return telux::common::Status::SUCCESS;
        default:
        std::cout << "unsupported Frequency " << std::endl;
        return telux::common::Status::FAILED;
    }
}

void VoiceMenu::onDtmfToneDetection(DtmfTone dtmfTone) {
    std::cout<< "Dtmf Tone Detected !!" << std::endl;
    std::cout << "Direction is " << static_cast<uint32_t>(dtmfTone.direction) << std::endl;
    std::cout << "Low Frequency is " << static_cast<uint32_t>(dtmfTone.lowFreq)<< std::endl;
    std::cout << "High Frequency is " <<  static_cast<uint32_t>(dtmfTone.highFreq)  << std::endl;
}


