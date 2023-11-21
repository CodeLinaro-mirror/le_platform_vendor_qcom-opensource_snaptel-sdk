/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       LocationConfiguratorServerImpl.hpp
 *
 *
 */

#ifndef LOC_CONFIG_SERVER_HPP
#define LOC_CONFIG_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <set>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>

#include "../../../protos/proto-src/loc.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using locStub::LocationConfiguratorService;

class LocationConfiguratorServerImpl final : public locStub::LocationConfiguratorService::Service,
    public std::enable_shared_from_this<LocationConfiguratorServerImpl> {
 public:
    LocationConfiguratorServerImpl();
    ~LocationConfiguratorServerImpl();
    grpc::Status InitService(ServerContext* context, const google::protobuf::Empty* request,
        locStub::GetServiceStatusReply* response);
    grpc::Status ConfigureCTUNC (ServerContext* context,
        const locStub::ConfigureCTUNCRequest* request, locStub::LocManagerCommandReply* response);
    grpc::Status ConfigurePACE (ServerContext* context,
        const locStub::ConfigurePACERequest* request, locStub::LocManagerCommandReply* response);
    grpc::Status DeleteAllAidingData (ServerContext* context,
        const google::protobuf::Empty* request, locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureLeverArm (ServerContext* context,
        const locStub::ConfigureLeverArmRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureConstellations (ServerContext* context,
        const locStub::ConfigureConstellationsRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureMinGpsWeek (ServerContext* context,
        const locStub::ConfigureMinGpsWeekRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status RequestMinGpsWeek (ServerContext* context, const google::protobuf::Empty* request,
        locStub::RequestMinGpsWeekReply* response);
    grpc::Status ConfigureMinSVElevation (ServerContext* context,
        const locStub::ConfigureMinSVElevationRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status RequestMinSVElevation (ServerContext* context,
        const google::protobuf::Empty* request, locStub::RequestMinSVElevationReply* response);
    grpc::Status ConfigureRobustLocation (ServerContext* context,
        const locStub::ConfigureRobustLocationRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status RequestRobustLocation (ServerContext* context,
        const google::protobuf::Empty* request, locStub::RequestRobustLocationReply* response);
    grpc::Status ConfigureSecondaryBand (ServerContext* context,
        const locStub::ConfigureSecondaryBandRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status RequestSecondaryBandConfig (ServerContext* context,
        const google::protobuf::Empty* request, locStub::RequestSecondaryBandConfigReply* response);
    grpc::Status DeleteAidingData (ServerContext* context,
        const locStub::DeleteAidingDataRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureDR (ServerContext* context,
        const locStub::ConfigureDRRequest* request, locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureEngineState (ServerContext* context,
        const locStub::ConfigureEngineStateRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status ProvideConsentForTerrestrialPositioning (ServerContext* context,
        const locStub::ProvideConsentForTerrestrialPositioningRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureNmeaTypes (ServerContext* context,
        const locStub::ConfigureNmeaTypesRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureNmea (ServerContext* context,
        const locStub::ConfigureNmeaRequest* request, locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureEngineIntegrityRisk (ServerContext* context,
        const locStub::ConfigureEngineIntegrityRiskRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureXtraParams (ServerContext* context,
        const locStub::ConfigureXtraParamsRequest* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status RequestXtraStatus (ServerContext* context, const google::protobuf::Empty* request,
            locStub::RequestXtraStatusReply* response);
    grpc::Status InjectMerkleTree (ServerContext* context, const google::protobuf::Empty* request,
            locStub::LocManagerCommandReply* response);
    grpc::Status ConfigureOsnma (ServerContext* context,
        const locStub::ConfigureOsnmaRequest* request, locStub::LocManagerCommandReply* response);
 private:
    void apiJsonReader(std::string apiName, locStub::LocManagerCommandReply* response);
};

#endif // LOC_CONFIG_SERVER_HPP