/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "ApSimProfileManagerStub.hpp"
#include "TelDefinesStub.hpp"

using namespace telux::tel;

ApSimProfileManagerStub::ApSimProfileManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    cbDelay_         = DEFAULT_DELAY;
}

void ApSimProfileManagerStub::setServiceStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status: ", static_cast<int>(status));
    {
        std::lock_guard<std::mutex> lock(mtx_);
        subSystemStatus_ = status;
    }
    if (initCb_) {
        auto f1 = std::async(std::launch::async, [this, status]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay_));
            initCb_(status);
        }).share();
        taskQ_->add(f1);
    } else {
        LOG(ERROR, __FUNCTION__, " Callback is NULL");
    }
}

telux::common::Status ApSimProfileManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IApSimProfileListener>>();
    if (!listenerMgr_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
        return telux::common::Status::FAILED;
    }
    stub_ = CommonUtils::getGrpcStub<::telStub::ApSimProfileService>();
    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate ap sim profile service");
        return telux::common::Status::FAILED;
    }
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if (!taskQ_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate AsyncTaskQueue");
        return telux::common::Status::FAILED;
    }
    initCb_     = callback;
    auto f      = std::async(std::launch::async, [this]() { this->initSync(); }).share();
    auto status = taskQ_->add(f);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to add task to queue");
        taskQ_ = nullptr;
    }
    return status;
}

void ApSimProfileManagerStub::initSync() {
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    noOfSlots_ = SLOT_ID_1;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {  // For DSDA slot count is 2.
        noOfSlots_ = MAX_SLOT_ID;
    }
    LOG(DEBUG, __FUNCTION__, " SlotCount: ", noOfSlots_);

    grpc::Status reqStatus                = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " InitService request failed");
    } else {
        cbStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay_ = static_cast<int>(response.delay());
    }
    LOG(DEBUG, __FUNCTION__, " callback delay ", cbDelay_, " callback status ",
        static_cast<int>(cbStatus));
    this->onServiceStatusChange(cbStatus);
    setServiceStatus(cbStatus);
}

ApSimProfileManagerStub::~ApSimProfileManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void ApSimProfileManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus ApSimProfileManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status ApSimProfileManagerStub::registerListener(
    std::weak_ptr<IApSimProfileListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status                           = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_AP_SIM_PROFILE_FILTER};
        std::vector<std::weak_ptr<IApSimProfileListener>> applisteners;
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 1) {
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(shared_from_this(), filters);
        } else {
            LOG(DEBUG, __FUNCTION__, " Not registering to client event manager already registered");
        }
    }
    return status;
}

telux::common::Status ApSimProfileManagerStub::deregisterListener(
    std::weak_ptr<telux::tel::IApSimProfileListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IApSimProfileListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_AP_SIM_PROFILE_FILTER};
            auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

void ApSimProfileManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IApSimProfileListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "ApSimProfile Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status ApSimProfileManagerStub::sendRetrieveProfileListResponse(SlotId slotId,
    ApduExchangeStatus result, uint32_t referenceId, std::vector<std::string> profileIccIds,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = static_cast<int>(slotId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " ApSimProfile Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::ProfileListResponseRequest request;
    ::telStub::ProfileListResponseReply response;
    ClientContext context;

    request.set_slot_id(slotId);
    request.set_reference_id(referenceId);
    request.set_result(static_cast<telStub::ApduExchangeStatus>(result));
    for (auto it : profileIccIds) {
        request.add_profile_iccid(it);
    }

    grpc::Status reqstatus = stub_->SendRetrieveProfileListResponse(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status   = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded          = static_cast<bool>(response.is_callback());
    int delay                      = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async, [this, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(error);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status ApSimProfileManagerStub::sendProfileOperationResponse(SlotId slotId,
    ApduExchangeStatus result, uint32_t referenceId, common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = static_cast<int>(slotId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " ApSimProfile Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::ProfileOperationResponseRequest request;
    ::telStub::ProfileOperationResponseReply response;
    ClientContext context;

    request.set_slot_id(slotId);
    request.set_reference_id(referenceId);
    request.set_result(static_cast<telStub::ApduExchangeStatus>(result));

    grpc::Status reqstatus = stub_->SendProfileOperationResponse(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status   = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded          = static_cast<bool>(response.is_callback());
    int delay                      = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async, [this, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(error);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

void ApSimProfileManagerStub::handleRetrieveProfileListRequest(
    ::telStub::ProfileListRequestEvent event) {
    LOG(INFO, __FUNCTION__);
    int phoneId          = event.slot_id();
    uint32_t referenceId = event.reference_id();

    std::vector<std::weak_ptr<IApSimProfileListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onRetrieveProfileListRequest(static_cast<SlotId>(phoneId), referenceId);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ApSimProfileManagerStub::handleProfileOperationRequest(
    ::telStub::ProfileOperationRequestEvent event) {
    LOG(INFO, __FUNCTION__);
    int phoneId          = event.slot_id();
    uint32_t referenceId = event.reference_id();
    std::string iccid    = event.iccid();
    int isEnable         = event.is_enable();

    std::vector<std::weak_ptr<IApSimProfileListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onProfileOperationRequest(
                    static_cast<SlotId>(phoneId), referenceId, iccid, isEnable);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ApSimProfileManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(INFO, __FUNCTION__);
    if (event.Is<::telStub::ProfileListRequestEvent>()) {
        ::telStub::ProfileListRequestEvent profileListRequestEvent;
        event.UnpackTo(&profileListRequestEvent);
        handleRetrieveProfileListRequest(profileListRequestEvent);
    } else if (event.Is<::telStub::ProfileOperationRequestEvent>()) {
        ::telStub::ProfileOperationRequestEvent profileOperationRequestEvent;
        event.UnpackTo(&profileOperationRequestEvent);
        handleProfileOperationRequest(profileOperationRequestEvent);
    } else {
        LOG(DEBUG, __FUNCTION__, " No handling required for other events");
    }
}
