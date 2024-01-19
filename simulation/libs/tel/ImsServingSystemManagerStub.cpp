/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "ImsServingSystemManagerStub.hpp"
#include "TelDefinesStub.hpp"

using namespace telux::common;
using namespace telux::tel;

ImsServingSystemManagerStub::ImsServingSystemManagerStub(SlotId slotId,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<::telStub::ImsServingSystem>();
    phoneId_ = static_cast<int>(slotId);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

void ImsServingSystemManagerStub::initSync(telux::common::InitResponseCb callback) {
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(phoneId_);

    stub_->InitService(&context, request, &response);

    telux::common::ServiceStatus cbStatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
    int cbDelay = static_cast<int>(response.delay());
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
    if(cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        listenerMgr_ =
            std::make_shared<telux::common::ListenerManager<IImsServingSystemListener>>();
        if(!listenerMgr_) {
            LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
            cbStatus = telux::common::ServiceStatus::SERVICE_FAILED;
        }
    }
    if(callback) {
        auto f1 = std::async(std::launch::async,
        [this, cbDelay, cbStatus, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(cbStatus);
        }).share();
        taskQ_->add(f1);
    }
}

ImsServingSystemManagerStub::~ImsServingSystemManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void ImsServingSystemManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus ImsServingSystemManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status status = stub_->GetServiceStatus(&context, request, &response);
    telux::common::ServiceStatus serviceStatus =
    static_cast<telux::common::ServiceStatus>(response.service_status());
    return serviceStatus;
}

telux::common::Status ImsServingSystemManagerStub::registerListener(
    std::weak_ptr<IImsServingSystemListener> listener) {
    return telux::common::Status::SUCCESS;
}

telux::common::Status ImsServingSystemManagerStub::deregisterListener(
    std::weak_ptr<telux::tel::IImsServingSystemListener> listener) {
     return telux::common::Status::SUCCESS;
}

telux::common::Status
    ImsServingSystemManagerStub::requestRegistrationInfo(ImsRegistrationInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims ServingSystem Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestRegistrationInfoRequest request;
    ::telStub::RequestRegistrationInfoReply response;
    ClientContext context;
    request.set_slot_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestRegistrationInfo(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ImsRegistrationInfo info;
    info.imsRegStatus = static_cast<telux::tel::RegistrationStatus>(response.ims_reg_status());
    info.rat = static_cast<telux::tel::RadioTechnology>(response.rat());
    info.errorCode = response.error_code();
    info.errorString = (response.error_string());

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, info, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(info, error);
            }
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status ImsServingSystemManagerStub::requestServiceInfo(ImsServiceInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims ServingSystem Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestServiceInfoRequest request;
    ::telStub::RequestServiceInfoReply response;
    ClientContext context;
    request.set_slot_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestServiceInfo(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ImsServiceInfo info;
    info.sms = static_cast<telux::tel::CellularServiceStatus>(response.sms());
    info.voice = static_cast<telux::tel::CellularServiceStatus>(response.voice());

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, info, error, callback, delay]() {
                if (callback) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(info, error);
                }
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status ImsServingSystemManagerStub::requestPdpStatus(ImsPdpStatusInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims ServingSystem Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestPdpStatusRequest request;
    ::telStub::RequestPdpStatusReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestPdpStatus(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ImsPdpStatusInfo info;
    telux::common::DataCallEndReason failureReason;
    info.isPdpConnected = response.is_pdp_connected();
    info.apnName = response.apn_name();
    info.failureCode = static_cast<telux::tel::PdpFailureCode>(response.failure_code());
    failureReason.type = static_cast<telux::common::EndReasonType>(response.failure_reason());
    info.failureReason = failureReason;

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, info, error, callback, delay]() {
                if (callback) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(info, error);
                }
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

void ImsServingSystemManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(ERROR, __FUNCTION__ , "Not Supported");
}


