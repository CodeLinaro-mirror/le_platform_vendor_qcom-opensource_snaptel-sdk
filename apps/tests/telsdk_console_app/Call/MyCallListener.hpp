/*
 *  Copyright (c) 2018,2020 The Linux Foundation. All rights reserved.
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

#ifndef MYCALLLISTENER_HPP
#define MYCALLLISTENER_HPP

#include <telux/tel/CallManager.hpp>
#include <telux/tel/CallListener.hpp>
#include <telux/common/CommonDefines.hpp>

#include "../Audio/AudioClient.hpp"

class MyCallListener : public telux::tel::ICallListener {
public:
   void onIncomingCall(std::shared_ptr<telux::tel::ICall> call) override;
   void onCallInfoChange(std::shared_ptr<telux::tel::ICall> call) override;
   void onECallMsdTransmissionStatus(int phoneId, telux::common::ErrorCode errorCode) override;

   std::string getCallStateString(telux::tel::CallState cs);
   std::string getCallEndCauseString(telux::tel::CallEndCause causeCode);
   std::string getCurrentTime();

   ~MyCallListener() {
   }
private:
    int getCallsOnSlot(SlotId slotId);
};

class MyDialCallback : public telux::tel::IMakeCallCallback {
public:
   void makeCallResponse(telux::common::ErrorCode error,
                         std::shared_ptr<telux::tel::ICall> call) override;
   void waitForResponse(int seconds);
   std::shared_ptr<telux::tel::ICall> getCallObj();

private:
   bool commndCallbackFound_ = false;
   std::shared_ptr<telux::tel::ICall> callObj_;
};

class MyCallCommandCallback : public telux::common::ICommandResponseCallback {
public:
   MyCallCommandCallback(std::string commandName);
   void commandResponse(telux::common::ErrorCode error) override;

private:
   std::string commandName_;
};

#endif  // MYCALLLISTENER_HPP
