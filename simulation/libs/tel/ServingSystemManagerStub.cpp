/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "ServingSystemManagerStub.hpp"
#include "TelDefinesStub.hpp"

#define DELAY 100

using namespace telux::common;
using namespace telux::tel;

ServingSystemManagerStub::ServingSystemManagerStub(int phoneId,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<::telStub::ServingSystemService>();
    phoneId_ = phoneId;
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

void ServingSystemManagerStub::initSync(telux::common::InitResponseCb callback) {
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
            std::make_shared<telux::common::ListenerManager<IServingSystemListener>>();
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

ServingSystemManagerStub::~ServingSystemManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void ServingSystemManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus ServingSystemManagerStub::getServiceStatus() {
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

std::future<bool> ServingSystemManagerStub::onSubsystemReady() {
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

bool ServingSystemManagerStub::isSubsystemReady() {
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

telux::common::Status ServingSystemManagerStub::registerListener(
    std::weak_ptr<IServingSystemListener> listener, ServingSystemNotificationMask mask) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {telux::tel::TEL_SERVING_SYSTEM_FILTER};
        std::vector<std::weak_ptr<IServingSystemListener>> applisteners;
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

telux::common::Status ServingSystemManagerStub::deregisterListener(
    std::weak_ptr<IServingSystemListener> listener, ServingSystemNotificationMask mask) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IServingSystemListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {telux::tel::TEL_SERVING_SYSTEM_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

telux::common::Status
    ServingSystemManagerStub::requestRatPreference(RatPreferenceCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestRATPreferenceRequest request;
    ::telStub::RequestRATPreferenceReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestRATPreference(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    RatPreference preference;
    for (auto &r : response.rat_pref_types()) {
        preference.set(static_cast<int>(r));
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, preference, error, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
            if (callback) {
                callback(preference, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::setServiceDomainPreference(
    ServiceDomainPreference serviceDomain, common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::SetServiceDomainPreferenceRequest request;
    ::telStub::SetServiceDomainPreferenceReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_service_domain_pref
        (static_cast<telStub::ServiceDomainPreference_Pref>(serviceDomain));
    grpc::Status reqstatus = stub_->SetServiceDomainPreference(&context, request, &response);
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
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::requestServiceDomainPreference(
    ServiceDomainPreferenceCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestServiceDomainPreferenceRequest request;
    ::telStub::RequestServiceDomainPreferenceReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestServiceDomainPreference(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ServiceDomainPreference preference =
        (static_cast<telux::tel::ServiceDomainPreference>(response.service_domain_pref()));
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, preference, error, callback]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(preference, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::getSystemInfo(ServingSystemInfo &sysInfo) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::GetSystemInfoRequest request;
    ::telStub::GetSystemInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetSystemInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    sysInfo.domain = static_cast<telux::tel::ServiceDomain>(response.current_domain());
    sysInfo.rat = static_cast<telux::tel::RadioTechnology>(response.current_rat());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

telux::common::Status ServingSystemManagerStub::requestNetworkTime(
    NetworkTimeResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestNetworkTimeRequest request;
    ::telStub::RequestNetworkTimeReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestNetworkTime(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    NetworkTimeInfo info;
    info.year = ((response.network_time_info()).year());
    info.month = ((response.network_time_info()).month());
    info.day = ((response.network_time_info()).day());
    info.hour = ((response.network_time_info()).hour());
    info.minute = ((response.network_time_info()).minute());
    info.second = ((response.network_time_info()).second());
    info.dayOfWeek = ((response.network_time_info()).day_of_week());
    info.timeZone = ((response.network_time_info()).time_zone());
    info.dstAdj = ((response.network_time_info()).dst_adj());
    info.nitzTime = ((response.network_time_info()).nitz_time());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, cbDelay, info, error, callback]() {
                if (callback) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                    callback(info, error);
                }
            }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::requestRFBandInfo(RFBandInfoCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::RequestRFBandInfoRequest request;
    ::telStub::RequestRFBandInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestRFBandInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    RFBandInfo info;
    info.band = static_cast<telux::tel::RFBand>(response.band());
    info.channel = response.channel();
    info.bandWidth = static_cast<telux::tel::RFBandWidth>(response.band_width());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int cbDelay = static_cast<int>(response.delay());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, cbDelay, info, error, callback]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(info, error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::tel::DcStatus ServingSystemManagerStub::getDcStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::GetDcStatusRequest request;
    ::telStub::GetDcStatusReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetDcStatus(&context, request, &response);
    DcStatus status;
    status.endcAvailability =
        static_cast<telux::tel::EndcAvailability>(response.endc_availability());
    status.dcnrRestriction = static_cast<telux::tel::DcnrRestriction>(response.dcnr_restriction());
    return status;
}

telux::common::Status ServingSystemManagerStub::setRatPreference(RatPreference ratPref,
    common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::SetRATPreferenceRequest request;
    ::telStub::SetRATPreferenceReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    int size = ratPref.size();
    for (int j = 0; j < size ; j++)
    {
        if(ratPref.test(j)) {
            request.add_rat_pref_types(static_cast<telStub::RatPrefType>(j));
        }
    }
    grpc::Status reqstatus = stub_->SetRATPreference(&context, request, &response);
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
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(error);
            }
        }).share();
    taskQ_->add(f);
    }
    return status;
}

telux::common::Status ServingSystemManagerStub::getNetworkRejectInfo
    (NetworkRejectInfo &rejectInfo) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::GetNetworkRejectInfoRequest request;
    ::telStub::GetNetworkRejectInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetNetworkRejectInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    rejectInfo.rejectSrvInfo.domain =
        static_cast<telux::tel::ServiceDomain>(response.reject_domain());
    rejectInfo.rejectSrvInfo.rat = static_cast<telux::tel::RadioTechnology>(response.reject_rat());
    rejectInfo.rejectCause = response.reject_cause();
    rejectInfo.mcc = response.mcc();
    rejectInfo.mnc = response.mnc();
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

telux::common::Status ServingSystemManagerStub::getCallBarringInfo
    (std::vector<CallBarringInfo> &barringInfo) {
    LOG(DEBUG, __FUNCTION__);
    ::telStub::GetCallBarringInfoRequest request;
    ::telStub::GetCallBarringInfoReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetCallBarringInfo(&context, request, &response);
    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    for (int i = 0; i < response.barring_infos_size(); i++) {
        CallBarringInfo info;
        info.rat = static_cast<telux::tel::RadioTechnology>(
            response.barring_infos(i).rat());
        info.domain = static_cast<telux::tel::ServiceDomain>(
            response.barring_infos(i).domain());
        info.callType = static_cast<telux::tel::CallsAllowedInCell>(
            response.barring_infos(i).call_type());
        barringInfo.emplace_back(info);
    }
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    return status;
}

void ServingSystemManagerStub::handleCallBarringInfosChanged
    (::telStub::CallBarringInfosEvent event) {
    LOG(DEBUG, __FUNCTION__);
    // update CallBarringInfos
    std::vector<CallBarringInfo> infos = {};
    for (int i = 0; i < event.barring_infos_size(); i++) {
        CallBarringInfo info;
        info.rat = static_cast<telux::tel::RadioTechnology>(
            event.barring_infos(i).rat());
        info.domain = static_cast<telux::tel::ServiceDomain>(
            event.barring_infos(i).domain());
        info.callType = static_cast<telux::tel::CallsAllowedInCell>(
            event.barring_infos(i).call_type());
        infos.emplace_back(info);
    }
    std::vector<std::weak_ptr<IServingSystemListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onCallBarringInfoChanged(infos);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ServingSystemManagerStub::onEventUpdate(google::protobuf::Any event) {
    if(event.Is<::telStub::CallBarringInfosEvent>()) {
        ::telStub::CallBarringInfosEvent callBarringInfosChangeEvent;
        event.UnpackTo(&callBarringInfosChangeEvent);
        handleCallBarringInfosChanged(callBarringInfosChangeEvent);
    }
}

