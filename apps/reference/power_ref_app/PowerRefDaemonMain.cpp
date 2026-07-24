/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <vector>
#include <getopt.h>
#include <telux/common/Log.hpp>
#include "common/RefAppUtils.hpp"
#include "Utils.hpp"

// Include appropriate headers based on build configuration
#include "slave/PowerRefDaemonSlave.hpp"
#ifndef TELUX_POWER_REFD_EAP
#include "PowerRefDaemon.hpp"
#endif

int main(int argc, char *argv[]) {
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
    return PowerRefDaemonSlave::getInstance().startDaemon(argc, argv);
#else

    bool isSlave   = false;
    bool isConsole = false;
    // Use the existing parseArguments function to handle other arguments
    if (PowerRefDaemon::parseArguments(argc, argv, isSlave, isConsole)
        != telux::common::Status::SUCCESS) {
        return EXIT_FAILURE;
    }

    // In normal mode, run as master or slave based on command line arguments
    if (isSlave) {
        LOGFD("Starting in slave mode");
        return PowerRefDaemonSlave::getInstance().startDaemon(argc, argv);
    } else {
        LOGFD("Starting in master mode");
        if (isConsole) {
            LOGFD("Running with console interface");
            PowerRefDaemon::getInstance().setConsoleMode(true);
        }
        return PowerRefDaemon::getInstance().startDaemon(argc, argv);
    }
#endif
}
