/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <thread>

#include "DataEventListener.hpp"
#include "QoSManagerStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include "data/TrafficFilterImpl.hpp"
#include <arpa/inet.h>
#include <bitset>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define DATA_QOS_FILTER "qos_filter"

namespace telux {
namespace data {
namespace net {

BandwidthConfig QoSManagerStub::protoToSdkBandwidthConfig(
    const dataStub::BandwidthConfig &protoConfig) {
    BandwidthConfig sdkConfig;
    sdkConfig.dlBandwidthConfigType
        = static_cast<net::BandwidthConfigType>(protoConfig.dl_bandwidth_config_type());
    sdkConfig.dlBandwidthValue.bandwidthRange.minBandwidth
        = protoConfig.dl_bandwidth_value().min_bandwidth();
    sdkConfig.dlBandwidthValue.bandwidthRange.maxBandwidth
        = protoConfig.dl_bandwidth_value().max_bandwidth();
    return sdkConfig;
}

dataStub::BandwidthConfig QoSManagerStub::sdkToProtoBandwidthConfig(
    const BandwidthConfig &sdkConfig) {
    dataStub::BandwidthConfig protoConfig;
    protoConfig.set_dl_bandwidth_config_type(
        static_cast<dataStub::BandwidthConfigType>(sdkConfig.dlBandwidthConfigType));
    protoConfig.mutable_dl_bandwidth_value()->set_min_bandwidth(
        sdkConfig.dlBandwidthValue.bandwidthRange.minBandwidth);
    protoConfig.mutable_dl_bandwidth_value()->set_max_bandwidth(
        sdkConfig.dlBandwidthValue.bandwidthRange.maxBandwidth);
    return protoConfig;
}

std::shared_ptr<ITcConfig> QoSManagerStub::protoToSdkTcConfig(dataStub::ITcConfig &protoTcConfig) {
    std::shared_ptr<TcConfigImpl> sdkTcConfig = std::make_shared<TcConfigImpl>();
    sdkTcConfig->setTrafficClass(static_cast<TrafficClass>(protoTcConfig.traffic_class()));
    sdkTcConfig->setDirection(static_cast<Direction>(protoTcConfig.direction()));
    sdkTcConfig->setDataPath(static_cast<DataPath>(protoTcConfig.data_path()));
    if (protoTcConfig.has_bandwidth_config()) {
        sdkTcConfig->setBandwidthConfig(
            protoToSdkBandwidthConfig(protoTcConfig.bandwidth_config()));
    }
    return sdkTcConfig;
}

dataStub::ITcConfig QoSManagerStub::sdkToProtoTcConfig(std::shared_ptr<ITcConfig> sdkTcConfig) {
    dataStub::ITcConfig protoTcConfig;
    protoTcConfig.set_validity_mask(sdkTcConfig->getTcConfigValidFields());
    protoTcConfig.set_traffic_class(static_cast<uint32_t>(sdkTcConfig->getTrafficClass()));
    protoTcConfig.set_direction(static_cast<dataStub::Direction>(sdkTcConfig->getDirection()));
    protoTcConfig.set_data_path(static_cast<dataStub::DataPath>(sdkTcConfig->getDataPath()));
    if (sdkTcConfig->getTcConfigValidFields() & TcConfigValidField::TC_BANDWIDTH_CONFIG_VALID) {
        *protoTcConfig.mutable_bandwidth_config()
            = sdkToProtoBandwidthConfig(sdkTcConfig->getBandwidthConfig());
    }
    return protoTcConfig;
}

net::QoSFilterStatus QoSManagerStub::protoToSdkQoSFilterStatus(
    const dataStub::QoSFilterStatus &protoStatus) {
    net::QoSFilterStatus sdkStatus;
    sdkStatus.ethStatus   = static_cast<net::FilterInstallationStatus>(protoStatus.eth_status());
    sdkStatus.modemStatus = static_cast<net::FilterInstallationStatus>(protoStatus.modem_status());
    sdkStatus.ipaStatus   = static_cast<net::FilterInstallationStatus>(protoStatus.ipa_status());
    return sdkStatus;
}

bool QoSManagerStub::isValidIPv4Address(const std::string &ipAddress) {
    if (ipAddress.empty()) {
        return false;
    }
    struct in_addr sa;
    return inet_pton(AF_INET, ipAddress.c_str(), &(sa)) != 0;
}

bool QoSManagerStub::isValidIPv6Address(const std::string &ipAddress) {
    if (ipAddress.empty()) {
        return false;
    }
    struct in6_addr sa;
    return inet_pton(AF_INET6, ipAddress.c_str(), &(sa)) != 0;
}

// ITrafficFilter conversion (simplified, assuming TrafficFilterImpl handles
// nested types)
QoSFilterErrorCode QoSManagerStub::sdkToProtoTrafficFilter(
    std::shared_ptr<ITrafficFilter> sdkFilter, dataStub::ITrafficFilter &protoFilter) {
    if (sdkFilter) {
        // Cast to TrafficFilterImpl to access specific fields
        std::shared_ptr<TrafficFilterImpl> trafficFilterImpl
            = std::static_pointer_cast<TrafficFilterImpl>(sdkFilter);
        TrafficFilterValidFields validityMask = trafficFilterImpl->getTrafficFilterValidFields();
        protoFilter.set_validity_mask(validityMask);

        if (validityMask & TrafficFilterValidField::TF_DIRECTION_VALID) {
            protoFilter.set_direction(
                static_cast<dataStub::Direction>(trafficFilterImpl->getDirection()));
        } else {
            return QoSFilterErrorCode::MISSING_DIRECTION;
        }

        const TrafficFilterValidFields sourceInfoMask
            = TrafficFilterValidField::TF_SOURCE_IPV4_ADDRESS_VALID
              | TrafficFilterValidField::TF_SOURCE_IPV6_ADDRESS_VALID
              | TrafficFilterValidField::TF_SOURCE_VLAN_LIST_VALID;

        if (std::bitset<sizeof(TrafficFilterValidFields) * 8>(validityMask & sourceInfoMask).count()
            > 1) {
            return QoSFilterErrorCode::INVALID_MULTIPLE_SOURCE_INFO;
        }

        const TrafficFilterValidFields destInfoMask
            = TrafficFilterValidField::TF_DESTINATION_IPV4_ADDRESS_VALID
              | TrafficFilterValidField::TF_DESTINATION_IPV6_ADDRESS_VALID
              | TrafficFilterValidField::TF_DESTINATION_VLAN_LIST_VALID;

        if (std::bitset<sizeof(TrafficFilterValidFields) * 8>(validityMask & destInfoMask).count()
            > 1) {
            return QoSFilterErrorCode::INVALID_MULTIPLE_DESTINATION_INFO;
        }

        if (validityMask
            & (TrafficFilterValidField::TF_SOURCE_IPV4_ADDRESS_VALID
                | TrafficFilterValidField::TF_SOURCE_IPV6_ADDRESS_VALID
                | TrafficFilterValidField::TF_SOURCE_VLAN_LIST_VALID))
            if (validityMask & TrafficFilterValidField::TF_IP_PROTOCOL_VALID) {
                protoFilter.set_ip_protocol(
                    static_cast<uint32_t>(trafficFilterImpl->getIPProtocol()));
            }
        if (validityMask & TrafficFilterValidField::TF_PCP_VALID) {
            int8_t pcpValue = trafficFilterImpl->getPCP();
            if (pcpValue < 1 || pcpValue > 7) {
                throw std::runtime_error(" PCP value must be between 1 and 7 (PCP 0 is reserved)");
            }
            protoFilter.set_pcp(static_cast<int32_t>(pcpValue));
        }
        if (validityMask & TrafficFilterValidField::TF_DATA_PATH_VALID) {
            protoFilter.set_data_path(
                static_cast<dataStub::DataPath>(trafficFilterImpl->getDataPath()));
        }

        // Source Fields
        if (validityMask & TrafficFilterValidField::TF_SOURCE_IPV4_ADDRESS_VALID) {
            std::string ipv4Addr = trafficFilterImpl->getIPv4Address(FieldType::SOURCE);
            if (!isValidIPv4Address(ipv4Addr)) {  // Robust validation
                throw std::runtime_error(" Invalid source IPv4 address format.");
            }
            protoFilter.set_source_ipv4_address(ipv4Addr);
        }
        if (validityMask & TrafficFilterValidField::TF_SOURCE_IPV6_ADDRESS_VALID) {
            std::string ipv6Addr = trafficFilterImpl->getIPv6Address(FieldType::SOURCE);
            if (!isValidIPv6Address(ipv6Addr)) {  // Robust validation
                throw std::runtime_error(" Invalid source IPv6 address format.");
            }
            protoFilter.set_source_ipv6_address(ipv6Addr);
        }
        if (validityMask & TrafficFilterValidField::TF_SOURCE_PORT_VALID) {
            protoFilter.set_source_port(trafficFilterImpl->getPort(FieldType::SOURCE));
        }
        if (validityMask & TrafficFilterValidField::TF_SOURCE_PORT_RANGE_VALID) {
            uint16_t startPort, range;
            trafficFilterImpl->getPortRange(FieldType::SOURCE, startPort, range);
            protoFilter.set_source_start_port(startPort);
            protoFilter.set_source_port_range(range);
        }
        if (validityMask & TrafficFilterValidField::TF_SOURCE_PORT_CONFIG_VALID) {
            PortConfig sdkPortConfig = trafficFilterImpl->getPortConfig(FieldType::SOURCE);
            dataStub::PortConfig *protoPortConfig = protoFilter.mutable_source_port_config();
            protoPortConfig->set_port(sdkPortConfig.port);
            protoPortConfig->set_range(sdkPortConfig.range);
            protoPortConfig->set_max_active_connections(sdkPortConfig.maxActiveConnections);
        }
        if (validityMask & TrafficFilterValidField::TF_SOURCE_VLAN_LIST_VALID) {
            for (int vlan : trafficFilterImpl->getVlanList(FieldType::SOURCE)) {
                protoFilter.add_source_vlan_list(vlan);
            }
        }

        // Destination Fields
        if (validityMask & TrafficFilterValidField::TF_DESTINATION_IPV4_ADDRESS_VALID) {
            std::string ipv4Addr = trafficFilterImpl->getIPv4Address(FieldType::DESTINATION);
            if (!isValidIPv4Address(ipv4Addr)) {  // Robust validation
                throw std::runtime_error(" Invalid destination IPv4 address format.");
            }
            protoFilter.set_dest_ipv4_address(ipv4Addr);
        }
        if (validityMask & TrafficFilterValidField::TF_DESTINATION_IPV6_ADDRESS_VALID) {
            std::string ipv6Addr = trafficFilterImpl->getIPv6Address(FieldType::DESTINATION);
            if (!isValidIPv6Address(ipv6Addr)) {  // Robust validation
                throw std::runtime_error(" Invalid destination IPv6 address format.");
            }
            protoFilter.set_dest_ipv6_address(ipv6Addr);
        }
        if (validityMask & TrafficFilterValidField::TF_DESTINATION_PORT_VALID) {
            protoFilter.set_dest_port(trafficFilterImpl->getPort(FieldType::DESTINATION));
        }
        if (validityMask & TrafficFilterValidField::TF_DESTINATION_PORT_RANGE_VALID) {
            uint16_t startPort, range;
            trafficFilterImpl->getPortRange(FieldType::DESTINATION, startPort, range);
            protoFilter.set_dest_start_port(startPort);
            protoFilter.set_dest_port_range(range);
        }
        if (validityMask & TrafficFilterValidField::TF_DESTINATION_PORT_CONFIG_VALID) {
            PortConfig sdkPortConfig = trafficFilterImpl->getPortConfig(FieldType::DESTINATION);
            dataStub::PortConfig *protoPortConfig = protoFilter.mutable_dest_port_config();
            protoPortConfig->set_port(sdkPortConfig.port);
            protoPortConfig->set_range(sdkPortConfig.range);
            protoPortConfig->set_max_active_connections(sdkPortConfig.maxActiveConnections);
        }
        if (validityMask & TrafficFilterValidField::TF_DESTINATION_VLAN_LIST_VALID) {
            for (int vlan : trafficFilterImpl->getVlanList(FieldType::DESTINATION)) {
                protoFilter.add_dest_vlan_list(vlan);
            }
        }
    }
    return QoSFilterErrorCode::SUCCESS;
}

std::shared_ptr<ITrafficFilter> QoSManagerStub::protoToSdkTrafficFilter(
    const dataStub::ITrafficFilter &protoFilter) {
    TrafficFilterBuilder builder;

    // Set properties based on validity mask from the proto message
    TrafficFilterValidFields validityMask = protoFilter.validity_mask();

    // Direction, IP Protocol, PCP, DataPath are not field-type specific, set them
    // directly
    if (validityMask & TrafficFilterValidField::TF_DIRECTION_VALID) {
        builder.setDirection(static_cast<Direction>(protoFilter.direction()));
    }
    if (validityMask & TrafficFilterValidField::TF_IP_PROTOCOL_VALID) {
        builder.setIPProtocol(static_cast<IpProtocol>(protoFilter.ip_protocol()));
    }
    if (validityMask & TrafficFilterValidField::TF_PCP_VALID) {
        builder.setPCP(static_cast<int8_t>(protoFilter.pcp()));
    }
    if (validityMask & TrafficFilterValidField::TF_DATA_PATH_VALID) {
        builder.setDataPath(static_cast<DataPath>(protoFilter.data_path()));
    }

    // Source Fields
    if (validityMask & TrafficFilterValidField::TF_SOURCE_IPV4_ADDRESS_VALID) {
        builder.setIPv4Address(protoFilter.source_ipv4_address(), FieldType::SOURCE);
    }
    if (validityMask & TrafficFilterValidField::TF_SOURCE_IPV6_ADDRESS_VALID) {
        builder.setIPv6Address(protoFilter.source_ipv6_address(), FieldType::SOURCE);
    }
    if (validityMask & TrafficFilterValidField::TF_SOURCE_PORT_VALID) {
        builder.setPort(static_cast<uint16_t>(protoFilter.source_port()), FieldType::SOURCE);
    }
    if (validityMask & TrafficFilterValidField::TF_SOURCE_PORT_RANGE_VALID) {
        builder.setPortRange(static_cast<uint16_t>(protoFilter.source_start_port()),
            static_cast<uint16_t>(protoFilter.source_port_range()), FieldType::SOURCE);
    }
    if (validityMask & TrafficFilterValidField::TF_SOURCE_PORT_CONFIG_VALID) {
        PortConfig sdkPortConfig;
        sdkPortConfig.port  = static_cast<uint16_t>(protoFilter.source_port_config().port());
        sdkPortConfig.range = static_cast<uint16_t>(protoFilter.source_port_config().range());
        sdkPortConfig.maxActiveConnections
            = static_cast<uint16_t>(protoFilter.source_port_config().max_active_connections());
        builder.setPortConfig(sdkPortConfig, FieldType::SOURCE);
    }
    if (validityMask & TrafficFilterValidField::TF_SOURCE_VLAN_LIST_VALID) {
        // protoFilter.source_vlan_list() returns a repeated field, which can be
        // directly used to initialize a std::vector
        std::vector<int> vlanList(
            protoFilter.source_vlan_list().begin(), protoFilter.source_vlan_list().end());
        builder.setVlanList(vlanList, FieldType::SOURCE);
    }

    // Destination Fields
    if (validityMask & TrafficFilterValidField::TF_DESTINATION_IPV4_ADDRESS_VALID) {
        builder.setIPv4Address(protoFilter.dest_ipv4_address(), FieldType::DESTINATION);
    }
    if (validityMask & TrafficFilterValidField::TF_DESTINATION_IPV6_ADDRESS_VALID) {
        builder.setIPv6Address(protoFilter.dest_ipv6_address(), FieldType::DESTINATION);
    }
    if (validityMask & TrafficFilterValidField::TF_DESTINATION_PORT_VALID) {
        builder.setPort(static_cast<uint16_t>(protoFilter.dest_port()), FieldType::DESTINATION);
    }
    if (validityMask & TrafficFilterValidField::TF_DESTINATION_PORT_RANGE_VALID) {
        builder.setPortRange(static_cast<uint16_t>(protoFilter.dest_start_port()),
            static_cast<uint16_t>(protoFilter.dest_port_range()), FieldType::DESTINATION);
    }
    if (validityMask & TrafficFilterValidField::TF_DESTINATION_PORT_CONFIG_VALID) {
        PortConfig sdkPortConfig;
        sdkPortConfig.port  = static_cast<uint16_t>(protoFilter.dest_port_config().port());
        sdkPortConfig.range = static_cast<uint16_t>(protoFilter.dest_port_config().range());
        sdkPortConfig.maxActiveConnections
            = static_cast<uint16_t>(protoFilter.dest_port_config().max_active_connections());
        builder.setPortConfig(sdkPortConfig, FieldType::DESTINATION);
    }
    if (validityMask & TrafficFilterValidField::TF_DESTINATION_VLAN_LIST_VALID) {
        std::vector<int> vlanList(
            protoFilter.dest_vlan_list().begin(), protoFilter.dest_vlan_list().end());
        builder.setVlanList(vlanList, FieldType::DESTINATION);
    }

    return builder.build();  // Build and return the ITrafficFilter object
}

std::shared_ptr<IQoSFilter> QoSManagerStub::protoToSdkQoSFilter(
    const dataStub::IQoSFilter &protoQoSFilter) {
    std::shared_ptr<QoSFilterImpl> sdkQoSFilter = std::make_shared<QoSFilterImpl>();
    sdkQoSFilter->setHandle(protoQoSFilter.handle());
    sdkQoSFilter->setTrafficClass(static_cast<TrafficClass>(protoQoSFilter.traffic_class()));
    sdkQoSFilter->setStatus(protoToSdkQoSFilterStatus(protoQoSFilter.status()));
    sdkQoSFilter->setTrafficFilter(protoToSdkTrafficFilter(protoQoSFilter.traffic_filter()));
    return sdkQoSFilter;
}

QoSManagerStub::QoSManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_           = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_     = std::make_shared<telux::common::ListenerManager<IQoSListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
}

QoSManagerStub::~QoSManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status QoSManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    initCb_ = callback;
    auto f
        = std::async(std::launch::async, [this, callback]() { this->initSync(callback); }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void QoSManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::QoSManager>();

    ::dataStub::InitRequest request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_operation_type(::dataStub::OperationType(OperationType::DATA_LOCAL));
    grpc::Status reqStatus                = stub_->InitService(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    int cbDelay                           = DEFAULT_DELAY;

    do {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " InitService request failed");
            break;
        }

        cbStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay  = static_cast<int>(response.delay());

        std::vector<std::string> filters = {DATA_QOS_FILTER};
        telux::common::Status status
            = telux::common::ClientEventManager::getInstance().registerListener(
                shared_from_this(), filters);
        LOG(INFO, __FUNCTION__, " event registration status : ", static_cast<int>(status));

        this->onServiceStatusChange(cbStatus);
        LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
    } while (0);

