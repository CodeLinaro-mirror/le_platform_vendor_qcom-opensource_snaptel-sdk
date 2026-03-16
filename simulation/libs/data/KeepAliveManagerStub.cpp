/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <thread>
#include <chrono>

#include "KeepAliveManagerStub.hpp"
#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100
#define SKIP_CALLBACK -1
#define KEEPALIVE_FILTER "keep_alive"

namespace telux {
namespace data {

KeepAliveManagerStub::KeepAliveManagerStub(SlotId slotId) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_           = std::make_shared<AsyncTaskQueue<void>>();
    listenerMgr_     = std::make_shared<telux::common::ListenerManager<IKeepAliveListener>>();
    subSystemStatus_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    slotId_          = slotId;
}

KeepAliveManagerStub::~KeepAliveManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::Status KeepAliveManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(INFO, __FUNCTION__, "Service already available.");
        if (callback) {
            callback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
        }
        return telux::common::Status::SUCCESS;
    }

    std::lock_guard<std::mutex> lck(initMtx_);
    auto f
        = std::async(std::launch::async, [this, callback]() { this->initSync(callback); }).share();

    auto status = taskQ_->add(f);
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Failed to add task to queue");
        return telux::common::Status::FAILED;
    }
    initCb_ = callback;
    return telux::common::Status::SUCCESS;
}

void KeepAliveManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);

    std::lock_guard<std::mutex> lck(initMtx_);
    stub_ = CommonUtils::getGrpcStub<::dataStub::KeepAliveManager>();

    ::dataStub::InitRequest request;
    ::dataStub::GetServiceStatusReply response;
    ClientContext context;

    request.set_operation_type(static_cast<dataStub::OperationType>(oprType_));
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

        this->onServiceStatusChange(cbStatus);
        LOG(DEBUG, __FUNCTION__, " ServiceStatus: ", static_cast<int>(cbStatus));
    } while (0);

    setSubSystemStatus(cbStatus);
    if (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {KEEPALIVE_FILTER};
        auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
        clientEventManager.registerListener(shared_from_this(), filters);
    }

    if (callback && (cbDelay != SKIP_CALLBACK)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
        LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
        invokeInitCallback(cbStatus);
    }
}

void KeepAliveManagerStub::invokeInitCallback(telux::common::ServiceStatus status) {
    LOG(INFO, __FUNCTION__);
    if (initCb_) {
        initCb_(status);
    }
    // Clear the callback after invocation to prevent multiple calls
    initCb_ = nullptr;
}

void KeepAliveManagerStub::setSubSystemStatus(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " to status: ", static_cast<int>(status));
    std::lock_guard<std::mutex> lk(mtx_);
    subSystemStatus_ = status;
}

telux::common::ServiceStatus KeepAliveManagerStub::getServiceStatus() {

    std::lock_guard<std::mutex> lk(mtx_);
    return subSystemStatus_;
}

void KeepAliveManagerStub::onServiceStatusChange(ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, "Notifying listeners for service status: ", static_cast<int>(status));
    if (listenerMgr_) {
        std::vector<std::weak_ptr<IKeepAliveListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "KeepAlive Manager: invoking onServiceStatusChange");
                sp->onServiceStatusChange(status);
            }
        }
    }
}

telux::common::Status KeepAliveManagerStub::registerListener(
    std::weak_ptr<IKeepAliveListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->registerListener(listener);
}

telux::common::Status KeepAliveManagerStub::deregisterListener(
    std::weak_ptr<IKeepAliveListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return listenerMgr_->deRegisterListener(listener);
}

void KeepAliveManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<dataStub::KeepAliveStatusChangeReply>()) {
        dataStub::KeepAliveStatusChangeReply kaStatusChange;
        event.UnpackTo(&kaStatusChange);
        this->handleKeepAliveStatusChangeEvent(kaStatusChange);
    } else {
        LOG(ERROR, __FUNCTION__, " Unknown event type received.");
    }
}

void KeepAliveManagerStub::handleKeepAliveStatusChangeEvent(
    const dataStub::KeepAliveStatusChangeReply &kaStatusChange) {
    LOG(DEBUG, __FUNCTION__, " Received KeepAliveStatusChange");

    if (listenerMgr_) {
        std::vector<std::weak_ptr<IKeepAliveListener>> listeners;
        listenerMgr_->getAvailableListeners(listeners);
        LOG(DEBUG, __FUNCTION__, " listeners size : ", listeners.size());

        for (auto &wp : listeners) {
            if (auto sp = wp.lock()) {
                LOG(DEBUG, "KeepAlive Manager: invoking onKeepAliveStatusChange");
                sp->onKeepAliveStatusChange(
                    static_cast<telux::common::ErrorCode>(kaStatusChange.error()),
                    kaStatusChange.offload_handle());
            }
        }
    }
}

