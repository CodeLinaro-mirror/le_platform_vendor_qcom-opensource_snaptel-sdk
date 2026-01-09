/* Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef AECSCALLLISTENER_HPP
#define AECSCALLLISTENER_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/CallListener.hpp>

#include "AecsCallManager.hpp"

class AecsCallListener : public telux::tel::ICallListener {
public:
   void onIncomingCall(std::shared_ptr<telux::tel::ICall> call) override;
   void onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) override;

   std::string getCallStateString(telux::tel::CallState cs);
   std::string getCallEndCauseString(telux::tel::CallEndCause causeCode);
   std::string getCurrentTime();

   ~AecsCallListener() {
   }
};

class AecsCallCommandCallback : public telux::common::ICommandResponseCallback {
public:
   AecsCallCommandCallback(std::string commandName);
   void commandResponse(telux::common::ErrorCode error) override;

private:
   std::string commandName_;
};

#endif  // AECSCALLLISTENER_HPP