    setSubSystemStatus(cbStatus);

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

void QoSManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
}

void QoSManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus QoSManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return subSystemStatus_;
}

telux::common::Status QoSManagerStub::registerListener(std::weak_ptr<IQoSListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status QoSManagerStub::deregisterListener(std::weak_ptr<IQoSListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void QoSManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IQoSListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "QoS Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::ErrorCode QoSManagerStub::addQoSFilter(QoSFilterConfig qosFilterConfig,
    QoSFilterHandle &filterHandle, QoSFilterErrorCode &qosFilterErrorCode) {
    LOG(DEBUG, __FUNCTION__);

    // Check if stub is initialized
    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
        // Set specific QoSFilterErrorCode to SUCCESS, as there's no suitable error
        // defined in its enum for this case. The actual error is conveyed by the
        // telux::common::ErrorCode return.
        qosFilterErrorCode = net::QoSFilterErrorCode::SUCCESS;
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::dataStub::AddQoSFilterRequest request;
    ::dataStub::AddQoSFilterReply response;
    grpc::ClientContext context;

    // Convert SDK QoSFilterConfig to Protobuf AddQoSFilterRequest
    request.set_traffic_class(static_cast<uint32_t>(qosFilterConfig.trafficClass));
    if (qosFilterConfig.trafficFilter) {
        try {
            qosFilterErrorCode = sdkToProtoTrafficFilter(
                qosFilterConfig.trafficFilter, *request.mutable_traffic_filter());
        } catch (const std::runtime_error &e) {
            LOG(ERROR, __FUNCTION__, "Unexpected error converting traffic filter: ", e.what());
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
        // Use the sdkToProtoTrafficFilter helper to convert the traffic filter
        if (qosFilterErrorCode != net::QoSFilterErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " qosFilterErrorCode: ", (int)qosFilterErrorCode);
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
    }

    grpc::Status grpcStatus = stub_->AddQoSFilter(&context, request, &response);

    if (grpcStatus.ok()) {
        // Server provided QoSFilterErrorCode is correctly mapped
        qosFilterErrorCode = static_cast<net::QoSFilterErrorCode>(response.qos_filter_error_code());
        if (response.reply().error() == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
            filterHandle = response.filter_handle();
            return telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__, " addQoSFilter failed with Telux common error: ",
                static_cast<int>(response.reply().error()));
            return static_cast<telux::common::ErrorCode>(response.reply().error());
        }
    } else {
        LOG(ERROR, __FUNCTION__, " gRPC AddQoSFilter failed: ", grpcStatus.error_code(), ": ",
            grpcStatus.error_message());
        // Set specific QoSFilterErrorCode to SUCCESS, as there's no suitable error
        // defined in its enum for this case. The actual error is conveyed by the
        // telux::common::ErrorCode return.
        qosFilterErrorCode = net::QoSFilterErrorCode::SUCCESS;
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }
}

telux::common::ErrorCode QoSManagerStub::getQosFilter(
    QoSFilterHandle filterHandle, std::shared_ptr<IQoSFilter> &qosFilter) {
    LOG(DEBUG, __FUNCTION__);

    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " QoS Manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::dataStub::GetQosFilterRequest request;
    ::dataStub::GetQosFilterReply response;
    grpc::ClientContext context;

    request.set_filter_handle(filterHandle);

    grpc::Status grpcStatus = stub_->GetQosFilter(&context, request, &response);

    if (grpcStatus.ok()) {
        telux::common::ErrorCode teluxError
            = static_cast<telux::common::ErrorCode>(response.reply().error());
        if (teluxError == telux::common::ErrorCode::SUCCESS) {
            qosFilter = protoToSdkQoSFilter(response.qos_filter());
            return telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__,
                " getQosFilter failed with Telux common error: ", static_cast<int>(teluxError));
            return teluxError;
        }
    } else {
        LOG(ERROR, __FUNCTION__, " gRPC GetQosFilter failed: ", grpcStatus.error_code(), ": ",
            grpcStatus.error_message());
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }
}

