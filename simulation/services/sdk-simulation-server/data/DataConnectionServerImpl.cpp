/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <ifaddrs.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <resolv.h>

#include "DataConnectionServerImpl.hpp"
#include "SimulationServer.hpp"

#include "../../../libs/common/SimulationConfigParser.hpp"
#include "../../../libs/data/DataUtilsStub.hpp"


#define DATA_CONNECTION_API_SLOT1_JSON "api/data/IDataConnectionManagerSlot1.json"
#define DATA_CONNECTION_API_SLOT2_JSON "api/data/IDataConnectionManagerSlot2.json"
#define DATA_CONNECTION_STATE_JSON "system-state/data/IDataConnectionManagerState.json"
#define SLOT_2 2

DataConnectionServerImpl::DataConnectionServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

DataConnectionServerImpl::~DataConnectionServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = nullptr;
}

grpc::Status DataConnectionServerImpl::InitService(ServerContext* context,
    const dataStub::SlotInfo* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDataConnectionManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDataConnectionManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::SetDefaultProfile(ServerContext* context,
    const dataStub::SetDefaultProfileRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "setDefaultProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " updated json with profileId:",
            request->profile_id());
        data.stateRootObj["IDataConnectionManager"]["getDefaultProfile"]["profileId"]
            = request->profile_id();

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::GetDefaultProfile(ServerContext* context,
    const dataStub::GetDefaultProfileRequest* request,
    dataStub::GetDefaultProfileReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "getDefaultProfile";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    int profileId;
    int slotID;

    if (data.status == telux::common::Status::SUCCESS) {
        profileId =
            data.stateRootObj["IDataConnectionManager"]["getDefaultProfile"]
            ["profileId"].asInt();
        slotID =
            static_cast<SlotId>(data.stateRootObj["IDataConnectionManager"]
            ["getDefaultProfile"]["slotId"].asInt());
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);
    response->set_slot_id(slotID);
    response->set_profile_id(profileId);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::SetRoamingMode(ServerContext* context,
    const dataStub::SetRoamingModeRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "setRoamingMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " updated json with roaming_mode:",
            request->roaming_mode());
        data.stateRootObj["IDataConnectionManager"]["requestRoamingMode"]["isRoamingEnabled"]
            = request->roaming_mode();
        data.stateRootObj["IDataConnectionManager"]["requestRoamingMode"]["profileId"]
            = request->profile_id();

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::RequestRoamingMode(ServerContext* context,
    const dataStub::RequestRoamingModeRequest* request,
    dataStub::RequestRoamingModeReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "requestRoamingMode";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    bool roamingMode;
    int profileId;

    if (data.status == telux::common::Status::SUCCESS) {
        roamingMode =
            data.stateRootObj["IDataConnectionManager"]["requestRoamingMode"]
            ["isRoamingEnabled"].asBool();
        profileId =
            data.stateRootObj["IDataConnectionManager"]["requestRoamingMode"]
            ["profileId"].asInt();
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);
    response->set_roaming_mode(roamingMode);
    response->set_profile_id(profileId);

    return grpc::Status::OK;
}

bool DataConnectionServerImpl::getIpv4Address(const std::string &ifaceName,
    std::string &ipAddress, std::string &gatewayAddress,
    std::string &dnsPrimaryAddress, std::string &dnsSecondaryAddress) {
    LOG(DEBUG, __FUNCTION__);
    bool ifaceFound = false;
    struct ifaddrs *ifaceAddresses;

    if(getifaddrs(&ifaceAddresses) < 0) {
        LOG(DEBUG, __FUNCTION__, " failure in fetching n/w interfaces");
        return ifaceFound;
    }
    struct ifaddrs *ifaddr;
    //traversing thru all the available interfaces
    for (ifaddr = ifaceAddresses; ifaddr != NULL; ifaddr=ifaddr->ifa_next) {
        if ((ifaddr->ifa_addr != NULL) &&
        (ifaddr->ifa_addr->sa_family == AF_INET)) {
            std::string ifName(ifaddr->ifa_name);
            //if type is v4 & iface name matches with user provided name
            if (ifaceName == ifName) {
                LOG(DEBUG, __FUNCTION__, " found interface:", ifaceName);

                //fetching ip address
                struct sockaddr_in* ipAddr =
                    (struct sockaddr_in*)ifaddr->ifa_addr;
                char ipAddrStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &ipAddr->sin_addr,
                    ipAddrStr, INET_ADDRSTRLEN);
                ipAddress = ipAddrStr;

                //fetching gw address
                struct sockaddr_in* gwAddr =
                    (struct sockaddr_in*)ifaddr->ifa_dstaddr;
                char gwAddrStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &gwAddr->sin_addr,
                    gwAddrStr, INET_ADDRSTRLEN);
                gatewayAddress = gwAddrStr;
                ifaceFound = true;

                //fetching dns address
                struct __res_state addr;
                res_ninit(&addr);
                auto dnsPrimaryAddr = addr.nsaddr_list[0].sin_addr.s_addr;
                auto dnsSecondaryAddr = addr.nsaddr_list[1].sin_addr.s_addr;
                char dnsPrimaryAddrStr[INET_ADDRSTRLEN];
                char dnsSecondaryAddrStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &dnsPrimaryAddr,
                    dnsPrimaryAddrStr, INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &dnsSecondaryAddr,
                    dnsSecondaryAddrStr, INET_ADDRSTRLEN);
                dnsPrimaryAddress = dnsPrimaryAddrStr;
                dnsSecondaryAddress = dnsSecondaryAddrStr;
            }
        }
    }
    freeifaddrs(ifaceAddresses);
    return ifaceFound;
}

