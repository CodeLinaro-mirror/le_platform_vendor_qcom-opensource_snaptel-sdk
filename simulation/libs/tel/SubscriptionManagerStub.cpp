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

#include "SubscriptionManagerStub.hpp"
#include "CardManagerStub.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using tel::CardService;

using namespace telux::common;
#define FIRST_SIM_SLOT_ID 1
#define INIT_DELAY 100

namespace telux {

namespace tel {

SubscriptionManagerStub::SubscriptionManagerStub(telux::common::InitResponseCb callback)
    :stub_(PhoneService::NewStub(grpc::CreateChannel("localhost:8089",
    grpc::InsecureChannelCredentials()))) {
    LOG(DEBUG, __FUNCTION__);
    cardstub_ = CardService::NewStub(grpc::CreateChannel("localhost:8089",
    grpc::InsecureChannelCredentials()));
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

SubscriptionManagerStub::~SubscriptionManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void SubscriptionManagerStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
   std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);

   for(auto &it : subscriptionMap_) {
      auto subscription = it.second;
      subscription->cleanup();
   }

   if(!subscriptionMap_.empty()) {
      subscriptionMap_.clear();
   }
}

void SubscriptionManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::tel::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;
    stub_->InitService(&context, request, &response);
    int numSlots = 0;

    int cbDelay = static_cast<int>(response.delay());
    telux::common::ServiceStatus servicestatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
    if(servicestatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::promise<telux::common::ServiceStatus> cardMgrprom;
        auto cardMgr = PhoneFactory::getInstance().
            getCardManager([&](telux::common::ServiceStatus status) {
            cardMgrprom.set_value(status);
        });
        if (!cardMgr) {
            LOG(ERROR, __FUNCTION__, " Failed to get CardManager");
            servicestatus = telux::common::ServiceStatus::SERVICE_FAILED;
        } else {
            telux::common::ServiceStatus cardMgrStatus = cardMgr->getServiceStatus();
            if (cardMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                LOG(DEBUG, __FUNCTION__, "Card Manager subsystem is not ready, Please wait.");
            }
            cardMgrStatus = cardMgrprom.get_future().get();
            if (cardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                LOG(INFO, __FUNCTION__, "Card Manager subsystem is ready");
                auto status = cardMgr->getSlotCount(numSlots);
                if (status != telux::common::Status::SUCCESS) {
                    LOG(ERROR, __FUNCTION__, " Unable to get slot count from the card manager");
                    servicestatus = telux::common::ServiceStatus::SERVICE_FAILED;
                }
                else {
                    LOG(DEBUG, __FUNCTION__, " slot count from the card manager", numSlots);
                }
                cardMgr->registerListener(shared_from_this());
                isSubscriptionChanged = false;
                for(int id = FIRST_SIM_SLOT_ID; id < (FIRST_SIM_SLOT_ID + numSlots); id++) {
                    //check for card state and create the subscription object only
                    //if the card is available
                    telux::common::Status status = createSubscriptionAndNotify(id);
                    if(status != telux::common::Status::SUCCESS) {
                        LOG(ERROR, __FUNCTION__, " unable to update subscription",
                            "map on slot ", id);
                        servicestatus = telux::common::ServiceStatus::SERVICE_FAILED;
                        break;
                    }
                }
            } else {
                LOG(ERROR, __FUNCTION__,
                    " Card Manager subsystem is not ready,",
                    "failed to initialize Subscription Manager");
                servicestatus = telux::common::ServiceStatus::SERVICE_FAILED;
            }
        }
    }
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::",
        static_cast<int>(servicestatus));
    if(callback) {
        auto f = std::async(std::launch::async, [this, cbDelay, servicestatus, callback]() {
            this->invokeInitResponseCallback(cbDelay, servicestatus, callback);
        }).share();
        taskQ_->add(f);
    }
}

