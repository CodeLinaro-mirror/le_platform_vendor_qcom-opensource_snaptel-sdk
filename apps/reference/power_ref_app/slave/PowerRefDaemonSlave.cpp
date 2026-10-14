/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <csignal>

extern "C" {
#include <getopt.h>
}

#include "common/ConfigParser.hpp"
#include "TcuActivityMonitor.hpp"
#include "PowerRefDaemonSlave.hpp"
#include "SignalHandler.hpp"

PowerRefDaemonSlave &PowerRefDaemonSlave::getInstance() {
    LOGFD();
    static PowerRefDaemonSlave instance;
    return instance;
}

telux::common::Status PowerRefDaemonSlave::init() {
    LOGFD();
    telux::common::Status initStatus = telux::common::Status::SUCCESS;
    config_                          = ConfigParser::getInstance();

    do {
        // Initialize TcuActivityMonitor
        std::shared_ptr<TcuActivityMonitor> tcuActivityMonitor = TcuActivityMonitor::getInstance();
        if (tcuActivityMonitor && tcuActivityMonitor->init()) {
            LOGFD(" TcuActivityMonitor init succeeded");
            tcuActivityMonitor_ = tcuActivityMonitor;
        } else {
            LOGFE(" TcuActivityMonitor init failed");
            initStatus = telux::common::Status::FAILED;
            break;
        }

        // Get and log the current TCU activity state
        TcuActivityState currentState = tcuActivityMonitor_->getCurrentActivityState();
        LOGFD(" Current TCU activity state: %d", static_cast<int>(currentState));

        // Get and log the local machine name
        std::string machineName = tcuActivityMonitor_->getLocalMachineName();
        LOGFD(" Local machine name: %s", machineName.c_str());

    } while (0);

    return initStatus;
}

int PowerRefDaemonSlave::startDaemon(int argc, char **argv) {
    LOGFD();
    if (parseArguments(argc, argv) != telux::common::Status::SUCCESS) {
        return EXIT_FAILURE;
    }

    // Process-directed signals are handled on a dedicated sigwait() thread
    // (see SignalHandler.cpp) instead of a real signal-handler context, so
    // stopDaemon() below can safely take mtx_ and touch tcuActivityMonitor_.
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGHUP);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    SignalHandlerCb cb = [this](int sig) {
        LOGFD("Received signal = %d terminating program.", sig);
        stopDaemon();
    };
    SignalHandler::registerSignalHandler(sigset, cb);

    if (init() != telux::common::Status::SUCCESS) {
        if (tcuActivityMonitor_) {
            tcuActivityMonitor_->cleanup();
            tcuActivityMonitor_ = nullptr;
        }
        return EXIT_FAILURE;
    }

    // block current thread, till we get signal
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this] { return exiting_; });

    return EXIT_SUCCESS;
}

void PowerRefDaemonSlave::stopDaemon() {
    LOGFD();
    std::lock_guard<std::mutex> lock(mtx_);
    if (tcuActivityMonitor_) {
        tcuActivityMonitor_->cleanup();
        tcuActivityMonitor_ = nullptr;
    }
    exiting_ = true;
    fflush(stdout);
    cv_.notify_all();
}

void PowerRefDaemonSlave::printUsage(char **argv) {
    std::cout << std::endl;
    std::cout << "Usage: " << std::string(argv[0]) << " [options] " << std::endl;
    std::cout << "Options: " << std::endl;
    std::cout << "\t -h --help        Print helpful information" << std::endl;
    std::cout << "\t -k --kpi         Enable KPI logging" << std::endl;
    std::cout << "Example: " << std::endl;
    std::cout << "   ./telux_power_refd " << std::endl;
    std::cout << "   ./telux_power_refd -k     To enable KPI logging" << std::endl;
    std::cout << std::endl;
}

telux::common::Status PowerRefDaemonSlave::parseArguments(int argc, char **argv) {
    LOGFD();
    int c;
    struct option long_options[]
        = {{"help", no_argument, 0, 'h'}, {"kpi", no_argument, 0, 'k'}, {0, 0, 0, 0}};
    while (1) {
        int option_index = 0;
        c                = getopt_long(argc, argv, "dshk", long_options, &option_index);
        /* Detect the end of the options. */
        if (c == -1) {
            break;
        }
        switch (c) {
            case 'k':
                // KPI logging is handled in PowerRefDaemonMain.cpp
                break;
            case 'h':
            default:
                printUsage(argv);
                return telux::common::Status::INVALIDPARAM;
                break;
        }
    }
    return telux::common::Status::SUCCESS;
}
