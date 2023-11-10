/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 #include "ServingSystemManagerStub.hpp"

 #include "../common/Logger.hpp"

namespace telux {
namespace data {

ServingSystemManagerStub::ServingSystemManagerStub (SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    slotId_ = slotId;
}

ServingSystemManagerStub::~ServingSystemManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status ServingSystemManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    return telux::common::Status::SUCCESS;
}

telux::common::Status ServingSystemManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f =
        std::async(std::launch::async, [this]() { this->initSync(); }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void ServingSystemManagerStub::initSync() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ServiceStatus status = telux::common::ServiceStatus::SERVICE_AVAILABLE;
    if (initCb_) {
        initCb_(status);
    }
}

telux::common::ServiceStatus ServingSystemManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);

    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

DrbStatus ServingSystemManagerStub::getDrbStatus() {
    LOG(DEBUG, __FUNCTION__);

    DrbStatus status = {};

    return status;
}

telux::common::Status ServingSystemManagerStub::requestServiceStatus(RequestServiceStatusResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    telux::data::ServiceStatus status = {telux::data::DataServiceState::IN_SERVICE,
        telux::data::NetworkRat::LTE};
    auto f =
        std::async(std::launch::async,
        [status, callback]() {
            callback(status, telux::common::ErrorCode::SUCCESS);
        }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

telux::common::Status ServingSystemManagerStub::requestRoamingStatus(RequestRoamingStatusResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    telux::data::RoamingStatus status = {false, telux::data::RoamingType::UNKNOWN};
    auto f =
        std::async(std::launch::async,
        [status, callback]() {
            callback(status, telux::common::ErrorCode::SUCCESS);
        }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

telux::common::Status ServingSystemManagerStub::makeDormant(telux::common::ResponseCallback callback = nullptr) {
    LOG(DEBUG, __FUNCTION__);

    auto f =
        std::async(std::launch::async,
        [callback]() {
            callback(telux::common::ErrorCode::SUCCESS);
        }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

telux::common::Status ServingSystemManagerStub::requestNrIconType(RequestNrIconTypeResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    auto f =
        std::async(std::launch::async,
        [callback]() {
            callback(telux::data::NrIconType::NONE, telux::common::ErrorCode::SUCCESS);
        }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

SlotId ServingSystemManagerStub::getSlotId() {
    LOG(DEBUG, __FUNCTION__);

    return slotId_;
}

telux::common::Status ServingSystemManagerStub::registerListener(std::weak_ptr<IServingSystemListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return telux::common::Status::SUCCESS;
}

telux::common::Status ServingSystemManagerStub::deregisterListener(std::weak_ptr<IServingSystemListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    return telux::common::Status::SUCCESS;
}

} // end of namespace data
} // end of namespace telux