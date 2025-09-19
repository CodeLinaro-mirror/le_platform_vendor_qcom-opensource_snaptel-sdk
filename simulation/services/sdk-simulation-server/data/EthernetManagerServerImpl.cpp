/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>

#include "EthernetManagerServerImpl.hpp"
#include "SimulationServer.hpp"

#include "libs/common/SimulationConfigParser.hpp"
#include "libs/data/DataUtilsStub.hpp"
#include "event/EventService.hpp"

#define ETHERNET_API_JSON "api/ethernet/IEthernetManager.json"
#define ETHERNET_STATE_JSON "system-state/ethernet/IEthernetManagerState.json"

EthernetManagerServerImpl::EthernetManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

EthernetManagerServerImpl::~EthernetManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = nullptr;
}

grpc::Status EthernetManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    if (!readConfig(ETHERNET_API_JSON, rootObj)) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IEthernetManager"]["InitServiceDelay"].asInt();
    std::string cbStatus = rootObj["IEthernetManager"]["InitService"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status EthernetManagerServerImpl::GetServiceStatus(ServerContext* context,
    const google::protobuf::Empty* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    if (!readConfig(ETHERNET_API_JSON, rootObj)) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IEthernetManager"]["GetServiceStatusDelay"].asInt();
    std::string cbStatus = rootObj["IEthernetManager"]["GetServiceStatus"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status EthernetManagerServerImpl::SetEthernetNicConfig(ServerContext* context,
    const dataStub::SetEthernetNicConfigRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = ETHERNET_API_JSON;
    std::string stateJsonPath = ETHERNET_STATE_JSON;
    std::string subsystem = "IEthernetManager";
    std::string method = "setEthernetNicConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " updated json with eth_config");
        const dataStub::EthConfig& ethConfig = request->eth_config();
        Json::Value ethConfigJson;
        ethConfigJson["mode"] = static_cast<int>(ethConfig.mode());
        for (const auto& nicConfig : ethConfig.v_eth_nic_config()) {
            Json::Value nicConfigJson;
            nicConfigJson["eth_iface_name"] = nicConfig.eth_iface_name();
            nicConfigJson["eth_nic_type"] = static_cast<int>(nicConfig.eth_nic_type());
            ethConfigJson["v_eth_nic_config"].append(nicConfigJson);
        }
        for (const auto& macsecConfig : ethConfig.v_macsec_nic_config()) {
            Json::Value macsecConfigJson;
            macsecConfigJson["state"] = static_cast<int>(macsecConfig.state());
            macsecConfigJson["macsec_iface_name"] = macsecConfig.macsec_iface_name();
            macsecConfigJson["macsec_mode"] = static_cast<int>(macsecConfig.macsec_mode());
            macsecConfigJson["eth_nic_iface_name"] = macsecConfig.eth_nic_iface_name();
            macsecConfigJson["mtu_size"] = macsecConfig.mtu_size();
            ethConfigJson["v_macsec_nic_config"].append(macsecConfigJson);
        }
        data.stateRootObj[subsystem][method]["eth_config"] = ethConfigJson;


        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status EthernetManagerServerImpl::GetEthernetNicConfig(ServerContext* context,
    const google::protobuf::Empty* request, dataStub::GetEthernetNicConfigReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = ETHERNET_API_JSON;
    std::string stateJsonPath = ETHERNET_STATE_JSON;
    std::string subsystem = "IEthernetManager";
    std::string method = "getEthernetNicConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    dataStub::EthConfig ethConfig;
    if (data.status == telux::common::Status::SUCCESS) {
        const Json::Value& ethConfigJson = data.stateRootObj[subsystem][method]["eth_config"];
        ethConfig.set_mode(static_cast<dataStub::EthModeEnum>(ethConfigJson["mode"].asInt()));
        for (const auto& nicConfigJson : ethConfigJson["v_eth_nic_config"]) {
            dataStub::EthNicConfig* nicConfig = ethConfig.add_v_eth_nic_config();
            nicConfig->set_eth_iface_name(nicConfigJson["eth_iface_name"].asString());
            nicConfig->set_eth_nic_type(static_cast<dataStub::EthNetworkTypeEnum>(nicConfigJson["eth_nic_type"].asInt()));
        }
        for (const auto& macsecConfigJson : ethConfigJson["v_macsec_nic_config"]) {
            dataStub::MacsecNicConfig* macsecConfig = ethConfig.add_v_macsec_nic_config();
            macsecConfig->set_state(static_cast<dataStub::ConfigStateEnum>(macsecConfigJson["state"].asInt()));
            macsecConfig->set_macsec_iface_name(macsecConfigJson["macsec_iface_name"].asString());
            macsecConfig->set_macsec_mode(static_cast<dataStub::MacsecModeEnum>(macsecConfigJson["macsec_mode"].asInt()));
            macsecConfig->set_eth_nic_iface_name(macsecConfigJson["eth_nic_iface_name"].asString());
            macsecConfig->set_mtu_size(macsecConfigJson["mtu_size"].asInt());
        }
    }

    response->mutable_eth_config()->CopyFrom(ethConfig);
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status EthernetManagerServerImpl::ActivateLAN(ServerContext* context,
    const google::protobuf::Empty* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    if (!readConfig(ETHERNET_API_JSON, rootObj)) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    //int cbDelay = rootObj["IEthernetManager"]["ActivateLANDelay"].asInt();
    //std::string cbStatus = rootObj["IEthernetManager"]["ActivateLAN"].asString();
    //telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    //LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    // Update state file to indicate LAN is active
    std::string stateJsonPath = ETHERNET_STATE_JSON;
    Json::Value stateRootObj;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(stateRootObj, stateJsonPath);
    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "State JSON read failed");
    }

    stateRootObj["IEthernetManager"]["ActivateLAN"]["is_active"] = true;
    JsonParser::writeToJsonFile(stateRootObj, stateJsonPath);

    response->set_status(static_cast<commonStub::Status>(telux::common::Status::SUCCESS));
    response->set_error(static_cast<commonStub::ErrorCode>(telux::common::ErrorCode::SUCCESS));
    //response->set_delay(cbDelay);

    return grpc::Status::OK;
}


grpc::Status EthernetManagerServerImpl::RegisterListener(ServerContext* context,
    const google::protobuf::Empty* request, grpc::ServerWriter<dataStub::RegisterListenerReply>* writer) {

    LOG(DEBUG, __FUNCTION__);
    static int clientIdCounter = 0;
    int clientId = ++clientIdCounter;
    telux::common::ServiceStatus status = telux::common::ServiceStatus::SERVICE_AVAILABLE;

    {
        std::lock_guard<std::mutex> lck(mtx_);
        listeners_[clientId] = status;
    }

    dataStub::RegisterListenerReply reply;
    reply.set_client_id(clientId);
    reply.set_status(static_cast<dataStub::ServiceStatus>(status));

    if (!writer->Write(reply)) {
        LOG(ERROR, __FUNCTION__, "Failed to write to stream");
        return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to write to stream");
    }

    // Simulate asynchronous event triggering
    auto f = std::async(std::launch::async, [this, clientId, status]() {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        triggerRegisterListenerEvent(clientId, status);
    }).share();
    taskQ_->add(f);

    return grpc::Status::OK;
}

grpc::Status EthernetManagerServerImpl::DeregisterListener(ServerContext* context,
    const dataStub::DeregisterListenerRequest* request, google::protobuf::Empty* response) {

    LOG(DEBUG, __FUNCTION__);
    int clientId = request->client_id();

    {
        std::lock_guard<std::mutex> lck(mtx_);
        listeners_.erase(clientId);
    }

    return grpc::Status::OK;
}

grpc::Status EthernetManagerServerImpl::GetOperationType(ServerContext* context,
    const google::protobuf::Empty* request, dataStub::GetOperationTypeReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    if (!readConfig(ETHERNET_API_JSON, rootObj)) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    std::string operationTypeStr = rootObj["IEthernetManager"]["GetOperationType"].asString();
    //telux::data::OperationType operationType = mapOperationType(operationTypeStr);
    //LOG(DEBUG, __FUNCTION__, " operationType::", operationTypeStr);

    //response->set_operation_type(static_cast<dataStub::OperationType>(operationType));

    return grpc::Status::OK;
}

bool EthernetManagerServerImpl::readConfig(const std::string& filePath, Json::Value& rootObj) {
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ");
        return false;
    }
    return true;
}

void EthernetManagerServerImpl::triggerRegisterListenerEvent(int clientId, telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__);

    dataStub::RegisterListenerReply reply;
    reply.set_client_id(clientId);
    //reply.set_status(static_cast<commonStub::Status>(status));

    // Assuming there's a way to notify all registered listeners
    // This is a placeholder for actual implementation
    // For example, you might have a list of writers and notify each one
}

void EthernetManagerServerImpl::clearCachedListeners() {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(mtx_);
    listeners_.clear();
}
