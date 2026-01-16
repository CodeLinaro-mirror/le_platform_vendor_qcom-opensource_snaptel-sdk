/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/audio/AudioManager.hpp>

class LoopbackMicSpeaker {

 public:
    int init();
    int createLoopbackStream();
    int deleteLoopbackStream();
    int startLoopback();
    int stopLoopback();

 private:
    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioLoopbackStream> audioLoopbackStream_;
};
