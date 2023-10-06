/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * @file       PhoneStub.hpp
 *
 * @brief      Implementation of PhoneStub
 *
 */

#ifndef PHONE_STUB_HPP
#define PHONE_STUB_HPP

#include "../common/Logger.hpp"
#include "../common/AsyncTaskQueue.hpp"
#include <telux/tel/Phone.hpp>
#include "../common/event-manager/EventManager.hpp"
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

class PhoneStub : public IPhone,
                  public std::enable_shared_from_this<PhoneStub> {
public:
   PhoneStub(int phoneId);
   telux::common::Status getPhoneId(int &phId);
   RadioState getRadioState();
   void setRadioState(RadioState radioState);
   ServiceState getServiceState();
   telux::common::Status requestVoiceRadioTechnology(VoiceRadioTechResponseCb callback) override;
   telux::common::Status
      requestVoiceServiceState(std::weak_ptr<IVoiceServiceStateCallback> callback);
   telux::common::Status setRadioPower(
      bool enable, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
   telux::common::Status requestCellInfo(telux::tel::CellInfoCallback callback);
   telux::common::Status setCellInfoListRate(uint32_t timeInterval,
        common::ResponseCallback callback);
   telux::common::Status requestSignalStrength(
      std::shared_ptr<telux::tel::ISignalStrengthCallback> callback = nullptr);
   virtual telux::common::Status setECallOperatingMode(ECallMode eCallMode,
        telux::common::ResponseCallback callback);
   virtual telux::common::Status requestECallOperatingMode(ECallGetOperatingModeCallback callback);
   bool isReady();
   bool isSubsystemReady();
   std::future<bool> onReady();
   telux::common::Status requestOperatorName(OperatorNameCallback callback);
   telux::common::Status configureSignalStrength(
      std::vector<SignalStrengthConfig> signalStrengthConfig, telux::common::ResponseCallback
      callback);

};

} // end of namespace tel

} // end of namespace telux

#endif // PHONE_STUB_HPP