/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "NetworkSelectionManagerStub.hpp"
#include "TelDefinesStub.hpp"

#define DELAY 100

using namespace telux::common;
using namespace telux::tel;

NetworkSelectionManagerStub::NetworkSelectionManagerStub(int phoneId,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<::telStub::NetworkSelectionService>();
    phoneId_ = phoneId;
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

void NetworkSelectionManagerStub::initSync(telux::common::InitResponseCb callback) {
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
            std::make_shared<telux::common::ListenerManager<INetworkSelectionListener>>();
        if(!listenerMgr_) {
            LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
            cbStatus = telux::common::ServiceStatus::SERVICE_FAILED;
        }
    }
    if(callback) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        callback(cbStatus);
    }
}

NetworkSelectionManagerStub::~NetworkSelectionManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void NetworkSelectionManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus NetworkSelectionManagerStub::getServiceStatus() {
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

std::future<bool> NetworkSelectionManagerStub::onSubsystemReady() {
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

bool NetworkSelectionManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    const ::commonStub::GetServiceStatusRequest request;
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

telux::common::Status NetworkSelectionManagerStub::registerListener(
    std::weak_ptr<INetworkSelectionListener> listener) {
    return telux::common::Status::SUCCESS;
}

telux::common::Status NetworkSelectionManagerStub::deregisterListener(
    std::weak_ptr<INetworkSelectionListener> listener) {
    return telux::common::Status::SUCCESS;
}

telux::common::Status NetworkSelectionManagerStub::requestNetworkSelectionMode
    (SelectionModeInfoCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestNetworkSelectionModeRequest request;
    ::telStub::RequestNetworkSelectionModeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestNetworkSelectionMode(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    NetworkModeInfo info = {};
    info.mode = static_cast<telux::tel::NetworkSelectionMode>(response.mode());
    info.mnc = response.mnc();
    info.mcc = response.mcc();
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, info, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(info, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::setNetworkSelectionMode
    (NetworkSelectionMode selectMode, std::string mcc, std::string mnc,
    common::ResponseCallback callback ) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::SetNetworkSelectionModeRequest request;
    ::telStub::SetNetworkSelectionModeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_mode(static_cast<telStub::NetworkSelectionMode_Mode>(selectMode));
    request.set_mcc(mcc);
    request.set_mnc(mnc);
    grpc::Status reqstatus = stub_->SetNetworkSelectionMode(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, cbDelay, error, callback]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                if (callback) {
                    callback(error);
                }
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::requestPreferredNetworks(
    PreferredNetworksCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestPreferredNetworksRequest request;
    ::telStub::RequestPreferredNetworksReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestPreferredNetworks(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo;
    for (int i = 0; i < response.preferred_size(); i++) {
        telux::tel::PreferredNetworkInfo info;
        info.mcc = static_cast<int>(response.mutable_preferred(i)->mcc());
        info.mnc = static_cast<int>(response.mutable_preferred(i)->mnc());
        for (auto &r : response.mutable_preferred(i)->types()) {
            int tmp =  static_cast<int>(r);
            info.ratMask.set(tmp);
        }
        preferredNetworks3gppInfo.emplace_back(info);
    }
    std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo;
    for (int i = 0; i < response.static_preferred_size(); i++) {
        telux::tel::PreferredNetworkInfo info;
        info.mcc = static_cast<int>(response.mutable_static_preferred(i)->mcc());
        info.mnc = static_cast<int>(response.mutable_static_preferred(i)->mnc());
        for (auto &r : response.mutable_static_preferred(i)->types()) {
            info.ratMask.set(static_cast<int>(r));
        }
        staticPreferredNetworksInfo.emplace_back(info);
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, preferredNetworks3gppInfo, staticPreferredNetworksInfo, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(preferredNetworks3gppInfo, staticPreferredNetworksInfo, error);
            }
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::setPreferredNetworks(
    std::vector<PreferredNetworkInfo> preferredNetworksInfo, bool clearPrevious,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::SetPreferredNetworksRequest request;
    ::telStub::SetPreferredNetworksReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_clear_previous(clearPrevious);
    int size = preferredNetworksInfo.size();
    for (int i = 0; i < size; i++) {
        telStub::PreferredNetworkInfo* info = request.add_preferred_networks_info();
        info->set_mcc(preferredNetworksInfo[i].mcc);
        info->set_mnc(preferredNetworksInfo[i].mnc);
        int dataSize = (preferredNetworksInfo[i].ratMask).size();
        for (int j = 0; j < dataSize; j++)
        {
            if ((preferredNetworksInfo[i].ratMask).test(j)) {
                info->add_types(static_cast<telStub::RatType_Type>(j));
            }
        }
    }
    grpc::Status reqstatus = stub_->SetPreferredNetworks(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(error);
            }
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::performNetworkScan(NetworkScanInfo info,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::PerformNetworkScanRequest request;
    ::telStub::PerformNetworkScanReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_scan_type(static_cast<::telStub::NetworkScanType>(info.scanType));
    int size = (info.ratMask).size();
    for (int j = 0; j < size ; j++)
    {
        if((info.ratMask).test(j)) {
            request.add_rat_types(static_cast<::telStub::RatType_Type>(j));
        }
    }

    grpc::Status reqstatus = stub_->PerformNetworkScan(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status NetworkSelectionManagerStub::requestNetworkSelectionMode
    (SelectionModeResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestNetworkSelectionModeRequest request;
    ::telStub::RequestNetworkSelectionModeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestNetworkSelectionMode(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }

    NetworkSelectionMode mode = static_cast<telux::tel::NetworkSelectionMode>(response.mode());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, mode, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(mode, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

void NetworkSelectionManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(ERROR, __FUNCTION__ , "Not Supported");
}

telux::common::Status NetworkSelectionManagerStub::performNetworkScan(
    NetworkScanCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

OperatorInfo::OperatorInfo(std::string networkName, std::string mcc, std::string mnc,
                           OperatorStatus operatorStatus)
   : networkName_(networkName)
   , mcc_(mcc)
   , mnc_(mnc)
   , rat_(telux::tel::RadioTechnology::RADIO_TECH_UNKNOWN)
   , operatorStatus_(operatorStatus) {
   LOG(DEBUG, "Operator Info");
}

OperatorInfo::OperatorInfo(std::string networkName, std::string mcc, std::string mnc,
   telux::tel::RadioTechnology rat, OperatorStatus operatorStatus)
   : networkName_(networkName)
   , mcc_(mcc)
   , mnc_(mnc)
   , rat_(rat)
   , operatorStatus_(operatorStatus) {
}

std::string OperatorInfo::getName() {
   return networkName_;
}

std::string OperatorInfo::getMcc() {
   return mcc_;
}

std::string OperatorInfo::getMnc() {
   return mnc_;
}

OperatorStatus OperatorInfo::getStatus() {
   return operatorStatus_;
}

RadioTechnology OperatorInfo::getRat() {
   return rat_;
}

