/* Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AECSCALLMANAGER_HPP
#define AECSCALLMANAGER_HPP

#include <memory>
#include <vector>
#include <map>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/DeviceConfig.hpp>
#include <telux/common/CommonDefines.hpp>

#include "AudioClient.hpp"
#include "ConfigParser.hpp"

class AecsCallManager {
public:
    static AecsCallManager &getInstance();

    bool init();

    // ----- Audio control -----
    void startAudio(int phoneId);
    void stopAudioAll();
    void stopAudioIfNoCalls(int phoneId);

    // ----- Emergency mode control -----
    telux::common::Status enterEmergencyMode(int phoneId, bool enableAntennaSwitch = false);
    telux::common::Status exitEmergencyMode(int phoneId);
    telux::common::Status setEmergencyMode(int phoneId, bool emergencyModeEnabled,
    bool antennaSwitchEnabled);
    bool isEmergencyMode(int phoneId) const;

    // Optional: expose CallManager
    std::shared_ptr<telux::tel::ICallManager> getCallManager() const { return callMgr_; }

    void setEmergencyModeResponse(telux::common::ErrorCode error);

    void setAecsCallDropStatus(bool aecsCallDrop);
    bool getAecsCallDropStatus();
    void setAecsCallFailStatus(bool aecsCallFail);
    bool getAecsCallFailStatus();

private:
    AecsCallManager();
    ~AecsCallManager();

    void parseAudioConfig();

    AecsCallManager(const AecsCallManager &) = delete;
    AecsCallManager &operator=(const AecsCallManager &) = delete;

    std::shared_ptr<telux::tel::ICallManager> callMgr_;
    // Audio
    std::shared_ptr<AudioClient> audioClient_;
     /** Variables to store audio settings for eCall voice conversation */
    std::vector<DeviceType> audioDevices_ {DeviceType::DEVICE_TYPE_SPEAKER,
                     DeviceType::DEVICE_TYPE_MIC};
    uint32_t voiceSampleRate_;
    AudioFormat voiceFormat_;
    ChannelTypeMask voiceChannels_;
    EcnrMode ecnrMode_;

    // Emergency-mode per phoneId
    std::map<int,bool> emergencyMode_;   // true when enabled
    bool aecsCallDrop_ = false;
    bool aecsCallFail_ = false;
};

#endif
