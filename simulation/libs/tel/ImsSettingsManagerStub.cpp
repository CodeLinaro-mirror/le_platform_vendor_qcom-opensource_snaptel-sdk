/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>
#include "ImsSettingsManagerStub.hpp"
#include "TelDefinesStub.hpp"

using namespace telux::tel;

ImsSettingsManagerStub::ImsSettingsManagerStub(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    stub_ = CommonUtils::getGrpcStub<::telStub::ImsService>();
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

void ImsSettingsManagerStub::initSync(telux::common::InitResponseCb callback) {
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;
    noOfSlots_ = SLOT_ID_1;
    if (telux::common::DeviceConfig::isMultiSimSupported()) {  // For DSDA slot count is 2.
        noOfSlots_ = MAX_SLOT_ID;
    }

    LOG(DEBUG, __FUNCTION__, " SlotCount: ", noOfSlots_);
    stub_->InitService(&context, request, &response);

    telux::common::ServiceStatus cbStatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
    int cbDelay = static_cast<int>(response.delay());
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
    if (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        listenerMgr_ =
            std::make_shared<telux::common::ListenerManager<IImsSettingsListener>>();
        if (!listenerMgr_) {
            LOG(ERROR, __FUNCTION__, " unable to instantiate ListenerManager");
            cbStatus = telux::common::ServiceStatus::SERVICE_FAILED;
        }
    }
    this->onServiceStatusChange(cbStatus);
    if (callback) {
        auto f1 = std::async(std::launch::async,
        [this, cbDelay, cbStatus, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
                callback(cbStatus);
        }).share();
        taskQ_->add(f1);
    }
}

ImsSettingsManagerStub::~ImsSettingsManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
    if (listenerMgr_) {
        listenerMgr_ = nullptr;
    }
    cleanup();
}

void ImsSettingsManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);

    ClientContext context;
    const ::google::protobuf::Empty request;
    ::google::protobuf::Empty response;

    stub_->CleanUpService(&context, request, &response);
}

telux::common::ServiceStatus ImsSettingsManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusReply response;
    ::commonStub::GetServiceStatusRequest request;
    ClientContext context;

    grpc::Status status = stub_->GetServiceStatus(&context, request, &response);
    telux::common::ServiceStatus serviceStatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
    return serviceStatus;
}

telux::common::Status ImsSettingsManagerStub::registerListener(
    std::weak_ptr<IImsSettingsListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        status = listenerMgr_->registerListener(listener);
        std::vector<std::string> filters = {TEL_IMS_SETTINGS_FILTER};
        std::vector<std::weak_ptr<IImsSettingsListener>> applisteners;
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 1) {
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.registerListener(shared_from_this(), filters);
        } else {
            LOG(DEBUG, __FUNCTION__,
                " Not registering to client event manager already registered");
        }
    }
    return status;
}

telux::common::Status ImsSettingsManagerStub::deregisterListener(
    std::weak_ptr<telux::tel::IImsSettingsListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status status = telux::common::Status::FAILED;
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IImsSettingsListener>> applisteners;
        status = listenerMgr_->deRegisterListener(listener);
        listenerMgr_->getAvailableListeners(applisteners);
        if (applisteners.size() == 0) {
            std::vector<std::string> filters = {TEL_IMS_SETTINGS_FILTER};
            auto &clientEventManager = telux::common::ClientEventManager::getInstance();
            clientEventManager.deregisterListener(shared_from_this(), filters);
        }
    }
    return status;
}

void ImsSettingsManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IImsSettingsListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "ImsSettings Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status
    ImsSettingsManagerStub::requestServiceConfig(SlotId slotId, ImsServiceConfigCb callback) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = static_cast<int>(slotId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims Settings Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestServiceConfigRequest request;
    ::telStub::RequestServiceConfigReply response;
    ClientContext context;
    request.set_phone_id(slotId);

    grpc::Status reqstatus = stub_->RequestServiceConfig(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }
    ImsServiceConfig config = {};
    config.configValidityMask.reset();
    if (response.mutable_config()->is_ims_service_enabled_valid()) {
        config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_IMS_SERVICE);
        config.imsServiceEnabled = response.mutable_config()->ims_service_enabled();
    }
    if (response.mutable_config()->is_voims_enabled_valid()) {
        config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_VOIMS);
        config.voImsEnabled = response.mutable_config()->voims_enabled();
    }
    if (response.mutable_config()->is_sms_enabled_valid()) {
        config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_SMS);
        config.smsEnabled = response.mutable_config()->sms_enabled();
    }
    if (response.mutable_config()->is_rtt_enabled_valid()) {
        config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_RTT);
        config.rttEnabled = response.mutable_config()->rtt_enabled();
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, slotId, config, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(slotId, config, error);
            } else {
                LOG(ERROR, __FUNCTION__, " Callback is null");
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status ImsSettingsManagerStub::setServiceConfig(SlotId slotId,
    ImsServiceConfig config, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = static_cast<int>(slotId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims Settings Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetServiceConfigRequest request;
    ::telStub::SetServiceConfigReply response;
    ClientContext context;

    request.set_phone_id(slotId);
    auto validityMask = config.configValidityMask;
    LOG(INFO, __FUNCTION__, " configValidityMask: ", validityMask.to_string());
    if (validityMask.test(IMSSETTINGS_IMS_SERVICE)) {
        request.mutable_config()->set_is_ims_service_enabled_valid(true);
        request.mutable_config()->set_ims_service_enabled(config.imsServiceEnabled);
    }
    if (validityMask.test(IMSSETTINGS_VOIMS)) {
        request.mutable_config()->set_is_voims_enabled_valid(true);
        request.mutable_config()->set_voims_enabled(config.voImsEnabled);
    }
    if (validityMask.test(IMSSETTINGS_SMS)) {
        request.mutable_config()->set_is_sms_enabled_valid(true);
        request.mutable_config()->set_sms_enabled(config.smsEnabled);
    }
    if (validityMask.test(IMSSETTINGS_RTT)) {
        request.mutable_config()->set_is_rtt_enabled_valid(true);
        request.mutable_config()->set_rtt_enabled(config.rttEnabled);
    }
    grpc::Status reqstatus = stub_->SetServiceConfig(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback, delay]() {
            if (callback) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                callback(error);
            }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status ImsSettingsManagerStub::requestSipUserAgent(SlotId slotId,
    ImsSipUserAgentConfigCb callback) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = static_cast<int>(slotId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims Settings Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::RequestSipUserAgentRequest request;
    ::telStub::RequestSipUserAgentReply response;
    ClientContext context;
    request.set_phone_id(slotId);

    grpc::Status reqstatus = stub_->RequestSipUserAgent(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }

    std::string sipUserAgent = response.sip_user_agent();
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, slotId, sipUserAgent, error, callback, delay]() {
                if (callback) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(slotId, sipUserAgent, error);
                }
        }).share();
        taskQ_->add(f1);
    }
    return status;
}

