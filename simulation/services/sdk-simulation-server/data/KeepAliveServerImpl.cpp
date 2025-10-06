/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "KeepAliveServerImpl.hpp"

#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>

#define KEEPALIVE_MANAGER_API_LOCAL_JSON "api/data/IKeepAliveManager.json"
#define KEEPALIVE_MANAGER_SUBSYSTEM "IKeepAliveManager"
#define KEEPALIVE_FILTER "keep_alive"
#define DEFAULT_DELIMITER " "
#define STATE_CHANGE_EVENT "stateChange"
#define INTERVAL 60000

KeepAliveServerImpl::KeepAliveServerImpl(std::shared_ptr<DataConnectionServerImpl> dcmServerImpl)
   : dcmServerImpl_(dcmServerImpl) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

KeepAliveServerImpl::~KeepAliveServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status KeepAliveServerImpl::InitService(grpc::ServerContext *context,
    const dataStub::InitRequest *request, dataStub::GetServiceStatusReply *response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath           = KEEPALIVE_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay          = rootObj[KEEPALIVE_MANAGER_SUBSYSTEM]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus = rootObj[KEEPALIVE_MANAGER_SUBSYSTEM]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    std::vector<std::string> filters = {"data_connection_server", "keep_alive"};
    auto &serverEventManager         = ServerEventManager::getInstance();
    serverEventManager.registerListener(shared_from_this(), filters);

    return grpc::Status::OK;
}

bool isValidIPAddress(const std::string &ip) {
    struct sockaddr_in sa4;
    struct sockaddr_in6 sa6;

    // Try IPv4
    if (inet_pton(AF_INET, ip.c_str(), &(sa4.sin_addr)) == 1) {
        return true;
    }
    // Try IPv6
    if (inet_pton(AF_INET6, ip.c_str(), &(sa6.sin6_addr)) == 1) {
        return true;
    }
    return false;
}