telux::common::ErrorCode QoSManagerStub::getQosFilters(
    std::vector<std::shared_ptr<IQoSFilter>> &qosFilter) {
    LOG(DEBUG, __FUNCTION__);

    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " QoS Manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;  // Request for all filters
    ::dataStub::GetQosFiltersReply response;
    grpc::ClientContext context;

    grpc::Status grpcStatus = stub_->GetQosFilters(&context, request, &response);

    if (grpcStatus.ok()) {
        telux::common::ErrorCode teluxError
            = static_cast<telux::common::ErrorCode>(response.reply().error());
        if (teluxError == telux::common::ErrorCode::SUCCESS) {
            for (const auto &protoQoSFilter : response.qos_filters()) {
                qosFilter.push_back(protoToSdkQoSFilter(protoQoSFilter));
            }
            return telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__,
                " getQosFilters failed with Telux common error: ", static_cast<int>(teluxError));
            return teluxError;
        }
    } else {
        LOG(ERROR, __FUNCTION__, " gRPC GetQosFilters failed: ", grpcStatus.error_code(), ": ",
            grpcStatus.error_message());
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }
}

telux::common::ErrorCode QoSManagerStub::deleteQosFilter(uint32_t policyHandle) {
    LOG(DEBUG, __FUNCTION__);

    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " QoS Manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::dataStub::DeleteQosFilterRequest request;
    ::dataStub::DefaultReply response;
    grpc::ClientContext context;

    request.set_filter_handle(policyHandle);

    grpc::Status grpcStatus = stub_->DeleteQosFilter(&context, request, &response);

    if (grpcStatus.ok()) {
        telux::common::ErrorCode teluxError
            = static_cast<telux::common::ErrorCode>(response.error());
        if (teluxError == telux::common::ErrorCode::SUCCESS) {
            return telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__,
                " deleteQosFilter failed with Telux common error: ", static_cast<int>(teluxError));
            return teluxError;
        }
    } else {
        LOG(ERROR, __FUNCTION__, " gRPC DeleteQosFilter failed: ", grpcStatus.error_code(), ": ",
            grpcStatus.error_message());
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }
}

