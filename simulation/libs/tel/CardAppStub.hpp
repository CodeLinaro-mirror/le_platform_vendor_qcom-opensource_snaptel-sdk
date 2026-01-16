/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       CardAppStub.hpp
 *
 * @brief      Implementation of ICardApp
 *
 */

#ifndef CARDAPP_STUB_HPP
#define CARDAPP_STUB_HPP

#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include <telux/tel/CardDefines.hpp>
#include <telux/tel/CardApp.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/CardManager.hpp>
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::CardService;

namespace telux {
namespace tel {

struct CardAppStatus {
    AppType appType;
    AppState appState;
    std::string aid;
};

class CardAppStub : public ICardApp {
 public:
    AppType getAppType() override;

    AppState getAppState() override;

    std::string getAppId() override;

    telux::common::Status changeCardPassword(CardLockType lockType, std::string oldPwd,
        std::string newPwd, PinOperationResponseCb callback) override;

    telux::common::Status unlockCardByPuk(CardLockType lockType, std::string puk,
        std::string newPin, PinOperationResponseCb callback) override;

    telux::common::Status unlockCardByPin(
        CardLockType lockType, std::string pin, PinOperationResponseCb callback) override;
    telux::common::Status queryPin1LockState(QueryPin1LockResponseCb callback) override;

    telux::common::Status queryFdnLockState(QueryFdnLockResponseCb callback) override;

    telux::common::Status setCardLock(CardLockType lockType, std::string password, bool isEnabled,
        PinOperationResponseCb callback) override;
    CardAppStub(int slotId, CardAppStatus cardAppStatus);
    void setlisteners(std::vector<std::weak_ptr<ICardListener>> listeners);
    std::vector<std::weak_ptr<ICardListener>> listeners_;
    bool match(CardAppStatus &cardAppStatus);
    telux::common::Status updateCardApp(CardAppStatus &cardAppStatus);

 private:
    int slotId_;
    CardAppStatus cardAppStatus_;
    std::unique_ptr<::telStub::CardService::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    void invokelisteners(int slotId);
    void invokeCallback(
        PinOperationResponseCb callback, telux::common::ErrorCode error, int retryCount, int delay);
    void invokeCallback(
        QueryPin1LockResponseCb callback, telux::common::ErrorCode error, int delay, bool state);
    void invokeCallback(bool isavailable, bool isenabled, QueryFdnLockResponseCb callback,
        telux::common::ErrorCode error, int delay);
    void invokeCallback(
        PinOperationResponseCb callback, int retrycount, telux::common::ErrorCode error, int delay);
};

}  // end of namespace tel

}  // end of namespace telux

#endif  // CARDAPP_STUB_HPP