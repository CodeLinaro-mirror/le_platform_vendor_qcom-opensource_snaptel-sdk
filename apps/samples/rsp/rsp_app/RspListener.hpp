/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef RSPLISTENER_HPP
#define RSPLISTENER_HPP

#include <telux/tel/SimProfileListener.hpp>
#include <telux/tel/SimProfileDefines.hpp>

class RspListener : public telux::tel::ISimProfileListener {
 public:
    void onDownloadStatus(SlotId slotId, telux::tel::DownloadStatus status,
        telux::tel::DownloadErrorCause cause) override;
    void onUserDisplayInfo(
        SlotId slotId, bool userConsentRequired, telux::tel::PolicyRuleMask mask) override;
    void onConfirmationCodeRequired(SlotId slotId, std::string profileName) override;

 private:
    std::string profileDownloadStatusToString(telux::tel::DownloadStatus status);
    std::string profileDownloadErrorCauseToString(telux::tel::DownloadErrorCause cause);
    std::string pprMaskToString(telux::tel::PolicyRuleMask mask);
};

#endif  // RSPLISTENER_HPP
