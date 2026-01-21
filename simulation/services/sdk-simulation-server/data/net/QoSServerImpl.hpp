/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef QOS_MANAGER_SERVER_HPP
#define QOS_MANAGER_SERVER_HPP

#include <telux/data/net/QoSManager.hpp>

#include "common/event-manager/ClientEventManager.hpp"
#include "data/DataConnectionServerImpl.hpp"
#include "event/ServerEventManager.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "protos/proto-src/data_simulation.grpc.pb.h"
#include <memory>

#define DATA_CONNECTION_FILTER "data_connection"
#define DATA_QOS_FILTER "qos_filter"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

class QoSServerImpl final : public dataStub::QoSManager::Service,
                            public IServerEventListener,
                            public telux::common::IEventListener,
                            public std::enable_shared_from_this<QoSServerImpl> {
 public:
    QoSServerImpl(std::shared_ptr<DataConnectionServerImpl> dcmServerImpl);
    ~QoSServerImpl();

    grpc::Status InitService(ServerContext *context, const dataStub::InitRequest *request,
        dataStub::GetServiceStatusReply *response) override;

    void onEventUpdate(google::protobuf::Any event) override;

    bool validateQoSFilterRequest(const dataStub::AddQoSFilterRequest *request,
        const telux::common::JsonData &jsonData, dataStub::QoSFilterErrorCode &errorCode);
    bool validateCreateTrafficClassRequest(const dataStub::CreateTrafficClassRequest *request,
        const telux::common::JsonData &jsonData, dataStub::TcConfigErrorCode &errorCode);

    grpc::Status CreateTrafficClass(ServerContext *context,
        const dataStub::CreateTrafficClassRequest *request,
        dataStub::CreateTrafficClassReply *response) override;
    grpc::Status GetAllTrafficClasses(ServerContext *context,
        const google::protobuf::Empty *request,
        dataStub::GetAllTrafficClassesReply *response) override;
    grpc::Status DeleteTrafficClass(ServerContext *context,
        const dataStub::DeleteTrafficClassRequest *request,
        dataStub::DefaultReply *response) override;
    grpc::Status AddQoSFilter(ServerContext *context, const dataStub::AddQoSFilterRequest *request,
        dataStub::AddQoSFilterReply *response) override;
    grpc::Status GetQosFilter(ServerContext *context, const dataStub::GetQosFilterRequest *request,
        dataStub::GetQosFilterReply *response) override;
    grpc::Status GetQosFilters(ServerContext *context, const google::protobuf::Empty *request,
        dataStub::GetQosFiltersReply *response) override;
    grpc::Status DeleteQosFilter(ServerContext *context,
        const dataStub::DeleteQosFilterRequest *request, dataStub::DefaultReply *response) override;
    grpc::Status DeleteAllQosConfigs(ServerContext *context, const google::protobuf::Empty *request,
        dataStub::DefaultReply *response) override;
    grpc::Status RegisterOnQoSFilterStatusChange(ServerContext *context,
        const google::protobuf::Empty *request,
        grpc::ServerWriter<dataStub::RegisterOnQoSFilterStatusChangeReply> *writer) override;
    grpc::Status DeRegisterOnQoSFilterStatusChange(ServerContext *context,
        const dataStub::DeRegisterNotificationRequest *request,
        google::protobuf::Empty *response) override;

    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

 private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::mutex qosFilterStatusMtx_;  // Mutex for protecting access to
                                     // qosFilterStatusWriters_
    std::condition_variable qosFilterStatusCv_;  // Condition variable for event stream
    std::vector<std::shared_ptr<grpc::ServerWriter<dataStub::RegisterOnQoSFilterStatusChangeReply>>>
        qosFilterStatusWriters_;  // Stores active writers for event stream
    std::shared_ptr<DataConnectionServerImpl> dcmServerImpl_;

    void syncQoSFilterStatus(dataStub::DataPath dataPath, Json::Value &jsonStatus);
    // IServerEventListener implementation
    Json::Value convertITcConfigToJson(const dataStub::ITcConfig &protoTcConfig);
    void convertJsonToITcConfig(
        const Json::Value &jsonTcConfig, dataStub::ITcConfig *protoTcConfig);

    // Conversions for IQoSFilter
    Json::Value convertIQoSFilterToJson(const dataStub::IQoSFilter &protoQoSFilter);
    void convertJsonToIQoSFilter(
        const Json::Value &jsonQoSFilter, dataStub::IQoSFilter *protoQoSFilter);

    // Helper for converting BandwidthConfig
    Json::Value convertBandwidthConfigToJson(const dataStub::BandwidthConfig &protoConfig);
    void convertJsonToBandwidthConfig(
        const Json::Value &jsonConfig, dataStub::BandwidthConfig *protoConfig);

    // Helper for converting QoSFilterStatus
    Json::Value convertQoSFilterStatusToJson(const dataStub::QoSFilterStatus &protoStatus);
    void convertJsonToQoSFilterStatus(
        const Json::Value &jsonStatus, dataStub::QoSFilterStatus *protoStatus);

    // Conversions for ITrafficFilter (more complex due to oneof)
    Json::Value convertITrafficFilterToJson(const dataStub::ITrafficFilter &protoFilter);
    void convertJsonToITrafficFilter(
        const Json::Value &jsonFilter, dataStub::ITrafficFilter *protoFilter);

    void handleStartDataCallEvent(::dataStub::StartDataCallEvent startEvent);
    void handleStopDataCallEvent(::dataStub::StopDataCallEvent stopEvent);
    bool validateModemPrioritizationFilter(
        const dataStub::ITrafficFilter &trafficFilter, uint32_t trafficClass);
};

#endif  // QOS_MANAGER_SERVER_HPP