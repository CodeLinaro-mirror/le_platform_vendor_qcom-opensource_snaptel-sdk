/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EthernetManagerStub.hpp"
//#include "EthernetEventListener.hpp"
#include "DataUtilsStub.hpp"
#include "DataEventListener.hpp"

#include "common/Logger.hpp"
#include "common/SimulationConfigParser.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1

namespace telux {
namespace data {

EthernetManagerStub::EthernetManagerStub()
{
    LOG(DEBUG, __FUNCTION__, " Initializing EthernetManagerStub");
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
}

EthernetManagerStub::~EthernetManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }

    //std::vector<std::string> filters = {ETHERNET_MANAGER_FILTER};
    //auto &clientEventManager = telux::common::ClientEventManager::getInstance();
    //clientEventManager.deregisterListener(eventListener_, filters);
}

telux::common::Status EthernetManagerStub::init(
    telux::common::InitResponseCb callback) {
    LOG(INFO, __FUNCTION__);
    auto f = std::async(std::launch::async,
             [this, callback]() {
                this->initSync(callback);
            }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void EthernetManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::EthernetManager>();

    ::google::protobuf::Empty request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

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
        //eventListener_ = std::make_shared<DataEventListener>(shared_from_this());
        //std::vector<std::string> filters = {ETHERNET_MANAGER_FILTER};
        //auto &clientEventManager = telux::common::ClientEventManager::getInstance();
        //clientEventManager.registerListener(eventListener_, filters);

    } while (0);

    bool isSubsystemReady = (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE)?
        true : false;
    setSubSystemStatus(cbStatus);
    setSubsystemReady(isSubsystemReady);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay,
            " cbStatus::", static_cast<int>(cbStatus));
        callback(cbStatus);
    }
}

void EthernetManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

void EthernetManagerStub::setSubsystemReady(bool status) {
    LOG(DEBUG, __FUNCTION__, " status: ", status);
    std::lock_guard<std::mutex> lk(mtx_);
    ready_ = status;
    cv_.notify_all();
}

telux::common::ServiceStatus EthernetManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

void EthernetManagerStub::invokeCallback(telux::common::ResponseCallback callback,
    telux::common::ErrorCode error, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback(error);
        }).share();
    taskQ_->add(f);
}

bool EthernetManagerStub::isSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    return ready_;
}

telux::common::Status EthernetManagerStub::setEthernetNicConfig(const EthConfig&  ethConfig,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Ethernet subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::dataStub::SetEthernetNicConfigRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;
/*
    // Manually copy fields from telux::data::EthConfig to dataStub::EthConfig
    auto* stubEthConfig = request.mutable_eth_config();
    stubEthConfig->set_mode(static_cast<::dataStub::EthModeEnum>(ethConfig.mode));
    for (const auto& nicConfig : ethConfig.vEthNicConfig) {
        auto* stubNicConfig = stubEthConfig->add_v_eth_nic_config();
        stubNicConfig->set_eth_iface_name(nicConfig.eth_iface_name());
        stubNicConfig->set_eth_nic_type(static_cast<::dataStub::EthNetworkTypeEnum>(nicConfig.eth_nic_type()));
    }
    for (const auto& macsecNicConfig : ethConfig.v_macsec_nic_config()) {
        auto* stubMacsecNicConfig = stubEthConfig->add_v_macsec_nic_config();
        stubMacsecNicConfig->set_state(static_cast<::dataStub::ConfigStateEnum>(macsecNicConfig.state));
        stubMacsecNicConfig->set_macsec_iface_name(macsecNicConfig.macsec_iface_name());
        stubMacsecNicConfig->set_macsec_mode(static_cast<::dataStub::MacsecModeEnum>(macsecNicConfig.macsec_mode));
        stubMacsecNicConfig->set_eth_nic_iface_name(macsecNicConfig.eth_nic_iface_name);
        stubMacsecNicConfig->set_mtu_size(macsecNicConfig.mtu_size());
    }
*/
    grpc::Status reqStatus = stub_->SetEthernetNicConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " SetEthernetNicConfig request failed");
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

