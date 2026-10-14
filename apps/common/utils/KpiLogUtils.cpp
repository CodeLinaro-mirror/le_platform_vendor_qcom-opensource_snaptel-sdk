/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <cerrno>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <telux/common/Log.hpp>

#include "KpiLogUtils.hpp"

#define KPI_FILE "/sys/kernel/boot_kpi/kpi_values"

void KpiLogUtils::logKpiFile(
    const std::string &message, bool includeTimestamp, const std::string &prefix) {
    // KPI file line limit is 50 bytes.
    // Timestamp suffix ": <sec>.<nsec>" = ~22 bytes.
    static const size_t KPI_LINE_LIMIT = 50;
    static const size_t TIMESTAMP_LEN  = 22;  // ": <10-digit>.<9-digit>"
    const size_t prefixLen             = prefix.size();
    const size_t msgBudget             = includeTimestamp
                                             ? (KPI_LINE_LIMIT > prefixLen + TIMESTAMP_LEN
                                                     ? KPI_LINE_LIMIT - prefixLen - TIMESTAMP_LEN
                                                     : 0)
                                             : (KPI_LINE_LIMIT > prefixLen ? KPI_LINE_LIMIT - prefixLen : 0);

    // Truncate message if it exceeds budget
    const std::string &msg = (message.size() <= msgBudget) ? message : message.substr(0, msgBudget);

    struct timespec ts = {};
    clock_gettime(CLOCK_REALTIME, &ts);

    std::ofstream kpiOutput(KPI_FILE);
    if (kpiOutput.is_open()) {
        kpiOutput << prefix << msg;
        if (includeTimestamp) {
            kpiOutput << ": " << ts.tv_sec << "." << ts.tv_nsec;
        }
        kpiOutput.close();
    } else {
        std::cerr << "Unable to open KPI file " << errno << ": " << strerror(errno) << std::endl;
    }

    LOG(ERROR, __FUNCTION__, " ", prefix, " ", msg, " ", ts.tv_sec, ".", ts.tv_nsec);
    std::cout << " " << prefix << " " << msg << " " << ts.tv_sec << "." << ts.tv_nsec << std::endl;
}
