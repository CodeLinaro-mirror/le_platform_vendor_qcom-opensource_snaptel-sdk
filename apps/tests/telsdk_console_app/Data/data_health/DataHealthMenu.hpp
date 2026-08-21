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
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <iomanip>

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
    bool addMenuCmds_;
    bool initHealthManagerAndListener();
    DataStallNetworkModule getNetworkModuleFromUser();

    std::shared_ptr<IDataHealthManager> dataHealthManager_;
    std::shared_ptr<DataHealthListener> dataHealthListener_;

    // DataFactory::getDataHealthManager() only wires its InitResponseCb into the manager on
    // the very first call for the process; on every later call (e.g. re-entering this menu)
    // the callback argument is silently ignored. The manager also re-invokes whatever
    // callback got wired up on every subsequent SSR transition, for the life of the process -
    // so the target must tolerate being notified an unbounded number of times and must not be
    // tied to any one DataHealthMenu instance (which is destroyed every time the user exits
    // the menu). Static members + a stateless callback satisfy both constraints; per-instance
    // members bound to a specific "this" would not (dangling-callback risk after the menu is
    // destroyed but the SDK singleton still holds the callback).
    static std::mutex initMutex_;
    static std::condition_variable initCv_;
};

#endif // DATAHEALTHMENU_HPP