bool DataConnectionServerImpl::getIpv6Address(const std::string &ifaceName,
    std::string &ipAddress, std::string &gatewayAddress) {
    LOG(DEBUG, __FUNCTION__);
    bool ifaceFound = false;
    struct ifaddrs *ifaceAddresses;

    if(getifaddrs(&ifaceAddresses) < 0) {
        LOG(DEBUG, __FUNCTION__, " failure in fetching n/w interfaces");
        return ifaceFound;
    }
    struct ifaddrs *ifaddr;
    //traversing thru all the available interfaces
    for (ifaddr = ifaceAddresses; ifaddr != NULL; ifaddr=ifaddr->ifa_next) {
        if ((ifaddr->ifa_addr != NULL) &&
        (ifaddr->ifa_addr->sa_family == AF_INET6)) {
            std::string ifName(ifaddr->ifa_name);
            //if type is v6 & iface name matches with user provided name
            if (ifaceName == ifName) {
                LOG(DEBUG, __FUNCTION__, " found interface:", ifaceName);

                //fetching ip address
                struct sockaddr_in6* ipAddr =
                    (struct sockaddr_in6*)ifaddr->ifa_addr;
                char ipAddrStr[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &ipAddr->sin6_addr,
                    ipAddrStr, INET6_ADDRSTRLEN);
                ipAddress = ipAddrStr;

                //fetching gw address
                // struct sockaddr_in6* gwAddr =
                //     (struct sockaddr_in6*)ifaddr->ifa_dstaddr;
                // char gwAddrStr[INET6_ADDRSTRLEN];
                // inet_ntop(AF_INET6, &gwAddr->sin6_addr,
                //     gwAddrStr, INET6_ADDRSTRLEN);
                // gatewayAddress = gwAddrStr;
                ifaceFound = true;
            }
        }
    }
    freeifaddrs(ifaceAddresses);
    return ifaceFound;
}

void DataConnectionServerImpl::triggerStartDataCallEvent(int profileId, int slotId,
    std::string ipFamilyType) {
    LOG(DEBUG, __FUNCTION__);
    bool dataCallExist = true;

    std::shared_ptr<DataCallParams> call;
    if (slotId == SLOT_ID_1) {
        call = dataCallsSlot1_[profileId];
    } else {
        call = dataCallsSlot2_[profileId];
    }

    if (!call) {
        call = std::make_shared<DataCallParams>();
        std::shared_ptr<SimulationConfigParser> config =
            std::make_shared<SimulationConfigParser>();

        call->ifaceName = config->getValue("DATA_INTERFACE_NAME");
        call->slotId = slotId;
        call->ipFamilyType = ipFamilyType;
        dataCallExist = false;
    }

    if (ipFamilyType ==
        DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV4) ||
        ipFamilyType ==
        DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV4V6)) {
            getIpv4Address(call->ifaceName, call->v4IpAddress, call->v4GwAddress,
                call->dnsPrimaryAddress, call->dnsSecondaryAddress);
    }

    if (ipFamilyType ==
        DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV6) ||
        ipFamilyType ==
        DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV4V6)) {
            getIpv6Address(call->ifaceName, call->v6IpAddress, call->v4GwAddress);
    }

    bool ipv4Supported = (call->v4IpAddress.length() == 0)? false : true;
    bool ipv6Supported = (call->v6IpAddress.length() == 0)? false : true;

    auto &simulationServer = SimulationServer::getInstance();
    std::string datacallEvent = "-f data_connection -e startDataCall "
        + std::to_string(profileId) + " " + std::to_string(slotId)
        + " " + call->ifaceName + " " + call->ipFamilyType + " " +
        call->v4IpAddress + " " + call->v4GwAddress + " " +
        call->dnsPrimaryAddress + " " + call->dnsSecondaryAddress + " " +
        call->v6IpAddress + " " + call->v6GwAddress;

    simulationServer.writeMessage(const_cast<char*>(datacallEvent.c_str()),
        datacallEvent.length());

    //keeping local copy of data call params in server
    if ((!dataCallExist) && (ipv4Supported || ipv6Supported)) {
        if (slotId == SLOT_ID_1) {
            dataCallsSlot1_[profileId] = call;
        } else {
            dataCallsSlot2_[profileId] = call;
        }
    }
}

