/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EventService.hpp"
#include "libs/common/Logger.hpp"

EventService::EventService() {
    LOG(DEBUG, __FUNCTION__);
}

EventService::~EventService() {
    LOG(DEBUG, __FUNCTION__);
}

EventService &EventService::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static EventService instance;
    return instance;
}