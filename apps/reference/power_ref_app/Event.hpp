/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef EVENT_HPP
#define EVENT_HPP

#include "common/define.hpp"

#include <memory>
#include <chrono>
#include <map>
#include <functional>
#include <telux/power/TcuActivityDefines.hpp>
#include <telux/common/Log.hpp>

using namespace telux::power;
using namespace telux::common;
using namespace std;

class Event : public enable_shared_from_this<Event> {
    static uint64_t nextId;

 private:
    uint64_t id_;
    TcuActivityState triggeredState_;
    std::string machineName_;
    map<EventStatus, time_t> timeStamps_;
    TriggerType triggerType_;
    EventStatus status_;

 public:
    Event(TcuActivityState event, std::string machineName, TriggerType triggerType);
    ~Event();

    uint64_t getId();
    TcuActivityState getTriggeredState();
    std::string getMachineName();
    map<EventStatus, time_t> getTimeStamps();

    TriggerType getTriggerType();
    EventStatus getEventStatus();
    void setEventStatus(EventStatus status);
    string toString();
};

#endif