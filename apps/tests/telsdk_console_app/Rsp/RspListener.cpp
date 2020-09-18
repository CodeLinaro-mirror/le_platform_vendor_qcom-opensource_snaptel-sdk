/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include <iostream>
#include <string>

#include "RspListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void RspListener::onAddProfileUpdate(SlotId slotId, bool userConsentRequired,
    telux::tel::DownloadStatus status, uint8_t percentage, telux::tel::DownloadErrorCause cause,
    telux::tel::PolicyRuleMask mask) {
    PRINT_NOTIFICATION << " onAddProfileUpdate" << std::endl;
    PRINT_NOTIFICATION << " Slot Id: " << static_cast<int>(slotId) << std::endl;
    PRINT_NOTIFICATION << " Profile Download Status: " << profileDownloadStatusToString(status)
                       << std::endl;
    PRINT_NOTIFICATION << " Percentage: " << static_cast<int>(percentage) << std::endl;
    PRINT_NOTIFICATION
        << " Profile Download Error Cause: " << profileDownloadErrorCauseToString(cause)
        << std::endl;
    std::string policyRule = pprMaskToString(mask);
    PRINT_NOTIFICATION << " Profile Policy Rule: " << policyRule << std::endl;
}

std::string RspListener::profileDownloadStatusToString(telux::tel::DownloadStatus status) {
    std::string downloadStatus;
    switch (status) {
        case telux::tel::DownloadStatus::DOWNLOAD_ERROR:
            downloadStatus = "DOWNLOAD_ERROR";
            break;
        case telux::tel::DownloadStatus::DOWNLOAD_IN_PROGRESS:
            downloadStatus = "DOWNLOAD_IN_PROGRESS";
            break;
        case telux::tel::DownloadStatus::DOWNLOAD_COMPLETE_INSTALLATION_IN_PROGRESS:
            downloadStatus = "DOWNLOAD_COMPLETE_INSTALLATION_IN_PROGRESS";
            break;
        case telux::tel::DownloadStatus::INSTALLATION_COMPLETE:
            downloadStatus = "INSTALLATION_COMPLETE";
            break;
        case telux::tel::DownloadStatus::USER_CONSENT_REQUIRED:
            downloadStatus = "USER_CONSENT_REQUIRED";
            break;
        default:
            downloadStatus = "UNKNOWN";
            break;
    }
    return downloadStatus;
}

std::string RspListener::profileDownloadErrorCauseToString(telux::tel::DownloadErrorCause cause) {
    std::string errorCause;
    switch (cause) {
        case telux::tel::DownloadErrorCause::GENERIC:
            errorCause = "GENERIC";
            break;
        case telux::tel::DownloadErrorCause::SIM:
            errorCause = "SIM";
            break;
        case telux::tel::DownloadErrorCause::NETWORK:
            errorCause = "NETWORK";
            break;
        case telux::tel::DownloadErrorCause::MEMORY:
            errorCause = "MEMORY";
            break;
        default:
            errorCause = "UNKNOWN";
            break;
    }
    return errorCause;
}

std::string RspListener::pprMaskToString(telux::tel::PolicyRuleMask mask) {
    std::string ppr = "UNKNOWN";
    if (
       mask[(telux::tel::PolicyRuleType)(telux::tel::PolicyRuleType::PROFILE_DISABLE_NOT_ALLOWED)]){
        ppr = "Profile disable not allowed";
    }
    if (mask[(telux::tel::PolicyRuleType)(telux::tel::PolicyRuleType::PROFILE_DELETE_NOT_ALLOWED)]){
        ppr = "Profile delete not allowed";
    }
    if (mask[(telux::tel::PolicyRuleType)(telux::tel::PolicyRuleType::PROFILE_DELETE_ON_DISABLE)]) {
        ppr = "Profile delete on disable";
    }
    return ppr;
}
