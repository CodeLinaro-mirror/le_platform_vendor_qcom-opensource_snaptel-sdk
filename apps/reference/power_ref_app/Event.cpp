/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "Event.hpp"

#include "common/RefAppUtils.hpp"
uint64_t Event::nextId;

/**
 * @brief Construct a new Event:: Event object
 *
 * @param event          Triggered tcu activity state
 * @param triggerType    Trigger type to identify who initiated it
 */
Event::Event(TcuActivityState triggeredState, std::string machineName, TriggerType triggerType)
   : id_(++nextId)
   , triggeredState_(triggeredState)
   , machineName_(machineName)
   , triggerType_(triggerType)
   , status_(EventStatus::INITIALIZED) {
    LOG(DEBUG, __FUNCTION__, toString());
    timeStamps_.insert({EventStatus::INITIALIZED,
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())});
}

Event::~Event() {
    LOG(DEBUG, __FUNCTION__, toString());
}

// getter setter
uint64_t Event::getId() {
    return id_;
}

TcuActivityState Event::getTriggeredState() {
    return triggeredState_;
}

std::string Event::getMachineName() {
    return machineName_;
}

TriggerType Event::getTriggerType() {
    return triggerType_;
}

string Event::toString() {
    string eventString = "trigger id = " + to_string(id_) + "  triggered by "
                         + RefAppUtils::triggerTypeToString(triggerType_)
                         + "  trigger status = " + RefAppUtils::eventStatusToString(status_)
                         + "  machine Name = " + machineName_ + "  TCU activity triggered state = "
                         + RefAppUtils::tcuActivityStateToString(triggeredState_);
    return eventString;
}

EventStatus Event::getEventStatus() {
    return status_;
}

void Event::setEventStatus(EventStatus status) {
    LOG(DEBUG, __FUNCTION__);
    status_ = status;
    timeStamps_.insert(
        {status, std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())});
}