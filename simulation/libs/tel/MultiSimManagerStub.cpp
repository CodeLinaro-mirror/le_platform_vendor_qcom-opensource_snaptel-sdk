/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "MultiSimManagerStub.hpp"
#include <telux/common/DeviceConfig.hpp>
#include "TelDefinesStub.hpp"

#define FIRST_SIM_SLOT_ID 1
#define INIT_DELAY 100

namespace telux {

namespace tel {

MultiSimManagerStub::MultiSimManagerStub() {
    LOG(DEBUG, __FUNCTION__);
    stub_    = CommonUtils::getGrpcStub<MultiSimService>();
    taskQ_   = std::make_shared<AsyncTaskQueue<void>>();
    cbDelay_ = INIT_DELAY;
}

telux::common::Status MultiSimManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    if (!taskQ_) {
        LOG(ERROR, __FUNCTION__, " unable to instantiate AsyncTaskQueue");
        return telux::common::Status::FAILED;
    }
    auto f
        = std::async(std::launch::async, [this, callback]() { this->initSync(callback); }).share();
    auto status = taskQ_->add(f);
    return status;
}

MultiSimManagerStub::~MultiSimManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void MultiSimManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__);
}

void MultiSimManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::commonStub::GetServiceStatusRequest request;
    ::commonStub::GetServiceStatusReply response;
    ClientContext context;
    LOG(DEBUG, __FUNCTION__);
    grpc::Status reqstatus                = stub_->GetServiceStatus(&context, request, &response);
    telux::common::ServiceStatus cbStatus = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    if (reqstatus.ok()) {
        cbStatus = static_cast<telux::common::ServiceStatus>(response.service_status());
        cbDelay_ = static_cast<int>(response.delay());
        if (cbStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            slotCount_ = 1;
            if (telux::common::DeviceConfig::isMultiSimSupported()) {
                slotCount_ = 2;
            }
        }
    }
    serviceStatus_ = cbStatus;
    LOG(DEBUG, __FUNCTION__, " Delay ", cbDelay_, " service status ", static_cast<int>(cbStatus));
    if (callback) {
        auto f = std::async(std::launch::async, [this, callback, cbStatus]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(INIT_DELAY));
            if (callback) {
                callback(cbStatus);
            }
        }).share();
        taskQ_->add(f);
    }
}

std::future<bool> MultiSimManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async, [this]() {
        while (!isSubsystemReady()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(INIT_DELAY));
        }
        return (isSubsystemReady());
    });
    return ((ready_future));
}

telux::common::ServiceStatus MultiSimManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return serviceStatus_;
}

telux::common::Status MultiSimManagerStub::registerListener(
    std::weak_ptr<IMultiSimListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    auto listenerPtr = listener.lock();
    if (!listenerPtr) {
        LOG(ERROR, __FUNCTION__, " invalid listener");
        return telux::common::Status::INVALIDPARAM;
    }

    {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        const auto it = std::find_if(listeners_.begin(), listeners_.end(),
            [listenerPtr](const std::weak_ptr<IMultiSimListener> &registeredListener) {
                return registeredListener.lock() == listenerPtr;
            });
        if (it != listeners_.end()) {
            return telux::common::Status::ALREADY;
        }
        listeners_.push_back(listener);
    }

    std::vector<std::string> filters = {TEL_MULTISIM_FILTER};
    auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
    return clientEventManager.registerListener(shared_from_this(), filters);
}

