/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <iostream>

#include "HpcmMenu.hpp"


HpcmMenu::HpcmMenu(std::string appName, std::string cursor,
                                            std::shared_ptr<IAudioManager> audioManager)
    : ConsoleApp(appName, cursor), slotId_(DEFAULT_SLOT_ID), ready_(false),  exitHpcm_(false),
      audioManager_(audioManager) {
}

HpcmMenu::~HpcmMenu() {
}

void HpcmMenu::init() {
    std::shared_ptr<ConsoleAppCommand> startHpcmCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Start HPCM", {},
            std::bind(&HpcmMenu::startHpcmAudio, this, std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> stopHpcmCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Stop HPCM", {},
            std::bind(&HpcmMenu::stopHpcmAudio, this, std::placeholders::_1)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> hpcmMenuCommandsList = {startHpcmCommand,
        stopHpcmCommand};

    ready_ = true;
    ConsoleApp::addCommands(hpcmMenuCommandsList);
}

void HpcmMenu::setSystemReady() {
    ready_ = true;
}

void HpcmMenu::cleanup() {
    std::lock_guard<std::mutex> lk(mutex_);
    ready_ = false;
    exitHpcm_ = true;
    captureCv_.notify_all();
    bufferReadyCv_.notify_all();

    for (std::thread &th : runningThreads_) {
        if (th.joinable()){
            th.join();
        }
    }

    audioCaptureStream_ = nullptr;
    audioPlayStream_ = nullptr;
    voiceSessions_.clear();
    activeSession_ = nullptr;

}

Status HpcmMenu::createAllStreams() {
    telux::common::Status status = telux::common::Status::FAILED;
    if (ready_) {
        std::cout <<"------------------------------------------------" << std::endl;
        std::cout << "Enter configuration for HPCM Stream" << std::endl;
        std::cout << "Supported sampling rates are 8kHz/16kHz." << std::endl;
        std::cout <<"------------------------------------------------" << std::endl;

        if (createActiveSession(slotId_) == Status::SUCCESS) {
            StreamConfig config;
            std::vector<telux::audio::Direction> direction{};
            config.slotId = SLOT_ID_1;
            config.type = StreamType::VOICE_CALL;
            config.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
            config.channelTypeMask = ChannelType::LEFT;
            config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
            config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
            config.enableHpcm = true;
            getUserSampleRateInput(config.sampleRate);
            takeUserVoicePathInput(direction);
            mutex_.lock();
            //Create Voice stream for HPCM usecases
            if(activeSession_) {
                status = activeSession_->createStream(config);
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Voice stream created on slotId : " << slotId_ << std::endl;
                } else {
                    deleteActiveSession(slotId_);
                    std::cout << "Voice stream creation failed on slotId : " << slotId_
                              << std::endl;
                    return Status::FAILED;
                }
            } else {
                std::cout << "Audio Service UNAVAILABLE" << std::endl;
                mutex_.unlock();
                return Status::FAILED;
            }

            config.voicePaths = direction;
            //Create HPCM play stream
            config.type = StreamType::PLAY;
            config.deviceTypes.clear();
            config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);
            status = createStream(config);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "HPCM play stream creation failed " << std::endl;
                return Status::FAILED;
            }

            //Create HPCM capture stream
            config.type = StreamType::CAPTURE;
            config.deviceTypes.clear();
            config.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);
            status = createStream(config);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "HPCM capture stream creation failed " << std::endl;
                return Status::FAILED;
            }
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status HpcmMenu::deleteAllStream() {
    telux::common::Status status = telux::common::Status::FAILED;
    if (ready_) {

        exitHpcm_ = true;

        for(std::thread &th : runningThreads_) {
            if(th.joinable()){
                th.join();
            }
        }

        //Delete HPCM play stream
        telux::common::Status deleteStreamStatus;
        std::promise<bool> promisePlay;
        if(audioPlayStream_){
            deleteStreamStatus = audioManager_-> deleteStream(
            audioPlayStream_, [&promisePlay,this](telux::common::ErrorCode error) {
                if (error == telux::common::ErrorCode::SUCCESS) {
                promisePlay.set_value(true);
                } else {
                promisePlay.set_value(false);
                std::cout << "Failed to delete HPCM play stream" << std::endl;
                }
            });
            if(deleteStreamStatus == Status::SUCCESS) {
            std::cout << "request to delete HPCM play stream sent" << std::endl;
            } else {
                std::cout << "Request to delete HPCM play stream failed"  << std::endl;
                return Status::FAILED;
            }
            if (promisePlay.get_future().get()) {
                audioPlayStream_= nullptr;
                std::cout << "Audio HPCM play stream is Deleted" << std::endl;
            }else {
                std::cout << "Failed to delete stream" << std::endl;
                return Status::FAILED;
            }
        }
        //Delete HPCM capture stream
        if(audioCaptureStream_){
            std::promise<bool> promiseCapture;
            deleteStreamStatus = audioManager_-> deleteStream(
            audioCaptureStream_, [&promiseCapture,this](telux::common::ErrorCode error) {
                if (error == telux::common::ErrorCode::SUCCESS) {
                promiseCapture.set_value(true);
                } else {
                promiseCapture.set_value(false);
                std::cout << "Failed to delete HPCM capture stream" << std::endl;
                }
            });

            if(deleteStreamStatus == Status::SUCCESS) {
            std::cout << "request to delete HPCM play stream sent" << std::endl;
            } else {
                std::cout << "Request to delete HPCM play stream failed"  << std::endl;
                return Status::FAILED;
            }

            if (promiseCapture.get_future().get()) {
                audioCaptureStream_= nullptr;
                std::cout << "Audio HPCM capture stream is Deleted" << std::endl;
            }else {
                std::cout << "Failed to delete stream" << std::endl;
                return Status::FAILED;
            }
        }

        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {

                status = activeSession_->deleteStream();
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    deleteActiveSession(slotId_);
                    std::cout << "Voice stream deleted on slotId : " << slotId_ << std::endl;
                } else {
                    std::cout << "Voice stream deletion failed on slotId : " << slotId_ << std::endl;
                    return Status::FAILED;
                }

            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
                return Status::FAILED;
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
            return Status::FAILED;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
        return Status::FAILED;
    }

    return Status::SUCCESS;
}