telux::common::ErrorCode QoSManagerStub::deleteAllQosConfigs() {
    LOG(DEBUG, __FUNCTION__);

    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " QoS Manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    ::dataStub::DefaultReply response;
    grpc::ClientContext context;

    grpc::Status grpcStatus = stub_->DeleteAllQosConfigs(&context, request, &response);

    if (grpcStatus.ok()) {
        telux::common::ErrorCode teluxError
            = static_cast<telux::common::ErrorCode>(response.error());
        if (teluxError == telux::common::ErrorCode::SUCCESS) {
            return telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__, " deleteAllQosConfigs failed with Telux common error: ",
                static_cast<int>(teluxError));
            return teluxError;
        }
    } else {
        LOG(ERROR, __FUNCTION__, " gRPC DeleteAllQosConfigs failed: ", grpcStatus.error_code(),
            ": ", grpcStatus.error_message());
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }
}

telux::common::ErrorCode QoSManagerStub::createTrafficClass(
    std::shared_ptr<ITcConfig> tcConfig, TcConfigErrorCode &tcConfigErrorCode) {
    LOG(DEBUG, __FUNCTION__);

    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
        // Set specific TcConfigErrorCode to SUCCESS, as there's no suitable error
        // defined in its enum for this case. The actual error is conveyed by the
        // telux::common::ErrorCode return.
        tcConfigErrorCode = net::TcConfigErrorCode::SUCCESS;
        return telux::common::ErrorCode::INVALID_STATE;
    }

    ::dataStub::CreateTrafficClassRequest request;
    ::dataStub::CreateTrafficClassReply response;
    grpc::ClientContext context;

    // Convert SDK ITcConfig to Protobuf ITcConfig
    if (tcConfig) {
        *request.mutable_tc_config() = sdkToProtoTcConfig(tcConfig);
    } else {
        LOG(ERROR, __FUNCTION__, " tcConfig is null.");
        // Set specific TcConfigErrorCode to SUCCESS, as there's no suitable error
        // defined in its enum for this case. The actual error is conveyed by the
        // telux::common::ErrorCode return.
        tcConfigErrorCode = net::TcConfigErrorCode::SUCCESS;
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    grpc::Status grpcStatus = stub_->CreateTrafficClass(&context, request, &response);

    if (grpcStatus.ok()) {
        // Server provided TcConfigErrorCode is correctly mapped
        tcConfigErrorCode = static_cast<net::TcConfigErrorCode>(response.tc_config_error_code());
        if (response.reply().error() == commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
            return telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__, " createTrafficClass failed with Telux common error: ",
                static_cast<int>(response.reply().error()));
            return static_cast<telux::common::ErrorCode>(response.reply().error());
        }
    } else {
        LOG(ERROR, __FUNCTION__, " gRPC CreateTrafficClass failed: ", grpcStatus.error_code(), ": ",
            grpcStatus.error_message());
        // Set specific TcConfigErrorCode to SUCCESS, as there's no suitable error
        // defined in its enum for this case. The actual error is conveyed by the
        // telux::common::ErrorCode return.
        tcConfigErrorCode = net::TcConfigErrorCode::SUCCESS;
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }
}

