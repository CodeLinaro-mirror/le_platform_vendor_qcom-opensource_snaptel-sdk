/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "NAOIpTrigger.hpp"
#include "common/RefAppUtils.hpp"

NAOIpTrigger::NAOIpTrigger(std::shared_ptr<EventManager> eventManager) {
    LOGFD();
    eventManager_ = eventManager;
}

NAOIpTrigger::~NAOIpTrigger() {
    LOGFD();
    eventManager_         = nullptr;
    dataFilterController_ = nullptr;
    if (connectionHandler_) {
        connectionHandler_->cleanup();
        connectionHandler_ = nullptr;
    }
    tcpKeepAliveHandler_ = nullptr;
}

bool NAOIpTrigger::init() {
    LOGFD();
    config_          = ConfigParser::getInstance();
    bool returnValue = false;
    do {
        if (!loadConfig()) {
            break;
        }
        weak_ptr<NAOIpTrigger> weakFromThis = shared_from_this();
        if (!eventManager_) {
            LOGFE("event manager is not available");
            break;
        }
        // Connection handler initialisation
        connectionHandler_ = ConnectionHandler::getInstance();
        if (!connectionHandler_) {
            return false;
        }
        std::shared_ptr<ISocketConnectionListener> listener = shared_from_this();
        // connectionHandler_->registerListener(listener);
        std::vector<std::shared_ptr<Connection>> connectionConfigList
            = RefAppUtils::getConnectionConfigs();
        connectionHandler_->start(connectionConfigList);
        for (const auto &connection : connectionConfigList) {
            if (connection->socketConnection && connection->dataConnectionManager) {
                connection->socketConnection->registerListener(shared_from_this());
                connection->dataConnectionManager->registerListener(shared_from_this());
            } else {
                LOGFE("error on connection");
                return false;
            }
        }
        tcpKeepAliveHandler_ = TCPKeepAliveHandler::getInstance(eventManager_);
        if (tcpKeepAliveHandler_ && tcpKeepAliveHandler_->init()) {
            LOGFD(" naoIpTrigger init succeed");
        } else {
            LOGFE(" naoIpTrigger init failed");
            return false;
        }

        dataFilterController_ = std::make_shared<DataFilterController>();
        if (dataFilterController_) {
            returnValue = dataFilterController_->initializeSDK();
            dataFilterController_->registerListener(weakFromThis);
            if (returnValue) {
                // Listen to all triggers to be able to add and remove data filters.
                eventManager_->registerListener(weakFromThis, TriggerType::UNKNOWN);
                break;
            } else {
                // telsdk initialisation failed wait for some time and retry
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            }
        } else {
            LOGFE("Unable to instantiate data controller");
        }

    } while (0);

    return returnValue;
}

bool NAOIpTrigger::enableFilter() {
    LOGFD();
    if (dataFilterController_) {
        if (!dataFilterController_->addFilter(connectionHandler_->getConnectionList())) {
            LOGFE("addFilter failed");
        }
        DataRestrictMode mode;
        // Note: If filter auto exit is enabled, it will disable the filter if any packets pass
        //       through a whitelisted filter, even if it is an unexpected packet.
        //       @ref DataRestrictMode
        // ex. mode.filterAutoExit = DataRestrictModeType::ENABLE;
        if (RefAppUtils::isAutoExitEnabled()) {
            mode.filterAutoExit = DataRestrictModeType::ENABLE;
        } else {
            mode.filterAutoExit = DataRestrictModeType::DISABLE;
        }
        mode.filterMode = DataRestrictModeType::ENABLE;

        if (dataFilterController_->sendSetDataRestrictMode(mode)) {
            return true;
        }
        LOGFE("sendSetDataRestrictMode failed");
    } else {
        LOGFE("dataFilterController is not ready");
    }
    return false;
}

bool NAOIpTrigger::disableFilter() {
    LOGFD();
    if (dataFilterController_) {
        DataRestrictMode mode;
        mode.filterMode = DataRestrictModeType::DISABLE;
        if (!dataFilterController_->sendSetDataRestrictMode(mode)) {
            LOGFE("sendSetDataRestrictMode is failed");
        }
        return true;
    } else {
        LOGFE("dataFilterController is not ready");
    }
    return false;
}

void NAOIpTrigger::onEventRejected(shared_ptr<Event> event, EventStatus reason) {
    LOGFD("reason = %d", (int)reason);
    if (event->getTriggeredState() == TcuActivityState::SUSPEND
        && reason == EventStatus::REJECTED_INVALID_STATE_TRANSITION) {
        if (tcpKeepAliveHandler_) {
            tcpKeepAliveHandler_->startKAOffload();
        }
        enableFilter();
    }
    if (event->getTriggeredState() == TcuActivityState::RESUME
        && reason == EventStatus::REJECTED_INVALID_STATE_TRANSITION) {
        if (tcpKeepAliveHandler_) {
            tcpKeepAliveHandler_->stopKAOffload();
        }
    }
}

void NAOIpTrigger::onEventProcessed(shared_ptr<Event> event, bool success) {
    LOGFD();

    if (success) {
        if (event->getTriggeredState() == TcuActivityState::SUSPEND) {
            if (tcpKeepAliveHandler_) {
                tcpKeepAliveHandler_->startKAOffload();
            }
            enableFilter();
        } else if (event->getTriggeredState() == TcuActivityState::RESUME) {
            RefAppUtils::logKpiFile("resumed to PMD");
            if (tcpKeepAliveHandler_) {
                tcpKeepAliveHandler_->stopKAOffload();
            }
        }
    }
}

