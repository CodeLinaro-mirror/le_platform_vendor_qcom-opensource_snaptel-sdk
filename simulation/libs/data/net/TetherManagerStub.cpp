/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "TetherManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

#include <thread>

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {
namespace net {

TetherManagerStub::TetherManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

TetherManagerStub::~TetherManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status TetherManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    auto f = std::async(std::launch::async,
        [this, callback]() {
           this->initSync(callback);
       }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void TetherManagerStub::initSync(telux::common::InitResponseCb callback) {
    std::lock_guard<std::mutex> lck(initMtx_);

    stub_ = CommonUtils::getGrpcStub<::dataStub::TetherManager>();
    ::google::protobuf::Empty request;
    ::dataStub::GetServiceStatusReply response;
    grpc::ClientContext context;

    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    int cbDelay = DEFAULT_DELAY;
    do {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " InitService request failed");
            break;
        }
        cbStatus =
            static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay = static_cast<int>(response.delay());
        this->onServiceStatusChange(cbStatus);
        LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
    } while (0);

    setSubSystemStatus(cbStatus);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        callback(cbStatus);
    }
}

void TetherManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::vector<std::shared_ptr<ITetherListener>> applisteners;
    this->getAvailableListeners(applisteners);
    for (auto &listener : applisteners) {
        listener->onServiceStatusChange(status);
    }
}

void TetherManagerStub::getAvailableListeners(std::vector<std::shared_ptr<telux::data::net::ITetherListener>> &listeners) {
    LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners_.size());
    std::lock_guard<std::mutex> listenerLock(listenersMutex_);
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto sp = it->lock();
        if (sp) {
            listeners.emplace_back(sp);
            ++it;
        } else {
            LOG(DEBUG, "erased obsolete weak pointer from TetherManagerImpl's listeners");
            it = listeners_.erase(it);
        }
    }
}

telux::common::ServiceStatus TetherManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status TetherManagerStub::startBTTether(const telux::data::net::BTTetherMode  btMode,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    ::dataStub::BTTetherMode request;
    ::dataStub::DefaultReply response;
    grpc::ClientContext context;

    request.set_mode(static_cast<::dataStub::BTTetherMode::Mode>(btMode));
    grpc::Status reqStatus = stub_->StartBTTether(&context, request, &response);

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int delay = static_cast<int>(response.delay());
    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " StartBTTether request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(error);
                }).share();
            taskQ_->add(f);
        }
    }
    return status;   
}

telux::common::Status TetherManagerStub::stopBTTether(telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    ::google::protobuf::Empty request;
    ::dataStub::DefaultReply response;
    grpc::ClientContext context;

    grpc::Status reqStatus = stub_->StopBTTether(&context, request, &response);

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int delay = static_cast<int>(response.delay());
    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " stopBTTether request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(error);
                }).share();
            taskQ_->add(f);
        }
    }
    return status;   
}

telux::common::Status TetherManagerStub::requestBTTetherStatus(telux::data::net::BTTetherCb callback) {
    LOG(DEBUG, __FUNCTION__);

    ::google::protobuf::Empty request;
    ::dataStub::BTTetherStatusReply response;
    grpc::ClientContext context;

    grpc::Status reqStatus = stub_->RequestBTTetherStatus(&context, request, &response);

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.mutable_reply()->error());
    telux::common::Status status = static_cast<telux::common::Status>(response.mutable_reply()->status());
    int delay = static_cast<int>(response.mutable_reply()->delay());
    telux::data::net::BTTetherStatus btTetheringStatus = static_cast<telux::data::net::BTTetherStatus> (response.mutable_bt_tether_status()->status());
    telux::data::net::BTTetherMode btTetheringMode = static_cast<telux::data::net::BTTetherMode> (response.mutable_bt_tether_mode()->mode());
    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " requestBTTetherStatus request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
                [this, btTetheringMode, btTetheringStatus, error, callback, delay]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(btTetheringMode, btTetheringStatus, error);
                }).share();
            taskQ_->add(f);
        }
    }
    return status;   
}

telux::common::Status TetherManagerStub::registerListener(std::weak_ptr<telux::data::net::ITetherListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> listenerLock(listenersMutex_);
    telux::common::Status status = telux::common::Status::SUCCESS;
    auto spt = listener.lock();
    if (spt != nullptr) {
        bool existing = false;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == iter->lock()) {
                existing = true;
                LOG(DEBUG, __FUNCTION__, "Register Listener : Existing");
                break;
            }
        }
        if (existing == false) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
        }
    }
    return status;
}

telux::common::Status TetherManagerStub::deregisterListener(std::weak_ptr<telux::data::net::ITetherListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status status = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(listenersMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == iter->lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " In deRegister Listener : Removing");
                status=telux::common::Status::SUCCESS;
                break;
            }
        }
    }
    return status;
}

telux::data::OperationType TetherManagerStub::getOperationType() {
    LOG(DEBUG, __FUNCTION__);
    return OperationType::DATA_LOCAL;
}

void TetherManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    subSystemStatus_ = status;
}

}

}

}