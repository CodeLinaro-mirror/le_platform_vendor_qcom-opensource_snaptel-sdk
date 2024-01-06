/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <errno.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <cstring>
#include <iostream>

#include <telux/audio/AudioFactory.hpp>

#include "BTHFVoiceCall.hpp"

/*
 * Initialize application and get an audio service.
 */
int BTHFVoiceCall::init() {

    std::promise<telux::common::ServiceStatus> p{};
    telux::common::ServiceStatus serviceStatus;

    /* Step - 1 */
    auto &audioFactory = telux::audio::AudioFactory::getInstance();

    /* Step - 2 */
    audioManager_ = audioFactory.getAudioManager(
            [&p](telux::common::ServiceStatus status) {
        if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            p.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        } else {
            p.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
        }
    });

    if (!audioManager_) {
        std::cout << "Can't get IAudioManager" << std::endl;
        return -ENOMEM;
    }

    /* Step - 3 */
    serviceStatus = audioManager_->getServiceStatus();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "audio service not ready, waiting..." << std::endl;
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "audio service unavailable" << std::endl;
            return -EIO;
        }
        std::cout << "audio service ready" << std::endl;
    }

    return 0;
}

int BTHFVoiceCall::createBTPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::PLAY;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_BT_SCO_SPEAKER);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            btPlayStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioPlayStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create bt playback stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create bt playback stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

int BTHFVoiceCall::deleteBTPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_-> deleteStream(btPlayStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete bt playback stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete bt playback stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

int BTHFVoiceCall::createBTCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::CAPTURE;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_BT_SCO_MIC);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            btCaptureStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioCaptureStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create bt capture stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create bt capture stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

int BTHFVoiceCall::deleteBTCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_-> deleteStream(btCaptureStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete bt capture stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete bt capture stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

int BTHFVoiceCall::createCodecPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::PLAY;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_SPEAKER);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            codecPlayStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioPlayStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create codec playback stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create codec playback stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

