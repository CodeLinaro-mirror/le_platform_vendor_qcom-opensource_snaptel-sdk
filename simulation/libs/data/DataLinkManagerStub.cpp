/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "DataLinkManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DELAY 100
#define DEFAULT_DELIMITER " "
#define DATA_LINK_SSR_FILTER "data_link_ssr"
#define ETH_DATA_LINK_STATE_CHANGE_FILTER "eth_data_link_state_change"
#define ETH_MODE_EVENTS_FILTER "data_link_eth_mode"

namespace telux {
namespace data {

using telux::common::Status;

DataLinkManagerStub::DataLinkManagerStub()
   : SimulationManagerStub<DataLinkManager>(std::string("IDataLinkManagerStub"))
   , clientEventMgr_(ClientEventManager::getInstance()) {
    LOG(DEBUG, __FUNCTION__);
}

DataLinkManagerStub::~DataLinkManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void DataLinkManagerStub::createListener() {
    LOG(DEBUG, __FUNCTION__);

    listenerMgr_ = std::make_shared<telux::common::ListenerManager<IDataLinkListener>>();
}

void DataLinkManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void DataLinkManagerStub::setInitCbDelay(uint32_t cbDelay) {
    cbDelay_ = cbDelay;
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
}

uint32_t DataLinkManagerStub::getInitCbDelay() {
    LOG(DEBUG, __FUNCTION__, ":: cbDelay_: ", cbDelay_);
    return cbDelay_;
}

Status DataLinkManagerStub::init() {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::SUCCESS;

    try {
        createListener();
    } catch (std::bad_alloc &e) {
        LOG(ERROR, __FUNCTION__, ": Invalid listener instance");
        return Status::FAILED;
    }
    status = registerDefaultIndications();
    return status;
}

Status DataLinkManagerStub::registerDefaultIndications() {
    Status status = Status::FAILED;
    LOG(INFO, __FUNCTION__, ":: Registering default SSR indications");

    status = clientEventMgr_.registerListener(shared_from_this(), DATA_LINK_SSR_FILTER);
    if ((status != Status::SUCCESS) && (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default SSR indications failed");
        return status;
    }

    status
        = clientEventMgr_.registerListener(shared_from_this(), ETH_DATA_LINK_STATE_CHANGE_FILTER);
    if ((status != Status::SUCCESS) && (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering eth datalink state change indications failed");
        return status;
    }

    status = clientEventMgr_.registerListener(shared_from_this(), ETH_MODE_EVENTS_FILTER);
    if ((status != Status::SUCCESS) && (status != Status::ALREADY)) {
        LOG(ERROR, __FUNCTION__, ":: Registering default indications failed");
        return status;
    }
    return Status::SUCCESS;
}

void DataLinkManagerStub::notifyServiceStatus(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    if (srvcStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
        // Deregister optional indications on SSR (if any)
    }
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDataLinkListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "Data link Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(srvcStatus);
            }
        }
    }
}

ServiceStatus DataLinkManagerStub::getServiceStatus() {
    return SimulationManagerStub::getServiceStatus();
}

Status DataLinkManagerStub::initSyncComplete(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__);

    Status status = Status::FAILED;
    status        = registerDefaultIndications();

    return status;
}

void DataLinkManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    // Execute all events in separate thread
    auto f = std::async(std::launch::deferred, [this, event]() {
        if (event.Is<commonStub::GetServiceStatusReply>()) {
            handleSSREvent(event);
        } else if (event.Is<dataStub::OnEthDataLinkStateChangeReply>()) {
            handleEthDatalinkChangeEvent(event);
        } else if (event.Is<::dataStub::EthModeChangeRequestEvent>()) {
            ::dataStub::EthModeChangeRequestEvent indication;
            event.UnpackTo(&indication);
            this->handleOnEthModeChangeRequest(indication);
        } else if (event.Is<::dataStub::EthModeChangeTransactionStatusEvent>()) {
            ::dataStub::EthModeChangeTransactionStatusEvent indication;
            event.UnpackTo(&indication);
            this->handleOnEthModeChangeTransactionStatus(indication);
        } else {
            LOG(ERROR, __FUNCTION__, ":: Invalid event");
        }
    }).share();

    taskQ_.add(f);
}

