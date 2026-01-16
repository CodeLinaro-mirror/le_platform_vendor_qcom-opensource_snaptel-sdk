/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <queue>
#include <atomic>
#include <condition_variable>

#include <telux/audio/AudioManager.hpp>

class BTHF {

 public:
    int init();
    int allocateBuffers();

    int createBTPlayStream();
    int deleteBTPlayStream();

    int createBTCaptureStream();
    int deleteBTCaptureStream();

    int createCodecPlayStream();
    int deleteCodecPlayStream();

    int createCodecCaptureStream();
    int deleteCodecCaptureStream();

    void readCompleteCodec(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer, telux::common::ErrorCode error);
    void writeCompleteCodec(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);

    void readCompleteBluetooth(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer, telux::common::ErrorCode error);
    void writeCompleteBluetooth(std::shared_ptr<telux::audio::IStreamBuffer> buffer,
        uint32_t bytesWritten, telux::common::ErrorCode error);

    void readFromBluetoothWriteOnCodec();
    void readFromCodecWriteOnBluetooth();

    bool keepRunning_ = true;
    std::mutex btReadMutex_;
    std::mutex codecReadMutex_;
    std::condition_variable btReadWaiterCv_;
    std::condition_variable codecReadWaiterCv_;

 private:
    const int32_t BUFFER_COUNT = 2;
    uint32_t btReadSize_;
    uint32_t codecReadSize_;

    int32_t btReadDone_;
    int32_t codecReadDone_;
    int32_t codecWritePossible_;
    int32_t codecReadPossible_;
    int32_t btReadPossible_;
    int32_t btWritePossible_;

    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> readyForCodecWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> readyForBluetoothWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> btReadBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> btWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> codecReadBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> codecWriteBuffers_;

    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioPlayStream> btPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> btCaptureStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> codecPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> codecCaptureStream_;
};
