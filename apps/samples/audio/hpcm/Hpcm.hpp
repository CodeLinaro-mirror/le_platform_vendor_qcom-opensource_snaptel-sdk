/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <queue>
#include <atomic>
#include <condition_variable>

#include <telux/audio/AudioManager.hpp>

class Hpcm {

 public:
    int init();

    int createVoiceStream();
    int deleteVoiceStream();
    int startVoiceStream();
    int stopVoiceStream();

    int allocateBuffers();

    int createTXPlayStream();
    int deleteTXPlayStream();

    int createTXCaptureStream();
    int deleteTXCaptureStream();

    int createRXPlayStream();
    int deleteRXPlayStream();

    int createRXCaptureStream();
    int deleteRXCaptureStream();

    void readCompleteTX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer, telux::common::ErrorCode error);
    void writeCompleteTX(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytesWritten,
        telux::common::ErrorCode error);

    void readCompleteRX(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer, telux::common::ErrorCode error);
    void writeCompleteRX(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytesWritten,
        telux::common::ErrorCode error);

    void readFromTXwriteOnTX();
    void readFromRXwriteOnRX();

    bool keepRunning_ = true;
    std::mutex txReadMutex_;
    std::mutex rxReadMutex_;
    std::condition_variable txReadWaiterCv_;
    std::condition_variable rxReadWaiterCv_;

 private:
    const int32_t BUFFER_COUNT = 2;
    uint32_t txReadSize_;
    uint32_t rxReadSize_;

    int32_t txReadDone_;
    int32_t rxReadDone_;
    int32_t txWritePossible_;
    int32_t txReadPossible_;
    int32_t rxReadPossible_;
    int32_t rxWritePossible_;

    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> readyForTxWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> readyForRxWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> txReadBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> txWriteBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> rxReadBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> rxWriteBuffers_;

    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> txPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> txCaptureStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> rxPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> rxCaptureStream_;
};
