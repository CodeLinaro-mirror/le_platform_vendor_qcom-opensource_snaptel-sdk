/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef POWER_REF_DAEMON_SLAVE_HPP
#define POWER_REF_DAEMON_SLAVE_HPP

#include <memory>
#include <mutex>
#include <condition_variable>
#include <telux/common/CommonDefines.hpp>

// Forward declarations - full definitions in .cpp
class ConfigParser;
class TcuActivityMonitor;

/**
 * @brief PowerRefDaemonSlave class initializes all triggers, EventManager instance and handles
 * signals
 */
class PowerRefDaemonSlave {
 public:
    telux::common::Status init();
    int startDaemon(int argc, char **argv);
    void stopDaemon();

    static PowerRefDaemonSlave &getInstance();

 private:
    std::mutex mtx_;
    std::condition_variable cv_;
    bool exiting_ = false;
    ConfigParser *config_;

    // TcuActivityMonitor instance
    std::shared_ptr<TcuActivityMonitor> tcuActivityMonitor_;

    static void signalHandler(int signum);
    void printUsage(char **argv);
    telux::common::Status parseArguments(int argc, char **argv);
};

#endif  // POWER_REF_DAEMON_SLAVE_HPP
