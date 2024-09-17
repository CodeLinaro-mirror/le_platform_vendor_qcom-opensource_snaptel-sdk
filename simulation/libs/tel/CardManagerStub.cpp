/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CardManagerStub.hpp"
#include <telux/common/DeviceConfig.hpp>
#include "common/event-manager/ClientEventManager.hpp"

#define DELAY 100

using namespace telux::common;

namespace telux {

namespace tel {

CardManagerStub::CardManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_ = DEFAULT_DELAY;
}

telux::common::Status CardManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<ICardListener>>();
    if(!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<CardService>();
    if(!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate card service");
        return telux::common::Status::FAILED;
    }
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if(!taskQ_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate AsyncTaskQueue");
        return telux::common::Status::FAILED;
    }
    initCb_ = callback;
    auto f = std::async(std::launch::async,
        [this]() {
            this->initSync();
        }).share();
    auto status = taskQ_->add(f);
    return status;
}

CardManagerStub::~CardManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void CardManagerStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
   for(const auto &card : cardMap_) {
      if(card.second != nullptr) {
         card.second->cleanup();
      }
   }
   cardMap_.clear();
}

void CardManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(cardManagerMutex_);
        subSystemStatus_ = status;
    }
    if(initCb_) {
        auto f1 = std::async(std::launch::async,
        [this, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay_));
                initCb_(status);
        }).share();
        taskQ_->add(f1);
    } else {
        LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
}

void CardManagerStub::initSync() {
    ::commonStub::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    LOG(DEBUG, __FUNCTION__);
    grpc::Status reqstatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    if (reqstatus.ok()) {
        cbStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay_ = static_cast<int>(response.delay());
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
        }
    }
    LOG(DEBUG, __FUNCTION__, " Delay ", cbDelay_, " service status ", static_cast<int>(cbStatus));
    setServiceStatus(cbStatus);
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
    return subSystemStatus_;
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

