/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <condition_variable>
#include <telux/audio/AudioPlayer.hpp>

class RepeatedPlayAMRWBPlus : public telux::audio::IPlayListListener {

 public:
    bool playStarted_   = false;
    bool playStopped_   = false;
    bool playFinished_  = false;
    bool errorOccurred_ = false;

    std::mutex playMutex_;
    std::condition_variable playCV_;

    int init();
    int start(std::shared_ptr<RepeatedPlayAMRWBPlus> statusListener);
    int wait();
    int stop();

    void onPlaybackStarted() override;
    void onPlaybackStopped() override;
    void onPlaybackFinished() override;
    void onFilePlayed(std::string file) override;
    void onError(telux::common::ErrorCode error, std::string file) override;

 private:
    std::shared_ptr<telux::audio::IAudioPlayer> audioPlayer_;
};
