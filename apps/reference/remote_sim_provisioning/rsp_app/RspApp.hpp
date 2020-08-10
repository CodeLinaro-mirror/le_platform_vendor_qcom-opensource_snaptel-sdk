/*
 *  Copyright (c) 2020 The Linux Foundation. All rights reserved.
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

/**
 * @file      RspApp.cpp
 * @brief     The reference application to demonstrate Remote SIM Provisioning features
 *            like addProfile, deleteProfile, setProfile, requestProfileList, updateNickName,
 *            provideUserConsent, requestEid.
 */

#ifndef RSPAPP_HPP
#define RSPAPP_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/rsp/SimProfileFactory.hpp>

#include "RspListener.hpp"

using telux::common::Status;

class RemoteSimProfile {
 public:
    static RemoteSimProfile &getInstance();
    Status parseArguments(int arg, char *argv[]);
    void init();

 private:
    SlotId slotId_;
    int profileId_;
    bool enableProfile_;
    bool userConsent_;
    // Profile activation code
    std::string activationCode_;
    std::string confirmationCode_;
    std::string nickname_;
    std::shared_ptr<telux::rsp::ISimProfileManager> simProfileManager_ = nullptr;
    std::shared_ptr<RspListener> rspListener_ = nullptr;

    RemoteSimProfile();
    ~RemoteSimProfile();
    void printUsage(char **argv);

    // Wrapper function for request Profile list, add, delete, enable or update profile
    void requestProfileList();
    void addProfile(const std::string &actCode, const std::string &confCode,
                    bool isUserConsentRequired);
    void deleteProfile(int profileId);
    void setProfile(int profileId, bool enable);
    void updateNickName(int profileId, const std::string &nickname);
    void provideUserConsent(bool isUserConsentRequired);
    void requestEid();

    // Response callbacks
    void onProfileListResponse(const std::vector<std::shared_ptr<telux::rsp::SimProfile>> &profiles,
        telux::common::ErrorCode errorCode);
    void onEidResponse(std::string eid, telux::common::ErrorCode errorCode);
    void onResponseCallback(telux::common::ErrorCode error);
};

#endif
