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

#include "PhoneFactoryImplStub.hpp"

namespace telux {
namespace tel {

PhoneFactoryImplStub::PhoneFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
    cardMgrInitStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    subscriptionMgrInitStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

PhoneFactoryImplStub::~PhoneFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
    // remove cardManager
    if (cardManager_) {
        (std::static_pointer_cast<CardManagerStub>(cardManager_))->cleanup();
    }
    // remove SubscriptionManagerStub
    if (subscriptionManager_) {
        (std::static_pointer_cast<SubscriptionManagerStub>(subscriptionManager_))->cleanup();
    }
    subscriptionMgrCallbacks_.clear();
    cardMgrCallbacks_.clear();
}
PhoneFactory::PhoneFactory() {
    LOG(DEBUG, __FUNCTION__);
}

PhoneFactory::~PhoneFactory() {
    LOG(DEBUG, __FUNCTION__);
}

PhoneFactory &PhoneFactoryImplStub::getInstance() {
    static PhoneFactoryImplStub instance;
    return instance;
}

PhoneFactory &PhoneFactory::getInstance() {
    return PhoneFactoryImplStub::getInstance();
}

std::shared_ptr<ISmsManager> PhoneFactoryImplStub::getSmsManager(
    int phoneId, telux::common::InitResponseCb callback) {
    return nullptr;
}

std::shared_ptr<IPhoneManager> PhoneFactoryImplStub::getPhoneManager(
    telux::common::InitResponseCb Callback) {
    return nullptr;
}

std::shared_ptr<ICallManager> PhoneFactoryImplStub::getCallManager(
    telux::common::InitResponseCb Callback) {
    return nullptr;
}

std::shared_ptr<ICardManager> PhoneFactoryImplStub::getCardManager(
    telux::common::InitResponseCb callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (cardManager_ == nullptr) {
        std::shared_ptr<CardManagerStub> cardManager = nullptr;
        auto initCb = [this](telux::common::ServiceStatus status) {
            LOG(DEBUG, __FUNCTION__, " CardManager initialization callback");
            this->onCardManagerResponse(status);
        };
        try {
            cardManager = std::make_shared<CardManagerStub>(initCb);
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        if (callback) {
            cardMgrCallbacks_.push_back(callback);
        } else {
            LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
        cardManager_ = cardManager;
    } else if (cardMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(DEBUG, __FUNCTION__, " Card manager is not yet initialized");
        if (callback) {
           cardMgrCallbacks_.push_back(callback);
        } else {
           LOG(DEBUG, __FUNCTION__, " Callback is NULL");
        }
    } else if (callback) {
        LOG(DEBUG, __FUNCTION__, " card manager is initialized, invoking app callback");
        std::thread appCallback(callback, cardMgrInitStatus_);
        appCallback.detach();
    } else {
        LOG(ERROR, __FUNCTION__, " Card manager is initialized, app Callback is NULL");
    }
    return cardManager_;
}

void PhoneFactoryImplStub::onCardManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> cardMgrCallbacks;
    LOG(INFO, __FUNCTION__, " Card Manager initialization status: " ,
        static_cast<int>(status));
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        cardMgrInitStatus_ = status;
        bool reportServiceStatus = false;
        switch(status) {
           case telux::common::ServiceStatus::SERVICE_FAILED:
              cardManager_ = NULL;
              reportServiceStatus = true;
              break;
           case telux::common::ServiceStatus::SERVICE_AVAILABLE:
              reportServiceStatus = true;
              break;
           default:
              break;
       }
       if (!reportServiceStatus) {
           return;
       }
       cardMgrCallbacks = cardMgrCallbacks_;
       cardMgrCallbacks_.clear();
    }
    for (auto &callback : cardMgrCallbacks) {
        if (callback) {
           callback(status);
        } else {
           LOG(INFO, __FUNCTION__, " Callback is NULL");
        }
    }
}

std::shared_ptr<ISapCardManager> PhoneFactoryImplStub::getSapCardManager(int slotId,
    telux::common::InitResponseCb callback) {
    return nullptr;
}

std::shared_ptr<ISubscriptionManager> PhoneFactoryImplStub::getSubscriptionManager(
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (subscriptionManager_ == nullptr) {
       std::shared_ptr<SubscriptionManagerStub> subscriptionMgr = nullptr;
       auto initCb = [this](telux::common::ServiceStatus status) {
          LOG(DEBUG, __FUNCTION__, " Subscription initialization callback");
          this->onSubscriptionManagerResponse(status);
       };
       try {
          subscriptionMgr = std::make_shared<SubscriptionManagerStub>(initCb);
       } catch (std::bad_alloc & e) {
          LOG(ERROR, __FUNCTION__ , e.what());
          return nullptr;
       }
       if (callback) {
          subscriptionMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
       subscriptionManager_ = subscriptionMgr;
    } else if (subscriptionMgrInitStatus_ == telux::common::ServiceStatus::SERVICE_UNAVAILABLE) {
       LOG(DEBUG, __FUNCTION__, " Subscription manager is not yet initialized");
       if (callback) {
          subscriptionMgrCallbacks_.push_back(callback);
       } else {
          LOG(DEBUG, __FUNCTION__, " Callback is NULL");
       }
    } else if (callback) {
       LOG(DEBUG, __FUNCTION__, " Subscription manager is initialized, invoking app callback");
       std::thread appCallback(callback, subscriptionMgrInitStatus_);
       appCallback.detach();
    } else {
       LOG(ERROR, __FUNCTION__, " Subscription manager is initialized, app Callback is NULL");
    }
    return subscriptionManager_;
}

void PhoneFactoryImplStub::onSubscriptionManagerResponse(telux::common::ServiceStatus status) {
    std::vector<telux::common::InitResponseCb> subscriptionCallbacks;
    LOG(INFO, __FUNCTION__, " Subscription Manager initialization status: " ,
      static_cast<int>(status));
    {
       std::lock_guard<std::recursive_mutex> lock(mutex_);
       subscriptionMgrInitStatus_ = status;
       bool reportServiceStatus = false;
       switch(status) {
          case telux::common::ServiceStatus::SERVICE_FAILED:
             subscriptionManager_ = NULL;
             reportServiceStatus = true;
             break;
          case telux::common::ServiceStatus::SERVICE_AVAILABLE:
             reportServiceStatus = true;
             break;
          default:
             break;
       }
       if (!reportServiceStatus) {
          return;
       }
       subscriptionCallbacks = subscriptionMgrCallbacks_;
       subscriptionMgrCallbacks_.clear();
    }
    for (auto &callback : subscriptionCallbacks) {
       if (callback) {
          callback(status);
       } else {
          LOG(INFO, __FUNCTION__, " Callback is NULL");
       }
    }
}

std::shared_ptr<telux::tel::IServingSystemManager> PhoneFactoryImplStub::getServingSystemManager(
    int slotId, telux::common::InitResponseCb callback) {
    return nullptr;
}

std::shared_ptr<telux::tel::INetworkSelectionManager> PhoneFactoryImplStub::getNetworkSelectionManager
    ( int slotId, telux::common::InitResponseCb  callback) {
    return nullptr;
}

std::shared_ptr<IRemoteSimManager> PhoneFactoryImplStub::getRemoteSimManager(int slotId,
    telux::common::InitResponseCb  callback) {
    return nullptr;
}

std::shared_ptr<IMultiSimManager> PhoneFactoryImplStub::getMultiSimManager(
    telux::common::InitResponseCb callback) {
    return nullptr;
}

std::shared_ptr<ICellBroadcastManager> PhoneFactoryImplStub::getCellBroadcastManager(SlotId slotId,
    telux::common::InitResponseCb  callback) {
    return nullptr;
}

std::shared_ptr<ISimProfileManager> PhoneFactoryImplStub::getSimProfileManager(
    telux::common::InitResponseCb  callback) {
    return nullptr;
}

std::shared_ptr<IImsSettingsManager> PhoneFactoryImplStub::getImsSettingsManager(
    telux::common::InitResponseCb callback) {
    return nullptr;
}

std::shared_ptr<IEcallManager> PhoneFactoryImplStub::getEcallManager(
    telux::common::InitResponseCb callback) {
    return nullptr;
}

std::shared_ptr<IHttpTransactionManager> PhoneFactoryImplStub::getHttpTransactionManager(
    telux::common::InitResponseCb  callback) {
    return nullptr;
}
std::shared_ptr<IImsServingSystemManager> PhoneFactoryImplStub::getImsServingSystemManager(
    SlotId slotId, telux::common::InitResponseCb callback) {
    return nullptr;
}
std::shared_ptr<ISuppServicesManager> PhoneFactoryImplStub::getSuppServicesManager( SlotId slotId,
    telux::common::InitResponseCb  callback) {
    return nullptr;
}

}  // namespace tel
}  // namespace telux