telux::common::ErrorCode QoSManagerStub::getAllTrafficClasses(
    std::vector<std::shared_ptr<ITcConfig>> &tcConfigs) {
    LOG(DEBUG, __FUNCTION__);

    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " QoS Manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::google::protobuf::Empty request;
    ::dataStub::GetAllTrafficClassesReply response;
    grpc::ClientContext context;

    grpc::Status grpcStatus = stub_->GetAllTrafficClasses(&context, request, &response);

    if (grpcStatus.ok()) {
        telux::common::ErrorCode teluxError
            = static_cast<telux::common::ErrorCode>(response.reply().error());
        if (teluxError == telux::common::ErrorCode::SUCCESS) {
            for (const auto &protoTcConfig : response.tc_configs()) {
                tcConfigs.push_back(
                    protoToSdkTcConfig(const_cast<dataStub::ITcConfig &>(protoTcConfig)));
            }
            return telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__, " getAllTrafficClasses failed with Telux common error: ",
                static_cast<int>(teluxError));
            return teluxError;
        }
    } else {
        LOG(ERROR, __FUNCTION__, " gRPC GetAllTrafficClasses failed: ", grpcStatus.error_code(),
            ": ", grpcStatus.error_message());
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }
}

telux::common::ErrorCode QoSManagerStub::deleteTrafficClass(std::shared_ptr<ITcConfig> tcConfig) {
    LOG(DEBUG, __FUNCTION__);

    if (!stub_) {
        LOG(ERROR, __FUNCTION__, " gRPC stub not initialized.");
        return telux::common::ErrorCode::INVALID_STATE;
    }

    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " QoS Manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ::dataStub::DeleteTrafficClassRequest request;
    ::dataStub::DefaultReply response;
    grpc::ClientContext context;

    // Convert SDK ITcConfig to Protobuf ITcConfig
    if (tcConfig) {
        *request.mutable_tc_config() = sdkToProtoTcConfig(tcConfig);
    } else {
        LOG(ERROR, __FUNCTION__, " tcConfig is null.");
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    grpc::Status grpcStatus = stub_->DeleteTrafficClass(&context, request, &response);

    if (grpcStatus.ok()) {
        telux::common::ErrorCode teluxError
            = static_cast<telux::common::ErrorCode>(response.error());
        if (teluxError == telux::common::ErrorCode::SUCCESS) {
            return telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(ERROR, __FUNCTION__, " deleteTrafficClass failed with Telux common error: ",
                static_cast<int>(teluxError));
            return teluxError;
        }
    } else {
        LOG(ERROR, __FUNCTION__, " gRPC DeleteTrafficClass failed: ", grpcStatus.error_code(), ": ",
            grpcStatus.error_message());
        return telux::common::ErrorCode::INTERNAL_ERROR;
    }
}

void QoSManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::dataStub::QoSFilterStatusChangeEvent>()) {
        ::dataStub::QoSFilterStatusChangeEvent qosFilterStatusChangeEvent;
        event.UnpackTo(&qosFilterStatusChangeEvent);
        handleQoSFilterStatusChangeEvent(qosFilterStatusChangeEvent);
    }
}

