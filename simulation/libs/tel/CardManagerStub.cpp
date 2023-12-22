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

#include "CardManagerStub.hpp"
#include <telux/common/DeviceConfig.hpp>
#include "common/event-manager/ClientEventManager.hpp"

#define DELAY 100

using namespace telux::common;

namespace telux {

namespace tel {

CardManagerStub::CardManagerStub(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<CardService>();
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

CardManagerStub::~CardManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void CardManagerStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
   for(const auto card : cardMap_) {
      if(card.second != nullptr) {
         card.second->cleanup();
      }
   }
   cardMap_.clear();
}

void CardManagerStub::initSync(telux::common::InitResponseCb callback) {
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    LOG(DEBUG, __FUNCTION__);
    grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    if (reqstatus.ok()) {
        telux::common::ServiceStatus cbStatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
        int cbDelay = static_cast<int>(response.delay());
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
        if(cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            slotCount_ = 1;
            if(telux::common::DeviceConfig::isMultiSimSupported()) {
                slotCount_ = 2;
            }
            for(int id = 1; id <= slotCount_; ++id) {
                simSlotIds_.emplace_back(id);
            }
            for(int &slotId : simSlotIds_) {
                auto card = std::make_shared<CardStub>(slotId);
            if (card) {
                cardMap_.emplace(slotId, card);
            } else {
                LOG(ERROR, __FUNCTION__, " Card is NULL for slotId: ", slotId);
            }
            }
            for (auto slotId:simSlotIds_) {
                LOG(DEBUG, __FUNCTION__,"SlotId is ",slotId);
                cardMap_[slotId]->updateSimStatus();
            }
            listenerMgr_ = std::make_shared<telux::common::ListenerManager<ICardListener>>();
            if(!listenerMgr_) {
                LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
                cbStatus = telux::common::ServiceStatus::SERVICE_FAILED;
            }
        }
        if(callback) {
            auto f = std::async(std::launch::async, [this, cbDelay, cbStatus, callback]() {
                this->invokeInitResponseCallback(cbDelay, cbStatus, callback);
            }).share();
            taskQ_->add(f);
        }
    } else {
        if(callback) {
            auto f = std::async(std::launch::async, [this, callback]() {
                this->invokeInitResponseCallback(DELAY,
                    telux::common::ServiceStatus::SERVICE_FAILED, callback);
            }).share();
            taskQ_->add(f);
        }
    }
}

void CardManagerStub::invokeInitResponseCallback(int cbDelay, telux::common::ServiceStatus cbStatus,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(cbStatus);
    }
}

std::future<bool> CardManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
    [this]() {
        while (!isSubsystemReady()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
        }
    return(isSubsystemReady());});
    return((ready_future));
}

telux::common::ServiceStatus CardManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    grpc::Status status = stub_->GetServiceStatus(&context, request, &response);
    telux::common::ServiceStatus serviceStatus =
    static_cast<telux::common::ServiceStatus>(response.service_status());

    return serviceStatus;
}

telux::common::Status CardManagerStub::getSlotIds(std::vector<int> &slotIds) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> lock(cardManagerMutex_);
    slotIds = simSlotIds_;

    return telux::common::Status::SUCCESS;
}

telux::common::Status CardManagerStub::getSlotCount(int &count) {
   LOG(DEBUG, __FUNCTION__);
   if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
       return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> lock(cardManagerMutex_);
    count = slotCount_;

    return telux::common::Status::SUCCESS;
}

std::shared_ptr<ICard> CardManagerStub::getCard(int slotId, telux::common::Status *status) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        if(status) {
            *status = telux::common::Status::NOTREADY;
        }
        return nullptr;
    }
    {
        std::lock_guard<std::mutex> lock(cardManagerMutex_);
        auto card = cardMap_.find(slotId);
        if(card != cardMap_.end()) {
            auto cardImpl = cardMap_[slotId];
            if(cardImpl) {
                if (status) {
                    *status = telux::common::Status::SUCCESS;
                }
                return cardImpl;
            } else {
                LOG(DEBUG, " cardImpl is empty");
            }
        }
    }
    LOG(INFO, "Unable to get the card instance for given slotId: ", slotId);
    if(status) {
        *status = telux::common::Status::NOTREADY;
    }
    return nullptr;
}

telux::common::Status CardManagerStub::cardPowerUp(SlotId slotId,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::CardPowerRequest request;
    ::telStub::CardPowerResponse response;
    ClientContext context;
    request.set_phone_id(slotId);
    request.set_powerup(true);

    grpc::Status reqstatus = stub_->CardPower(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback, delay]() {
                this->invokeCallback(callback, error, delay);
            }).share();
        taskQ_->add(f1);

        if (error != telux::common::ErrorCode::NO_EFFECT) {
            int slotid = static_cast<int>(slotId);
            auto f2 = std::async(std::launch::async,
                [this, slotid]() {
                    this->invokelisteners(slotid);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}

void CardManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

void CardManagerStub::invokelisteners(int slotId) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::weak_ptr<ICardListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for(auto &wp : applisteners) {
            if(auto sp = wp.lock()) {
                sp->onCardInfoChanged(slotId);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

telux::common::Status CardManagerStub::cardPowerDown(SlotId slotId,
    telux::common::ResponseCallback callback) {
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::CardPowerRequest request;
    ::telStub::CardPowerResponse response;
    ClientContext context;
    request.set_phone_id(slotId);
    request.set_powerup(false);

    grpc::Status reqstatus = stub_->CardPower(&context, request, &response);

    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback, delay]() {
                this->invokeCallback(callback, error, delay);
            }).share();
        taskQ_->add(f1);

        if (error != telux::common::ErrorCode::NO_EFFECT) {
            int slotid = static_cast<int>(slotId);
            auto f2 = std::async(std::launch::async,
                [this, slotid]() {
                    this->invokelisteners(slotid);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}

telux::common::Status CardManagerStub::registerListener(std::shared_ptr<ICardListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_CARD_FILTER};
        auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(shared_from_this(), filters);
    }
    return status;
}

telux::common::Status  CardManagerStub::removeListener(std::shared_ptr<ICardListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ICardListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_CARD_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

bool CardManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    grpc::Status status = stub_->GetServiceStatus(&context, request, &response);
    telux::common::ServiceStatus serviceStatus =
    static_cast<telux::common::ServiceStatus>(response.service_status());
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        return true;
    } else {
        return false;
    }
}

void CardManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::telStub::cardInfoChange>()) {
        ::telStub::cardInfoChange cardEvent;
        event.UnpackTo(&cardEvent);
        handleCardInfoChanged(cardEvent);
    }
}

void CardManagerStub::handleCardInfoChanged(::telStub::cardInfoChange event) {
    int slotId = event.phone_id();
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", slotId);
    invokelisteners(slotId);
}

} // end of namespace tel

} // end of namespace telux

