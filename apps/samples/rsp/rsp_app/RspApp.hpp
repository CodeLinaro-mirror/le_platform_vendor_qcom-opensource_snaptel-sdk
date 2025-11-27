/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file      RspApp.hpp
 * @brief     The reference application to demonstrate how to use the Remote SIM Provisioning API
 *            for performing SIM profile management operations on the eUICC such as add profile,
 *            enable/disable profile, delete profile, query profile list, configure server address
 *            and perform memory reset.
 */

#ifndef RSPAPP_HPP
#define RSPAPP_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SimProfileManager.hpp>
#include <telux/tel/CardManager.hpp>

#include "RspListener.hpp"

using telux::common::Status;

class RemoteSimProfile {
 public:
    static RemoteSimProfile &getInstance();
    Status parseArguments(int arg, char *argv[]);
    bool init();
    void cleanup();

 private:
    SlotId slotId_;
    int profileId_;
    bool enableProfile_;
    bool userConsent_;
    int reason_;
    int resetOption_;
    // Profile activation code
    std::string activationCode_;
    std::string confirmationCode_;
    std::string nickname_;
    std::string smdpAddress_;
    std::shared_ptr<telux::tel::ISimProfileManager> simProfileManager_ = nullptr;
    std::shared_ptr<RspListener> rspListener_                          = nullptr;
    std::shared_ptr<telux::tel::ICardManager> cardManager_             = nullptr;
    std::vector<std::shared_ptr<telux::tel::ICard>> cards_;

    RemoteSimProfile();
    ~RemoteSimProfile();
    void printUsage(char **argv);

    // Wrapper function for request Profile list, add, delete, enable or update profile
    void requestProfileList();
    void addProfile(
        const std::string &actCode, const std::string &confCode, bool isUserConsentRequired);
    void deleteProfile(int profileId);
    void setProfile(int profileId, bool enable);
    void updateNickName(int profileId, const std::string &nickname);
    void provideUserConsent(bool isUserConsentRequired, int reason);
    void provideUserConfirmation(std::string confirmationCode);
    void setServerAddress(std::string serverAddress);
    void getServerAddress();
    void memoryReset(int resetOption);
    void requestEid();

    // Response callbacks
    void onProfileListResponse(const std::vector<std::shared_ptr<telux::tel::SimProfile>> &profiles,
        telux::common::ErrorCode errorCode);
    void onEidResponse(std::string eid, telux::common::ErrorCode errorCode);
    void serverAddressResponse(
        std::string smdpAddress, std::string smdsAddress, telux::common::ErrorCode error);
    void onResponseCallback(telux::common::ErrorCode error);
};

#endif
