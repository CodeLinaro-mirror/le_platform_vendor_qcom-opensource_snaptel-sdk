/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SensorReportService.hpp"
#include "libs/common/Logger.hpp"

SensorReportService::SensorReportService() {
    LOG(DEBUG, __FUNCTION__);
}
SensorReportService::~SensorReportService() {
    LOG(DEBUG, __FUNCTION__);
}
SensorReportService &SensorReportService::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static SensorReportService instance;
    return instance;
}