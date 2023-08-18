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

#include "PhoneManagerStub.hpp"
#include <telux/common/DeviceConfig.hpp>
#include <bits/stdc++.h>

using namespace telux::common;
using namespace telux::tel;
using namespace std;

PhoneManagerStub::PhoneManagerStub(telux::common::InitResponseCb callback) {
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    int numSlots = 1;
    if(telux::common::DeviceConfig::isMultiSimSupported()) {
        numSlots = 2;
    }
    for(int id = 1; id <= numSlots; id++) {
         phoneIds_.emplace_back(id);
    }
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

void PhoneManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    int cbDelay = 100;
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
    if(callback) {
        this->invokeInitResponseCallback(cbDelay, cbStatus, callback);
    }
}

void PhoneManagerStub::invokeInitResponseCallback(int cbDelay, telux::common::ServiceStatus cbStatus,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(cbStatus);
    }
}

PhoneManagerStub::~PhoneManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::ServiceStatus PhoneManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    return serviceStatus;
}

telux::common::Status PhoneManagerStub::registerListener(std::weak_ptr<IPhoneListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Phone Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> listenerLock(phoneManagerMutex_);
    telux::common::Status status = telux::common::Status::FAILED;
    auto spt = listener.lock();
    if (spt != nullptr) {
        if (listeners_.size() == 0) {
            try {
            } catch(exception const & ex) {
                LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
                status = telux::common::Status::NOMEMORY;
                return status;
            }
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
            status = telux::common::Status::SUCCESS;
        }
    } else {
        LOG(ERROR, "Null listener");
        return telux::common::Status::INVALIDPARAM;
    }
    return status;
}

telux::common::Status PhoneManagerStub::removeListener(
        std::weak_ptr<IPhoneListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " Phone Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(phoneManagerMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " Erasing listener");
                retVal = telux::common::Status::SUCCESS;
                break;
            }
        }
    } else {
        LOG(WARNING, "listener is null");
        retVal = telux::common::Status::NOSUCH;
    }
    return (retVal);
}

telux::common::Status PhoneManagerStub::getPhoneIds(std::vector<int> &phoneIds) {
    phoneIds = phoneIds_;
    return telux::common::Status::SUCCESS;
}

int PhoneManagerStub::getPhoneIdFromSlotId(int slotId) {
    return 1;   /* Todo: DSDS/DSDA */
}

int PhoneManagerStub::getSlotIdFromPhoneId(int phoneId) {
    return 1;   /* Todo: DSDS/DSDA */
}

std::shared_ptr<IPhone> PhoneManagerStub::getPhone(int phoneId) {
    LOG(DEBUG, __FUNCTION__);
   if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       LOG(ERROR, " PhoneManager is not ready");
       return nullptr;
   }
   std::vector<int> phoneIds;
   getPhoneIds(phoneIds);
   auto iter = std::find_if(std::begin(phoneIds), std::end(phoneIds),
                            [=](int id) { return id == phoneId; });
   if(iter != std::end(phoneIds)) {
      LOG(DEBUG, "Found given phoneId: ", phoneId);
   } else {
      LOG(INFO, "given invalid phoneId: ", phoneId);
      return nullptr;
   }

   auto ph = phoneMap_.find(phoneId);
   if(ph != phoneMap_.end()) {
      LOG(DEBUG, "Found phoneId (", phoneId, ") in the phoneMap");
      return phoneMap_[phoneId];
   } else {
      LOG(DEBUG, "updating phoneMap_");
      auto ph = std::make_shared<PhoneStub>(phoneId);
      phoneMap_.emplace(phoneId, ph);
      return ph;
   }
}

telux::common::Status PhoneManagerStub::requestCellularCapabilityInfo(
    std::shared_ptr<ICellularCapabilityCallback> callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneManagerStub::setOperatingMode(OperatingMode operatingMode,
    telux::common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneManagerStub::requestOperatingMode(
    std::shared_ptr<IOperatingModeCallback> callback) {
    telux::tel::OperatingMode operatingMode = telux::tel::OperatingMode::ONLINE;
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    auto f1 = std::async(std::launch::async,
        [this, operatingMode, callback, error]() {
            this->invokeOperatingModeCallback(operatingMode, error, callback);
        }).share();
    taskQ_->add(f1);
    return telux::common::Status::SUCCESS;
}

void PhoneManagerStub::invokeOperatingModeCallback(
    telux::tel::OperatingMode operatingMode,
    telux::common::ErrorCode error, std::shared_ptr<IOperatingModeCallback> callback) {
    auto f = std::async(std::launch::async,
        [this, callback, error , operatingMode]() {
            callback->operatingModeResponse(operatingMode, error);
        }).share();
    taskQ_->add(f);
}

telux::common::Status PhoneManagerStub::resetWwan(telux::common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

bool PhoneManagerStub::isSubsystemReady() {
    return true;
}

std::future<bool> PhoneManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
    [this]() {
        while (!isSubsystemReady()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    return(isSubsystemReady());});
    return((ready_future));
}