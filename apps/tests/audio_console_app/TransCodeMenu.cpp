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
#include <dirent.h>

#include<telux/audio/AudioFactory.hpp>
#include "TransCodeMenu.hpp"

#define TOTAL_READ_BUFFERS 1
#define TOTAL_WRITE_BUFFERS 1
#define EOF_REACHED 1
#define EOF_NOT_REACHED 0
#define GAURD_FOR_WAITING 100

TransCodeMenu::TransCodeMenu(std::string appName, std::string cursor)
    : ConsoleApp(appName, cursor) {
    pipeLineEmpty_ = true;
    writeStatus_ = false;
    readStatus_ = false;
    ready_ = false;
}

TransCodeMenu::~TransCodeMenu() {
    writeStatus_ = false;
    readStatus_ = false;
    for(std::thread &th : runningThreads_) {
        if(th.joinable()){
            th.join();
        }
    }
    transcoder_ = nullptr;
    pipeLineEmpty_ = true;
}

void TransCodeMenu::init() {
    std::shared_ptr<ConsoleAppCommand> startTranscodingCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", " Start Transcoding",
         {}, std::bind(&TransCodeMenu::startTranscoding, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> abortTranscodingCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", " Abort Transcoding",
         {}, std::bind(&TransCodeMenu::tearDown, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> transCodeMenuCommandList
      = {startTranscodingCommand, abortTranscodingCommand};
    ConsoleApp::addCommands(transCodeMenuCommandList);
    auto &audioFactory = telux::audio::AudioFactory::getInstance();
    audioManager_ = audioFactory.getAudioManager();
    if (audioManager_) {
        ready_ = true;
    }
}

void TransCodeMenu::cleanup() {
    ready_ = false;
    writeStatus_ = false;
    readStatus_ = false;
    cv_.notify_all();
    for (std::thread &th : runningThreads_) {
        if (th.joinable()) {
            th.join();
        }
    }
    transcoder_ = nullptr;
    pipeLineEmpty_ = true;
}

void TransCodeMenu::finishTranscoding() {
    fflush(readFile_);
    fclose(readFile_);
    fflush(writeFile_);
    fclose(writeFile_);
    tearDown({});
}

void TransCodeMenu::setSystemReady() {
    ready_ = true;
}

void TransCodeMenu::createTranscoder() {
    std::promise<bool> p;
    std::cout << "Enter configuration for input samples" << std::endl;
    std::cout << "-------------------------------------" << std::endl;
    while (1) {
        std::cout << "Enter file path : "  ;
        if (std::getline(std::cin, writeFilePath_)) {
            DIR* directory = opendir(writeFilePath_.c_str());
            if (directory != NULL) {
                std::cout << "Please enter valid path" << std::endl;
            } else {
                FILE* file;
                file = fopen(writeFilePath_.c_str(),"r");
                if (file) {
                    break;
                    fclose(file);
                } else {
                    perror("Error :");
                }
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
    takeFormatData(inputConfig_);
    std::cout << "Enter configuration for output samples" << std::endl;
    std::cout << "-------------------------------------" << std::endl;
    while (1) {
        std::cout << "Enter file path : "  ;
        if (std::getline(std::cin, readFilePath_)) {
            DIR* directory = opendir(readFilePath_.c_str());
            if (directory != NULL) {
                std::cout << "Please enter valid path" << std::endl;
            } else {
                break;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
    takeFormatData(outputConfig_);
    AmrwbpParams* inputParams = new AmrwbpParams();
    AmrwbpParams* outputParams = new AmrwbpParams();
    if (inputParams && outputParams) {
        if (inputConfig_.format == AudioFormat::AMRWB_PLUS) {
        inputParams->bitWidth = 16;
        inputParams->frameFormat = AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
        inputConfig_.params = inputParams;
        } else {
        inputConfig_.params = nullptr;
        }

        if (outputConfig_.format == AudioFormat::AMRWB_PLUS) {
            outputParams->bitWidth = 16;
            outputParams->frameFormat = AmrwbpFrameFormat::FILE_STORAGE_FORMAT;
            outputConfig_.params = outputParams;
        } else {
            outputConfig_.params = nullptr;
        }

        audioManager_->createTranscoder(inputConfig_, outputConfig_,
            [&p,this](std::shared_ptr<telux::audio::ITranscoder> &transcoder,
            telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                transcoder_ = transcoder;
                registerListener();
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "failed to create transcoder" << std::endl;
            }
            });
        if (p.get_future().get()) {
            std::cout << "Transcoder Created" << std::endl;
        }
    } else {
        std::cout << "Memory allocation failure" << std::endl;
    }
    delete inputParams;
    delete outputParams;
    return;
}

void TransCodeMenu::writeCallback(std::shared_ptr<telux::audio::IAudioBuffer> buffer,
        uint32_t bytes, telux::common::ErrorCode error) {
    if (error != telux::common::ErrorCode::SUCCESS || buffer->getDataSize() != bytes) {
        pipeLineEmpty_ = false;
        std::cout <<
            "Bytes Requested " << buffer->getDataSize() << " Bytes Written " << bytes << std::endl;
        // We are seeking back so that left over buffer can be resent again.
        long offset = -1 * (static_cast<long>((buffer->getDataSize() - bytes)));
        fseek(writeFile_, offset, SEEK_CUR);
    }
    buffer->reset();
    writeBuffers_.push(buffer);
    cv_.notify_all();
    return;
}

void TransCodeMenu::write() {
    while (!writeBuffers_.empty()) {
        writeBuffers_.pop();
    }
    writeFile_ = fopen(writeFilePath_.c_str(),"r");
    if (writeFile_) {
        fseek(writeFile_, 0, SEEK_SET);
    } else {
        std::cout <<"Unable to open file for reading samples" << std::endl;
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_);
    uint32_t size = 0;
    std::string userInput ="";
    uint32_t numBytes =0;
    std::shared_ptr<telux::audio::IAudioBuffer> audioBuffer;
    for(int i = 0; i < TOTAL_WRITE_BUFFERS; i++) {
        audioBuffer = transcoder_->getWriteBuffer();
        if (audioBuffer != nullptr) {
            size = audioBuffer->getMinSize();
            if(size == 0) {
                size =  audioBuffer->getMaxSize();
            }
            writeBuffers_.push(audioBuffer);
        } else {
            std::cout << "Failed to get Buffers for Write operation " << std::endl;
            return;
        }
    }

    writeStatus_ = true;
    while (!feof(writeFile_) && writeStatus_) {
        if (!writeBuffers_.empty() && (pipeLineEmpty_)) {
            audioBuffer = writeBuffers_.front();
            writeBuffers_.pop();
            numBytes = fread(audioBuffer->getRawBuffer(), 1, size, writeFile_);
            if (numBytes != size && !feof(writeFile_)) {
                std::cout << "Unable to read specified bytes, bytes read: " << numBytes<< std::endl;
                audioBuffer->reset();
                writeBuffers_.push(audioBuffer);
                writeStatus_ = false;
                break;
            }
            audioBuffer->setDataSize(numBytes);
            auto writeCb = std::bind(&TransCodeMenu::writeCallback, this, std::placeholders::_1,
                        std::placeholders::_2, std::placeholders::_3);
            telux::common::Status status = telux::common::Status::FAILED;
            if (feof(writeFile_)) {
                status = transcoder_->write(audioBuffer, EOF_REACHED,  writeCb);
            } else {
                status = transcoder_->write(audioBuffer, EOF_NOT_REACHED,  writeCb);
            }
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "write() failed with error" << static_cast<unsigned int>(status)
                <<std::endl;
            }
        } else {
            cv_.wait(lock);
        }
    }
    writeStatus_ = false;
}

void TransCodeMenu::read() {
    uint32_t bytesToRead = 0;
    std::unique_lock<std::mutex> lock(mutex_);

    while (!readBuffers_.empty()) {
        readBuffers_.pop();
    }
    std::shared_ptr<telux::audio::IAudioBuffer> audioBuffer;
    for (int i = 0; i < TOTAL_READ_BUFFERS; i++) {
        audioBuffer = transcoder_->getReadBuffer();
        if (audioBuffer != nullptr) {
            // Setting the bytesToRead (bytes to be readed from stream) as minimum size
            // required by stream. In any case if size returned is 0, using the Maximum Buffer
            // Size, any buffer size between min and max can be used
            bytesToRead = audioBuffer->getMinSize();
            if (bytesToRead == 0) {
                bytesToRead = audioBuffer->getMaxSize();
            }
            std::cout << "Bytes to read " << bytesToRead << std::endl;
            readBuffers_.push(audioBuffer);
        } else {
            std::cout << "Failed to get Stream Buffer " << std::endl;
            return;
        }
    }
    // numChannels here stores num of channels;
    int sampleRate = outputConfig_.sampleRate;;
    int numChannels = (outputConfig_.mask == 3) ? 2 : 1;
    readFile_ = fopen(readFilePath_.c_str(),"w");
    if (readFile_) {
            fseek(readFile_, 0, SEEK_SET);
    } else {
        std::cout << "Unable to open file for writing samples " <<std::endl;
        return;
    }
    readStatus_ = true;
    while (readStatus_) {
        if (!readBuffers_.empty()) {
            audioBuffer = readBuffers_.front();
            readBuffers_.pop();
            auto readCb =  std::bind(&TransCodeMenu::readCallback, this,
                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
            telux::common::Status status = transcoder_->read(audioBuffer, bytesToRead, readCb);
            if (status != telux::common::Status::SUCCESS) {
                std::cout << "read() failed with error" << static_cast<unsigned int>(status)
                <<std::endl;
            }
        } else {
            cv_.wait(lock);
        }
    }
    // waitTime is time required to receive one last remaining buffer
    int waitTime = (8*(audioBuffer->getMaxSize())*1000)/
                        (sampleRate*numChannels*16);
    waitTime = waitTime+ GAURD_FOR_WAITING;
    while (readBuffers_.size() != TOTAL_READ_BUFFERS && ready_) {
        cv_.wait_for(lock, std::chrono::milliseconds(waitTime));
    }
    if (ready_) {
        finishTranscoding();
    }
    std::cout << "Transcoding Successful" <<std::endl;
}

void TransCodeMenu::readCallback(std::shared_ptr<telux::audio::IAudioBuffer> buffer,
         uint32_t isLastBuffer, telux::common::ErrorCode error) {
    uint32_t bytesWrittenToFile = 0;
    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read() returned with error " << static_cast<unsigned int>(error) << std::endl;
    } else {
        uint32_t size = buffer->getDataSize();
        bytesWrittenToFile = fwrite(buffer->getRawBuffer(), 1, size, readFile_);
        if (bytesWrittenToFile != size) {
            std::cout << "Write Size mismatch while writing to file" << std::endl;
        }
    }
    buffer->reset();
    readBuffers_.push(buffer);
    cv_.notify_all();
    if (isLastBuffer) {
        readStatus_ = false;
    }
    return;
}

void TransCodeMenu::startTranscoding(std::vector<std::string> userInput) {
    if (ready_) {
        createTranscoder();
        if (transcoder_) {
            std::thread writeThread(&TransCodeMenu::write, this);
            runningThreads_.emplace_back(std::move(writeThread));
            std::thread readThread(&TransCodeMenu::read, this);
            runningThreads_.emplace_back(std::move(readThread));
        } else {
            std::cout << "Transcoder not avaialble" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }

}

void TransCodeMenu::tearDown(std::vector<std::string> userInput) {
    if (transcoder_) {
        std::promise<bool> p;
        auto status = transcoder_->tearDown([&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                p.set_value(false);
                std::cout << "Failed to tear down" << std::endl;
            }
            });
        if (status == telux::common::Status::SUCCESS) {
            std::cout << "Request to Teardown transcoder sent" << std::endl;
        } else {
            std::cout << "Request to Teardown transcoder failed" << std::endl;
        }
        if (p.get_future().get()) {
            transcoder_ = nullptr;
            std::cout << "Tear Down successful" << std::endl;
        }
        readStatus_ = false;
    } else {
        std::cout << "No transcoder Exists" << std::endl;
    }
}

void TransCodeMenu::onReadyForWrite() {
    // This event is received in case of compressed audio format playback, it is received when the
    // buffer pipeline is ready to accept new buffers.
    std::cout << "Pipeline Ready to receive buffer " << std::endl;
    pipeLineEmpty_ = true;
    cv_.notify_all();
}

void TransCodeMenu::registerListener() {
    telux::common::Status status = transcoder_ ->registerListener(shared_from_this());
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request to register Transcode Listener Sent" << std::endl;
    }
}

void TransCodeMenu::deRegisterListener() {
    telux::common::Status status = transcoder_ ->deRegisterListener(shared_from_this());
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Request to deregister Transcode Listener Sent" << std::endl;
    }
}

void TransCodeMenu::takeFormatData(FormatInfo &info) {
    std::string userInput ="";
    while (1) {
        std::cout << "Enter channel mask : (1->left, 2->right, 3->both) : ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if((inputStream >> info.mask)) {
                if (info.mask < 1 && info.mask > 3) {
                    std::cout << "Invalid Input" << std::endl;
                } else {
                    break;
                }
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }

    while (1) {
        std::cout << "Enter sample rate : (16000, 32000, 48000) : ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if(!(inputStream >> info.sampleRate)) {
                std::cout << "Invalid Input" << std::endl;
            } else {
                break;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }

    while (1) {
    int audioFormat = -1;
        std::cout << "Enter audio Format : (0->PCM, 1->AMRNB, 2->AMRWB, 3->AMRWB+) : ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if((inputStream >> audioFormat)) {
                if (audioFormat < 0 && audioFormat > 3) {
                    std::cout << "Invalid Input" << std::endl;
                } else {
                    if (audioFormat == 0) {
                        info.format = AudioFormat::PCM_16BIT_SIGNED;
                    } else if (audioFormat == 1) {
                        info.format = AudioFormat::AMRNB;
                    } else if (audioFormat == 2) {
                        info.format = AudioFormat::AMRWB;
                    } else if (audioFormat == 3) {
                        info.format = AudioFormat::AMRWB_PLUS;
                    }
                    break;
                }
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

