/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is Data Settings Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to interface with Settings applicable to Data Subsystem
 */

#ifndef DATASETTINGSMENU_HPP
#define DATASETTINGSMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <map>


#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

using namespace telux::data;
using namespace telux::common;

class DataSettingsMenu : public ConsoleApp ,
                         public IDataSettingsListener,
                         public std::enable_shared_from_this<DataSettingsMenu> {
 public:
    // initialize menu
    bool init();

    // Data Settings Manager APIs
    void setBackhaulPref(std::vector<std::string> inputCommand);
    void requestBackhaulPref(std::vector<std::string> inputCommand);
    void setBandInterferenceConfig(std::vector<std::string> inputCommand);
    void requestBandInterferenceConfig(std::vector<std::string> inputCommand);

    void setWwanConnectivityConfig(std::vector<std::string> inputCommand);
    void requestWwanConnectivityConfig(std::vector<std::string> inputCommand);
    void setMacSecState(std::vector<std::string> inputCommand);
    void requestMacSecState(std::vector<std::string> inputCommand);
    void setLatencyConfig(std::vector<std::string> inputCommand);
    void getLatencyConfig(std::vector<std::string> inputCommand);
    void cleanupSettings(std::vector<std::string> inputCommand);
    void setAutoConnectConfig(std::vector<std::string> inputCommand);
    void getAutoConnectConfig(std::vector<std::string> inputCommand);
    void onWwanConnectivityConfigChange(SlotId slotId, bool isConnectivityAllowed) override;
    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    DataSettingsMenu(std::string appName, std::string cursor);
    ~DataSettingsMenu();
 private:
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::map<telux::data::OperationType,
      std::shared_ptr<telux::data::IDataSettingsManager>> dataSettingsManagerMap_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool initDataSettingsManager(telux::data::OperationType opType);
    std::string cleanupTypeToString(telux::data::CleanupConfigType cleanupType);
    bool profileUserInput(OperationType &opType, telux::data::AutoConnectProfile &profile);
};
#endif
