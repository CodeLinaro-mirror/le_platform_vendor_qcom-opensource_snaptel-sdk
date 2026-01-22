/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <queue>
#include <condition_variable>

#include <telux/audio/AudioManager.hpp>

class InCallRecordPCM {

 public:
    int init();
    int createVoiceStream();
    int deleteVoiceStream();
    int startVoiceStream();
    int stopVoiceStream();
    int createIncallRecordStream();
    int deleteIncallRecordStream();
    void record();
    void readComplete(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer, telux::common::ErrorCode error);

    char *recordingDuration_;
    char *fileToSaveRecordingPath_;

 private:
    const int32_t TIME_10_SECONDS  = 10;
    const int32_t BUFFER_POOL_SIZE = 2;
    bool errorOccurred_;
    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> audioCaptureStream_;
    uint32_t recordingDurationMs_;
    FILE *fileToSaveRecording_;
    std::mutex captureMutex_;
    std::condition_variable cv_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> bufferPool_;
};