grpc::Status KeepAliveServerImpl::EnableTCPMonitor(grpc::ServerContext *context,
    const dataStub::EnableTCPMonitorRequest *request, dataStub::EnableTCPMonitorReply *response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    Json::Value rootObj;
    std::string filePath           = KEEPALIVE_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);

    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Json read failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if (!dcmServerImpl_->isAnyDataCallActive(static_cast<SlotId>(request->slot_id()))) {
        LOG(DEBUG, __FUNCTION__, " NO ACTIVE DATA CALL.");
        data.error  = telux::common::ErrorCode::GENERIC_FAILURE;
        data.status = telux::common::Status::FAILED;
    } else {
        LOG(DEBUG, __FUNCTION__, "active data call success");
        if (!request->has_tcp_ka_params()) {
            LOG(ERROR, __FUNCTION__, "TCPKAParams not provided.");
            data.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
        } else {
            const auto &tcpKaParams = request->tcp_ka_params();
            if (tcpKaParams.src_ip().empty() || tcpKaParams.dst_ip().empty()
                || tcpKaParams.src_port() == 0 || tcpKaParams.dst_port() == 0) {
                LOG(DEBUG, __FUNCTION__, "TCPKAParams contain null values.");
                data.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
            } else {
                if ((!isValidIPAddress(tcpKaParams.src_ip()))
                    && (!isValidIPAddress(tcpKaParams.dst_ip()))) {
                    LOG(DEBUG, __FUNCTION__, "source and destination ip's are invalid");
                }
                LOG(DEBUG, __FUNCTION__, "SUCCESS.");
                data.error = telux::common::ErrorCode::SUCCESS;
            }
        }

        if (data.error == telux::common::ErrorCode::SUCCESS) {
            uint32_t monHandle = nextMonitorHandle_++;
            response->set_monitor_handle(monHandle);

            std::lock_guard<std::mutex> lock(handleMapMutex_);
            activeMonitorHandles_[monHandle] = true;
        }
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status KeepAliveServerImpl::DisableTCPMonitor(grpc::ServerContext *context,
    const dataStub::DisableTCPMonitorRequest *request, dataStub::DefaultReply *response) {

    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    Json::Value rootObj;
    std::string filePath           = KEEPALIVE_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);

    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Json read failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    {
        uint32_t monHandle = request->monitor_handle();
        std::lock_guard<std::mutex> lock(handleMapMutex_);

        if (activeMonitorHandles_.count(monHandle)) {
            LOG(INFO, __FUNCTION__, "Monitor handle ", monHandle, " found in active monitors.");
            activeMonitorHandles_.erase(monHandle);
            data.error = telux::common::ErrorCode::SUCCESS;
        } else {
            LOG(INFO, __FUNCTION__, "Monitor handle ", monHandle, " not found in active monitors.");
            data.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
        }
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status KeepAliveServerImpl::StartTCPKeepAliveOffload(grpc::ServerContext *context,
    const dataStub::StartTCPKeepAliveOffloadRequest *request,
    dataStub::StartTCPKeepAliveOffloadReply *response) {

    JsonData data;
    Json::Value rootObj;
    std::string filePath           = KEEPALIVE_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);

    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Json read failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (!dcmServerImpl_->isAnyDataCallActive(static_cast<SlotId>(request->slot_id()))) {
        data.error  = telux::common::ErrorCode::GENERIC_FAILURE;
        data.status = telux::common::Status::FAILED;
    } else {

        uint32_t offloadHandle = 0;

        if (request->has_monitor_handle_params()) {
            offloadHandle = request->monitor_handle_params().monitor_handle();
            std::lock_guard<std::mutex> lock(handleMapMutex_);
            bool isValid = true;

            if (!activeMonitorHandles_.count(offloadHandle)) {
                LOG(ERROR, __FUNCTION__, "Monitor handle ", offloadHandle, " is not active.");
                isValid = false;
            }

            if (request->interval() < INTERVAL) {
                LOG(ERROR, __FUNCTION__, "Interval too short: ", request->interval());
                isValid = false;
            }

            data.error = isValid ? telux::common::ErrorCode::SUCCESS
                                 : telux::common::ErrorCode::INVALID_ARGUMENTS;

            if (data.error == telux::common::ErrorCode::SUCCESS) {
                activeOffloadHandles_[offloadHandle] = true;
            }
        } else if (request->has_session_params()) {
            // Case 2: Start with TCPSessionParams
            const auto &sessionParams = request->session_params();
            if (!sessionParams.has_tcp_ka_params() || !sessionParams.has_tcp_session_params()) {
                LOG(ERROR, __FUNCTION__, "StartTCPKeepAliveOffload Missing TCPKAParams.");
                data.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
            } else {
                LOG(DEBUG, __FUNCTION__, "--Received interval: ", request->interval());

                const auto &tcpKaParams      = sessionParams.tcp_ka_params();
                const auto &tcpSessionParams = sessionParams.tcp_session_params();
                if (tcpKaParams.src_ip().empty() || tcpKaParams.dst_ip().empty()
                    || tcpKaParams.src_port() == 0 || tcpKaParams.dst_port() == 0
                    || tcpSessionParams.recv_next() == 0 || tcpSessionParams.recv_window() == 0
                    || tcpSessionParams.send_next() == 0 || tcpSessionParams.send_window() == 0
                    || request->interval() < INTERVAL) {
                    LOG(ERROR, __FUNCTION__, "TCPSessionParams check interval duration.");
                    data.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
                } else {
                    LOG(DEBUG, __FUNCTION__, "Received interval: ", request->interval());
                    data.error = telux::common::ErrorCode::SUCCESS;
                }
            }
        } else {
            LOG(ERROR, __FUNCTION__, "StartTCPKeepAliveOffload: wrong params provided.");
            data.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
        }

        if (data.error == telux::common::ErrorCode::SUCCESS) {
            offloadHandle = nextOffloadHandle_++;
            std::lock_guard<std::mutex> lock(handleMapMutex_);
            activeOffloadHandles_[offloadHandle] = true;
        }

        if (data.error == telux::common::ErrorCode::SUCCESS) {
            response->set_offload_handle(offloadHandle);
        }
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status KeepAliveServerImpl::StopTCPKeepAliveOffload(grpc::ServerContext *context,
    const dataStub::StopTCPKeepAliveOffloadRequest *request, dataStub::DefaultReply *response) {

    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    Json::Value rootObj;
    std::string filePath           = KEEPALIVE_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);

    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Json read failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    uint32_t offloadHandle = request->offload_handle();
    std::lock_guard<std::mutex> lock(handleMapMutex_);
    if (activeOffloadHandles_.count(offloadHandle)) {
        activeOffloadHandles_.erase(offloadHandle);
        data.error = telux::common::ErrorCode::SUCCESS;
    } else {
        LOG(INFO, __FUNCTION__, "Offload handle ", offloadHandle, "not found activeoffloads.");
        data.error = telux::common::ErrorCode::INVALID_ARGUMENTS;
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

void KeepAliveServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {

    if (message.filter() == KEEPALIVE_FILTER) {
        onEventUpdate(message.event());
    }
}

void KeepAliveServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, "Received event string: ", event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "Parsed event token: ", token);

    if (STATE_CHANGE_EVENT == token) {
        handleKeepAliveStateChangeRequest(event);
    } else {
        LOG(ERROR, __FUNCTION__, "Unknown event flag: ", token);
    }
}

void KeepAliveServerImpl::handleKeepAliveStateChangeRequest(std::string event) {

    LOG(DEBUG, __FUNCTION__, "Handling KeepAlive state change event: ", event);
    uint32_t offloadHandle             = 0;
    telux::common::ErrorCode errorCode = telux::common::ErrorCode::NETWORK_ERR;
    std::string handleParam            = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    try {
        offloadHandle = std::stoul(handleParam);
    } catch (const std::exception &ex) {
        LOG(ERROR, __FUNCTION__, "Exception parsing offload handle: ", ex.what());
        return;
    }

    LOG(DEBUG, __FUNCTION__, "Parsed: offloadHandle=", offloadHandle);

    {
        std::lock_guard<std::mutex> lock(handleMapMutex_);
        if (activeOffloadHandles_.find(offloadHandle) == activeOffloadHandles_.end()) {
            LOG(ERROR, __FUNCTION__, "Offload handle ", offloadHandle, " is not active.");
            return;
        }
    }

    dataStub::KeepAliveStatusChangeReply kaStatusChange;
    kaStatusChange.set_error(static_cast<commonStub::ErrorCode>(errorCode));
    kaStatusChange.set_offload_handle(offloadHandle);

    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter(KEEPALIVE_FILTER);
    anyResponse.mutable_any()->PackFrom(kaStatusChange);

    auto &eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}