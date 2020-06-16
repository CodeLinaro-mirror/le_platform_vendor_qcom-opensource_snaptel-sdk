/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ECALLMANAGER_HPP
#define ECALLMANAGER_HPP

#include <telux/tel/ECallDefines.hpp>

#include "TelClient.hpp"
#include "LocationClient.hpp"
#include "AudioClient.hpp"
#include "ThermClient.hpp"
#include "MsdProvider.hpp"
#include "ConfigParser.hpp"

class ECallManager : public LocationListener,
                     public CallStatusListener,
                     public std::enable_shared_from_this<ECallManager> {
public:
    /**
     * Initialize necessary Telematics-SDK components like location, audio, etc. and and get
     * required parameters from the configuration file
     */
    telux::common::Status init();

    /**
     * This function triggers the standard eCall procedure(eg.112)
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     * @param [in] category     ECallCategory
     * @param [in] transmitMsd  Configures MSD transmission at MO call connect
     * @param [in] variant      ECallVariant
     *
     * @returns Status of triggerECall i.e success or suitable status code.
     */
    telux::common::Status triggerECall(int phoneId, ECallCategory category, ECallVariant variant,
                                       bool transmitMsd);

    /**
     * This function triggers a voice eCall procedure to the specified phone number
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     * @param [in] category     ECallCategory
     * @param [in] transmitMsd  Configures MSD transmission at MO call connect
     * @param [in] dialNumber   phone number to be dialed
     *
     * @returns Status of triggerECall i.e success or suitable status code.
     */
    telux::common::Status triggerECall(int phoneId, ECallCategory category,
                                       const std::string dialNumber, bool transmitMsd);

    /**
     * This function answers an incoming call
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     *
     * @returns Status of answerCall i.e success or suitable status code.
     *
     */
    telux::common::Status answerCall(int phoneId);

    /**
     * This function hangs up an ongoing call dialed/answered previously
     *
     * @param [in] phoneId     Represents phone corresponding to which the operation is performed
     * @param [in] callIndex   Represents the call on which the operation is performed
     *
     * @returns Status of hangupCall i.e success or suitable status code.
     *
     */
    telux::common::Status hangupCall(int phoneId, int callIndex);

    /**
     * Dump the list of calls in progress
     *
     * @returns Status of getCalls i.e success or suitable status code.
     */
    telux::common::Status getCalls();

    /**
     * This function requests status of various eCall HLAP timers
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     *
     * @returns Status of requestHlapTimerStatus i.e success or suitable status code.
     *
     */
    telux::common::Status requestHlapTimerStatus(int phoneId);

    void onLocationUpdate(ECallLocationInfo locInfo) override;
    void onCallDisconnect() override;

    ECallManager();
    ~ECallManager();

private:
    /**
     * This function updates the cached MSD data stored in Modem
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     *
     * @returns Status of updateMSD i.e success or suitable status code.
     *
     */
    telux::common::Status updateMSD(int phoneId);

    /**
     * This function enables necessary functionalities in various subsystems(location, audio, etc.),
     * that are required for an eCall
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     *
     */
    void setup(int phoneId);

    /**
     * This function disables the functionalities in various subsystems(location, audio, etc.)
     * Typically performed when an eCall ends
     */
    void cleanup();

    /**
     * This function indicates if atleast one location fix is received after the eCall is triggered.
     * Useful in creating the MSD with valid location information.
     */
    bool isLocationReceived();
    void setLocationReceived(bool state);

    /**
     * Function to parse the settings from the eCall configuration file and fetch the static MSD
     * Data
     */
    void parseAppConfig();

    /** Member variables to hold Manager objects of various Telematics-SDK components */
    std::shared_ptr<TelClient> telClient_;
    std::shared_ptr<LocationClient> locClient_;
    std::shared_ptr<AudioClient> audioClient_;
    std::shared_ptr<ThermClient> thermClient_;

    /** Represents the phone corresponding to the eCall session */
    int phoneId_;
    /** Local copy of MSD that will be used in transmission */
    ECallMsdData msdData_;
    /** Interval for which the location-fix updates needs to be received */
    uint32_t locUpdateIntervalMs_;
    std::mutex mutex_;
    bool locFixReceived_;
    std::condition_variable locUpdateCV_;
    /** Variables to store audio settings for eCall voice conversation */
    DeviceType audioDevice_;
    uint32_t voiceSampleRate_;
    AudioFormat voiceFormat_;
    ChannelTypeMask voiceChannels_;
};

#endif  // ECALLMANAGER_HPP
