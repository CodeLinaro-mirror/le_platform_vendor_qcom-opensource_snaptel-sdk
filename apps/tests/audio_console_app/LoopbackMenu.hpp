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

#ifndef LOOPBACKMENU_HPP
#define LOOPBACKMENU_HPP

#include "ConsoleApp.hpp"
#include "AudioClient.hpp"

class LoopbackMenu : public ConsoleApp {
public:
    LoopbackMenu(std::string appName, std::string cursor, std::shared_ptr<AudioClient> audioClient);
    ~LoopbackMenu();
    void init();
    void cleanup();
    void setSystemReady();

private:
    void createStream(std::vector<std::string> userInput);
    void deleteStream(std::vector<std::string> userInput);
    void getDevice(std::vector<std::string> userInput);
    void setDevice(std::vector<std::string> userInput);
    void getVolume(std::vector<std::string> userInput);
    void setVolume(std::vector<std::string> userInput);
    void getMute(std::vector<std::string> userInput);
    void setMute(std::vector<std::string> userInput);
    void startLoopback(std::vector<std::string> userInput);
    void stopLoopback(std::vector<std::string> userInput);

    std::shared_ptr<IAudioLoopbackStream> audioLoopbackStream_;
    std::shared_ptr<AudioClient> audioClient_;
    std::atomic<bool> loopbackStarted_;
    std::atomic<bool> ready_;
};

#endif // LOOPBACKMENU_HPP