telux::common::Status ImsSettingsManagerStub::setSipUserAgent(SlotId slotId,
    std::string userAgent, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = static_cast<int>(slotId);
    if (phoneId <= 0 || phoneId > noOfSlots_) {
        LOG(DEBUG, __FUNCTION__, " Invalid PhoneId");
        return telux::common::Status::INVALIDPARAM;
    }
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Ims Settings Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::telStub::SetSipUserAgentRequest request;
    ::telStub::SetSipUserAgentReply response;
    ClientContext context;
    request.set_phone_id(slotId);
    request.set_sip_user_agent(userAgent);

    grpc::Status reqstatus = stub_->SetSipUserAgent(&context, request, &response);

    if (!reqstatus.ok()) {
        LOG(ERROR, __FUNCTION__, " Request failed ", reqstatus.error_message());
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.is_callback());
    int delay = static_cast<int>(response.delay());

    if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, error, callback, delay]() {
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

void ImsSettingsManagerStub::handleImsServiceConfigsChange(
    ::telStub::ImsServiceConfigsChangeEvent event) {
    LOG(INFO, __FUNCTION__);
    int phoneId = event.phone_id();

    ImsServiceConfig config = {};
    config.configValidityMask.reset();
    if (event.mutable_config()->is_ims_service_enabled_valid()) {
        config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_IMS_SERVICE);
        config.imsServiceEnabled = event.mutable_config()->ims_service_enabled();
    }
    if (event.mutable_config()->is_voims_enabled_valid()) {
        config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_VOIMS);
        config.voImsEnabled = event.mutable_config()->voims_enabled();
    }
    if (event.mutable_config()->is_sms_enabled_valid()) {
        config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_SMS);
        config.smsEnabled = event.mutable_config()->sms_enabled();
    }
    if (event.mutable_config()->is_rtt_enabled_valid()) {
        config.configValidityMask.set(telux::tel::ImsServiceConfigType::IMSSETTINGS_RTT);
        config.rttEnabled = event.mutable_config()->rtt_enabled();
    }

    std::vector<std::weak_ptr<IImsSettingsListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onImsServiceConfigsChange(static_cast<SlotId>(phoneId), config);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ImsSettingsManagerStub::handleImsSipUserAgentChange(
    ::telStub::ImsSipUserAgentChangeEvent event) {
    LOG(INFO, __FUNCTION__);
    int phoneId = event.phone_id();
    std::string sipUserAgent = event.sip_user_agent();

    std::vector<std::weak_ptr<IImsSettingsListener>> applisteners;
    if (listenerMgr_) {
        listenerMgr_->getAvailableListeners(applisteners);
        // Notify respective events
        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onImsSipUserAgentChange(static_cast<SlotId>(phoneId), sipUserAgent);
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " listenerMgr is null");
    }
}

void ImsSettingsManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(INFO, __FUNCTION__);
    if (event.Is<::telStub::ImsServiceConfigsChangeEvent>()) {
        ::telStub::ImsServiceConfigsChangeEvent imsServiceConfigsChangeEvent;
        event.UnpackTo(&imsServiceConfigsChangeEvent);
        handleImsServiceConfigsChange(imsServiceConfigsChangeEvent);
    } else if (event.Is<::telStub::ImsSipUserAgentChangeEvent>()) {
        ::telStub::ImsSipUserAgentChangeEvent imsSipUserAgentChangeEvent;
        event.UnpackTo(&imsSipUserAgentChangeEvent);
        handleImsSipUserAgentChange(imsSipUserAgentChangeEvent);
    } else {
        LOG(DEBUG, __FUNCTION__, " No handling required for other events");
    }
}