telux::common::ErrorCode KeepAliveManagerStub::enableTCPMonitor(
    const TCPKAParams &tcpKaParams, MonitorHandleType &monHandle) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " KeepAlive manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ClientContext context;
    dataStub::EnableTCPMonitorRequest request;
    dataStub::EnableTCPMonitorReply response;

    request.mutable_tcp_ka_params()->set_src_ip(tcpKaParams.srcIp);
    request.mutable_tcp_ka_params()->set_dst_ip(tcpKaParams.dstIp);
    request.mutable_tcp_ka_params()->set_src_port(tcpKaParams.srcPort);
    request.mutable_tcp_ka_params()->set_dst_port(tcpKaParams.dstPort);
    request.set_operation_type(static_cast<dataStub::OperationType>(oprType_));
    request.set_slot_id(slotId_);

    telux::common::ErrorCode error;
    grpc::Status reqStatus = stub_->EnableTCPMonitor(&context, request, &response);
    if (!reqStatus.ok()) {
        LOG(ERROR, __FUNCTION__, " enableTCPMonitor request failed");
        error = telux::common::ErrorCode::INTERNAL_ERROR;
    } else {
        error = static_cast<telux::common::ErrorCode>(response.reply().error());
    }

    if (error == telux::common::ErrorCode::SUCCESS) {
        monHandle = response.monitor_handle();
    }
    return error;
}

telux::common::ErrorCode KeepAliveManagerStub::disableTCPMonitor(
    const MonitorHandleType monHandle) {

    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " KeepAlive manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ClientContext context;
    dataStub::DisableTCPMonitorRequest request;
    dataStub::DefaultReply response;

    request.set_monitor_handle(monHandle);
    request.set_operation_type(static_cast<dataStub::OperationType>(oprType_));

    grpc::Status reqStatus         = stub_->DisableTCPMonitor(&context, request, &response);
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " disableTCPMonitor request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }
    return error;
}

telux::common::ErrorCode KeepAliveManagerStub::startTCPKeepAliveOffload(
    const TCPKAParams &tcpKaParams, const TCPSessionParams &tcpSessionParams,
    const uint32_t interval, TCPKAOffloadHandle &handle) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " KeepAlive manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ClientContext context;
    dataStub::StartTCPKeepAliveOffloadRequest request;
    dataStub::StartTCPKeepAliveOffloadReply response;

    auto session_params = request.mutable_session_params();
    session_params->mutable_tcp_ka_params()->set_src_ip(tcpKaParams.srcIp);
    session_params->mutable_tcp_ka_params()->set_dst_ip(tcpKaParams.dstIp);
    session_params->mutable_tcp_ka_params()->set_src_port(tcpKaParams.srcPort);
    session_params->mutable_tcp_ka_params()->set_dst_port(tcpKaParams.dstPort);
    session_params->mutable_tcp_session_params()->set_recv_next(tcpSessionParams.recvNext);
    session_params->mutable_tcp_session_params()->set_recv_window(tcpSessionParams.recvWindow);
    session_params->mutable_tcp_session_params()->set_send_next(tcpSessionParams.sendNext);
    session_params->mutable_tcp_session_params()->set_send_window(tcpSessionParams.sendWindow);

    request.set_interval(interval);
    request.set_slot_id(slotId_);
    request.set_operation_type(static_cast<dataStub::OperationType>(oprType_));

    grpc::Status reqStatus = stub_->StartTCPKeepAliveOffload(&context, request, &response);
    telux::common::ErrorCode error
        = static_cast<telux::common::ErrorCode>(response.reply().error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " startTCPKeepAliveOffload request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        handle = response.offload_handle();
    }
    return error;
}

telux::common::ErrorCode KeepAliveManagerStub::startTCPKeepAliveOffload(
    const MonitorHandleType monHandle, const uint32_t interval, TCPKAOffloadHandle &handle) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " KeepAlive manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ClientContext context;
    dataStub::StartTCPKeepAliveOffloadRequest request;
    dataStub::StartTCPKeepAliveOffloadReply response;

    request.mutable_monitor_handle_params()->set_monitor_handle(monHandle);
    request.set_interval(interval);
    request.set_slot_id(slotId_);
    request.set_operation_type(static_cast<dataStub::OperationType>(oprType_));

    grpc::Status reqStatus = stub_->StartTCPKeepAliveOffload(&context, request, &response);
    telux::common::ErrorCode error
        = static_cast<telux::common::ErrorCode>(response.reply().error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " startTCPKeepAliveOffload request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
        handle = response.offload_handle();
    }
    return error;
}

telux::common::ErrorCode KeepAliveManagerStub::stopTCPKeepAliveOffload(
    const TCPKAOffloadHandle handle) {
    LOG(DEBUG, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " KeepAlive manager not ready");
        return telux::common::ErrorCode::SUBSYSTEM_UNAVAILABLE;
    }

    ClientContext context;
    dataStub::StopTCPKeepAliveOffloadRequest request;
    dataStub::DefaultReply response;

    request.set_offload_handle(handle);
    request.set_operation_type(static_cast<dataStub::OperationType>(oprType_));

    grpc::Status reqStatus         = stub_->StopTCPKeepAliveOffload(&context, request, &response);
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());

    if (error == telux::common::ErrorCode::SUCCESS) {
        if (!reqStatus.ok()) {
            LOG(ERROR, __FUNCTION__, " stopTCPKeepAliveOffload request failed");
            error = telux::common::ErrorCode::INTERNAL_ERROR;
        }
    }
    return error;
}

}  // end of namespace data
}  // end of namespace telux
