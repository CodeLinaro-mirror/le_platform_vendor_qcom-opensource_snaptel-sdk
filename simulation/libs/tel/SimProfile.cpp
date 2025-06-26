/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * SimProfile  implementation
 */

#include <sstream>
#include <telux/tel/SimProfile.hpp>
#include "common/Logger.hpp"

namespace telux {
namespace tel {

SimProfile::SimProfile(int profileId, ProfileType profileType, const std::string &iccid,
    bool isActive, const std::string &nickName, const std::string &spn, const std::string &name,
    IconType iconType, std::vector<uint8_t> icon, ProfileClass profileClass,
    PolicyRuleMask policyRuleMask, int slotId, uint8_t portId)
   : profileId_(profileId)
   , profileType_(profileType)
   , iccid_(iccid)
   , isActive_(isActive)
   , nickName_(nickName)
   , spn_(spn)
   , name_(name)
   , iconType_(iconType)
   , icon_(icon)
   , profileClass_(profileClass)
   , policyRuleMask_(policyRuleMask)
   , slotId_(slotId)
   , portId_(portId) {
}

int SimProfile::getSlotId() {
    return slotId_;
}

int SimProfile::getProfileId() {
    return profileId_;
}

ProfileType SimProfile::getType() {
    return profileType_;
}

const std::string &SimProfile::getIccid() {
    return iccid_;
}

bool SimProfile::isActive() {
    return isActive_;
}

const std::string &SimProfile::getNickName() {
    return nickName_;
}

const std::string &SimProfile::getSPN() {
    return spn_;
}

const std::string &SimProfile::getName() {
    return name_;
}

IconType SimProfile::getIconType() {
    return iconType_;
}

std::vector<uint8_t> SimProfile::getIcon() {
    return icon_;
}

ProfileClass SimProfile::getClass() {
    return profileClass_;
}

PolicyRuleMask SimProfile::getPolicyRule() {
    return policyRuleMask_;
}

std::string profileTypeToString(ProfileType profileType) {
    std::string type = "";
    switch (profileType) {
        case ProfileType::REGULAR:
            type = "REGULAR";
            break;
        case ProfileType::EMERGENCY:
            type = "EMERGENCY";
            break;
        default:
            type = "UNKNOWN";
            break;
    }
    return type;
}
std::string profileClassToString(ProfileClass profileClass) {
    std::string profClass = "";
    switch (profileClass) {
        case ProfileClass::TEST:
            profClass = "TEST";
            break;
        case ProfileClass::PROVISIONING:
            profClass = "PROVISIONING";
            break;
        case ProfileClass::OPERATIONAL:
            profClass = "OPERATIONAL";
            break;
        default:
            profClass = "UNKNOWN";
            break;
    }
    return profClass;
}

std::string iconTypeToString(IconType iconType) {
    std::string icon = "";
    switch (iconType) {
        case IconType::JPEG:
            icon = "JPEG";
            break;
        case IconType::PNG:
            icon = "PNG";
            break;
        default:
            icon = "UNKNOWN";
            break;
    }
    return icon;
}

std::string convertPolicyRuleMaskToString(PolicyRuleMask policyRuleMask) {
    std::string ruleMask = "";
    if (policyRuleMask[static_cast<int>(telux::tel::PolicyRuleType::PROFILE_DISABLE_NOT_ALLOWED)]) {
        ruleMask = ruleMask + " PROFILE_DISABLE_NOT_ALLOWED";
    }
    if (policyRuleMask[static_cast<int>(telux::tel::PolicyRuleType::PROFILE_DELETE_NOT_ALLOWED)]) {
        ruleMask = ruleMask + " PROFILE_DELETE_NOT_ALLOWED";
    }
    if (policyRuleMask[static_cast<int>(telux::tel::PolicyRuleType::PROFILE_DELETE_ON_DISABLE)]) {
        ruleMask = ruleMask + " PROFILE_DELETE_ON_DISABLE";
    }

    if (ruleMask.empty()) {
        ruleMask = " No PPR/s set";
    }
    return ruleMask;
}

std::string SimProfile::toString() {
    std::stringstream ss;
    ss << " Profile Id: " << profileId_ << ", Profile Type: " << profileTypeToString(profileType_)
       << ", ICCID: " << iccid_ << ", Active: " << isActive_ << ", NickName: " << nickName_
       << ", SPN: " << spn_ << ", Profile Name: " << name_
       << ", Profile Icon Type: " << iconTypeToString(iconType_)
       << ", Profile Class: " << profileClassToString(profileClass_)
       << ", \n Policy Rules: " << convertPolicyRuleMaskToString(policyRuleMask_)
       << ", \n Port Id: " << static_cast<int>(portId_);
    return ss.str();
}

uint8_t SimProfile::getPortId() {
    return portId_;
}

}  // end of namespace tel

}  // end namespace telux
