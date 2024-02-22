/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       CallManagerStub.hpp
 *
 * @brief      Implementation of CallManager
 *
 */

#ifndef CALL_MANAGER_STUB_HPP
#define CALL_MANAGER_STUB_HPP


#include <telux/tel/CallManager.hpp>
#include <telux/tel/CallListener.hpp>
#include "CallStub.hpp"
#include "../common/event-manager/ClientEventManager.hpp"
#include "../common/ListenerManager.hpp"
#include "TelDefinesStub.hpp"
#include "Helper.hpp"

namespace telux {
namespace tel {

class CallManagerStub : public ICallManager,
                        public IEventListener,
                        public std::enable_shared_from_this<CallManagerStub> {
public:

    CallManagerStub(telux::common::InitResponseCb clientCallback);

    void initSync(telux::common::InitResponseCb callback);

    telux::common::ServiceStatus getServiceStatus() override;

    telux::common::Status registerListener
        (std::shared_ptr<telux::tel::ICallListener> listener) override;

    telux::common::Status removeListener
        (std::shared_ptr<telux::tel::ICallListener> listener) override;

    telux::common::Status makeCall(int phoneId, const std::string &dialNumber,
                                        std::shared_ptr<IMakeCallCallback> callback) override;
    telux::common::Status makeECall(int phoneId, const ECallMsdData &eCallMsdData,
                                           int category, int variant,
                                           std::shared_ptr<IMakeCallCallback> callback) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber,
        const std::vector<uint8_t> &msdPdu, CustomSipHeader header,
        MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, const std::vector<uint8_t> &msdPdu,
                                            int category, int variant,
                                            MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber,
                                           const std::vector<uint8_t> &msdPdu, int category,
                                           MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, int category, int variant,
                                            MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber, int category,
                                           MakeCallCallback callback) override;
    telux::common::Status makeECall(int phoneId, const std::string dialNumber,
                                           const ECallMsdData &eCallMsdData, int category,
                                           std::shared_ptr<IMakeCallCallback> callback) override;
    telux::common::Status updateECallMsd(int phoneId, const ECallMsdData &eCallMsd,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback) override;
    telux::common::Status updateECallMsd(int phoneId, const std::vector<uint8_t> &msdPdu,
        telux::common::ResponseCallback callback) override;
    telux::common::Status requestECallHlapTimerStatus(int phoneId,
        ECallHlapTimerStatusCallback callback) override;
    std::vector<std::shared_ptr<ICall>> getInProgressCalls() override;
    telux::common::Status conference(std::shared_ptr<ICall> call1, std::shared_ptr<ICall> call2,
                    std::shared_ptr<telux::common::ICommandResponseCallback> callback) override;
    telux::common::Status swap(std::shared_ptr<ICall> callToHold,
        std::shared_ptr<ICall> callToActivate,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback) override;
    telux::common::Status hangupForegroundResumeBackground(int phoneId,
        common::ResponseCallback callback ) override;
    telux::common::Status hangupWaitingOrBackground(int phoneId,
        common::ResponseCallback callback) override;
    telux::common::Status requestEcbm(int phoneId, EcbmStatusCallback callback) override;
    telux::common::Status exitEcbm(int phoneId, common::ResponseCallback callback) override;
    telux::common::Status requestNetworkDeregistration(int phoneId,
        common::ResponseCallback callback) override;
    telux::common::Status updateEcallHlapTimer(int phoneId, HlapTimerType type,
        uint32_t timeDuration, common::ResponseCallback callback = nullptr) override;
    telux::common::Status requestEcallHlapTimer(int phoneId, HlapTimerType type,
        ECallHlapTimerCallback callback) override;
    telux::common::Status setECallConfig(EcallConfig config) override;
    telux::common::Status getECallConfig(EcallConfig &config) override;
    ~CallManagerStub();
    void cleanup();
    void onEventUpdate(google::protobuf::Any event)  override;

private:
    int noOfSlots_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::mutex callManagerMutex_;
    std::shared_ptr<telux::common::ListenerManager<ICallListener>> listenerMgr_;
    void handleEcallEvent(::telStub::ECallInfoEvent event);
    void handleCallInfoChanged(::telStub::CallStateChangeEvent event);
    void handleMsdUpdateRequest(::telStub::MsdPullRequestEvent event);
    void handleIncomingCall(::telStub::Call event);
    void handleHangup(::telStub::HangupCallEvent event);
    void invokeECallHlapTimerEventlisteners(int phoneId,
        ECallHlapTimerEvents timersStatus);
    void invokeECallMsdTransmissionStatuslisteners(int phoneId,
        telux::tel::ECallMsdTransmissionStatus msdTransmissionStatus );
    void invokeECallMsdTransmissionStatuslisteners(int phoneId,
        telux::common::ErrorCode errorCode );
    void logCallDetails(std::shared_ptr<ICall> info);
    std::unique_ptr<::telStub::DialerService::Stub> stub_;
    std::vector<std::shared_ptr<CallStub>> calls_;
    void onEventUpdate(std::string event);
};

} // end of namespace tel

} // end of namespace telux

#endif // CALL_MANAGER_STUB_HPP