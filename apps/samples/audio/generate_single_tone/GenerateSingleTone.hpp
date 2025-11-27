/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/audio/AudioManager.hpp>

class GenerateSingleTone {

 public:
    int init();
    int createToneStream();
    int deleteToneStream();
    int generateSingleTone();
    int stopGeneratingTone();

 private:
    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioToneGeneratorStream> audioToneStream_;
};