telux::common::Status MultiSimManagerStub::deregisterListener(
    std::weak_ptr<IMultiSimListener> listener) {
    LOG(DEBUG, __FUNCTION__);

    auto listenerPtr = listener.lock();
    if (!listenerPtr) {
        LOG(ERROR, __FUNCTION__, " invalid listener");
        return telux::common::Status::INVALIDPARAM;
    }

    bool removed     = false;
    bool noListeners = false;
    {
        std::lock_guard<std::mutex> lock(listenersMutex_);
        for (auto it = listeners_.begin(); it != listeners_.end(); ++it) {
            if (it->lock() == listenerPtr) {
                listeners_.erase(it);
                removed = true;
                break;
            }
        }
        noListeners = listeners_.empty();
    }

    if (!removed) {
        return telux::common::Status::NOSUCH;
    }

    if (noListeners) {
        std::vector<std::string> filters = {TEL_MULTISIM_FILTER};
        auto &clientEventManager         = telux::common::ClientEventManager::getInstance();
        clientEventManager.deregisterListener(shared_from_this(), filters);
    }

    return telux::common::Status::SUCCESS;
}

bool MultiSimManagerStub::isSubsystemReady() {
    return serviceStatus_ == telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::Status MultiSimManagerStub::getSlotCount(int &count) {
    LOG(INFO, __FUNCTION__);
    if (getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(ERROR, __FUNCTION__, " Multi Sim subsystem not ready");
        return telux::common::Status::NOTREADY;
    }
    count = slotCount_;
    return telux::common::Status::SUCCESS;
}

telux::common::Status MultiSimManagerStub::requestHighCapability(HighCapabilityCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::setHighCapability(
    int slotId, common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::switchActiveSlot(
    SlotId slotId, common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::requestSlotStatus(SlotStatusCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::configureLogicalSlotMapping(
    std::map<LogicalSlotId, LogicalSlotMapInfo> mapInfo, common::ResponseCallback callback) {

    LOG(DEBUG, __FUNCTION__);
    ::telStub::ConfigureLogicalSlotMappingRequest request;
    ::telStub::ConfigureLogicalSlotMappingResponse response;
    ClientContext context;

    // Populate repeated map_info entries (instead of protobuf map indexing)
    for (const auto &entry : mapInfo) {
        // Add a new repeated entry
        auto *slotEntry = request.add_map_info();

        // Key
        slotEntry->set_logical_slot_id(static_cast<::telStub::LogicalSlotId>(entry.first));
        // Value (LogicalSlotMapInfo)
        auto *info = slotEntry->mutable_map_info();
        info->set_physical_slot(static_cast<::telStub::PhysicalSlotId>(entry.second.physicalSlot));
        info->set_port_id(entry.second.portId);
    }

    grpc::Status reqStatus = stub_->ConfigureLogicalSlotMapping(&context, request, &response);
    if (!reqStatus.ok()) {
        return telux::common::Status::FAILED;
    }

    // Response fields: status/error + callback control flags (same pattern as CardPowerUp)
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status   = static_cast<telux::common::Status>(response.status());
    int delayMs                    = static_cast<int>(response.delay());

    if (status == telux::common::Status::SUCCESS && response.is_callback() && callback) {
        auto f1 = std::async(std::launch::async, [this, error, callback, delayMs]() {
            this->invokeCallback(callback, error, delayMs);
        }).share();
        taskQ_->add(f1);
    }

    return status;
}

void MultiSimManagerStub::invokeCallback(
    telux::common::ResponseCallback callback, telux::common::ErrorCode error, int cbDelayMs) {

    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelayMs));

    auto f = std::async(std::launch::async, [error, callback]() { callback(error); }).share();

    taskQ_->add(f);
}

telux::common::ErrorCode MultiSimManagerStub::getLogicalSlotMapping(
    std::map<LogicalSlotId, LogicalSlotMapInfo> &mapInfo) {
    LOG(DEBUG, __FUNCTION__);

    ::telStub::GetLogicalSlotMappingResponse response;
    const ::google::protobuf::Empty request;
    ClientContext context;

    grpc::Status status = stub_->GetLogicalSlotMapping(&context, request, &response);

    if (!status.ok()) {
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }

    if (response.error() != static_cast<int32_t>(telux::common::ErrorCode::SUCCESS)) {
        return static_cast<telux::common::ErrorCode>(response.error());
    }

    mapInfo.clear();

    for (int i = 0; i < response.map_info_size(); ++i) {
        const auto &entry = response.map_info(i);

        // ✅ logicalSlotId is now explicit
        LogicalSlotId logicalSlotId = static_cast<LogicalSlotId>(entry.logical_slot_id());

        LogicalSlotMapInfo mapInfoEntry;
        mapInfoEntry.physicalSlot = static_cast<PhysicalSlotId>(entry.map_info().physical_slot());
        mapInfoEntry.portId       = entry.map_info().port_id();

        // ✅ Populate output map
        mapInfo[logicalSlotId] = mapInfoEntry;

        LOG(DEBUG, __FUNCTION__, " logicalSlotId: ", static_cast<int>(logicalSlotId),
            " physicalSlot: ", static_cast<int>(mapInfoEntry.physicalSlot),
            " portId: ", mapInfoEntry.portId);
    }

    return telux::common::ErrorCode::SUCCESS;
}

telux::common::ErrorCode MultiSimManagerStub::getPhysicalSlotStatus(
    std::map<PhysicalSlotId, SimSlotStatus> &slotStatus) {

    LOG(DEBUG, "getPhysicalSlotStatus: Enter");

    const ::google::protobuf::Empty request;
    ::telStub::GetSimSlotStatusResponse response;
    ClientContext context;

    grpc::Status rpcStatus = stub_->GetSimSlotStatus(&context, request, &response);

    if (!rpcStatus.ok()) {
        LOG(ERROR, "RPC failed");
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }

    LOG(DEBUG, "RPC success, response.error = " + std::to_string(response.error()));

    if (response.error() != static_cast<int32_t>(telux::common::ErrorCode::SUCCESS)) {
        LOG(ERROR, "Server returned error");
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }

    slotStatus.clear();

    LOG(DEBUG, "slotstatus_size = " + std::to_string(response.slotstatus_size()));

    // Iterate over PhysicalSimSlotStatus
    for (int i = 0; i < response.slotstatus_size(); ++i) {

        const auto &slotEntry = response.slotstatus(i);

        PhysicalSlotId physicalSlotId = static_cast<PhysicalSlotId>(slotEntry.physical_slot());

        const auto &slotStatusProto = slotEntry.status();

        LOG(DEBUG, "---- SLOT INDEX " + std::to_string(i)
                       + " physicalSlotId=" + std::to_string(slotEntry.physical_slot()));

        SimSlotStatus statusEntry;

        /* ---------- isMep ---------- */
        statusEntry.isMep = slotStatusProto.is_mep();
        LOG(DEBUG, "isMep = " + std::string(statusEntry.isMep ? "true" : "false"));

        /* ---------- MEP Slot Info ---------- */
        const auto &mepProto = slotStatusProto.mep_slot_info();

        LOG(DEBUG, "mepSlotInfo.mode (raw enum) = " + std::to_string(mepProto.mode()));

        LOG(DEBUG, "mepSlotInfo.port_size = " + std::to_string(mepProto.port_size()));

        // ✅ Always read mepSlotInfo (server now always populates it)
        statusEntry.mepSlotInfo.mode = static_cast<Mode>(mepProto.mode());

        for (int j = 0; j < mepProto.port_size(); ++j) {

            const auto &portProto = mepProto.port(j);

            PortInfo portInfo;
            portInfo.state  = toClientPortState(portProto.state());
            portInfo.iccId  = portProto.icc_id();
            portInfo.slotId = static_cast<LogicalSlotId>(portProto.slot_id());

            LOG(DEBUG, "  Port[" + std::to_string(j)
                           + "]"
                             " state="
                           + std::to_string(portProto.state()) + " slotId="
                           + std::to_string(portProto.slot_id()) + " iccId=" + portProto.icc_id());

            statusEntry.mepSlotInfo.port.push_back(portInfo);
        }

        /* ---------- Slot / Card ---------- */
        statusEntry.slotState = toClientSlotState(slotStatusProto.slot_state());
        statusEntry.cardState = static_cast<CardState>(slotStatusProto.card_state());
        statusEntry.cardError = static_cast<CardError>(slotStatusProto.card_error());

        LOG(DEBUG, "slotState=" + std::to_string(slotStatusProto.slot_state())
                       + " cardState=" + std::to_string(slotStatusProto.card_state())
                       + " cardError=" + std::to_string(slotStatusProto.card_error()));

        // Map reconstruction
        slotStatus[physicalSlotId] = statusEntry;

        LOG(DEBUG, "FINAL STUB SLOT SUMMARY:"
                   " physicalSlot="
                       + std::to_string(physicalSlotId)
                       + " isMep=" + std::to_string(statusEntry.isMep)
                       + " mode=" + std::to_string(mepProto.mode())
                       + " ports=" + std::to_string(statusEntry.mepSlotInfo.port.size()));
    }

    LOG(DEBUG, "getPhysicalSlotStatus: Exit SUCCESS");
    return telux::common::ErrorCode::SUCCESS;
}

SlotState MultiSimManagerStub::toClientSlotState(::telStub::SlotState protoState) {
    switch (protoState) {
        case telStub::SlotState::SLOT_STATE_INACTIVE:
            return SlotState::INACTIVE;
        case telStub::SlotState::SLOT_STATE_ACTIVE:
            return SlotState::ACTIVE;
        case telStub::SlotState::SLOT_STATE_UNKNOWN:
        default:
            return SlotState::UNKNOWN;
    }
}

PortState MultiSimManagerStub::toClientPortState(::telStub::PortState protoState) {
    switch (protoState) {
        case telStub::PortState::PORT_STATE_INACTIVE:
            return PortState::INACTIVE;
        case telStub::PortState::PORT_STATE_ACTIVE:
            return PortState::ACTIVE;
        case telStub::PortState::PORT_STATE_UNKNOWN:
        default:
            return PortState::UNKNOWN;
    }
}

void MultiSimManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);

    ::telStub::SlotStatusChangedEvent slotStatusChangedEvent;
    if (event.Is<::telStub::SlotStatusChangedEvent>()) {
        event.UnpackTo(&slotStatusChangedEvent);

        // Convert protobuf data to native types
        std::map<PhysicalSlotId, SimSlotStatus> slotStatusMap;

        for (int i = 0; i < slotStatusChangedEvent.slotstatus_size(); ++i) {
            const auto &slotStatusProto = slotStatusChangedEvent.slotstatus(i);

            PhysicalSlotId physicalSlotId
                = static_cast<PhysicalSlotId>(slotStatusProto.physical_slot());

            SimSlotStatus status;
            status.isMep = slotStatusProto.status().is_mep();

            if (status.isMep) {
                status.mepSlotInfo.mode
                    = static_cast<Mode>(slotStatusProto.status().mep_slot_info().mode());

                for (int j = 0; j < slotStatusProto.status().mep_slot_info().port_size(); ++j) {
                    const auto &portProto = slotStatusProto.status().mep_slot_info().port(j);

                    PortInfo portInfo;
                    portInfo.state  = toClientPortState(portProto.state());
                    portInfo.iccId  = portProto.icc_id();
                    portInfo.slotId = static_cast<LogicalSlotId>(portProto.slot_id());

                    status.mepSlotInfo.port.push_back(portInfo);
                }
            }

            status.slotState = toClientSlotState(slotStatusProto.status().slot_state());
            status.cardState = static_cast<CardState>(slotStatusProto.status().card_state());
            status.cardError = static_cast<CardError>(slotStatusProto.status().card_error());

            slotStatusMap[physicalSlotId] = status;
        }

        // Notify listeners
        std::lock_guard<std::mutex> lock(listenersMutex_);
        for (auto it = listeners_.begin(); it != listeners_.end();) {
            auto listener = it->lock();
            if (listener) {
                auto f = std::async(std::launch::async, [listener, slotStatusMap]() {
                    listener->onSlotStatusChanged(slotStatusMap);
                }).share();
                taskQ_->add(f);
                ++it;
            } else {
                it = listeners_.erase(it);
            }
        }
    }
}

}  // end of namespace tel

}  // end of namespace telux
