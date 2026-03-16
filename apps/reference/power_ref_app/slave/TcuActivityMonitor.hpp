/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TCU_ACTIVITY_MONITOR_HPP
#define TCU_ACTIVITY_MONITOR_HPP

#include <memory>
#include <string>
#include <vector>
#include <future>
#include <mutex>

#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/common/Log.hpp>

#include "../TCPKeepAliveHandler.hpp"
#include "DataFilterController.hpp"
#include "ConnectionHandler.hpp"

using namespace telux::power;
using namespace telux::common;

/**
 * @brief TcuActivityMonitor provides a singleton class to monitor TCU activity state changes
 * and manage TCP Keep-Alive functionality based on power state transitions.
 */
class TcuActivityMonitor : public ITcuActivityListener,
                           public IDataFilterListener,
                           public std::enable_shared_from_this<TcuActivityMonitor> {
 public:
    /**
     * @brief Get the singleton instance of TcuActivityMonitor
     *
     * @return std::shared_ptr<TcuActivityMonitor> Shared pointer to the singleton instance
     */
    static std::shared_ptr<TcuActivityMonitor> getInstance();

    /**
     * @brief Initialize the TCU activity monitor
     *
     * @return bool True if initialization was successful, false otherwise
     */
    bool init();

    /**
     * @brief Clean up resources
     */
    void cleanup();

    /**
     * @brief Get the current TCU activity state
     *
     * @return TcuActivityState Current TCU activity state
     */
    TcuActivityState getCurrentActivityState();

    /**
     * @brief Get the name of the local machine
     *
     * @return std::string Name of the local machine
     */
    std::string getLocalMachineName();

    /**
     * @brief Add data filter before starting Keep-Alive
     *
     * @return bool True if filter was added successfully, false otherwise
     */
    bool addDataFilter();

    // ITcuActivityListener implementation
    void onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) override;

    // IServiceStatusListener implementation
    void onServiceStatusChange(telux::common::ServiceStatus status) override;

    // IDataFilterListener implementation
    void onDataRestrictModeChange(DataRestrictMode mode) override;

    ~TcuActivityMonitor();

 private:
    // Private constructor for singleton pattern
    TcuActivityMonitor();

    // TCU activity manager instance
    std::shared_ptr<ITcuActivityManager> tcuActivityMgr_;

    // TCP Keep-Alive handler
    std::shared_ptr<TCPKeepAliveHandler> tcpKeepAliveHandler_;

    // Data filter controller
    std::shared_ptr<DataFilterController> dataFilterController_;

    // Connection handler
    std::shared_ptr<ConnectionHandler> connectionHandler_;

    // Mutex for thread safety
    std::mutex mutex_;
    std::string localMachineName_ = "";
};

#endif  // TCU_ACTIVITY_MONITOR_HPP