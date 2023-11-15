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

#include "CellBroadcastManagerStub.hpp"

#define DELAY 100
using namespace telux::common;
using namespace telux::tel;
using namespace std;

CellBroadcastManagerStub::CellBroadcastManagerStub(int phoneId,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if(callback) {
        auto f = std::async(std::launch::async,
            [this, callback]() {
                this->initSync(callback);
            }).share();
        taskQ_->add(f);
    }
}

void CellBroadcastManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    if(cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        listenerMgr_ = std::make_shared<telux::common::ListenerManager<ICellBroadcastListener>>();
        if(!listenerMgr_) {
            LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
            cbStatus = telux::common::ServiceStatus::SERVICE_FAILED;
        }
    }
    if(callback) {
        this->invokeInitResponseCallback(cbStatus, callback);
    }
}

bool CellBroadcastManagerStub::isSubsystemReady() {
    return true;
}

std::future<bool> CellBroadcastManagerStub::onSubsystemReady() {
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

void CellBroadcastManagerStub::invokeInitResponseCallback(telux::common::ServiceStatus cbStatus,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));

    if (callback) {
        callback(cbStatus);
    }
}

telux::common::ServiceStatus CellBroadcastManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    return serviceStatus;
}

telux::common::Status
    CellBroadcastManagerStub::registerListener(std::weak_ptr<ICellBroadcastListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " CellBroadCastManager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        if(status != telux::common::Status::SUCCESS ) {
            return status;
        }
        auto &eventManager = telux::common::EventManager::getInstance();
        eventManager.connectToSimulationServer();
        eventManager.registerListener(shared_from_this(), TEL_CELL_BROADCAST_FILTER);
    }
    return status;
}

telux::common::Status CellBroadcastManagerStub::deregisterListener(
    std::weak_ptr<ICellBroadcastListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " CellBroadCastManager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<ICellBroadcastListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        if(status != telux::common::Status::SUCCESS ) {
            return status;
        }
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            auto &eventManager = telux::common::EventManager::getInstance();
            eventManager.deregisterListener(shared_from_this());
        }
    }
    return status;
}

SlotId CellBroadcastManagerStub::getSlotId() {
    return static_cast<SlotId>(DEFAULT_SLOT_ID);
}

telux::common::Status CellBroadcastManagerStub::updateMessageFilters(
    std::vector<CellBroadcastFilter> filters, telux::common::ResponseCallback callback) {
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " CellBroadCastManager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    if(callback) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback]() {
                this->invokeCallback(callback, error);
            }).share();
        taskQ_->add(f1);
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status CellBroadcastManagerStub::requestMessageFilters(
    RequestFiltersResponseCallback callback) {
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " CellBroadCastManager is not ready");
        return telux::common::Status::NOTREADY;
    }
    std::vector<CellBroadcastFilter> filters = {};
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    if(callback) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback, filters]() {
                this->invokeCallback(callback, error, filters);
            }).share();
        taskQ_->add(f1);
    }
    return telux::common::Status::SUCCESS;
}

void CellBroadcastManagerStub::invokeCallback(RequestFiltersResponseCallback callback,
    telux::common::ErrorCode error, std::vector<CellBroadcastFilter> filters) {
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
    if(callback) {
        auto f = std::async(std::launch::async,
            [this, error , filters, callback]() {
                callback(filters, error);
            }).share();
        taskQ_->add(f);
    }
}

telux::common::Status CellBroadcastManagerStub::setActivationStatus(bool activate,
    telux::common::ResponseCallback callback) {
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " CellBroadCastManager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    if(callback) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback]() {
                this->invokeCallback(callback, error);
            }).share();
        taskQ_->add(f1);
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status CellBroadcastManagerStub::requestActivationStatus(
    RequestActivationStatusResponseCallback callback) {
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " CellBroadCastManager is not ready");
        return telux::common::Status::NOTREADY;
    }
    bool isActivated = true;
    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    if(callback) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback, isActivated]() {
                this->invokeCallback(callback, error, isActivated);
            }).share();
        taskQ_->add(f1);
    }
    return telux::common::Status::SUCCESS;
}

void CellBroadcastManagerStub::invokeCallback(RequestActivationStatusResponseCallback callback,
    telux::common::ErrorCode error, bool isActivated) {
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
    if(callback) {
        auto f = std::async(std::launch::async,
            [this, error , isActivated, callback]() {
                callback(isActivated, error);
            }).share();
        taskQ_->add(f);
    }
}

void CellBroadcastManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error) {
    std::this_thread::sleep_for(std::chrono::milliseconds(DELAY));
    if(callback) {
        auto f = std::async(std::launch::async,
            [this, error , callback]() {
                callback(error);
            }).share();
        taskQ_->add(f);
    }
}