void DataLinkManagerStub::handleEthDatalinkChangeEvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    dataStub::OnEthDataLinkStateChangeReply indication;
    event.UnpackTo(&indication);

    telux::data::LinkState linkState;

    auto ethLinkState = indication.eth_datalink_state().link_state();

    if (ethLinkState == ::dataStub::LinkStateEnum_LinkState_UP) {
        linkState = telux::data::LinkState::UP;
    } else if (ethLinkState == ::dataStub::LinkStateEnum_LinkState_DOWN) {
        linkState = telux::data::LinkState::DOWN;
    } else {
        // Ignore
        LOG(ERROR, __FUNCTION__, ":: INVALID eth link state event");
        return;
    }

    std::vector<std::weak_ptr<IDataLinkListener>> applisteners;
    listenerMgr_->getAvailableListeners(applisteners);
    LOG(DEBUG, __FUNCTION__, ":: Notifying eth data link state change event ",
        " to listeners: ", applisteners.size());

    for (auto &wp : applisteners) {
        if (auto sp = wp.lock()) {
            sp->onEthDataLinkStateChange(linkState);
        }
    }
}

void DataLinkManagerStub::handleSSREvent(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    commonStub::GetServiceStatusReply ssrResp;
    event.UnpackTo(&ssrResp);

    ServiceStatus srvcStatus = ServiceStatus::SERVICE_FAILED;

    if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_AVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_AVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_UNAVAILABLE) {
        srvcStatus = ServiceStatus::SERVICE_UNAVAILABLE;
    } else if (ssrResp.service_status() == commonStub::ServiceStatus::SERVICE_FAILED) {
        srvcStatus = ServiceStatus::SERVICE_FAILED;
    } else {
        // Ignore
        LOG(ERROR, __FUNCTION__, ":: INVALID SSR event");
        return;
    }
    setServiceReady(srvcStatus);
    onServiceStatusChange(srvcStatus);
}

void DataLinkManagerStub::onServiceStatusChange(ServiceStatus srvcStatus) {
    LOG(DEBUG, __FUNCTION__, ":: Service Status: ", static_cast<int>(srvcStatus));

    if (srvcStatus == getServiceStatus()) {
        return;
    }
    if (srvcStatus == ServiceStatus::SERVICE_UNAVAILABLE) {
        LOG(ERROR, __FUNCTION__, ":: Datalink Service is UNAVAILABLE");
        setServiceStatus(srvcStatus);
    } else {
        LOG(INFO, __FUNCTION__, ":: Datalink Service is AVAILABLE");
        auto f = std::async(std::launch::async, [this]() { this->initSync(); }).share();
        taskQ_.add(f);
    }
}

telux::common::Status DataLinkManagerStub::registerListener(
    std::weak_ptr<IDataLinkListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status DataLinkManagerStub::deregisterListener(
    std::weak_ptr<IDataLinkListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

telux::common::ErrorCode DataLinkManagerStub::setEthDataLinkState(
    telux::data::LinkState linkState) {
    LOG(DEBUG, __FUNCTION__);

    ::dataStub::SetEthDatalinkStateRequest request;
    ::dataStub::SetEthDatalinkStateReply response;
    ClientContext context;

    if (linkState == telux::data::LinkState::UP) {
        request.mutable_eth_datalink_state()->set_link_state(
            ::dataStub::LinkStateEnum_LinkState_UP);
    } else {
        request.mutable_eth_datalink_state()->set_link_state(
            ::dataStub::LinkStateEnum_LinkState_DOWN);
    }

    grpc::Status reqStatus = stub_->SetEthDataLinkState(&context, request, &response);

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, ", errorcode: ", static_cast<int>(reqStatus.error_code()));
            LOG(ERROR, __FUNCTION__, " seEthDataLinkState request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }

    return error;
}

telux::common::ErrorCode DataLinkManagerStub::getEthDataLinkState(
    telux::data::LinkState &ethLinkState) {
    LOG(DEBUG, __FUNCTION__);

    ::google::protobuf::Empty request;
    ::dataStub::GetEthDataLinkStateReply response;
    ClientContext context;
    ethLinkState           = telux::data::LinkState::UNKNOWN;
    grpc::Status reqStatus = stub_->GetEthDataLinkState(&context, request, &response);

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, ", errorcode: ", static_cast<int>(reqStatus.error_code()));
            LOG(ERROR, __FUNCTION__, " getEthDataLinkState request failed");
            return telux::common::ErrorCode::INTERNAL_ERROR;
        }

        switch (response.eth_datalink_state().link_state()) {
            case ::dataStub::LinkStateEnum_LinkState_UP:
                ethLinkState = telux::data::LinkState::UP;
                break;
            case ::dataStub::LinkStateEnum_LinkState_DOWN:
                ethLinkState = telux::data::LinkState::DOWN;
                break;
            default:
                ethLinkState = telux::data::LinkState::UNKNOWN;
                break;
        }
    }

    return error;
}