void QoSManagerStub::handleStartDataCallEvent(::dataStub::StartDataCallEvent startEvent) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<IQoSFilter>> qosFilters;
    getQosFilters(qosFilters);
    for (auto filter : qosFilters) {
        if (filter->getStatus().modemStatus == FilterInstallationStatus::PENDING) {
            QoSFilterStatus status = filter->getStatus();
            status.modemStatus     = FilterInstallationStatus::SUCCESS;
            std::shared_ptr<QoSFilterImpl> filterUpdate
                = std::static_pointer_cast<QoSFilterImpl>(filter);
            filterUpdate->setStatus(status);
            if (listenerMgr_) {
                std::vector<std::weak_ptr<IQoSListener>> listeners;
                listenerMgr_->getAvailableListeners(listeners);
                LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
                for (auto &wp : listeners) {
                    if (auto sp = wp.lock()) {
                        LOG(DEBUG, "QoS Manager: invoking onQoSFilterStatusChange");
                        sp->onQoSFilterStatusChange(filter);
                    }
                }
            }
        }
    }
}

void QoSManagerStub::handleStopDataCallEvent(::dataStub::StopDataCallEvent startEvent) {
    LOG(DEBUG, __FUNCTION__);
    auto dcmStub = CommonUtils::getGrpcStub<::dataStub::DataConnectionManager>();
    const google::protobuf::Empty request;
    ::dataStub::IsAnyDataCallActiveReply response;
    grpc::ClientContext context;

    grpc::Status grpcStatus = dcmStub->IsAnyDataCallActive(&context, request, &response);

    if (grpcStatus.ok() && (!response.isanydatacallactive())) {

        std::vector<std::shared_ptr<IQoSFilter>> qosFilters;
        getQosFilters(qosFilters);
        for (auto filter : qosFilters) {
            if (filter->getStatus().modemStatus == FilterInstallationStatus::SUCCESS) {
                QoSFilterStatus status = filter->getStatus();
                status.modemStatus     = FilterInstallationStatus::PENDING;
                std::shared_ptr<QoSFilterImpl> filterUpdate
                    = std::static_pointer_cast<QoSFilterImpl>(filter);
                filterUpdate->setStatus(status);
                if (listenerMgr_) {
                    std::vector<std::weak_ptr<IQoSListener>> listeners;
                    listenerMgr_->getAvailableListeners(listeners);
                    LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
                    for (auto &wp : listeners) {
                        if (auto sp = wp.lock()) {
                            LOG(DEBUG, "QoS Manager: invoking onQoSFilterStatusChange");
                            sp->onQoSFilterStatusChange(filter);
                        }
                    }
                }
            }
        }
    }
}