grpc::Status DataConnectionServerImpl::StartDatacall(ServerContext* context,
    const dataStub::DataCallInputParams* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "startDataCall";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    int profileId = request->profile_id();
    int slotId = request->slot_id();
    bool ipFamilyMismatch = false;
    std::string ipFamilyType = DataUtilsStub::convertIpFamilyEnumToString(
                request->ip_family_type().ip_family_type());
    std::shared_ptr<DataCallParams> dataCall;

    if (slotId == SLOT_ID_1) {
        dataCall = dataCallsSlot1_[profileId];
    } else {
        dataCall = dataCallsSlot2_[profileId];
    }

    //updating the datacall status, of locally stored datacall in server.
    if (dataCall) {
        auto currentFamily = dataCall->ipFamilyType;
        if (ipFamilyType != currentFamily) {
            dataCall->ipFamilyType =
                DataUtilsStub::convertIpFamilyEnumToString(::dataStub::IpFamilyType::IPV4V6);
            //to cover IpFamilyType mismatch usecases For ex: user starts v4 datacall first
            // & later starts v6 datacall for same profile.
            ipFamilyMismatch = true;
        }
    }

    //If datacall doesn't exist or there is an IPFamily mismatch
    //trigger the start datacall event with new IPFamilyType.
    if ((!dataCall) || (ipFamilyMismatch)) {
        auto f = std::async(std::launch::deferred,
                [this, profileId, slotId, ipFamilyType, data]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(data.cbDelay));
                this->triggerStartDataCallEvent(profileId, slotId, ipFamilyType);
            }).share();
        taskQ_->add(f);
    }

    return grpc::Status::OK;
}

void DataConnectionServerImpl::triggerStopDataCallEvent(int profileId, int slotId,
    std::string ipFamilyType) {
    LOG(DEBUG, __FUNCTION__);

    auto &simulationServer = SimulationServer::getInstance();
    std::string datacallEvent = "-f data_connection -e stopDataCall "
        + std::to_string(profileId) + " " + std::to_string(slotId)
        + " " + ipFamilyType;

    simulationServer.writeMessage(const_cast<char*>(datacallEvent.c_str()),
        datacallEvent.length());
}

grpc::Status DataConnectionServerImpl::StopDatacall(ServerContext* context,
    const dataStub::DataCallInputParams* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string ipFamilyType = DataUtilsStub::convertIpFamilyEnumToString(
            request->ip_family_type().ip_family_type());
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "stopDataCall";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    int profileId = request->profile_id();
    int slotId = request->slot_id();
    std::shared_ptr<DataCallParams> dataCall;

    if (slotId == SLOT_ID_1) {
        dataCall = dataCallsSlot1_[profileId];
    } else {
        dataCall = dataCallsSlot2_[profileId];
    }

    if (dataCall) {
        auto currentFamily = dataCall->ipFamilyType;
        if (ipFamilyType == currentFamily) {
            if (slotId == SLOT_ID_1) {
                dataCallsSlot1_.erase(profileId);
            } else {
                dataCallsSlot2_.erase(profileId);
            }
        }

        auto f = std::async(std::launch::async, [this, profileId, slotId,
            ipFamilyType, data]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(data.cbDelay));
                this->triggerStopDataCallEvent(profileId, slotId, ipFamilyType);
            }).share();
        taskQ_->add(f);
    }

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::RequestDatacallList(ServerContext* context,
    const dataStub::DataCallInputParams* request, dataStub::RequestDataCallListReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->slot_id() == SLOT_2)? DATA_CONNECTION_API_SLOT2_JSON
        : DATA_CONNECTION_API_SLOT1_JSON;
    std::string stateJsonPath = DATA_CONNECTION_STATE_JSON;
    std::string subsystem = "IDataConnectionManager";
    std::string method = "requestDataCallList";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataConnectionServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {

    dataCallsSlot1_.clear();
    dataCallsSlot2_.clear();
    return grpc::Status::OK;
}