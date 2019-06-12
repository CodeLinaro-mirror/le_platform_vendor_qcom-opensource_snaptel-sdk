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

#include "PlayMenu.hpp"

#define TOTAL_BUFFERS 2

PlayMenu::PlayMenu(std::string appName, std::string cursor,
                                            std::shared_ptr<AudioClient> audioClient)
   : ConsoleApp(appName, cursor),
   audioClient_(audioClient) {
}

PlayMenu::~PlayMenu() {
    audioClient_ = nullptr;
    playStatus_ = false;

    for(std::thread &th : runningThreads_) {
        if(th.joinable()){
            th.join();
        }
    }
}

void PlayMenu::init() {
    std::shared_ptr<ConsoleAppCommand> createStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Create Stream",
         {}, std::bind(&PlayMenu::createStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> deleteStreamCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Delete Stream",
         {}, std::bind(&PlayMenu::deleteStream, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Get Device",
         {}, std::bind(&PlayMenu::getDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setDeviceCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Set Device",
         {}, std::bind(&PlayMenu::setDevice, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Get Volume",
         {}, std::bind(&PlayMenu::getVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setVolumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Set Volume",
         {}, std::bind(&PlayMenu::setVolume, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("7", "Get Mute Status",
         {}, std::bind(&PlayMenu::getMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> setMuteCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("8", "Set Mute",
         {}, std::bind(&PlayMenu::setMute, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> startPlayCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("9", "Start Play",
         {}, std::bind(&PlayMenu::startPlay, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopPlayCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("10", "Stop Play",
         {}, std::bind(&PlayMenu::stopPlay, this, std::placeholders::_1)));


     std::vector<std::shared_ptr<ConsoleAppCommand>> playMenuCommandsList
      = {createStreamCommand,
         deleteStreamCommand,
         getDeviceCommand,
         setDeviceCommand,
         getVolumeCommand,
         setVolumeCommand,
         getMuteCommand,
         setMuteCommand,
         startPlayCommand,
         stopPlayCommand};
   if(audioClient_){
        audioPlayStream_ = std::dynamic_pointer_cast<IAudioPlayStream>(
           audioClient_->getStream(StreamType::PLAY));
        ConsoleApp::addCommands(playMenuCommandsList);
   } else {
       std::cout << "AudioClient not initialized " << std::endl;
   }
}

void PlayMenu::createStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if(audioClient_) {
        if(!audioPlayStream_) {
            status = audioClient_->createStream(StreamType::PLAY);
            if(status == telux::common::Status::SUCCESS) {
                audioPlayStream_ = std::dynamic_pointer_cast<IAudioPlayStream>(
                audioClient_->getStream(StreamType::PLAY));
            }
        } else {
            std::cout << "Stream exist please delete first" << std::endl;
        }
    } else {
       std::cout << "AudioClient not initialized " << std::endl;
    }
}

void PlayMenu::deleteStream(std::vector<std::string> userInput) {
    telux::common::Status status = telux::common::Status::FAILED;
    if(audioPlayStream_) {
        playStatus_ = false;
        for(std::thread &th : runningThreads_) {
            if(th.joinable()){
                th.join();
            }
        }
        status = audioClient_->deleteStream(StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }

    if(status == telux::common::Status::SUCCESS) {
        audioPlayStream_ = nullptr;
    }
}

void PlayMenu::getDevice(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->getStreamDevice(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::setDevice(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->setStreamDevice(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::getVolume(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->getVolume(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::setVolume(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->setVolume(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::getMute(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->getMute(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::setMute(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        audioClient_->setMute(telux::audio::StreamType::PLAY);
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::startPlay(std::vector<std::string> userInput) {
    if(audioPlayStream_) {
        std::thread playThread(&PlayMenu::play, this);
        runningThreads_.emplace_back(std::move(playThread));
    } else {
        std::cout << "No running Play session please create one" << std::endl;
    }
}

void PlayMenu::stopPlay(std::vector<std::string> userInput) {
    playStatus_ = false;
}

void PlayMenu::writeCallback(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytes,
                telux::common::ErrorCode error) {
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "write() returned with error " << static_cast<unsigned int>(error)
            << std::endl;
    }
    buffer->reset();
    freeBuffers_.push(buffer);
    cv_.notify_all();
    return;
}

void PlayMenu::play() {
    while(!freeBuffers_.empty()) {
        freeBuffers_.pop();
    }
    filePath_ = audioClient_->getFilePathForPlay();
    FILE * file;
    file = fopen(filePath_.c_str(),"r");
    if(file) {
        fseek(file, 0, SEEK_SET);
    } else {
        std::cout <<"Unable to read file" << std::endl;
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    uint32_t size = 0;
    std::string userInput ="";
    uint32_t numBytes =0;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    for(int i = 0; i < TOTAL_BUFFERS; i++) {
        streamBuffer = audioPlayStream_->getStreamBuffer();
        if(streamBuffer != nullptr) {
            size = streamBuffer->getMinSize();
            if(size == 0) {
                size =  streamBuffer->getMaxSize();
            }
            streamBuffer->setDataSize(size);
            std::cout << "Buffer no. " << i << " buffer size "<< streamBuffer->getDataSize()
                    << std::endl;
            freeBuffers_.push(streamBuffer);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
            return;
        }
    }
    playStatus_ = true;
    std::cout << "Audio play started" << std::endl;
    while (!feof(file) && playStatus_)
    {
        if(!freeBuffers_.empty()) {
            streamBuffer = freeBuffers_.front();
            freeBuffers_.pop();
            numBytes = fread(streamBuffer->getRawBuffer(),1,size,file);
            if(numBytes != size) {
                std::cout << "Readsize mismatch Error or EOF reached " << numBytes<< std::endl;
            }
            auto writeCb = std::bind(&PlayMenu::writeCallback, this, std::placeholders::_1,
                        std::placeholders::_2, std::placeholders::_3);
            telux::common::Status status = audioPlayStream_->write(streamBuffer,writeCb);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "write() failed with error" << static_cast<unsigned int>(status)
                <<std::endl;
            }
        } else {
            cv_.wait(lock);
        }
    }
    while(freeBuffers_.size() != TOTAL_BUFFERS) {
        cv_.wait(lock);
    }
    if(playStatus_) {
        std::cout << "File played SuccessFully" <<std::endl;
    } else {
        std::cout << "Play Stopped" << std::endl;
    }
    playStatus_ = false;
    fflush(file);
    fclose(file);
}