telux::common::Status SubscriptionManagerStub::createSubscriptionAndNotify(int slotId) {

   LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
   telux::common::Status status = telux::common::Status::FAILED;
   auto cardMgr = PhoneFactory::getInstance().getCardManager();
   if(cardMgr) {
      auto card = cardMgr->getCard(slotId, &status);
      if(card) {
         int mapSize = -1;
         if(status == telux::common::Status::SUCCESS) {
            telux::tel::CardState cardState;
            status = card->getState(cardState);
            if(status == telux::common::Status::SUCCESS) {
               switch(cardState) {
                  case CardState::CARDSTATE_ABSENT:
                  case CardState::CARDSTATE_ERROR: {
                     LOG(DEBUG, __FUNCTION__, " card is absent or error ");
                     {
                        std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
                        auto it = subscriptionMap_.find(slotId);
                        if(it != subscriptionMap_.end()) {
                           subscriptionMap_.erase(it);
                           LOG(DEBUG, __FUNCTION__, " removed slot id ", slotId,
                              " from map and mapSize is ", mapSize);
                        }
                        mapSize = subscriptionMap_.size();
                     }
                     notifyNumberOfSubscriptions(mapSize);
                     notifySubscriptionListener(nullptr);
                  } break;
                  case CardState::CARDSTATE_PRESENT: {
                     LOG(DEBUG, __FUNCTION__, " card state is present ");
                     std::vector<std::shared_ptr<ICardApp>> apps = card->getApplications();
                     // wait for app state to be READY
                     bool isAppReady = false;
                     for(auto &it : apps) {
                        if(it->getAppType() != AppType::APPTYPE_UNKNOWN
                           && it->getAppType() != AppType::APPTYPE_CSIM) {
                           if(it->getAppState() == AppState::APPSTATE_READY) {
                              LOG(DEBUG, __FUNCTION__, " App State is ready");
                              isAppReady = true;
                           } else {
                              isAppReady = false;
                              LOG(DEBUG, __FUNCTION__, " Apps were not ready, appState: ",
                                  static_cast<int>(it->getAppState()));
                              break;
                           }
                        }
                     }
                     if(isAppReady) {
                        addNewOrUpdateSubscription(slotId);
                           {
                              std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
                              mapSize = subscriptionMap_.size();
                           }
                        notifyNumberOfSubscriptions(mapSize);
                    }
                  } break;
                  case CardState::CARDSTATE_UNKNOWN:
                  default: {
                     LOG(DEBUG, __FUNCTION__, " card state is unknown or invalid: ",
                         static_cast<int>(cardState));
                  }
               }
            } else {
               LOG(DEBUG, __FUNCTION__, " unable to get card state ");
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " Unable to get card instance ");
        }
      } else {
         LOG(ERROR, __FUNCTION__, " Card is NULL ");
      }
   } else {
      LOG(ERROR, __FUNCTION__, " Card Manager is NULL ");
   }
   return status;
}

void SubscriptionManagerStub::onCardInfoChanged(int slotId) {
   LOG(DEBUG, __FUNCTION__, " SlotId: ", slotId);
   createSubscriptionAndNotify(slotId);
}

void SubscriptionManagerStub::notifyNumberOfSubscriptions(int count) {
   LOG(DEBUG, __FUNCTION__);
   int size = listeners_.size();
   LOG(DEBUG, __FUNCTION__, "Size is ", size);
   LOG(DEBUG, __FUNCTION__, "count is ", count);
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt) {
            LOG(DEBUG, __FUNCTION__, "Just check if listener is present");
           spt->onNumberOfSubscriptionsChanged(count);
            ++iter;
        } else {
            LOG(DEBUG, __FUNCTION__, "Erasing");
            listeners_.erase(iter);
        }
    }
    LOG(DEBUG, __FUNCTION__, "Just check if listener end");
}

void SubscriptionManagerStub::notifySubscriptionListener
    (std::shared_ptr<ISubscription> subscription) {
    LOG(DEBUG, __FUNCTION__);
    int size = listeners_.size();
    LOG(DEBUG, __FUNCTION__, "Size is ", size);
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt) {
            spt->onSubscriptionInfoChanged(subscription);
            ++iter;
        } else {
            LOG(DEBUG, __FUNCTION__, "Erasing");
            listeners_.erase(iter);
        }
    }
}

telux::common::Status SubscriptionManagerStub::addNewOrUpdateSubscription(int slotId) {
    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
    std::shared_ptr<SubscriptionStub> iSub = std::make_shared<SubscriptionStub>(slotId);
    subscriptionMap_[slotId] = iSub;
    if(isSubscriptionChanged) {
        notifySubscriptionListener(subscriptionMap_[slotId]);
    }
    isSubscriptionChanged = false;
    return telux::common::Status::SUCCESS;
}

