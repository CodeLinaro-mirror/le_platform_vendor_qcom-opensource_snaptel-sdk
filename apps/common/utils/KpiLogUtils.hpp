/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       KpiLogUtils.hpp
 * @brief      Provide API to log KPI messages to the KPI file
 *             (/sys/kernel/boot_kpi/kpi_values).
 */

#ifndef KPI_LOG_UTILS_HPP
#define KPI_LOG_UTILS_HPP

#include <string>

class KpiLogUtils {
 public:
    /**
     * Log a message to the KPI file (/sys/kernel/boot_kpi/kpi_values).
     * Writes "<prefix><message>[: <sec>.<nsec>]" respecting the 50-byte line limit.
     *
     * @param message          Message string to write.
     * @param includeTimestamp Append a CLOCK_REALTIME timestamp suffix (default: false).
     * @param prefix           KPI line prefix (default: "M - TelSDKApp ").
     */
    static void logKpiFile(const std::string &message, bool includeTimestamp = false,
        const std::string &prefix = "M - TelSDKApp ");
};

#endif
