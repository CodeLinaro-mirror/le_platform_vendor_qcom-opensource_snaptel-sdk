/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "./WakeupHandler.hpp"
#include "common/RefAppUtils.hpp"
#include <future>

std::shared_ptr<WakeupHandler> WakeupHandler::getInstance(
    std::shared_ptr<EventManager> eventManager) {
    static std::shared_ptr<WakeupHandler> instance(new WakeupHandler(eventManager));
    return instance;
}

// Private constructor
WakeupHandler::WakeupHandler(std::shared_ptr<EventManager> eventManager)
   : eventManager_(eventManager) {
    LOG(DEBUG, __FUNCTION__);
}

WakeupHandler::~WakeupHandler() {
}

bool WakeupHandler::init() {
    LOG(DEBUG, __FUNCTION__);
    auto &powerFactory = telux::power::PowerFactory::getInstance();

    // Wakeup manager initialisation
    std::promise<telux::common::ServiceStatus> mgrProm;
    std::shared_ptr<telux::power::IWakeupManager> wakeupManager
        = powerFactory.getWakeupManager([&mgrProm](telux::common::ServiceStatus status) {
              LOG(DEBUG, __FUNCTION__, " Callback invoked ", static_cast<int>(status));
              mgrProm.set_value(status);
          });

    if (!wakeupManager) {
        LOG(DEBUG, __FUNCTION__, " Failed to get Wakeup Manager object");
        return false;
    }

    LOG(DEBUG, __FUNCTION__, " Initializing Wakeup Manager subsystem Please wait");
    telux::common::ServiceStatus subSystemStatus = mgrProm.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, __FUNCTION__, " Wakeup Manager is ready");
    } else {
        LOG(DEBUG, __FUNCTION__, " Wakeup Manager is failed");
        wakeupManager = nullptr;
        return false;
    }
    resumeWakeupConfig_ = RefAppUtils::getTriggerResumeOnWakeupConfig();

    telux::power::WakeupIndications indications;
    indications.set(telux::power::WakeupIndicationsType::DEFAULT);
    if (resumeWakeupConfig_.test(RESUME_ON_QMI_BIT)) {
        indications.set(telux::power::WakeupIndicationsType::QMI_WAKEUP);
    }
    if (resumeWakeupConfig_.test(RESUME_ON_WOW_BIT)) {
        indications.set(telux::power::WakeupIndicationsType::WOW_WAKEUP);
    }

    auto ec = wakeupManager->registerListener(shared_from_this(), indications);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__,
            " Failed to register wakeup listener, error: ", static_cast<int>(ec));
        wakeupManager = nullptr;
        return false;
    }

    wakeupManager_ = wakeupManager;
    return true;
}

void WakeupHandler::onEventRejected(shared_ptr<Event> event, EventStatus reason) {
    LOG(ERROR, __FUNCTION__, " Wakeup Handler EventStatus: ", static_cast<int>(reason),
        " event: ", event->toString());
}

void WakeupHandler::onEventProcessed(shared_ptr<Event> event, bool success) {
    LOG(ERROR, __FUNCTION__, " Wakeup Handler status: ", static_cast<int>(success),
        " event: ", event->toString());
}

void WakeupHandler::preProcessEvent(shared_ptr<Event> event) {
}

void WakeupHandler::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(ERROR, __FUNCTION__, "  Wakeup Handler status: ", static_cast<int>(status));
}

void WakeupHandler::onWakeup(const telux::power::WakeupEventInfo &wakeupInfo) {
    eventManager_->holdWakeLock("onWakeupIndication");

    bool triggerResume = false;
    if (wakeupInfo.type == telux::power::WakeupType::QMI) {
        triggerResume = resumeWakeupConfig_.test(RESUME_ON_QMI_BIT);
    } else if (wakeupInfo.type == telux::power::WakeupType::WOW) {
        triggerResume = resumeWakeupConfig_.test(RESUME_ON_WOW_BIT);
    }

    if (triggerResume) {
        std::shared_ptr<Event> event = std::make_shared<Event>(
            telux::power::TcuActivityState::RESUME, ALL_MACHINES, TriggerType::WAKEUP_IND_TRIGGER);
        if (event) {
            eventManager_->pushEvent(event);
        } else {
            LOG(ERROR, __FUNCTION__, " unable to create event");
        }
    }

    LOG(DEBUG, __FUNCTION__, " wakeupType: ", static_cast<int>(wakeupInfo.type));
    if (wakeupInfo.type == telux::power::WakeupType::QMI) {
        std::ostringstream ss;
        ss << "\n serviceId: " << wakeupInfo.qmi.serviceId
           << "\n sourceNodeId: " << wakeupInfo.qmi.sourceNodeId
           << "\n destinationNodeId: " << wakeupInfo.qmi.destinationNodeId << "\n msgId: "
           << (wakeupInfo.qmi.isMsgIdValid ? std::to_string(wakeupInfo.qmi.msgId) : "Invalid")
           << "\n pid: "
           << (wakeupInfo.qmi.isPIDValid ? std::to_string(wakeupInfo.qmi.pid) : "Invalid")
           << "\n processName: "
           << (wakeupInfo.qmi.isProcessNameValid ? wakeupInfo.qmi.processName : "Invalid");
        LOG(DEBUG, __FUNCTION__, " QMI info: ", ss.str());
    } else if (wakeupInfo.type == telux::power::WakeupType::WOW) {
        std::ostringstream ss;
        ss << "\n timestamp: " << wakeupInfo.wow.timestamp
           << "\n wakeupCategory: " << wowCategoryToStr(wakeupInfo.wow.wakeupCategory)
           << "\n interfaceName: " << wakeupInfo.wow.interfaceName
           << "\n macAddress: " << wakeupInfo.wow.macAddress << "\n pbmBuffer: "
           << (wakeupInfo.wow.pbmBuffer.empty() ? "Empty" : wakeupInfo.wow.pbmBuffer);
        LOG(DEBUG, __FUNCTION__, " WoW info: ", ss.str());
    }
    eventManager_->releaseWakeLock("onWakeupIndication");
}