telux::common::Status EthernetManagerStub::getEthernetNicConfig(EthConfigCb callback) {
    LOG(DEBUG, __FUNCTION__);

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Ethernet subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay = 2000;

    ::google::protobuf::Empty request;
    ::dataStub::GetEthernetNicConfigReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->GetEthernetNicConfig(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    //status = static_cast<telux::common::Status>(response.status());
    //delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " GetEthernetNicConfig request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        EthConfig ethConfig;
/*
        // Manually copy fields from dataStub::EthConfig to telux::data::EthConfig
        ethConfig.set_mode(static_cast<telux::data::EthModeEnum>(response.eth_config().mode));
        for (const auto& nicConfig : response.eth_config().v_eth_nic_config()) {
            auto* stubNicConfig = ethConfig.add_v_eth_nic_config();
            stubNicConfig->set_eth_iface_name(nicConfig.eth_iface_name());
            stubNicConfig->set_eth_nic_type(static_cast<telux::data::EthNetworkTypeEnum>(nicConfig.eth_nic_type));
        }
        for (const auto& macsecNicConfig : response.eth_config().v_macsec_nic_config()) {
            auto* stubMacsecNicConfig = ethConfig.add_v_macsec_nic_config();
            stubMacsecNicConfig->set_state(static_cast<telux::data::ConfigStateEnum>(macsecNicConfig.state()));
            stubMacsecNicConfig->set_macsec_iface_name(macsecNicConfig.macsec_iface_name());
            stubMacsecNicConfig->set_macsec_mode(static_cast<telux::data::MacsecModeEnum>(macsecNicConfig.macsec_mode()));
            stubMacsecNicConfig->set_eth_nic_iface_name(macsecNicConfig.eth_nic_iface_name());
            stubMacsecNicConfig->set_mtu_size(macsecNicConfig.mtu_size());
        }

*/
        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
             [this, ethConfig, error, delay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(ethConfig, error);
               }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status EthernetManagerStub::activateLAN(telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);

    if (!isSubsystemReady()) {
        LOG(ERROR, __FUNCTION__, " Ethernet subsystem not ready");
        return telux::common::Status::NOTREADY;
    }

    telux::common::ErrorCode error = telux::common::ErrorCode::SUCCESS;
    telux::common::Status status = telux::common::Status::SUCCESS;
    int delay;

    ::google::protobuf::Empty request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    grpc::Status reqStatus = stub_->ActivateLAN(&context, request, &response);

    error = static_cast<telux::common::ErrorCode>(response.error());
    status = static_cast<telux::common::Status>(response.status());
    delay = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " ActivateLAN request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }

        if (callback && (delay != SKIP_CALLBACK)) {
            auto f = std::async(std::launch::async,
             [this, error, delay, callback]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    callback(error);
               }).share();
            taskQ_->add(f);
        }
    }

    return status;
}

telux::common::Status EthernetManagerStub::registerListener(std::weak_ptr<IEthernetListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> listenerLock(mtx_);
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

telux::common::Status EthernetManagerStub::deregisterListener(std::weak_ptr<IEthernetListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(mtx_);
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

telux::data::OperationType EthernetManagerStub::getOperationType() {
    LOG(DEBUG, __FUNCTION__);
    return OperationType::DATA_LOCAL;
}

void EthernetManagerStub::handleSetEthernetNicConfigEvent(const ::dataStub::SetEthernetNicConfigRequest& event) {
    LOG(DEBUG, __FUNCTION__);
}

void EthernetManagerStub::handleGetEthernetNicConfigEvent(const ::dataStub::GetEthernetNicConfigReply& event) {
    LOG(DEBUG, __FUNCTION__);
}

void EthernetManagerStub::handleActivateLANEvent(const ::dataStub::DefaultReply& event) {
    LOG(DEBUG, __FUNCTION__);
}

void EthernetManagerStub::getAvailableListeners(
    std::vector<std::shared_ptr<IEthernetListener>> &listeners) {
    LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners_.size());
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto it = listeners_.begin(); it != listeners_.end();) {
        auto sp = (*it).lock();
        if (sp) {
            listeners.emplace_back(sp);
            ++it;
        } else {
            LOG(DEBUG, "erased obsolete weak pointer from EthernetManagerImpl's listeners");
            it = listeners_.erase(it);
        }
    }
}

void EthernetManagerStub::invokeEthernetListener(std::shared_ptr<IEthernetManager> manager) {
    LOG(DEBUG, __FUNCTION__);
}

void EthernetManagerStub::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::vector<std::shared_ptr<IEthernetListener>> applisteners;
    this->getAvailableListeners(applisteners);
    for (auto &listener : applisteners) {
        listener->onServiceStatusChange(status);
    }
}

} // end of namespace data
} // end of namespace telux
