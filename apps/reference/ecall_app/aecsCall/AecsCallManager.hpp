/*
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

#define MIN_SIM_SLOT_COUNT 1
#define MAX_SIM_SLOT_COUNT 2

// ---- MSD Retry Policy Constants ----
constexpr int SEC_PER_MIN                = 60;
constexpr int AECS_CALL_OR_MSD_RETRY_INTERVAL_MAX_S    = 2 * SEC_PER_MIN;      // <= 2 min
constexpr int AECS_CALL_OR_MSD_RETRY_DURATION_MIN_S    = 60 * SEC_PER_MIN;     // >= 60 min

class AecsCallManager {
public:
    static AecsCallManager &getInstance();

    bool init();

    // ----- Audio control -----
    void startAudio(int phoneId);
    void stopAudioAll();
    void stopAudioIfNoCalls(int phoneId);

    // ----- Emergency mode control -----
    telux::common::Status setEmergencyMode(int phoneId, bool emergencyModeEnabled,
        bool antennaSwitchEnabled);
    bool isEmergencyMode(int phoneId) const;

    // CallManager && SmsManager objects
    std::shared_ptr<telux::tel::ICallManager> getCallManager() const { return callMgr_; }
    std::shared_ptr<telux::tel::ISmsManager> getSmsManager(int phoneId) const {
        // phoneId is 1-based slot index in your app
        auto it = smsMgrs_.find(phoneId);
        if (it != smsMgrs_.end()) {
            return it->second;
        }
        return nullptr;
    }

    const std::map<int, std::shared_ptr<telux::tel::ISmsManager>> &getAllSmsManagers() const {
        return smsMgrs_;
    }

    void setEmergencyModeResponse(telux::common::ErrorCode error);

    void setAecsCallDropStatus(bool aecsCallDrop);
    bool getAecsCallDropStatus();
    void setAecsCallFailStatus(bool aecsCallFail);
    bool getAecsCallFailStatus();
    int getAecsRetryInterval();
    int getAecsRetryDuration();
    int getAecsOemRetryInterval();
    int getAecsOemRetryDuration();
    std::vector<telux::tel::PduBuffer> getRetryRawPdu();
    void setRetryRawPdu(std::vector<telux::tel::PduBuffer> rawPdus);

private:
    AecsCallManager();
    ~AecsCallManager();

    void parseConfig();

    AecsCallManager(const AecsCallManager &) = delete;
    AecsCallManager &operator=(const AecsCallManager &) = delete;

    // Call manager already exists
    std::shared_ptr<telux::tel::ICallManager> callMgr_;

    // New: one SMS manager per slot/phoneId
    std::map<int, std::shared_ptr<telux::tel::ISmsManager>> smsMgrs_;
    // Audio
    std::shared_ptr<AudioClient> audioClient_;
     /** Variables to store audio settings for eCall voice conversation */
    std::vector<DeviceType> audioDevices_ {DeviceType::DEVICE_TYPE_SPEAKER,
                     DeviceType::DEVICE_TYPE_MIC};
    uint32_t voiceSampleRate_;
    AudioFormat voiceFormat_;
    ChannelTypeMask voiceChannels_;
    EcnrMode ecnrMode_;
    int aecsRetryInterval_;
    int aecsRetryDuration_;
    int aecsOemRetryInterval_;
    int aecsOemRetryDuration_;
    std::vector<telux::tel::PduBuffer> retryRawPdu_;

    // Emergency-mode per phoneId
    std::map<int,bool> emergencyMode_;   // true when enabled
    mutable std::mutex emergencyModeMutex_;  // Add mutex for thread safety
    bool aecsCallDrop_ = false;
    bool aecsCallFail_ = false;
};

#endif