void QoSManagerStub::handleQoSFilterStatusChangeEvent(
    ::dataStub::QoSFilterStatusChangeEvent qosEvent) {
    LOG(DEBUG, __FUNCTION__);
    std::vector<std::shared_ptr<IQoSFilter>> qosFilters;
    getQosFilters(qosFilters);
    for (auto filter : qosFilters) {
        if (filter->getHandle() == qosEvent.handle()) {
            std::shared_ptr<QoSFilterImpl> filterUpdate
                = std::static_pointer_cast<QoSFilterImpl>(filter);
            filterUpdate->setStatus(protoToSdkQoSFilterStatus(qosEvent.status()));
            if (listenerMgr_) {
                std::vector<std::weak_ptr<IQoSListener>> listeners;
                listenerMgr_->getAvailableListeners(listeners);
                LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());
                for (auto &wp : listeners) {
                    if (auto sp = wp.lock()) {
                        LOG(DEBUG, "QoS Manager: invoking onQoSFilterStatusChange");
                        sp->onQoSFilterStatusChange(filter);
                    }
                }
            }
        }
    }
}

uint32_t QoSFilterImpl::getHandle() {
    return handle_;
}
TrafficClass QoSFilterImpl::getTrafficClass() {
    return trafficClass_;
}
std::shared_ptr<ITrafficFilter> QoSFilterImpl::getTrafficFilter() {
    return trafficFilter_;
}
QoSFilterStatus QoSFilterImpl::getStatus() {
    return status_;
}
std::string QoSFilterImpl::toString() {
    std::stringstream outStr;
    outStr << " handle: " << static_cast<int>(handle_) << std::endl
           << " status: " << std::endl
           << "   ethStatus: " << filterInstallationStatusToString(status_.ethStatus) << std::endl
           << "   modemStatus: " << filterInstallationStatusToString(status_.modemStatus)
           << std::endl
           << "   ipaStatus: " << filterInstallationStatusToString(status_.ipaStatus) << std::endl
           << " traffic Class: " << +trafficClass_ << std::endl
           << " TrafficFilter: " << std::endl
           << trafficFilter_->toString() << std::endl;
    return outStr.str();
}

