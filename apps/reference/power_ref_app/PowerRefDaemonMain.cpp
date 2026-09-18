/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <vector>
#include <getopt.h>
#include <telux/common/Log.hpp>
#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
#include <clog.h>
#include <clog_ids.h>
#endif
#include "common/RefAppUtils.hpp"
#include "Utils.hpp"

// Include appropriate headers based on build configuration
#include "slave/PowerRefDaemonSlave.hpp"
#ifndef TELUX_POWER_REFD_EAP
#include "PowerRefDaemon.hpp"
#endif

int main(int argc, char *argv[]) {
#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
    CLOG_REGISTER_APP(CLOG_APP_TPR, "TPR", "Telux power refd");
#endif

    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "radio", "logd", "dlt"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        LOGFD("Adding supplementary groups failed");
    }

    // Check for KPI logging option
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--kpi") == 0) {
            RefAppUtils::setKpiLoggingEnabled(true);
            LOGFD("KPI logging enabled");
            break;
        }
    }

#ifdef TELUX_POWER_REFD_EAP
    // In EAP mode, always run as slave
    LOGFD("Starting in slave mode (EAP build)");
    int exitCode = PowerRefDaemonSlave::getInstance().startDaemon(argc, argv);
#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
    CLOG_UNREGISTER_APP();
#endif
    return exitCode;
#else

    bool isSlave   = false;
    bool isConsole = false;
    // Use the existing parseArguments function to handle other arguments
    if (PowerRefDaemon::parseArguments(argc, argv, isSlave, isConsole)
        != telux::common::Status::SUCCESS) {
#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
        CLOG_UNREGISTER_APP();
#endif
        return EXIT_FAILURE;
    }

    // In normal mode, run as master or slave based on command line arguments
    if (isSlave) {
        LOGFD("Starting in slave mode");
        int exitCode = PowerRefDaemonSlave::getInstance().startDaemon(argc, argv);
#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
        CLOG_UNREGISTER_APP();
#endif
        return exitCode;
    } else {
        LOGFD("Starting in master mode");
        if (isConsole) {
            LOGFD("Running with console interface");
            PowerRefDaemon::getInstance().setConsoleMode(true);
        }
        int exitCode = PowerRefDaemon::getInstance().startDaemon(argc, argv);
#ifdef TELSDK_FEATURE_COMMONLOG_SUPPORT_ENABLED
        CLOG_UNREGISTER_APP();
#endif
        return exitCode;
    }
#endif
}
