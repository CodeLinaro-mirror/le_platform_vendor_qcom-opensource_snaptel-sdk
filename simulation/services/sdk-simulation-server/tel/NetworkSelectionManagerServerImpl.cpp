/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "NetworkSelectionManagerServerImpl.hpp"

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#define JSON_PATH1 "api/tel/INetworkSelectionManagerSlot1.json"
#define JSON_PATH2 "api/tel/INetworkSelectionManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/INetworkSelectionManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/INetworkSelectionManagerStateSlot2.json"
#define MANAGER "INetworkSelectionManager"
#define SLOT_1 1
#define SLOT_2 2

NetworkSelectionManagerServerImpl::NetworkSelectionManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status NetworkSelectionManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_NETWORK_SELECTION_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    std::string srvStatus = rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::RequestNetworkSelectionMode(ServerContext* context,
    const ::telStub::RequestNetworkSelectionModeRequest* request,
    telStub::RequestNetworkSelectionModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestNetworkSelectionMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int mode =  data.stateRootObj[MANAGER]["NetworkSelectionMode"]\
            ["networkSelectionMode"].asInt();
        std::string mcc = data.stateRootObj[MANAGER]["NetworkSelectionMode"]["mcc"].asString();
        std::string mnc = data.stateRootObj[MANAGER]["NetworkSelectionMode"]["mnc"].asString();
        response->set_mcc(mcc);
        response->set_mnc(mnc);
        response->set_mode(static_cast<telStub::NetworkSelectionMode_Mode>(mode));
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::SetNetworkSelectionMode(ServerContext* context,
    const ::telStub::SetNetworkSelectionModeRequest* request,
    telStub::SetNetworkSelectionModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setNetworkSelectionMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::string mcc = request->mcc();
        std::string mnc = request->mnc();
        int mode = static_cast<int>(request->mode());
        data.stateRootObj[MANAGER]["NetworkSelectionMode"]["networkSelectionMode"] = mode;
        data.stateRootObj[MANAGER]["NetworkSelectionMode"]["mcc"] = mcc;
        data.stateRootObj[MANAGER]["NetworkSelectionMode"]["mnc"] = mnc;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status NetworkSelectionManagerServerImpl::SetPreferredNetworks(ServerContext* context,
    const ::telStub::SetPreferredNetworksRequest* request,
    telStub::SetPreferredNetworksReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setPreferredNetworks";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::vector<telux::tel::PreferredNetworkInfo> prefNwInfos;
        for(auto nwInfo: request->preferred_networks_info()) {
            prefNwInfos.push_back(parsePreferredNetworkInfo(nwInfo));
        }
        setPreferredNetworks(request->phone_id(), prefNwInfos, request->clear_previous());
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

telux::tel::PreferredNetworkInfo NetworkSelectionManagerServerImpl::parsePreferredNetworkInfo(
    telStub::PreferredNetworkInfo input) {
    telux::tel::PreferredNetworkInfo nwInfo;
    nwInfo.mcc = input.mcc();
    nwInfo.mnc = input.mnc();
    int size = (input.types()).size();
    for(int i = 0 ; i < size ; i++) {
        nwInfo.ratMask.set(static_cast<int>(input.types(i)));
    }
    return nwInfo;
}

void NetworkSelectionManagerServerImpl::setPreferredNetworks(int phoneId,
    std::vector<telux::tel::PreferredNetworkInfo> preferredNetworksInfo,
    bool clearPrevPreferredNetworks) {
    std::string apiJsonPath = (phoneId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    JsonData rootObj;
    std::string subsystem = MANAGER;
    std::string method = "setPreferredNetworks";
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, rootObj);
    int size = preferredNetworksInfo.size();
    if(clearPrevPreferredNetworks) {
        Json::Value newRoot;
        rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"] = newRoot["PreferredNetworksInfo"];
        JsonParser::writeToJsonFile(rootObj.stateRootObj, stateJsonPath);
        for (int j = 0; j < size; j++) {
            rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][j]["mcc"] =
                preferredNetworksInfo[j].mcc;
            rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][j]["mnc"] =
                preferredNetworksInfo[j].mnc;
            std::vector<uint8_t> data;
            int dataSize = preferredNetworksInfo[j].ratMask.size();
            for (int k = 0; k < dataSize; k++) {
                if(preferredNetworksInfo[j].ratMask.test(k)) {
                    data.emplace_back(k);
                }
            }
            std::string input = CommonUtils::convertVectorToString(data, false);
            rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][j]["ratTypes"] = input;
            JsonParser::writeToJsonFile(rootObj.stateRootObj, stateJsonPath);
        }
    } else {
        Json::Value newData;
        int currentCount = rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"].size();
        LOG(DEBUG, __FUNCTION__,"Current Count is : ", currentCount);
        for (int i = 0 ; i < size; i++) {
            newData[i]["mcc"] = preferredNetworksInfo[i].mcc;
            newData[i]["mnc"] = preferredNetworksInfo[i].mnc;
            std::vector<uint8_t> data;
            int dataSize = preferredNetworksInfo[i].ratMask.size();
            for (int k = 0; k < dataSize; k++) {
                if(preferredNetworksInfo[i].ratMask.test(k)) {
                    data.emplace_back(k);
                }
            }
            std::string input = CommonUtils::convertVectorToString(data, false);
            newData[i]["ratTypes"] = input;
            rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][currentCount + i] = newData[i];
            JsonParser::writeToJsonFile(rootObj.stateRootObj, stateJsonPath);
        }
        sortDatabase(phoneId, newData, size);
    }
}