telux::common::Status DataLinkManagerStub::getEthCapability(
    telux::data::EthCapability &ethCapability) {
    LOG(DEBUG, __FUNCTION__);

    ::google::protobuf::Empty request;
    ::dataStub::GetEthCapabilityReply response;
    ClientContext context;

    grpc::Status status = stub_->GetEthCapability(&context, request, &response);

    if (!status.ok()) {
        LOG(ERROR, __FUNCTION__, " GetEthCapability request failed: ", status.error_message());
        return telux::common::Status::FAILED;
    }
    LOG(DEBUG, __FUNCTION__, " GetEthCapability response status: ", response.status());
    if (response.status() != commonStub::Status::SUCCESS) {
        return static_cast<telux::common::Status>(response.status());
    }
    ethCapability.ethModes = 0;
    for (const auto &mode : response.capability().eth_modes()) {
        ethCapability.ethModes |= (1 << mode);
    }

    return telux::common::Status::SUCCESS;
}

telux::common::Status DataLinkManagerStub::setPeerEthCapability(
    telux::data::EthCapability ethCapability) {
    LOG(DEBUG, __FUNCTION__);
    ::dataStub::SetPeerEthCapabilityRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    for (int i = 0; i < 32; ++i) {  // Iterate up to 31st bit to check each EthModeType
        if ((ethCapability.ethModes >> i) & 1) {
            request.mutable_capability()->add_eth_modes(static_cast<dataStub::EthModeEnum>(1 << i));
        }
    }

    grpc::Status reqStatus = stub_->SetPeerEthCapability(&context, request, &response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__,
            " SetPeerEthCapability request failed: ", reqStatus.error_message());
        return telux::common::Status::FAILED;
    }
    return static_cast<telux::common::Status>(response.status());
}

telux::common::Status DataLinkManagerStub::setLocalEthOperatingMode(
    telux::data::EthModeType ethModeType, telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    ::dataStub::SetLocalEthOperatingModeRequest request;
    ::dataStub::SetLocalEthOperatingModeReply response;
    grpc::ClientContext context;

    request.set_eth_mode(static_cast<::dataStub::EthModeEnum>(ethModeType));

    grpc::Status reqStatus = stub_->SetLocalEthOperatingMode(&context, request, &response);

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " SetLocalEthOperatingMode request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
            return telux::common::Status::FAILED;
        } else {
            if (listenerMgr_) {
                std::vector<std::weak_ptr<IDataLinkListener>> listeners;
                listenerMgr_->getAvailableListeners(listeners);
                LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
                for (auto &wp : listeners) {
                    if (auto sp = wp.lock()) {
                        LOG(DEBUG, "DataLink Manager: invoking onEthModeChangeRequest");
                        sp->onEthModeChangeRequest(ethModeType);
                    }
                }
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, " SetLocalEthOperatingMode error");
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status DataLinkManagerStub::setPeerModeChangeRequestStatus(
    LinkModeChangeStatus status) {

    LOG(DEBUG, __FUNCTION__);
    ::dataStub::SetPeerModeChangeRequestStatusRequest request;
    ::dataStub::DefaultReply response;
    ClientContext context;

    request.set_status(static_cast<dataStub::ModeChangeStatusEnum>(status));

    grpc::Status reqStatus = stub_->SetPeerModeChangeRequestStatus(&context, request, &response);

    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__,
            " SetPeerModeChangeRequestStatus request failed: ", reqStatus.error_message());
        return telux::common::Status::FAILED;
    }

    telux::common::Status serverStatus = static_cast<telux::common::Status>(response.status());

    if (serverStatus != telux::common::Status::SUCCESS) {
        return telux::common::Status::FAILED;
    }

    return static_cast<telux::common::Status>(response.status());
}

