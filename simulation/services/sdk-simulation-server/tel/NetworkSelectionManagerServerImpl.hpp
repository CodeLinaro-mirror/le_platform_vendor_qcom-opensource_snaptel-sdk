/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       NetworkSelectionManagerServerImpl.hpp
 *
 */

#ifndef NETWORK_SELECTION_MANAGER_SERVER_HPP
#define NETWORK_SELECTION_MANAGER_SERVER_HPP

#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"

#include "protos/proto-src/tel_simulation.grpc.pb.h"

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/NetworkSelectionManager.hpp>

#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

class NetworkSelectionManagerServerImpl final
   : public telStub::NetworkSelectionService::Service,
     public IServerEventListener,
     public std::enable_shared_from_this<NetworkSelectionManagerServerImpl> {

 public:
    NetworkSelectionManagerServerImpl();
    ~NetworkSelectionManagerServerImpl();
    grpc::Status InitService(ServerContext *context,
        const ::commonStub::GetServiceStatusRequest *request,
        commonStub::GetServiceStatusReply *response) override;
    grpc::Status GetServiceStatus(ServerContext *context,
        const ::commonStub::GetServiceStatusRequest *request,
        commonStub::GetServiceStatusReply *response) override;
    grpc::Status CleanUpService(ServerContext *context, const ::google::protobuf::Empty *request,
        ::google::protobuf::Empty *response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message) override;
    grpc::Status RequestNetworkSelectionMode(ServerContext *context,
        const ::telStub::RequestNetworkSelectionModeRequest *request,
        telStub::RequestNetworkSelectionModeReply *response) override;
    grpc::Status SetNetworkSelectionMode(ServerContext *context,
        const ::telStub::SetNetworkSelectionModeRequest *request,
        telStub::SetNetworkSelectionModeReply *response) override;
    grpc::Status RequestPreferredNetworks(ServerContext *context,
        const ::telStub::RequestPreferredNetworksRequest *request,
        telStub::RequestPreferredNetworksReply *response) override;
    grpc::Status PerformNetworkScan(ServerContext *context,
        const ::telStub::PerformNetworkScanRequest *request,
        telStub::PerformNetworkScanReply *response) override;
    grpc::Status SetPreferredNetworks(ServerContext *context,
        const ::telStub::SetPreferredNetworksRequest *request,
        telStub::SetPreferredNetworksReply *response) override;
    grpc::Status SetLteDubiousCell(ServerContext *context,
        const ::telStub::SetLteDubiousCellRequest *request,
        ::telStub::SetLteDubiousCellReply *response) override;
    grpc::Status SetNrDubiousCell(ServerContext *context,
        const ::telStub::SetNrDubiousCellRequest *request,
        ::telStub::SetNrDubiousCellReply *response) override;
    grpc::Status AbortNetworkScan(ServerContext *context,
        const ::telStub::AbortNetworkScanRequest *request,
        ::telStub::AbortNetworkScanReply *response) override;
    grpc::Status SetCoverageArea(ServerContext *context,
        const ::telStub::SetCoverageAreaRequest *request,
        ::telStub::SetCoverageAreaReply *response) override;

 private:
    void createPreferredNetworkInfo(
        telux::tel::PreferredNetworkInfo input, telStub::PreferredNetworkInfo *output);
    void requestPreferredNetworks(int phoneId,
        std::vector<telux::tel::PreferredNetworkInfo> &preferredNetworks3gppInfo,
        std::vector<telux::tel::PreferredNetworkInfo> &staticPreferredNetworksInfo);
    void setPreferredNetworks(int phoneId,
        std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo,
        bool clearPrevPreferredNetworks);
    telux::tel::PreferredNetworkInfo parsePreferredNetworkInfo(telStub::PreferredNetworkInfo input);
    /**
     * @brief Merges/upserts the preferred network database.
     * @ref [INetworkSelectionManager][PreferredNetworksInfo]
     * New mcc/mnc entries not already present are added to the top of the list.
     * Entries that already exist (matching mcc/mnc) are updated in place, preserving
     * their original position in the list.
     * @param phoneId - Represents the phoneId for database selection.
     * @param newData - Database of new preferred networks list.
     * @param index - Size of new preferred network
     */
    void sortDatabase(int phoneId, const Json::Value &newData, int index);
    void handleSelectionModeChanged(std::string eventParams);
    void handleNetworkScanResultsChanged(std::string eventParams);
    void triggerNetworkSelectionModeEvent(::telStub::SelectionModeChangeEvent event);
    void onEventUpdate(std::string event);
    void triggerNetworkScanResultsEvent(::telStub::NetworkScanResultsChangeEvent event);
    ::telStub::RadioTechnology converRatTypeToRadioTechnology(::telStub::RatType_Type rat);
    ::telStub::RadioTechnology converRatPrefTypeToRadioTechnology(::telStub::RatPrefType rat);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

#endif  // NETWORK_SELECTION_MANAGER_SERVER_HPP
