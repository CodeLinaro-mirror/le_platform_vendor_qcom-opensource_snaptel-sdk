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
 * @file       AudioConsoleApp.cpp
 *
 * @brief      This is entry class for console application for audio,
 *             It allows one to interactively invoke most of the public APIs in audio.
 */

#include <iostream>
#include <memory>

extern "C" {
#include <cxxabi.h>
#include <execinfo.h>
#include <signal.h>
}

#include "VoiceMenu.hpp"
#include "PlayMenu.hpp"
#include "CaptureMenu.hpp"
#include "LoopbackMenu.hpp"
#include "ToneMenu.hpp"

#define APP_NAME "audio_console_app"

#include "AudioConsoleApp.hpp"

AudioConsoleApp::AudioConsoleApp(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
}

AudioConsoleApp::~AudioConsoleApp() {
    voiceMenu_ = nullptr;
    playMenu_ = nullptr;
    captureMenu_ = nullptr;
    cleanup();
    audioClient_ = nullptr;
}

void AudioConsoleApp::init() {

    audioClient_ = std::make_shared<AudioClient>();
    audioClient_->init();
    std::shared_ptr<ConsoleAppCommand> voiceMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Voice Call", {},
        std::bind(&AudioConsoleApp::voiceMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> playMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Playback", {},
        std::bind(&AudioConsoleApp::playMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> captureMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "Capture", {},
        std::bind(&AudioConsoleApp::captureMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> loopbackMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Loopback", {},
        std::bind(&AudioConsoleApp::loopbackMenu, this, std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> toneMenuCommand
    = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("5", "Tone", {},
        std::bind(&AudioConsoleApp::toneMenu, this, std::placeholders::_1)));

     std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
        = {voiceMenuCommand, playMenuCommand, captureMenuCommand, loopbackMenuCommand,
            toneMenuCommand};

    voiceMenu_ = std::make_shared<VoiceMenu>("Voice Menu", "voice> ", audioClient_);
    voiceMenu_->init();
    playMenu_ = std::make_shared<PlayMenu>("Play Menu", "play> ", audioClient_);
    playMenu_->init();
    captureMenu_ = std::make_shared<CaptureMenu>("Capture Menu", "capture> ", audioClient_);
    captureMenu_->init();
    loopbackMenu_ = std::make_shared<LoopbackMenu>("Loopback Menu", "loopback> ", audioClient_);
    loopbackMenu_->init();
    toneMenu_ = std::make_shared<ToneMenu>("Tone menu", "tone> ", audioClient_);
    toneMenu_->init();

    ConsoleApp::addCommands(mainMenuCommands);
    ConsoleApp::displayMenu();
}

void AudioConsoleApp::voiceMenu(std::vector<std::string> userInput) {
    voiceMenu_->displayMenu();
    voiceMenu_->mainLoop();
}

void AudioConsoleApp::playMenu(std::vector<std::string> userInput) {
    playMenu_->displayMenu();
    playMenu_->mainLoop();
}

void AudioConsoleApp::captureMenu(std::vector<std::string> userInput) {
    captureMenu_->displayMenu();
    captureMenu_->mainLoop();
}

void AudioConsoleApp::loopbackMenu(std::vector<std::string> userInput) {
    loopbackMenu_->displayMenu();
    loopbackMenu_->mainLoop();
}

void AudioConsoleApp::toneMenu(std::vector<std::string> userInput) {
    toneMenu_->displayMenu();
    toneMenu_->mainLoop();
}

void AudioConsoleApp::cleanup() {
    auto audioVoiceStream_ = std::dynamic_pointer_cast<IAudioVoiceStream>(
           audioClient_->getStream(StreamType::VOICE_CALL));
    if(audioVoiceStream_){
        audioClient_->deleteStream(StreamType::VOICE_CALL);
    }

    auto audioPlayStream_ = std::dynamic_pointer_cast<IAudioPlayStream>(
           audioClient_->getStream(StreamType::PLAY));
    if(audioPlayStream_){
        audioClient_->deleteStream(StreamType::PLAY);
    }

    auto audioCaptureStream_ = std::dynamic_pointer_cast<IAudioCaptureStream>(
           audioClient_->getStream(StreamType::CAPTURE));
    if(audioCaptureStream_){
        audioClient_->deleteStream(StreamType::CAPTURE);
    }

    auto audioLoopbackStream_ = std::dynamic_pointer_cast<IAudioLoopbackStream>(
           audioClient_->getStream(StreamType::LOOPBACK));
    if(audioLoopbackStream_){
        audioClient_->deleteStream(StreamType::LOOPBACK);
    }
}

int main(int argc, char **argv) {

    AudioConsoleApp audioConsoleApp(APP_NAME, "audio> ");

    audioConsoleApp.init();  // initialize commands and display

    return audioConsoleApp.mainLoop();  // Main loop to continuously read and execute commands

}