void SubscriptionManagerStub::invokeInitResponseCallback(int cbDelay,
    telux::common::ServiceStatus cbStatus, telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(cbStatus);
    }
}

std::future<bool> SubscriptionManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
        [this]() {
            while (!isSubsystemReady()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(INIT_DELAY));
            }
            return(isSubsystemReady());});
    return((ready_future));
}

telux::common::ServiceStatus SubscriptionManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::tel::GetServiceStatusReply response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    grpc::Status status = stub_->GetServiceStatus(&context, request, &response);
    telux::common::ServiceStatus serviceStatus =
    static_cast<telux::common::ServiceStatus>(response.service_status());

    return serviceStatus;
}

telux::common::Status SubscriptionManagerStub::registerListener(
    std::weak_ptr<ISubscriptionListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SubscriptionManager is not ready");
        return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
    telux::common::Status status = telux::common::Status::SUCCESS;
    auto spt = listener.lock();
    if (spt != nullptr) {
        if (listeners_.size() == 0) {
            try {
            } catch(exception const & ex) {
                LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
                status = telux::common::Status::NOMEMORY;
                return status;
            }
            auto &eventManager = telux::common::EventManager::getInstance();
            eventManager.connectToSimulationServer();
            eventManager.registerListener(shared_from_this(), TEL_SUBSCRIPTION_FILTER);
        }
        bool existing = 0;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, "listener already exists");
                return telux::common::Status::ALREADY;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
        }
    } else {
        LOG(ERROR, "Null listener");
        return telux::common::Status::INVALIDPARAM;
    }
    return status;
}

telux::common::Status SubscriptionManagerStub::removeListener(
    std::weak_ptr<ISubscriptionListener> listener) {
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SubscriptionManager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " Erasing listener");
                retVal=telux::common::Status::SUCCESS;
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

bool SubscriptionManagerStub::isSubsystemReady() {
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
std::shared_ptr<ISubscription> SubscriptionManagerStub::getSubscription(int slotId,
    telux::common::Status *status) {
    LOG(DEBUG, __FUNCTION__, " slotId: ", slotId);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Subscription Manager not ready ");
        if(status) {
            *status = telux::common::Status::NOTREADY;
        }
        return nullptr;
    }
    telux::common::Status subStatus = telux::common::Status::FAILED;
    std::shared_ptr<ISubscription> subscription = nullptr;
    {
        std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
        auto it = subscriptionMap_.find(slotId);
        if(it != subscriptionMap_.end()) {
            subscription = it->second;
            subStatus = telux::common::Status::SUCCESS;
        }
    }
    if(status) {
        *status = subStatus;
    }
    return subscription;
}

std::vector<std::shared_ptr<ISubscription>>
    SubscriptionManagerStub::getAllSubscriptions(telux::common::Status *status) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<ISubscription>> subs;
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Subscription Manager not ready ");
        if(status) {
            *status = telux::common::Status::NOTREADY;
        }
        return subs;
    }
    {
        std::lock_guard<std::mutex> lock(subscriptionManagerMutex_);
        for(auto &it : subscriptionMap_) {
            subs.emplace_back(it.second);
        }
    }
    if(status) {
        *status = telux::common::Status::SUCCESS;
    }
    return subs;
}

void SubscriptionManagerStub::onEventUpdate(std::string event) {
    std::string token;
    if (EVENT_FLAG == EventParserUtil::getNextToken(event, DEFAULT_DELIMITER)) {
        token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
        handleEvent(token, event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
    return;
}

void SubscriptionManagerStub::handleEvent(std::string token , std::string event) {
    LOG(DEBUG, __FUNCTION__, "The received event is: \"",token,"\"");
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token,"The leftover string is: ", event);
    if (token == "subscriptionInfoChanged") {
        handlesubscriptionInfoChanged(event);
    } else {
        LOG(DEBUG, __FUNCTION__, "No handling required for other events");
    }
    return;
}

void SubscriptionManagerStub::handlesubscriptionInfoChanged(std::string eventParams) {
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
    LOG(DEBUG, __FUNCTION__, "The fetched slot id is: ", slotId
        ,"The leftover string is: ", eventParams);
    isSubscriptionChanged = true;
    createSubscriptionAndNotify(slotId);
}

} // end of namespace tel

} // end of namespace telux

