/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "TcuActivityMonitor.hpp"
#include <iostream>
#include "common/RefAppUtils.hpp"

std::shared_ptr<TcuActivityMonitor> TcuActivityMonitor::getInstance() {
    static std::shared_ptr<TcuActivityMonitor> instance(new TcuActivityMonitor());
    return instance;
}

TcuActivityMonitor::TcuActivityMonitor()
   : tcuActivityMgr_(nullptr)
   , tcpKeepAliveHandler_(nullptr)
   , dataFilterController_(nullptr)
   , connectionHandler_(nullptr) {
}

TcuActivityMonitor::~TcuActivityMonitor() {
    cleanup();
}

void TcuActivityMonitor::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (tcuActivityMgr_) {
        tcuActivityMgr_->deregisterListener(shared_from_this());
        tcuActivityMgr_ = nullptr;
    }

    // Clean up TCP Keep-Alive handler
    if (tcpKeepAliveHandler_) {
        tcpKeepAliveHandler_->stopKAOffload();
        tcpKeepAliveHandler_ = nullptr;
    }

    // Clean up data filter controller
    if (dataFilterController_) {
        dataFilterController_ = nullptr;
    }

    // Clean up connection handler
    if (connectionHandler_) {
        connectionHandler_->cleanup();
        connectionHandler_ = nullptr;
    }
}

bool TcuActivityMonitor::init() {
    std::lock_guard<std::mutex> lock(mutex_);

    // Initialize TCP Keep-Alive handler
    tcpKeepAliveHandler_ = TCPKeepAliveHandler::getInstance(nullptr);
    if (!tcpKeepAliveHandler_) {
        LOG(ERROR, __FUNCTION__, " Failed to get TCPKeepAliveHandler instance");
        return false;
    }

    if (!tcpKeepAliveHandler_->init()) {
        LOG(ERROR, __FUNCTION__, " Failed to initialize TCPKeepAliveHandler");
        return false;
    }

    // Initialize connection handler
    connectionHandler_ = ConnectionHandler::getInstance();
    if (!connectionHandler_) {
        LOG(ERROR, __FUNCTION__, " Failed to get ConnectionHandler instance");
        return false;
    }

    std::vector<std::shared_ptr<Connection>> connectionConfigList
        = RefAppUtils::getConnectionConfigs();
    if (!connectionHandler_->start(connectionConfigList)) {
        LOG(ERROR, __FUNCTION__, " Failed to start ConnectionHandler");
        return false;
    }

    // Initialize data filter controller
    dataFilterController_ = std::make_shared<DataFilterController>();
    if (!dataFilterController_) {
        LOG(ERROR, __FUNCTION__, " Failed to create DataFilterController instance");
    } else if (!dataFilterController_->initializeSDK()) {
        LOG(ERROR, __FUNCTION__, " Failed to initialize DataFilterController SDK");
        dataFilterController_ = nullptr;
    }
    if (dataFilterController_) {
        weak_ptr<TcuActivityMonitor> weakFromThis = shared_from_this();
        dataFilterController_->registerListener(shared_from_this());
    }

    // Configure client instance
    ClientInstanceConfig config;
    config.clientType  = ClientType::SLAVE;
    config.clientName  = "PowerRefDaemonSlave_" + std::to_string(getpid());
    config.machineName = ALL_MACHINES;

    // Get power factory instance
    auto &powerFactory = PowerFactory::getInstance();

    // Get TCU activity manager
    std::promise<telux::common::ServiceStatus> servicePromise;
    tcuActivityMgr_ = powerFactory.getTcuActivityManager(
        config, [&servicePromise](
                    telux::common::ServiceStatus status) { servicePromise.set_value(status); });

    if (!tcuActivityMgr_) {
        LOG(ERROR, __FUNCTION__, " Failed to get TCU activity manager instance");
        return false;
    }

    // Wait for TCU activity manager to be ready
    telux::common::ServiceStatus serviceStatus = servicePromise.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " TCU activity manager service is not available");
        return false;
    }

    // Get local machine name
    if (tcuActivityMgr_->getMachineName(localMachineName_) != Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to get local machine name");
        localMachineName_ = LOCAL_MACHINE;
    }

    // Register listeners
    Status status = tcuActivityMgr_->registerListener(shared_from_this());
    if (status != Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to register TCU activity listener");
        return false;
    }

    // Register registerServiceStateListener
    status = tcuActivityMgr_->registerServiceStateListener(shared_from_this());
    if (status != Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to register TCU activity listener");
        return false;
    }
    return true;
}

TcuActivityState TcuActivityMonitor::getCurrentActivityState() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!tcuActivityMgr_) {
        return TcuActivityState::UNKNOWN;
    }

    TcuActivityState state;
    ErrorCode ec = tcuActivityMgr_->getActivityState(localMachineName_, state);

    if (ec != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__,
            " Failed to get activity state, error code: ", static_cast<int>(ec));
        return TcuActivityState::UNKNOWN;
    }

    return state;
}

std::string TcuActivityMonitor::getLocalMachineName() {
    std::lock_guard<std::mutex> lock(mutex_);
    return localMachineName_;
}

void TcuActivityMonitor::onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) {
    LOG(DEBUG, __FUNCTION__, " TCU Activity state changed to ", static_cast<int>(state),
        " for machine ", machineName);

    // Handle TCP Keep-Alive based on state
    if (tcpKeepAliveHandler_) {
        if (state == TcuActivityState::SUSPEND) {
            if (!(dataFilterController_
                    && dataFilterController_->addFilter(connectionHandler_->getConnectionList()))) {
                LOG(ERROR, __FUNCTION__, " addFilter failed");
            }
            LOG(DEBUG, __FUNCTION__, " Starting TCP Keep-Alive offload due to SUSPEND state");
            if (tcpKeepAliveHandler_->startKAOffload()) {
                LOG(DEBUG, __FUNCTION__, " TCP Keep-Alive offload started successfully");
            } else {
                LOG(ERROR, __FUNCTION__, " Failed to start TCP Keep-Alive offload");
            }
        } else if (state == TcuActivityState::RESUME) {
            LOG(DEBUG, __FUNCTION__, " Stopping TCP Keep-Alive offload due to RESUME state");
            RefAppUtils::logKpiFile("resume from PMD");
            if (tcpKeepAliveHandler_->sendMessageToAll(localMachineName_ + " got resumed")) {
                RefAppUtils::logKpiFile("TCP send");
            }
            tcpKeepAliveHandler_->stopKAOffload();
        }
    }

    // Auto-acknowledge for SUSPEND and SHUTDOWN states
    if (state == TcuActivityState::SUSPEND || state == TcuActivityState::SHUTDOWN) {
        if (tcuActivityMgr_) {
            Status ackStatus
                = tcuActivityMgr_->sendActivityStateAck(StateChangeResponse::ACK, state);
            if (ackStatus == Status::SUCCESS) {
                LOG(DEBUG, __FUNCTION__, " Sent ",
                    (state == TcuActivityState::SUSPEND ? "SUSPEND" : "SHUTDOWN"),
                    " acknowledgement");
            } else {
                LOG(ERROR, __FUNCTION__, " Failed to send ",
                    (state == TcuActivityState::SUSPEND ? "SUSPEND" : "SHUTDOWN"),
                    " acknowledgement");
            }
        }
    }
}

void TcuActivityMonitor::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service status changed to ", static_cast<int>(status));
}

void TcuActivityMonitor::onDataRestrictModeChange(DataRestrictMode mode) {
    LOG(DEBUG, __FUNCTION__, " mode: ", static_cast<int>(mode.filterMode));
}