void NAOIpTrigger::preProcessEvent(shared_ptr<Event> event) {
    LOGFD();
    if (event->getTriggeredState() == TcuActivityState::RESUME) {
        disableFilter();
        RefAppUtils::logKpiFile("disabled filter");
    }
}

void NAOIpTrigger::triggerEvent(TcuActivityState eventState, std::string machineName) {
    LOGFD();
    std::shared_ptr<Event> event
        = std::make_shared<Event>(eventState, machineName, TriggerType::NAOIP_TRIGGER);
    if (event) {
        if (eventManager_) {
            RefAppUtils::logKpiFile(event);
            eventManager_->pushEvent(event);
        } else {
            LOGFE("event manager is not available");
        }
    } else {
        LOGFE("unable to create event");
    }
}

bool NAOIpTrigger::validateTrigger(
    char *buffer, int length, TcuActivityState &tcuActivityState, std::string &machineName) {
    LOGFD();
    string text(buffer, length);
    // to avoid \n in a string which might lead to not matching trigger text
    text.erase(std::remove(text.begin(), text.end(), '\n'), text.cend());
    LOGFD("%s", text.c_str());
    size_t deliminatorPosition = 0;
    if ((deliminatorPosition = text.find(MACHINE_NAME_DELIMINATOR)) != std::string::npos) {
        machineName
            = text.substr(deliminatorPosition + sizeof(MACHINE_NAME_DELIMINATOR), text.length());
        text = text.substr(0, deliminatorPosition);
    }
    if (triggerText_.find(text) == triggerText_.end()) {
        LOGFE("invalid trigger text, text = %s", text.c_str());
    } else {
        LOGFI("valid trigger text, text = %s", text.c_str());
        tcuActivityState = triggerText_[text];
        return true;
    }
    return false;
}

void NAOIpTrigger::onDataRestrictModeChange(DataRestrictMode mode) {
    LOGFD();
    if (mode.filterMode == DataRestrictModeType::DISABLE) {
        eventManager_->holdWakeLock("DataFilterDisabled");
        std::unique_lock<std::mutex> lock(messageMtx_);
        if (messageCv_.wait_for(lock, std::chrono::seconds(2)) == std::cv_status::timeout) {
            if (eventManager_->getActivityState() == TcuActivityState::SUSPEND) {
                if (tcpKeepAliveHandler_) {
                    tcpKeepAliveHandler_->stopKAOffload();
                    tcpKeepAliveHandler_->startKAOffload();
                }
                enableFilter();
            }
        }
        eventManager_->releaseWakeLock("DataFilterDisabled");
    }
}

void NAOIpTrigger::messageReceived(
    IPMessage msg, int length, std::shared_ptr<Connection> connection) {
    eventManager_->holdWakeLock("MessageReceived");
    LOGFD();
    TcuActivityState triggerState = TcuActivityState::UNKNOWN;
    std::string machineName       = ALL_MACHINES;
    if (validateTrigger(msg.msg, length, triggerState, machineName)) {
        triggerEvent(triggerState, machineName);
    } else {
        RefAppUtils::logKpiFile("received TCP");
        LOGFE("trigger not match");
        if (eventManager_->getActivityState() == TcuActivityState::SUSPEND) {
            if (tcpKeepAliveHandler_) {
                tcpKeepAliveHandler_->stopKAOffload();
                tcpKeepAliveHandler_->startKAOffload();
            }
            enableFilter();
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    eventManager_->releaseWakeLock("MessageReceived");
    messageCv_.notify_all();
}

bool NAOIpTrigger::loadConfig() {
    LOGFD();
    std::map<std::string, TcuActivityState> expectedTrigger{
        {TRIGGER_SUSPEND, TcuActivityState::SUSPEND}, {TRIGGER_RESUME, TcuActivityState::RESUME},
        {TRIGGER_SHUTDOWN, TcuActivityState::SHUTDOWN}};
    try {
        std::string configTriggerText = "";
        for (auto itr = expectedTrigger.begin(); itr != expectedTrigger.end(); ++itr) {
            configTriggerText = config_->getValue("NAOIP_TRIGGER", itr->first);
            if (!configTriggerText.empty()) {
                if (triggerText_.find(configTriggerText) != triggerText_.end()) {
                    LOGFE("Error : same trigger for multiple state");
                    return false;
                }
                triggerText_.insert({configTriggerText, itr->second});
            }
        }
    } catch (const std::invalid_argument &ia) {
        LOGFE("Error : invalid argument");
        return false;
    }
    return true;
}

void NAOIpTrigger::onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
    eventManager_->holdWakeLock("DataCallInfoChanged");
    if ((dataCall->getDataCallStatus() == DataCallStatus::NET_CONNECTED)
        && (eventManager_->getActivityState() == TcuActivityState::SUSPEND)) {
        for (auto connection : connectionHandler_->getConnectionList()) {
            if (dataCall->getProfileId() == connection->profileId
                && dataCall->getSlotId() == connection->slotId) {
                if (tcpKeepAliveHandler_) {
                    tcpKeepAliveHandler_->startKAOffload();
                }
            }

            // The data filter is disabled if all data calls go down.
            // When a new data call is brought up during suspend, re-enable the data filter
            enableFilter();
        }
    }
    eventManager_->releaseWakeLock("DataCallInfoChanged");
}