telux::data::EthModeType mapProtoEthModeToTelux(dataStub::EthModeEnum protoMode) {
    switch (protoMode) {
        case dataStub::EthModeEnum_USXGMII_10G:
            return telux::data::ETHMODE_USXGMII_10G;
        case dataStub::EthModeEnum_USXGMII_5G:
            return telux::data::ETHMODE_USXGMII_5G;
        case dataStub::EthModeEnum_USXGMII_2_5G:
            return telux::data::ETHMODE_USXGMII_2_5G;
        case dataStub::EthModeEnum_USXGMII_1G:
            return telux::data::ETHMODE_USXGMII_1G;
        case dataStub::EthModeEnum_USXGMII_100M:
            return telux::data::ETHMODE_USXGMII_100M;
        case dataStub::EthModeEnum_USXGMII_10M:
            return telux::data::ETHMODE_USXGMII_10M;
        case dataStub::EthModeEnum_SGMII_2_5G:
            return telux::data::ETHMODE_SGMII_2_5G;
        case dataStub::EthModeEnum_SGMII_1G:
            return telux::data::ETHMODE_SGMII_1G;
        case dataStub::EthModeEnum_SGMII_100M:
            return telux::data::ETHMODE_SGMII_100M;
        default:
            return telux::data::ETHMODE_UNKNOWN;
    }
}

telux::data::LinkModeChangeStatus mapProtoModeChangeStatusToTelux(
    dataStub::ModeChangeStatusEnum protoStatus) {
    switch (protoStatus) {
        case dataStub::ModeChangeStatusEnum_ACCEPTED:
            return telux::data::LinkModeChangeStatus::ACCEPTED;
        case dataStub::ModeChangeStatusEnum_COMPLETED:
            return telux::data::LinkModeChangeStatus::COMPLETED;
        case dataStub::ModeChangeStatusEnum_FAILED:
            return telux::data::LinkModeChangeStatus::FAILED;
        case dataStub::ModeChangeStatusEnum_REJECTED:
            return telux::data::LinkModeChangeStatus::REJECTED;
        case dataStub::ModeChangeStatusEnum_TIMEOUT:
            return telux::data::LinkModeChangeStatus::TIMEOUT;
        default:
            return telux::data::LinkModeChangeStatus::UNKNOWN;
    }
}

void DataLinkManagerStub::handleOnEthModeChangeRequest(
    ::dataStub::EthModeChangeRequestEvent indication) {
    LOG(DEBUG, __FUNCTION__);

    telux::data::EthModeType ethModeType = mapProtoEthModeToTelux(indication.eth_mode_type());

    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDataLinkListener>> applisteners;
        listenerMgr_->getAvailableListeners(applisteners);
        LOG(DEBUG, __FUNCTION__,
            ":: Notifying onEthModeChangeRequest event to listeners: ", applisteners.size());

        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onEthModeChangeRequest(ethModeType);
            }
        }
    }
}

void DataLinkManagerStub::handleOnEthModeChangeTransactionStatus(
    ::dataStub::EthModeChangeTransactionStatusEvent indication) {
    LOG(DEBUG, __FUNCTION__);

    telux::data::EthModeType ethModeType     = mapProtoEthModeToTelux(indication.eth_mode_type());
    telux::data::LinkModeChangeStatus status = mapProtoModeChangeStatusToTelux(indication.status());

    if (listenerMgr_) {
        std::vector<std::weak_ptr<IDataLinkListener>> applisteners;
        listenerMgr_->getAvailableListeners(applisteners);
        LOG(DEBUG, __FUNCTION__, "Notifying event to listeners: ", applisteners.size());

        for (auto &wp : applisteners) {
            if (auto sp = wp.lock()) {
                sp->onEthModeChangeTransactionStatus(ethModeType, status);
            }
        }
    }
}

}  // end of namespace data
}  // end of namespace telux
