/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a Data Health Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs for monitoring data health and detecting data
 * stalls across various network modules including Ethernet, IPA, WWAN and WLAN.
 */

#ifndef DATAHEALTHMENU_HPP
#define DATAHEALTHMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <iomanip>
#include <mutex>
#include <condition_variable>

#include <telux/data/DataHealthManager.hpp>
#include "console_app_framework/ConsoleApp.hpp"
#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

using namespace telux::data;
using namespace telux::common;

/**
 * Listener class for Data Health Manager events
 */
class DataHealthListener : public IDataHealthManagerListener {
public:
    DataHealthListener() = default;
    ~DataHealthListener() = default;

    // IDataHealthManagerListener interface implementations
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onDataStallDetectionStateChanged(const DataStallDetectionState &state) override;
    void onDataStallDetected(const DataStallInfo &info) override;
    void onDataStallRecoveryTriggered(const DataStallRecoveryStatus &status) override;
    void onDataStallRecoveryRestartTimerUpdate(const DataStallRecoveryRestartStatus &status) override;
};

class DataHealthMenu : public ConsoleApp {
public:
    DataHealthMenu(std::string appName, std::string cursor);
    ~DataHealthMenu();

    bool init();
    bool displayMenu();

    void configureDataStallDetection(std::vector<std::string> inputCommand);
    void setDataStallConfig(std::vector<std::string> inputCommand);
    void getDataStallConfig(std::vector<std::string> inputCommand);
    void getEnabledDataStallModules(std::vector<std::string> inputCommand);

private:
    bool subSystemStatusUpdated_;
    std::mutex mtx_;
    std::condition_variable cv_;

    bool initHealthManagerAndListener();
    DataStallNetworkModule getNetworkModuleFromUser();
    void onInitCompleted(telux::common::ServiceStatus status);

    std::shared_ptr<IDataHealthManager> dataHealthManager_;
    std::shared_ptr<DataHealthListener> dataHealthListener_;
};

#endif // DATAHEALTHMENU_HPP