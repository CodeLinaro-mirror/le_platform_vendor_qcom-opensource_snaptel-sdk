/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       PhoneStub.hpp
 *
 * @brief      Implementation of Phone
 *
 */

#ifndef PHONE_STUB_HPP
#define PHONE_STUB_HPP

#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/EventManager.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Phone.hpp>

#define INVALID -1
using telStub::PhoneService;

namespace telux {
namespace tel {

class PhoneStub : public IPhone,
                  public IVoiceServiceStateCallback,
                  public std::enable_shared_from_this<PhoneStub> {
 public:
    PhoneStub(int phoneId);
    ~PhoneStub();
    void init();
    telux::common::Status getPhoneId(int &phId);
    RadioState getRadioState();
    void setRadioState(RadioState radioState);
    ServiceState getServiceState();
    void setServiceState(ServiceState serviceState);
    void updateRadioState(RadioState radioState);
    telux::common::Status requestVoiceRadioTechnology(VoiceRadioTechResponseCb callback) override;
    telux::common::Status requestVoiceServiceState(
        std::weak_ptr<IVoiceServiceStateCallback> callback);
    telux::common::Status setRadioPower(
        bool enable, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status requestCellInfo(telux::tel::CellInfoCallback callback);
    telux::common::Status setCellInfoListRate(
        uint32_t timeInterval, telux::common::ResponseCallback callback);
    virtual telux::common::Status requestECallOperatingMode(ECallGetOperatingModeCallback callback);
    bool isReady();
    bool isSubsystemReady();
    std::future<bool> onReady();
    telux::common::Status requestOperatorName(OperatorNameCallback callback);
    telux::common::Status configureSignalStrength(
        std::vector<SignalStrengthConfig> signalStrengthConfig,
        telux::common::ResponseCallback callback);
    telux::common::Status requestSignalStrength(
        std::shared_ptr<telux::tel::ISignalStrengthCallback> callback = nullptr);
    virtual telux::common::Status setECallOperatingMode(
        ECallMode eCallMode, telux::common::ResponseCallback callback);
    telux::common::Status requestOperatorInfo(OperatorInfoCallback callback);
    telux::common::Status configureSignalStrength(
        std::vector<SignalStrengthConfigEx> signalStrengthConfigEx, uint16_t hysteresisMs = 0,
        telux::common::ResponseCallback callback = nullptr);

 private:
    int phoneId_ = INVALID;
    std::atomic<bool> ready_;
    std::atomic<RadioState> radioState_;
    std::atomic<bool> radioStateInitialized_;
    std::atomic<ServiceState> serviceState_;
    std::atomic<bool> serviceStateInitialized_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::unique_ptr<::telStub::PhoneService::Stub> stub_;
    std::mutex phoneMutex_;
    void updateReady();
    void handleDeprecatedVoiceServiceStateResponse(
        const std::shared_ptr<VoiceServiceInfo> &serviceInfo);
    void initSync(telux::common::InitResponseCb callback);
};

}  // end of namespace tel
}  // end of namespace telux

#endif  // PHONE_STUB_HPP
