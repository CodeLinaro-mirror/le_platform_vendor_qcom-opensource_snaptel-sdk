/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <queue>
#include <condition_variable>

#include <telux/audio/AudioManager.hpp>

class BTHFGVoiceCall {

 public:
    int init();
    int createVoiceStream();
    int deleteVoiceStream();
    int startVoiceStream();
    int stopVoiceStream();

 private:
    std::shared_ptr<telux::audio::IAudioManager> audioManager_;
    std::shared_ptr<telux::audio::IAudioVoiceStream> audioVoiceStream_;
};
