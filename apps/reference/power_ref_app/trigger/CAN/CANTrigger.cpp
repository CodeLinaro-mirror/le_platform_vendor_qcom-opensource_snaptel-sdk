/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CANTrigger.hpp"
#include "common/RefAppUtils.hpp"

std::shared_ptr<CANTrigger> CANTrigger::canTrigger_ = nullptr;

CANTrigger::CANTrigger(std::shared_ptr<EventManager> eventManager) {
    LOGFD();
    eventManager_ = eventManager;
}

CANTrigger::~CANTrigger() {
    LOGFD();
    deRegisterCanListener();
}

bool CANTrigger::init() {
    LOGFD();
    config_ = ConfigParser::getInstance();
    loadTrigger();
    canWrapper_ = CanWrapper::getInstance();
    return registerCanListener();
}

void CANTrigger::deRegisterCanListener() {
    LOGFD();
    for (auto trigger : triggers_) {
        if (trigger.second.second) {
            canWrapper_->unregisterListener(trigger.second.second);
        }
    }
}

bool CANTrigger::registerCanListener() {
    LOGFD();
    bool status = true;
    for (auto trigger : triggers_) {
        if (trigger.first) {
            LOGFI("trigger id %u", trigger.first);
            // registering for CAN trigger to receive notifications when new CAN messages arrive
            RegistrationToken token = canWrapper_->registerListener(trigger.first, CwBase::MASK29,
                CANTrigger::triggerEvent, this, 0, CwBase::IFACE_ANY);
            if (token) {
                trigger.second.second = token;
                LOGFD("registered for id %u", trigger.first);
            } else {
                status = false;
                LOGFE("unable to register for %u", trigger.first);
            }
        } else {
            LOGFE("could not register for %u", trigger.first);
        }
    }
    return status;
}

void CANTrigger::onEventRejected(shared_ptr<Event> event, EventStatus reason) {
    LOGFD("%s", event->toString().c_str());
}

void CANTrigger::onEventProcessed(shared_ptr<Event> event, bool success) {
    LOGFD("%s", event->toString().c_str());
}

void CANTrigger::triggerEvent(CwFrame *pf, void *userData, int ifNo) {
    CANTrigger *canTriggerPtr = (CANTrigger *)userData;
    if (!canTriggerPtr) {
        LOGFE("no can trigger instance available");
        return;
    }
    canTriggerPtr->eventManager_->holdWakeLock("CANReceived");
    LOGFD();
    LOGFD("received frame id = %u", pf->getId());
    std::shared_ptr<Event> eventPtr;
    for (auto trigger : canTriggerPtr->triggers_) {
        // ignore identifier extension (IDE) bit of CAN frame ID
        LOGFD("compare with trigger id = %u", trigger.first);
        if (trigger.first << 1 == pf->getId() << 1) {
            std::string machineName = "";
            int dataLength          = pf->getDataLen();
            if (dataLength > 0) {
                uint8_t *pdata = (uint8_t *)malloc(dataLength + 1);
                if (pdata != NULL) {
                    pf->getData(pdata, dataLength);
                    pdata[dataLength] = '\0';
                    machineName       = std::string((char const *)pdata);
                    free(pdata);
                } else {
                    LOGFE("memory allocation failed to fetch CAN frame");
                }
            }

            LOGFD("machineName %s, machineName.length() %zu", machineName.c_str(),
                machineName.length());

            if (machineName.empty()) {
                machineName = ALL_MACHINES;
            }
            eventPtr = std::make_shared<Event>(
                trigger.second.first, machineName, TriggerType::CAN_TRIGGER);
            break;
        }
    }

    if (eventPtr) {
        if (canTriggerPtr->eventManager_) {
            RefAppUtils::logKpiFile(eventPtr);
            canTriggerPtr->eventManager_->pushEvent(eventPtr);
        } else {
            LOGFE("event manager is not available");
        }
    } else {
        LOGFE("unable to create event");
    }
    canTriggerPtr->eventManager_->releaseWakeLock("CANReceived");
}

std::shared_ptr<CANTrigger> CANTrigger::getInstance(std::shared_ptr<EventManager> eventManager) {
    LOGFD();
    if (!canTrigger_ && eventManager) {
        CANTrigger::canTrigger_ = std::make_shared<CANTrigger>(eventManager);
    }

    if (!CANTrigger::canTrigger_) {
        LOGFE("failed to create CANTrigger instance");
    }
    return CANTrigger::canTrigger_;
}

bool CANTrigger::loadTrigger() {
    LOGFD();
    std::map<std::string, TcuActivityState> expectedTrigger{
        {TRIGGER_SUSPEND, TcuActivityState::SUSPEND}, {TRIGGER_RESUME, TcuActivityState::RESUME},
        {TRIGGER_SHUTDOWN, TcuActivityState::SHUTDOWN}};
    try {
        std::string configText = "";
        uint32_t triggerCANId  = 0;
        for (auto itr = expectedTrigger.begin(); itr != expectedTrigger.end(); ++itr) {
            configText = config_->getValue("CAN_TRIGGER", itr->first);
            if (!configText.empty()) {
                triggerCANId = std::stoul(configText, nullptr, 16);
                if (triggers_.find(triggerCANId) != triggers_.end()) {
                    LOGFE("Error : same trigger for multiple state");
                    return false;
                }
                triggers_.insert({triggerCANId, {itr->second, 0}});
            }
        }
    } catch (const std::invalid_argument &ia) {
        LOGFE("Error : invalid argument");
        return false;
    }
    return true;
}