std::string QoSFilterImpl::filterInstallationStatusToString(
    FilterInstallationStatus filterInstallationStatus) {
    std::string status = "";
    switch (filterInstallationStatus) {
        case FilterInstallationStatus::SUCCESS:
            status = "SUCCESS";
            break;
        case FilterInstallationStatus::FAILED:
            status = "FAILED";
            break;
        case FilterInstallationStatus::PENDING:
            status = "PENDING";
            break;
        case FilterInstallationStatus::NOT_APPLICABLE:
            status = "NOT_APPLICABLE";
            break;
        default:
            LOG(ERROR, __FUNCTION__, " status is unexpected");
    }
    return status;
}

void QoSFilterImpl::setHandle(uint32_t handle) {
    handle_ = handle;
}
void QoSFilterImpl::setTrafficClass(TrafficClass trafficClass) {
    trafficClass_ = trafficClass;
}
void QoSFilterImpl::setTrafficFilter(std::shared_ptr<ITrafficFilter> trafficFilter) {
    trafficFilter_ = trafficFilter;
}
void QoSFilterImpl::setStatus(QoSFilterStatus status) {
    status_ = status;
}

TrafficClass TcConfigImpl::getTrafficClass() {
    return trafficClass_;
}
Direction TcConfigImpl::getDirection() {
    return direction_;
}
DataPath TcConfigImpl::getDataPath() {
    return dataPath_;
}
BandwidthConfig TcConfigImpl::getBandwidthConfig() {
    return bandwidthConfig_;
}
std::string TcConfigImpl::toString() {
    std::stringstream outStr;
    outStr << " Traffic class: " << +trafficClass_
           << ", Data path: " << TrafficFilterImpl::dataPathToString(dataPath_)
           << ", direction : " << TrafficFilterImpl::directionToString(direction_);
    if (validityMask_ & TcConfigValidField::TC_BANDWIDTH_CONFIG_VALID) {
        outStr << ", Min bandwidth config : "
               << +bandwidthConfig_.dlBandwidthValue.bandwidthRange.minBandwidth
               << ", Max bandwidth config : "
               << +bandwidthConfig_.dlBandwidthValue.bandwidthRange.maxBandwidth;
    }
    outStr << std::endl;
    return outStr.str();
}

TcConfigValidFields TcConfigImpl::getTcConfigValidFields() {
    return validityMask_;
}

void TcConfigImpl::setTrafficClass(TrafficClass trafficClass) {
    validityMask_ = validityMask_ | TcConfigValidField::TC_TRAFFIC_CLASS_VALID;
    trafficClass_ = trafficClass;
}
void TcConfigImpl::setDirection(Direction direction) {
    validityMask_ = validityMask_ | TcConfigValidField::TC_DIRECTION_VALID;
    direction_    = direction;
}
void TcConfigImpl::setDataPath(DataPath dataPath) {
    validityMask_ = validityMask_ | TcConfigValidField::TC_DATA_PATH_VALID;
    dataPath_     = dataPath;
}
void TcConfigImpl::setBandwidthConfig(BandwidthConfig bandwidthConfig) {
    validityMask_    = validityMask_ | TcConfigValidField::TC_BANDWIDTH_CONFIG_VALID;
    bandwidthConfig_ = bandwidthConfig;
}

TcConfigBuilder &TcConfigBuilder::setTrafficClass(TrafficClass trafficClass) {
    if (tcConfig_ == nullptr) {
        tcConfig_ = std::make_shared<TcConfigImpl>();
    }
    std::static_pointer_cast<TcConfigImpl>(tcConfig_)->setTrafficClass(trafficClass);
    return *this;
}

TcConfigBuilder &TcConfigBuilder::setDirection(Direction direction) {
    if (tcConfig_ == nullptr) {
        tcConfig_ = std::make_shared<TcConfigImpl>();
    }
    std::static_pointer_cast<TcConfigImpl>(tcConfig_)->setDirection(direction);
    return *this;
}

TcConfigBuilder &TcConfigBuilder::setBandwidthConfig(BandwidthConfig bandwidthConfig) {
    if (tcConfig_ == nullptr) {
        tcConfig_ = std::make_shared<TcConfigImpl>();
    }
    std::static_pointer_cast<TcConfigImpl>(tcConfig_)->setBandwidthConfig(bandwidthConfig);
    return *this;
}

TcConfigBuilder &TcConfigBuilder::setDataPath(DataPath dataPath) {
    if (tcConfig_ == nullptr) {
        tcConfig_ = std::make_shared<TcConfigImpl>();
    }
    std::static_pointer_cast<TcConfigImpl>(tcConfig_)->setDataPath(dataPath);
    return *this;
}

std::shared_ptr<ITcConfig> TcConfigBuilder::build() {
    return tcConfig_;
}

}  // end of namespace net
}  // end of namespace data
}  // end of namespace telux
