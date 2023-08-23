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
#define DELAY 100

using namespace telux::common;

namespace telux {

namespace tel {

CardManagerStub::CardManagerStub(telux::common::InitResponseCb callback)
    :stub_(CardService::NewStub(grpc::CreateChannel("localhost:8089",
    grpc::InsecureChannelCredentials()))) {
    LOG(DEBUG, __FUNCTION__);
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
    ::tel::GetServiceStatusReply response;
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
    ::tel::GetServiceStatusReply response;
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
    ::tel::CardPowerRequest request;
    ::tel::CardPowerResponse response;
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
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt) {
            LOG(DEBUG, __FUNCTION__, "The fetched slot id is: ",slotId);
            spt->onCardInfoChanged(slotId);
            ++iter;
        } else {
            LOG(DEBUG, __FUNCTION__, "No valid listener found: ");
            iter = listeners_.erase(iter);
        }
    }
}

telux::common::Status CardManagerStub::cardPowerDown(SlotId slotId,
    telux::common::ResponseCallback callback) {
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::tel::CardPowerRequest request;
    ::tel::CardPowerResponse response;
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
    std::lock_guard<std::mutex> lock(cardManagerMutex_);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listener != nullptr) {
        if (listeners_.size() == 0) {
            try {
            } catch(exception const & ex) {
                LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
                return telux::common::Status::NOMEMORY;
            }
            auto &eventManager = telux::common::EventManager::getInstance();
            eventManager.connectToSimulationServer();
            eventManager.registerListener(shared_from_this(), TEL_CARD_FILTER);
        }
        bool existing = 0;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (listener == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, "listener already exists");
                return telux::common::Status::ALREADY;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " creates a new listener entry");
            return telux::common::Status::SUCCESS;
        }
        for (auto slotId:simSlotIds_) {
            LOG(DEBUG, __FUNCTION__,"SlotId is ",slotId);
            cardMap_[slotId]->setlisteners(listeners_);
        }
    } else {
        LOG(ERROR, "Null listener");
        return telux::common::Status::INVALIDPARAM;

    }
    return status;
}

telux::common::Status  CardManagerStub::removeListener(std::shared_ptr<ICardListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Card Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> lock(cardManagerMutex_);;
    if (listener != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (listener == (*iter).lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " In deRegister Listener : Removing");
                retVal = telux::common::Status::SUCCESS;
                break;
            }
        }
        if (listeners_.size() == 0) {
            auto &eventManager = telux::common::EventManager::getInstance();
            eventManager.deregisterListener(shared_from_this());
        }
    } else {
        LOG(WARNING, "listener is null");
        retVal = telux::common::Status::NOSUCH;
    }
    return (retVal);
}

bool CardManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    ::tel::GetServiceStatusReply response;
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

void CardManagerStub::onEventUpdate(std::string event) {
    std::string token;
    if (EVENT_FLAG == EventParserUtil::getNextToken(event, DEFAULT_DELIMITER)) {
        token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
        handleEvent(token, event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void CardManagerStub::handleEvent(std::string token , std::string event) {
    LOG(DEBUG, __FUNCTION__, "The received event is: \"",token,"\"");
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token, "The leftover string is: ", event);
    if (token == "cardInfoChanged") {
        handleCardInfoChanged(event);
    }
}

void CardManagerStub::handleCardInfoChanged(std::string eventParams) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    int slotId;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        slotId = 1;
    } else {
        try {
            slotId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    invokelisteners(slotId);
}

} // end of namespace tel

} // end of namespace telux