int BTHFVoiceCall::deleteCodecPlayStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_-> deleteStream(codecPlayStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete codec playback stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete codec playback stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

int BTHFVoiceCall::createCodecCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::audio::StreamConfig sc{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    sc.type = telux::audio::StreamType::CAPTURE;
    sc.sampleRate = 8000;
    sc.format = telux::audio::AudioFormat::PCM_16BIT_SIGNED;
    sc.channelTypeMask = telux::audio::ChannelType::LEFT;
    sc.deviceTypes.emplace_back(telux::audio::DeviceType::DEVICE_TYPE_MIC);

    status = audioManager_->createStream(sc, [&p, this] (
            std::shared_ptr<telux::audio::IAudioStream> &audioStream,
            telux::common::ErrorCode result) {
        if (result == telux::common::ErrorCode::SUCCESS) {
            codecCaptureStream_ = std::dynamic_pointer_cast<
                telux::audio::IAudioCaptureStream>(audioStream);
        }
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request create codec capture stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed create codec capture stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

int BTHFVoiceCall::deleteCodecCaptureStream() {

    std::promise<telux::common::ErrorCode> p{};
    telux::common::Status status;
    telux::common::ErrorCode ec;

    status = audioManager_-> deleteStream(codecCaptureStream_, [&p, this] (
            telux::common::ErrorCode result) {
        p.set_value(result);
    });

    if (status != telux::common::Status::SUCCESS) {
        std::cout << "can't request delete codec capture stream"  << std::endl;
        return -EIO;
    }

    ec = p.get_future().get();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "failed delete codec capture stream, err " <<
            static_cast<int>(ec) << std::endl;
        return -EIO;
    }

    return 0;
}

int BTHFVoiceCall::allocateBuffers() {

    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;

    btReadSize_ = 0;
    codecReadSize_ = 0;

    for (int x = 0; x < BUF_COUNT; x++) {
        streamBuffer = btCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get bt capture stream buffer" << std::endl;
            return -ENOMEM;
        }

        btReadSize_ = streamBuffer->getMinSize();
        if (!btReadSize_) {
            btReadSize_ =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(btReadSize_);
        btReadBuffers_.push(streamBuffer);
    }

    for (int x = 0; x < BUF_COUNT; x++) {
        streamBuffer = codecCaptureStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get codec capture stream buffer" << std::endl;
            return -ENOMEM;
        }

        codecReadSize_ = streamBuffer->getMinSize();
        if (!codecReadSize_) {
            codecReadSize_ =  streamBuffer->getMaxSize();
        }

        streamBuffer->setDataSize(codecReadSize_);
        codecReadBuffers_.push(streamBuffer);
    }

    for (int x = 0; x < BUF_COUNT; x++) {
        streamBuffer = btPlayStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get bt play stream buffer" << std::endl;
            return -ENOMEM;
        }

        streamBuffer->setDataSize(codecReadSize_);
        btWriteBuffers_.push(streamBuffer);
    }

    for (int x = 0; x < BUF_COUNT; x++) {
        streamBuffer = codecPlayStream_->getStreamBuffer();
        if (!streamBuffer) {
            std::cout << "can't get codec play stream buffer" << std::endl;
            return -ENOMEM;
        }

        streamBuffer->setDataSize(btReadSize_);
        codecWriteBuffers_.push(streamBuffer);
    }

    return 0;
}

void BTHFVoiceCall::writeCompleteCodec(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "write codec err " << static_cast<int>(error) << std::endl;
        {
            std::lock_guard<std::mutex> codecReadLock(codecReadMutex_);
            keepRunning_ = false;
            codecReadWaiterCv_.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> btReadLock(btReadMutex_);

        codecWriteBuffers_.push(buffer);

        if (error == telux::common::ErrorCode::SUCCESS) {
            ++codecWritePossible_;
        }

        btReadWaiterCv_.notify_all();
    }
}

void BTHFVoiceCall::readCompleteBluetooth(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read bt err " << static_cast<int>(error) << std::endl;
        {
            std::lock_guard<std::mutex> codecReadLock(codecReadMutex_);
            keepRunning_ = false;
            codecReadWaiterCv_.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> btReadLock(btReadMutex_);

        readyForCodecWriteBuffers_.push(buffer);
        btReadBuffers_.push(buffer);

        if (error == telux::common::ErrorCode::SUCCESS) {
            ++btReadPossible_;
            if (btReadDone_ < BUF_COUNT) {
                ++btReadDone_;
            }
        }

        btReadWaiterCv_.notify_all();
    }
}

void BTHFVoiceCall::readFromBluetoothWriteOnCodec() {

    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::shared_ptr<telux::audio::IStreamBuffer> tmpBufferPtr;

    auto btReadCompleteCb = std::bind(&BTHFVoiceCall::readCompleteBluetooth,
            this, std::placeholders::_1, std::placeholders::_2);

    auto codecWriteCompleteCb = std::bind(&BTHFVoiceCall::writeCompleteCodec,
            this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::unique_lock<std::mutex> btReadLock(btReadMutex_);

    btReadDone_ = 0;
    btReadPossible_ = BUF_COUNT;
    codecWritePossible_ = BUF_COUNT;

    std::cout << "read from bt and write on codec started!" << std::endl;

    while(keepRunning_) {
        if (btReadDone_ && codecWritePossible_) {

            tmpBufferPtr = readyForCodecWriteBuffers_.front();
            readyForCodecWriteBuffers_.pop();

            streamBuffer = codecWriteBuffers_.front();
            codecWriteBuffers_.pop();

            btReadDone_--;

            std::memcpy(streamBuffer->getRawBuffer(),
                tmpBufferPtr->getRawBuffer(), btReadSize_);

            status = codecPlayStream_->write(streamBuffer, codecWriteCompleteCb);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "codec write err " << static_cast<int>(status) << std::endl;
                keepRunning_ = false;
                codecReadWaiterCv_.notify_all();
                break;
            }

            codecWritePossible_--;
        }

        if (btReadPossible_) {
            streamBuffer = btReadBuffers_.front();
            btReadBuffers_.pop();

            status = btCaptureStream_->read(streamBuffer, btReadSize_, btReadCompleteCb);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "bt read err " << static_cast<int>(status) << std::endl;
                keepRunning_ = false;
                codecReadWaiterCv_.notify_all();
                break;
            }

            btReadPossible_--;
        }

        btReadWaiterCv_.wait(btReadLock, [this] {
            return ((btReadPossible_ ||
                (btReadDone_ && codecWritePossible_)) ? true : false);
        });
    }

    while ((btReadBuffers_.size() != static_cast<uint32_t>(BUF_COUNT)) &&
            (codecWriteBuffers_.size() != static_cast<uint32_t>(BUF_COUNT))) {
        btReadWaiterCv_.wait(btReadLock);
    }

    std::cout << "read from bt and write on codec completed!" << std::endl;
}

void BTHFVoiceCall::writeCompleteBluetooth(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "write bt err " << static_cast<int>(error) << std::endl;
        {
            std::lock_guard<std::mutex> btReadLock(btReadMutex_);
            keepRunning_ = false;
            btReadWaiterCv_.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> codecReadLock(codecReadMutex_);

        btWriteBuffers_.push(buffer);

        if (error == telux::common::ErrorCode::SUCCESS) {
            ++btWritePossible_;
        }

        codecReadWaiterCv_.notify_all();
    }
}

void BTHFVoiceCall::readCompleteCodec(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        telux::common::ErrorCode error) {

    if (error != telux::common::ErrorCode::SUCCESS) {
        std::cout << "read codec err " << static_cast<int>(error) << std::endl;
        {
            std::lock_guard<std::mutex> btReadLock(btReadMutex_);
            keepRunning_ = false;
            btReadWaiterCv_.notify_all();
        }
    }

    {
        std::lock_guard<std::mutex> codecReadLock(codecReadMutex_);

        readyForBluetoothWriteBuffers_.push(buffer);
        codecReadBuffers_.push(buffer);

        if (error == telux::common::ErrorCode::SUCCESS) {
            ++codecReadPossible_;
            if (codecReadDone_ < BUF_COUNT) {
                ++codecReadDone_;
            }
        }

        codecReadWaiterCv_.notify_all();
    }
}

