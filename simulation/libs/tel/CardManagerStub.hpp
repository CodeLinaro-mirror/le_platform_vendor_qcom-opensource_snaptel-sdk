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
 * @file       CardManagerStub.hpp
 *
 * @brief      Implementation of ICardManager
 *
 */

#ifndef CARD_MANAGER_STUB_HPP
#define CARD_MANAGER_STUB_HPP

#include "CardStub.hpp"
#include "../common/Logger.hpp"
#include "../common/ListenerManager.hpp"
#include <telux/common/CommonDefines.hpp>
#include "../common/AsyncTaskQueue.hpp"
#include "../common/event-manager/EventManager.hpp"
#include "../common/ResponseHandler.hpp"
#include "TelDefinesStub.hpp"
#include <telux/tel/CardManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include "CardAppStub.hpp"
#include "../common/event-manager/EventParserUtil.hpp"
#include <grpcpp/grpcpp.h>
#include "../../protos/proto-src/tel.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::CardService;

#define INVALID_SLOT_COUNT -1
namespace telux {
namespace tel {


class CardManagerStub : public ICardManager,
                        public IEventListener,
                        public std::enable_shared_from_this<CardManagerStub> {
public:
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status getSlotCount(int &count) override;
    telux::common::Status getSlotIds(std::vector<int> &slotIds) override;
    std::shared_ptr<ICard> getCard(int slotId = DEFAULT_SLOT_ID,
        telux::common::Status *status = nullptr) override;
    telux::common::Status cardPowerUp(SlotId slotId,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status cardPowerDown(SlotId slotId,
        telux::common::ResponseCallback callback = nullptr) override;
    telux::common::Status registerListener(std::shared_ptr<ICardListener> listener) override;
    telux::common::Status removeListener(std::shared_ptr<ICardListener> listener) override;
    void onEventUpdate(std::string event);
    CardManagerStub(telux::common::InitResponseCb clientCallback);
    void cleanup();
    ~CardManagerStub();

private:
    int slotCount_ = INVALID_SLOT_COUNT;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    telux::common::InitResponseCb initCb_;
    std::mutex mutex_;
    std::shared_ptr<telux::common::ListenerManager<ICardListener>> listenerMgr_;
    std::shared_ptr<telux::common::ResponseHandler> cannedResponseManager_;
    std::unique_ptr<::telStub::CardService::Stub> stub_;
    void initSync(telux::common::InitResponseCb callback);
    std::mutex cardManagerMutex_;
    std::vector<int> simSlotIds_;
    std::map<int, std::shared_ptr<CardStub>> cardMap_;
    void invokeInitResponseCallback(int cbDelay, telux::common::ServiceStatus cbStatus,
    telux::common::InitResponseCb callback);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error, int cbDelay );
    void handleEvent(std::string token, std::string event);
    void invokelisteners (int slotId);
    void handleCardInfoChanged(std::string eventParams);
};

} // end of namespace tel

} // end of namespace telux



#endif // CARD_MANAGER_STUB_HPP