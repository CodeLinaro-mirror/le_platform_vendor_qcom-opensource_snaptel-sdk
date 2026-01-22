/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef IEVENTLISTENER_HPP
#define IEVENTLISTENER_HPP

#include "Event.hpp"

/**
 * @brief Event listener provide interface to listen to status of event
 *
 */
class IEventListener {
 public:
    virtual void onEventRejected(shared_ptr<Event> event, EventStatus reason){};
    virtual void onEventProcessed(shared_ptr<Event> event, bool success){};
    virtual void preProcessEvent(shared_ptr<Event> event){};
};

#endif
