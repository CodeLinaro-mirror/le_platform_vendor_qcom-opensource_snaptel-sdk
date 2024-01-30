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
 * @file       SubscriptionManagerStub.hpp
 *
 * @brief      Implementation of ISubscriptionManager
 *
 */

#ifndef SUBSCRIPTION_MANAGER_STUB_HPP
#define SUBSCRIPTION_MANAGER_STUB_HPP

#include "SubscriptionStub.hpp"
#include <telux/tel/CardManager.hpp>
#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include <telux/tel/SubscriptionManager.hpp>
#include "common/ListenerManager.hpp"
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "CardAppStub.hpp"

using telStub::PhoneService;

namespace telux {
namespace tel {

class SubscriptionManagerStub : public ISubscriptionManager,
                                public IEventListener,
                                public ICardListener,
                                public ISubscriptionListener,
                                public std::enable_shared_from_this<SubscriptionManagerStub>  {
public:
    SubscriptionManagerStub(telux::common::InitResponseCb clientCallback);
    ~SubscriptionManagerStub();
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<ISubscriptionListener> listener) override;
    telux::common::Status removeListener(std::weak_ptr<ISubscriptionListener> listener) override;
    std::shared_ptr<ISubscription> getSubscription(int slotId = DEFAULT_SLOT_ID,
        telux::common::Status *status = nullptr) override;
    std::vector<std::shared_ptr<ISubscription>>
        getAllSubscriptions(telux::common::Status *status = nullptr) override;
    void onEventUpdate(google::protobuf::Any event);
    void cleanup();
private:
    SlotId slotId_;
    void initSync(telux::common::InitResponseCb callback);
    std::mutex subscriptionManagerMutex_;
    std::unique_ptr<::telStub::PhoneService::Stub> stub_;
    std::unique_ptr<::telStub::CardService::Stub> cardstub_;
    std::map<int, std::shared_ptr<SubscriptionStub>> subscriptionMap_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::shared_ptr<telux::common::ListenerManager<ISubscriptionListener>> listenerMgr_;
    void invokeInitResponseCallback(int cbDelay, telux::common::ServiceStatus cbStatus,
    telux::common::InitResponseCb callback);
    void notifyNumberOfSubscriptions(int count);
    void notifySubscriptionListener(std::shared_ptr<ISubscription> subscription);
    telux::common::Status createSubscriptionAndNotify(int slotId);
    telux::common::Status addNewOrUpdateSubscription(int slotId);
    void handleEvent(std::string token, std::string event);
    void handleSubscriptionInfoChanged(::telStub::SubscriptionEvent event);
    void handleCardInfoChanged(::telStub::cardInfoChange event);
    void onCardInfoChanged(int slotId);
    telux::common::Status getState(CardState &cardState, int phoneId);
    telux::common::Status getAppInfo(std::vector<CardAppStatus> &apps, int phoneId);
    telux::common::Status fetchSubscription(int slotId, std::string *carrierName,
        std::string *iccId, int* mcc, int* mnc, std::string *number, std::string *imsi,
        std::string *gid1, std::string *gid2 );
    void onEventUpdate(std::string event);
};

} // end of namespace tel

} // end of namespace telux

#endif // SUBSCRIPTION_MANAGER_STUB_HPP