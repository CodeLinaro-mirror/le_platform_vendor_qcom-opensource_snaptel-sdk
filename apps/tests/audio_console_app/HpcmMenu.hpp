/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef HPCMMENU_HPP
#define HPCMMENU_HPP

#include <queue>

#include "ConsoleApp.hpp"
#include "AudioClient.hpp"
#include "../../common/Audio/VoiceSession.hpp"
#include "../../common/Audio/AudioHelper.hpp"
#include <telux/audio/AudioManager.hpp>

#define NO_PLAY_BUFFER 2

class HpcmMenu : public ConsoleApp, public std::enable_shared_from_this<HpcmMenu> {
 public:
    HpcmMenu(std::string appName, std::string cursor, std::shared_ptr<IAudioManager> audioManager);
    ~HpcmMenu();
    void init();
    void setSystemReady();
    void cleanup();

 private:
    Status createVoiceStream(StreamConfig &config);
    Status createHpcmRecordStream(StreamConfig &config);
    Status createHpcmPlayStream(StreamConfig &config);
    Status startVoiceStream();
    Status startHpcm();
    Status deleteHpcmRecordStream();
    Status deleteHpcmPlayStream();
    Status stopVoiceStream();
    Status deleteVoiceStream();
    void startHpcmAudio(std::vector<std::string> userInput);
    void stopHpcmAudio(std::vector<std::string> userInput);
    void deleteActiveSession(SlotId slotId);
    Status createActiveSession(SlotId slotId);
    Status setActiveSession(SlotId slotId);
    void play();
    void record();
    void writeCompletion(std::shared_ptr<telux::audio::IStreamBuffer> buffer, uint32_t bytesWritten,
        telux::common::ErrorCode error);
    void readCompletion(
        std::shared_ptr<telux::audio::IStreamBuffer> buffer, telux::common::ErrorCode error);
    void takeUserVoicePathInput(std::vector<telux::audio::Direction> &direction);
    Status createStream(StreamConfig &streamConfig);
    void getUserSampleRateInput(uint32_t &sampleRate);
    void getUserSlotIdInput(SlotId &slotId);

    SlotId slotId_;
    std::atomic<bool> hpcmReady_;
    std::atomic<bool> exitHpcm_;
    std::atomic<bool> readErrorOccurred_;
    std::atomic<bool> writeErrorOccurred_;
    std::atomic<bool> exitPlayThread_;
    std::atomic<bool> exitRecordThread_;
    std::mutex mutex_;
    std::mutex captureMutex_;
    std::mutex bufferReadyMutex_;
    std::condition_variable captureCv_;
    std::condition_variable bufferReadyCv_;
    std::vector<std::thread> runningThreads_;
    std::shared_ptr<VoiceSession> activeSession_;
    std::map<SlotId, std::shared_ptr<VoiceSession>> voiceSessions_;
    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream_;
    std::shared_ptr<telux::audio::IAudioPlayStream> audioPlayStream_;
    std::shared_ptr<telux::audio::IAudioCaptureStream> audioCaptureStream_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> freePlayBuffers_;
    std::queue<std::shared_ptr<telux::audio::IStreamBuffer>> freeCaptureBuffers_;
};

#endif  // HPCMMENU_HPP