void BTHFVoiceCall::readFromCodecWriteOnBluetooth() {

    telux::common::Status status;
    std::shared_ptr<telux::audio::IStreamBuffer> streamBuffer;
    std::shared_ptr<telux::audio::IStreamBuffer> tmpBufferPtr;

    auto codecReadCompleteCb = std::bind(&BTHFVoiceCall::readCompleteCodec,
            this, std::placeholders::_1, std::placeholders::_2);

    auto btWriteCompleteCb = std::bind(&BTHFVoiceCall::writeCompleteBluetooth,
            this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    std::unique_lock<std::mutex> codecReadLock(codecReadMutex_);

    codecReadDone_ = 0;
    codecReadPossible_ = BUF_COUNT;
    btWritePossible_ = BUF_COUNT;

    std::cout << "read from codec and write on bt started!" << std::endl;

    while(keepRunning_) {
        if (codecReadDone_ && btWritePossible_) {

            tmpBufferPtr = readyForBluetoothWriteBuffers_.front();
            readyForBluetoothWriteBuffers_.pop();

            streamBuffer = btWriteBuffers_.front();
            btWriteBuffers_.pop();

            codecReadDone_--;

            std::memcpy(streamBuffer->getRawBuffer(),
                tmpBufferPtr->getRawBuffer(), codecReadSize_);

            status = btPlayStream_->write(streamBuffer, btWriteCompleteCb);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "bt write err " << static_cast<int>(status) << std::endl;
                keepRunning_ = false;
                btReadWaiterCv_.notify_all();
                break;
            }

            btWritePossible_--;
        }

        if (codecReadPossible_) {
            streamBuffer = codecReadBuffers_.front();
            codecReadBuffers_.pop();

            status = codecCaptureStream_->read(
                streamBuffer, codecReadSize_, codecReadCompleteCb);
            if(status != telux::common::Status::SUCCESS) {
                std::cout << "codec read err " << static_cast<int>(status) << std::endl;
                keepRunning_ = false;
                btReadWaiterCv_.notify_all();
                break;
            }

            codecReadPossible_--;
        }

        codecReadWaiterCv_.wait(codecReadLock, [this] {
            return ((codecReadPossible_ ||
                (codecReadDone_ && btWritePossible_)) ? true : false);
        });
    }

    while ((codecReadBuffers_.size() != static_cast<uint32_t>(BUF_COUNT)) &&
            (btWriteBuffers_.size() != static_cast<uint32_t>(BUF_COUNT))) {
        codecReadWaiterCv_.wait(codecReadLock);
    }

    std::cout << "read from codec and write on bt completed!" << std::endl;
}

int main(int argc, char **argv) {

    int ret;
    std::shared_ptr<BTHFVoiceCall> app;

    try {
        app = std::make_shared<BTHFVoiceCall>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate BTHFVoiceCall" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->createBTPlayStream();
    if (ret < 0) {
        return ret;
    }

    ret = app->createBTCaptureStream();
    if (ret < 0) {
        app->deleteBTPlayStream();
        return ret;
    }

    ret = app->createCodecPlayStream();
    if (ret < 0) {
        app->deleteBTCaptureStream();
        app->deleteBTPlayStream();
        return ret;
    }

    ret = app->createCodecCaptureStream();
    if (ret < 0) {
        app->deleteBTCaptureStream();
        app->deleteBTPlayStream();
        app->deleteCodecPlayStream();
        return ret;
    }

    ret = app->allocateBuffers();
    if (ret < 0) {
        app->deleteBTCaptureStream();
        app->deleteBTPlayStream();
        app->deleteCodecCaptureStream();
        app->deleteCodecPlayStream();
        return ret;
    }

    std::thread captureBluetooth(
        &BTHFVoiceCall::readFromBluetoothWriteOnCodec, &(*app));
    std::thread captureCodec(
        &BTHFVoiceCall::readFromCodecWriteOnBluetooth, &(*app));

    /* Run the use case for 5 minutes */
    std::this_thread::sleep_for(std::chrono::minutes(1)); //TODO
    app->keepRunning_ = false;
    {
        std::lock_guard<std::mutex> btReadLock(app->btReadMutex_);
        app->btReadWaiterCv_.notify_all();
    }
    {
        std::lock_guard<std::mutex> codecReadLock(app->codecReadMutex_);
        app->codecReadWaiterCv_.notify_all();
    }

    captureBluetooth.join();
    captureCodec.join();

    ret = app->deleteBTCaptureStream();
    if (ret < 0) {
        app->deleteCodecCaptureStream();
        app->deleteBTPlayStream();
        app->deleteCodecPlayStream();
        return ret;
    }

    ret = app->deleteCodecCaptureStream();
    if (ret < 0) {
        app->deleteBTPlayStream();
        app->deleteCodecPlayStream();
        return ret;
    }

    ret = app->deleteBTPlayStream();
    if (ret < 0) {
        app->deleteCodecPlayStream();
        return ret;
    }

    ret = app->deleteCodecPlayStream();
    if (ret < 0) {
        return ret;
    }

    return 0;
}