void HpcmMenu::startHpcmAudio(std::vector<std::string> userInput) {
    if (ready_) {
        Status status = createAllStreams();
        if(status != Status::SUCCESS) {
            return;
        }
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {
                status = activeSession_->startAudio();
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Audio started on slotId : " << slotId_ << std::endl;
                } else {
                    std::cout << "Failed to start audio on slotId : " << slotId_ << std::endl;
                    return;
                }

                if(audioCaptureStream_) {
                    std::thread recordThread(&HpcmMenu::record, this);
                    runningThreads_.emplace_back(std::move(recordThread));
                }

                if(audioPlayStream_) {
                    std::thread playThread(&HpcmMenu::play, this);
                    runningThreads_.emplace_back(std::move(playThread));
                }

            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }
}

void HpcmMenu::stopHpcmAudio(std::vector<std::string> userInput) {
    if (ready_) {
        if (setActiveSession(slotId_) == Status::SUCCESS) {
            mutex_.lock();
            if(activeSession_) {
                Status status = activeSession_->stopAudio();
                mutex_.unlock();
                if (status == Status::SUCCESS) {
                    std::cout << "Audio stopped on slotId : " << slotId_ << std::endl;
                } else {
                    std::cout << "Failed to stop audio on slotId : " << slotId_ << std::endl;
                    return;
                }

                exitHpcm_ = true;

            } else {
                std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
                mutex_.unlock();
            }
        } else {
            std::cout << "No running voice session for slotId : " << slotId_
                      << ", please create one" << std::endl;
        }
    } else {
        std::cout << "Audio Service UNAVAILABLE" << std::endl;
    }

    deleteAllStream();
}

Status HpcmMenu::createActiveSession(SlotId slotId) {
    if (setActiveSession(slotId) != Status::SUCCESS) {
        std::lock_guard<std::mutex> lk(mutex_);
        try {
            voiceSessions_[slotId] = std::make_shared<VoiceSession>();
            activeSession_ = voiceSessions_[slotId];
        } catch (std::bad_alloc &e) {
            std::cout << "Error: Create active session failed! NOMEMORY!" << std::endl;
            return Status::NOMEMORY;
        }
    }
    return Status::SUCCESS;
}

void HpcmMenu::deleteActiveSession(SlotId slotId) {
    std::lock_guard<std::mutex> lk(mutex_);
    voiceSessions_.erase(slotId);
    activeSession_ = nullptr;
    std::cout << "Voice session deleted on slotId : " << slotId_ << std::endl;
}

Status HpcmMenu::setActiveSession(SlotId slotId) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (voiceSessions_.count(slotId) && (voiceSessions_[slotId])) {
        activeSession_ = voiceSessions_[slotId];
        return Status::SUCCESS;
    } else {
        activeSession_ = nullptr;
    }
    return Status::NOSUCH;
}

/*
 *  Gets called whenever audio samples are read from the hpcm capture stream. The captured buffer is
 *  then passed to hpcm playback stream.
 */
void HpcmMenu::readCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {
    uint32_t bytesRead;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer = audioCaptureStream_->getStreamBuffer();

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read failed, err: " << static_cast<int>(error) << std::endl;
        readErrorOccurred_ = true;
    } else {
        bytesRead = buffer->getDataSize();
        streamBuffer->setDataSize(bytesRead);
        std::cout << "bytes read: " << bytesRead << std::endl;
        memcpy(streamBuffer->getRawBuffer(), buffer->getRawBuffer(), bytesRead);
        freePlayBuffers_.push(streamBuffer);
        bufferReadyCv_.notify_all();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    buffer->reset();
    freeCaptureBuffers_.push(buffer);
    captureCv_.notify_all();
}


/*
 *  Read samples from the hpcm capture stream.
 */
