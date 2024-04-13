/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       PhoneManagerStub.hpp
 *
 * @brief      Implementation of PhoneManager on client side
 *
 */

#ifndef TELUX_TEL_PHONEMANAGERSTUB_HPP
#define TELUX_TEL_PHONEMANAGERSTUB_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneManager.hpp>
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/ListenerManager.hpp"
#include "PhoneStub.hpp"
#include "TelDefinesStub.hpp"

using telStub::PhoneService;
using telStub::CardService;

namespace telux {
namespace tel {

class PhoneManagerStub : public IPhoneManager,
                         public IEventListener,
                         public std::enable_shared_from_this<PhoneManagerStub> {
public:

    PhoneManagerStub(telux::common::InitResponseCb clientCallback);
    ~PhoneManagerStub();
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status getPhoneIds(std::vector<int> &phoneIds);
    int getPhoneIdFromSlotId(int slotId);
    int getSlotIdFromPhoneId(int phoneId);
    std::shared_ptr<IPhone> getPhone(int phoneId);
    telux::common::Status registerListener(std::weak_ptr<IPhoneListener> listener);
    telux::common::Status removeListener(std::weak_ptr<IPhoneListener> listener);
    telux::common::Status requestCellularCapabilityInfo(
        std::shared_ptr<ICellularCapabilityCallback> callback = nullptr) override;
    telux::common::Status setOperatingMode(telux::tel::OperatingMode operatingMode,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestOperatingMode(std::shared_ptr<IOperatingModeCallback> callback
        = nullptr) override;
    telux::common::Status resetWwan(telux::common::ResponseCallback callback
        = nullptr) override;
    void onEventUpdate(google::protobuf::Any event)  override;

private:
    int noOfSlots_;
    bool ready_ = false;
    std::condition_variable cv_;
    std::vector<int> phoneIds_;
    std::map<int, std::shared_ptr<PhoneStub>> phoneMap_;
    std::map<int, int> phoneSlotIdsMap_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::mutex phoneManagerMutex_;
    std::shared_ptr<telux::common::ListenerManager<IPhoneListener>> listenerMgr_;
    std::unique_ptr<::telStub::PhoneService::Stub> phoneStub_;
    std::unique_ptr<::telStub::CardService::Stub> cardStub_;
    void initSync(telux::common::InitResponseCb callback);
    void setSubsystemReady(bool status);
    bool waitForInitialization();
    void handleSignalStrengthChanged(::telStub::SignalStrengthChangeEvent event);
    void handleCellInfoListChanged(::telStub::CellInfoListEvent event);
    void handleVoiceServiceStateChanged(::telStub::VoiceServiceStateEvent event);
    void handleOperatingModeChanged(::telStub::OperatingModeEvent event);
    void handleECallOperatingModeChanged(::telStub::ECallModeInfoChangeEvent event);
    void handleOperatorInfoChanged(::telStub::OperatorInfoEvent event);
    void onEventUpdate(std::string event);
    void updateRadioState(OperatingMode optMode);
};

} // end of namespace tel
} // end of namespace telux

#endif // TELUX_TEL_PHONEMANAGERSTUB_HPP