void NetworkSelectionManagerServerImpl::sortDatabase(int phoneId, Json::Value newData, int index) {
    LOG(DEBUG, __FUNCTION__,"Index is : ", index);
    std::string apiJsonPath = (phoneId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    JsonData rootObj;
    std::string subsystem = MANAGER;
    std::string method = "setPreferredNetworks";
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, rootObj);
    int currentCount = rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"].size();
    LOG(DEBUG, __FUNCTION__,"Current count is : ", currentCount);
    for (int i = currentCount - index - 1; i >= 0 ; --i ) {
        rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][i + index] =
        rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][i];
    }
    for (int i = 0; i < index ; i++ ) {
        rootObj.stateRootObj[MANAGER]["PreferredNetworksInfo"][i] = newData[i];
    }
    JsonParser::writeToJsonFile(rootObj.stateRootObj, stateJsonPath);
}

grpc::Status NetworkSelectionManagerServerImpl::RequestPreferredNetworks(ServerContext* context,
    const ::telStub::RequestPreferredNetworksRequest* request,
    telStub::RequestPreferredNetworksReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestPreferredNetworks";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::vector<telux::tel::PreferredNetworkInfo> preferredNetworks3gppInfo;
        std::vector<telux::tel::PreferredNetworkInfo> staticPreferredNetworksInfo;
        requestPreferredNetworks(request->phone_id(), preferredNetworks3gppInfo,
        staticPreferredNetworksInfo);
        for(auto prefNwInfo : preferredNetworks3gppInfo) {
            telStub::PreferredNetworkInfo *nwInfo = response->add_preferred();
            createPreferredNetworkInfo(prefNwInfo, nwInfo);
        }
        // Set static network info
        for(auto staticNwInfo : staticPreferredNetworksInfo) {
            telStub::PreferredNetworkInfo *nwInfo = response->add_static_preferred();
            createPreferredNetworkInfo(staticNwInfo, nwInfo);
        }
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void NetworkSelectionManagerServerImpl::requestPreferredNetworks(int phoneId,
    std::vector<telux::tel::PreferredNetworkInfo>& preferredNetworks3gppInfo,
    std::vector<telux::tel::PreferredNetworkInfo>& staticPreferredNetworksInfo) {
    std::string apiJsonPath = (phoneId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    JsonData rootObj;
    std::string subsystem = MANAGER;
    std::string method = "requestPreferredNetworks";
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, rootObj);
    for (int i = 0 ; i < 2 ; i++) { // To fill the data of two JSON objects
        std::string networkInfoType = "";
        if(i == 0) {
            networkInfoType = "PreferredNetworksInfo";
        } else {
            networkInfoType = "PreferredNetworksStaticInfo";
        }
        int size = rootObj.stateRootObj[MANAGER][networkInfoType].size();
        for (int j = 0; j < size; j++) {
            telux::tel::PreferredNetworkInfo info;
            std::vector<int> data;
            std::string input = rootObj.stateRootObj[MANAGER][networkInfoType]\
                [j]["ratTypes"].asString();
            info.mcc = rootObj.stateRootObj[MANAGER][networkInfoType][j]["mcc"].asInt();
            info.mnc = rootObj.stateRootObj[MANAGER][networkInfoType][j]["mnc"].asInt();
            data = CommonUtils::convertStringToVector(input);
            int dataSize = data.size();
            for (int k = 0; k < dataSize; k++) {
                info.ratMask.set(data[k]);
            }
            if(i == 0) {
                preferredNetworks3gppInfo.emplace_back(info);
            } else {
                staticPreferredNetworksInfo.emplace_back(info);
            }
        }
    }
    }

void NetworkSelectionManagerServerImpl::createPreferredNetworkInfo(
    telux::tel::PreferredNetworkInfo input, telStub::PreferredNetworkInfo* output) {
    output->set_mcc(input.mcc);
    output->set_mnc(input.mnc);
    int size = input.ratMask.size();
    // Converting RATMask bitset to vector.
    for (int i = 0; i < size ; i++) {
        if (input.ratMask.test(i)) {
            output->add_types(static_cast<telStub::RatType_Type>(i));
        }
    }
}

grpc::Status NetworkSelectionManagerServerImpl::PerformNetworkScan(ServerContext* context,
    const ::telStub::PerformNetworkScanRequest* request,
    telStub::PerformNetworkScanReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "performNetworkScan";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        /*TODO: Add server logic as per user RAT selection in later releases*/
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void NetworkSelectionManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    LOG(DEBUG, __FUNCTION__, "Not Supported");
    /*TODO: Add event handling in later release*/
}