void HpcmMenu::record() {

    uint32_t bytesToRead = 0;
    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::unique_lock<std::mutex> lock(captureMutex_);
    readErrorOccurred_ = false;

    for (int x = 0; x < 1; x++) {
        streamBuffer = audioCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get stream buffer" << std::endl;
            return;
        }
        freeCaptureBuffers_.push(streamBuffer);

        bytesToRead = streamBuffer->getMinSize();
        if (!bytesToRead) {
            bytesToRead =  streamBuffer->getMaxSize();
        }
        streamBuffer->setDataSize(bytesToRead);
    }

    auto readCb = std::bind(&HpcmMenu::readCompletion, this,
        std::placeholders::_1, std::placeholders::_2);

    std::cout << "HPCM recording started" << std::endl;

    while(1) {
        streamBuffer = freeCaptureBuffers_.front();
        freeCaptureBuffers_.pop();

        status = audioCaptureStream_->read(streamBuffer, bytesToRead, readCb);
        if(status != telux::common::Status::SUCCESS) {
            std::cout << "can't read, err " << static_cast<int>(status) << std::endl;
            readErrorOccurred_ = true;
            break;
        }

        if(freeCaptureBuffers_.empty()) {
            captureCv_.wait(lock);
        }

        if (readErrorOccurred_ || exitHpcm_) {
            /* error occurred during recording, terminate the thread */
            break;
        }
    }

    if (readErrorOccurred_) {
        std::cout << "recording finished with error" << std::endl;
    } else {
        std::cout << "recording finished" << std::endl;
    }
    /*If read operation returns with error then record thread will exit but play thread
    will be waiting for buffer. To avoid that notify play thread to exit. */
    bufferReadyCv_.notify_all();
}

/*
 *  Gets called to confirm how many bytes were actually written to stream.
 */
void HpcmMenu::writeCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    std::cout << "bytes played: " << bytesWritten << std::endl;

    if ((error != telux::common::ErrorCode::SUCCESS) ||
            (buffer->getDataSize() != bytesWritten)) {
        std::cout << "error in writting" << std::endl;
        writeErrorOccurred_ = true;
    }
    buffer->reset();
}

/*
 *  This function waits for buffer to be read from hpcm capture stream and write samples
 *  on playback stream.
 */
void HpcmMenu::play() {

    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    writeErrorOccurred_ = false;

    auto writeCb = std::bind(&HpcmMenu::writeCompletion, this,
        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::cout << "HPCM playback started" << std::endl;

    while(1) {
        //waiting for hpcm read buffer to be ready
        std::unique_lock<std::mutex> lck(bufferReadyMutex_);
        bufferReadyCv_.wait(lck);

        streamBuffer = freePlayBuffers_.front();
        freePlayBuffers_.pop();

        status = audioPlayStream_->write(streamBuffer, writeCb);
        if(status != telux::common::Status::SUCCESS) {
            std::cout << "can't write, err "<< static_cast<unsigned int>(status) << std::endl;
            writeErrorOccurred_ = true;
            break;
        }

        if(exitHpcm_ || readErrorOccurred_){
            break;
        }
    }

    if (writeErrorOccurred_) {
        std::cout << "Playback finished with error" << std::endl;
    } else {
        std::cout << "Playback finished" << std::endl;
    }
}

void HpcmMenu::takeUserVoicePathInput(std::vector<telux::audio::Direction> &direction) {
    std::string userInput = "";
    int command = -1;
    while(1) {
        std::cout << "Enter voice path type (1 for RX, 2 for TX): ";
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> command) {
                if (command == 1) {
                    direction.emplace_back(telux::audio::Direction::RX);
                    break;
                }
                else if (command == 2) {
                    direction.emplace_back(telux::audio::Direction::TX);
                    break;
                }
                else {
                    std::cout << "Invalid Input" << std::endl;
                }
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

void HpcmMenu::getUserSampleRateInput(uint32_t &sampleRate) {
    std::string userInput = "";
    while(1) {
        std::cout << "Enter Sample Rate (8000 16000) :" ;
        if (std::getline(std::cin, userInput)) {
            std::stringstream inputStream(userInput);
            if (inputStream >> sampleRate) {
                break;
            } else {
                std::cout << "Invalid Input" << std::endl;
            }
        } else {
            std::cout << "Invalid Input" << std::endl;
        }
    }
}

Status HpcmMenu::createStream(StreamConfig &streamConfig){
    std::promise<bool> p;
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
        std::cout << "Request to create stream failed"  << std::endl;
        return Status::FAILED;
    }

    if (p.get_future().get()) {
        if(myAudioStream->getType() == StreamType::PLAY) {
            audioPlayStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioPlayStream>(myAudioStream);
            std::cout<< "Audio HPCM Play Stream is Created" << std::endl;
        } else if(myAudioStream->getType() == StreamType::CAPTURE) {
            audioCaptureStream_ = std::dynamic_pointer_cast<
                        telux::audio::IAudioCaptureStream>(myAudioStream);
            std::cout<< "Audio HPCM Capture Stream is Created" << std::endl;
        } else {
            std::cout << "Unknown Stream type is generated" << std::endl;
        }
    } else {
        return Status::FAILED;
    }
    return Status::SUCCESS;
}
