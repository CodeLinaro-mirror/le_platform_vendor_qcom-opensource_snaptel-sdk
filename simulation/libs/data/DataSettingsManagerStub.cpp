/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataSettingsManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {

DataSettingsManagerStub::DataSettingsManagerStub(
    OperationType oprType)
    : oprType_(oprType){
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

DataSettingsManagerStub::~DataSettingsManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status DataSettingsManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(INFO, __FUNCTION__);
    initCb_ = callback;
    auto f = std::async(std::launch::async,
            [this, callback]() {
                this->initSync(callback);
            }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void DataSettingsManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::DataSettingsManager>();

    ::dataStub::InitRequest request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus =
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
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
        invokeInitCallback(cbStatus);
    }
}

telux::common::ServiceStatus DataSettingsManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

void DataSettingsManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

void DataSettingsManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void DataSettingsManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

telux::common::Status DataSettingsManagerStub::requestDdsSwitch(
    DdsInfo info, telux::common::ResponseCallback callback) {
    LOG(INFO, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetDdsSwitchRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_slot_id(info.slotId);
    request.set_switch_type(static_cast<int>(info.type));
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->SetDdsSwitch(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " DdsSwitch request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }

        if (error == telux::common::ErrorCode::SUCCESS) {
            this->onDdsChange(info);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::requestCurrentDds(
    RequestCurrentDdsResponseCb callback) {
    LOG(INFO, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::CurrentDdsSwitchRequest request;
    ::dataStub::CurrentDdsSwitchResponse response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->RequestCurrentDdsSwitch(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    DdsInfo ddsResponse;
    ddsResponse.slotId = static_cast<SlotId>(response.slot_id());
    ddsResponse.type = static_cast<DdsType>(response.current_switch());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " Request DDS failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, ddsResponse, callback]() {
                   callback(ddsResponse, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::restoreFactorySettings(
    OperationType operationType, telux::common::ResponseCallback callback,
    bool isRebootNeeded) {
    LOG(INFO, __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status DataSettingsManagerStub::setBackhaulPreference(
    std::vector<BackhaulType> backhaulPref, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::setBackhaulPreferenceRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    for(size_t i=0; i<backhaulPref.size(); ++i) {
        request.add_backhaul_pref(
            static_cast<::dataStub::BackhaulPreference>(
            backhaulPref[i]));
    }
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->setBackhaulPreference(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setBackhaulPreference request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::requestBackhaulPreference(
    RequestBackhaulPrefResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::RequestBackhaulPreference request;
    ::dataStub::BackhaulPreferenceReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->requestBackhaulPreference(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    std::vector<BackhaulType> backhaulPref;
    for (auto& pref: response.backhaul_pref()) {
        backhaulPref.emplace_back(static_cast<BackhaulType>(pref));
    }

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request BackhaulPreference failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, backhaulPref, callback]() {
                   callback(backhaulPref, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::setBandInterferenceConfig(bool enable,
    std::shared_ptr<BandInterferenceConfig> config, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::BandInterferenceConfig request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_enable(enable);
    if(enable) {
        request.set_priority(static_cast<int>(config->priority));
        request.set_wlan_wait_time_in_sec(config->wlanWaitTimeInSec);
        request.set_n79_wait_time_in_sec(config->n79WaitTimeInSec);
        request.set_operation_type(::dataStub::OperationType(oprType_));
    }

    grpc::Status reqStatus = stub_->setBandInterferenceConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setBandInterferenceConfig request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::requestBandInterferenceConfig(
    RequestBandInterferenceConfigResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::BandInterferenceRequest request;
    ::dataStub::BandInterferenceReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus =
        stub_->requestBandInterferenceConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());


    bool enabled = response.config().enable();
    std::shared_ptr<BandInterferenceConfig> config = nullptr;
    if (enabled) {
        config = std::make_shared<BandInterferenceConfig>();
        config->priority = static_cast<BandPriority>(response.config().priority());
        config->n79WaitTimeInSec = response.config().wlan_wait_time_in_sec();
        config->wlanWaitTimeInSec = response.config().n79_wait_time_in_sec();
    }

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " request BandInterferenceConfig failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, enabled, config, callback]() {
                   callback(enabled, config, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::setWwanConnectivityConfig(SlotId slotId,
    bool allow, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetWwanConnectivityConfigRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_slot_id(slotId);
    request.set_is_wwan_connectivity_allowed(allow);
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->SetWwanConnectivityConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setWwanConnectivityConfig failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    if (error == telux::common::ErrorCode::SUCCESS) {
        this->onWwanConnectivityConfigChange(slotId, allow);
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::requestWwanConnectivityConfig(SlotId slotId,
    requestWwanConnectivityConfigResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::WwanConnectivityConfigRequest request;
    ::dataStub::WwanConnectivityConfigReply response;
    ClientContext context;

    request.set_slot_id(slotId);
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->RequestWwanConnectivityConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    bool isallowed = response.is_wwan_connectivity_allowed();

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " Request WwanConnectivity failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, slotId, isallowed, callback]() {
                   callback(slotId, isallowed, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::switchBackHaul(BackhaulInfo source,
    BackhaulInfo dest, bool applyToAll, telux::common::ResponseCallback callback) {
    LOG(DEBUG,__FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::switchBackHaulRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_backhaul_type(static_cast<::dataStub::BackhaulPreference>(dest.backhaul));
    request.set_slot_id(dest.slotId);
    request.set_profile_id(dest.profileId);
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->switchBackHaul(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " switchBackHaul request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::registerListener(
    std::weak_ptr<IDataSettingsListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> listenerLock(mutex_);
    telux::common::Status status = telux::common::Status::SUCCESS;
    auto spt = listener.lock();
    if (spt != nullptr) {
        bool existing = 0;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, "Register Listener : Existing");
                break;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::deregisterListener(
    std::weak_ptr<IDataSettingsListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " In deRegister Listener : Removing");
                retVal=telux::common::Status::SUCCESS;
                break;
            }
        }
    }

    return (retVal);
}

telux::common::Status DataSettingsManagerStub::setMacSecState(
    bool enable, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetMacSecStateRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_enabled(enable);
    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->SetMacSecState(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " setMacSecState request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    }

    return status;
}

telux::common::Status DataSettingsManagerStub::requestMacSecState(
    RequestMacSecSateResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Data settings manager not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::MacSecStateRequest request;
    ::dataStub::MacSecStateReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(oprType_));
    grpc::Status reqStatus = stub_->RequestMacSecState(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.reply().error());
    status = static_cast<telux::common::Status>(response.reply().status());
    delay = static_cast<int>(response.reply().delay());

    bool isenabled = static_cast<SlotId>(response.enabled());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " Request MacSecState failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            auto f = std::async(std::launch::async,
                [this, error, isenabled, callback]() {
                   callback(isenabled, error);
                }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

void DataSettingsManagerStub::getAvailableListeners(
    std::vector<std::shared_ptr<IDataSettingsListener>> &listeners) {
    LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners_.size());
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto sp = (*it).lock();
        if (sp) {
            listeners.emplace_back(sp);
            ++it;
        } else {
            LOG(DEBUG, "erased obsolete weak pointer from DataConnectionManagerImpl's listeners");
            it = listeners_.erase(it);
        }
    }
}

void DataSettingsManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);

    setSubSystemStatus(status);
    invokeInitCallback(status);

    std::vector<std::shared_ptr<IDataSettingsListener>> applisteners;
    this->getAvailableListeners(applisteners);
    for (auto& listener : applisteners) {
        listener->onServiceStatusChange(status);
    }
}

void DataSettingsManagerStub::onWwanConnectivityConfigChange(
    SlotId slotId, bool isConnectivityAllowed) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::shared_ptr<IDataSettingsListener>> listeners;
    this->getAvailableListeners(listeners);
    for(auto& listener : listeners) {
        listener->onWwanConnectivityConfigChange(slotId, isConnectivityAllowed);
    }
}

void DataSettingsManagerStub::onDdsChange(DdsInfo currentState) {
    LOG(DEBUG, __FUNCTION__);

    std::vector<std::shared_ptr<IDataSettingsListener>> listeners;
    this->getAvailableListeners(listeners);
    for(auto& listener : listeners) {
        listener->onDdsChange(currentState);
    }
}

}  // namespace data
}  // namespace telux

