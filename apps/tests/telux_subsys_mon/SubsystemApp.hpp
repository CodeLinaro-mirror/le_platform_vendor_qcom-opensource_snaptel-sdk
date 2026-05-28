/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SUBSYSTEMAPP_HPP
#define SUBSYSTEMAPP_HPP

#include <memory>

#include <telux/platform/SubsystemFactory.hpp>
#include <telux/platform/SubsystemManager.hpp>

#include "common/console_app_framework/ConsoleApp.hpp"

#include "UserUtils.hpp"

// Forward declaration
class SubsystemApp;

class StateChangeListener : public telux::platform::ISubsystemListener {

 public:
    StateChangeListener() = default;
    void setApp(std::weak_ptr<SubsystemApp> app) {
        app_ = app;
    }

    void onStateChange(telux::common::SubsystemInfo subsystemInfo,
        telux::common::OperationalStatus newOperationalStatus) override;

 private:
    std::weak_ptr<SubsystemApp> app_;
};

/* Registered automatically at startup to receive EDL notifications only. */
class EdlListener : public telux::platform::ISubsystemListener {

 public:
    void onEdlConfigUpdate(const telux::platform::EdlConfigs &edlConfigs) override;
    void onEdlStateChanged(telux::platform::EdlState edlState) override;
    void onEdlOperationResult(bool success) override;
};

class SubsystemApp : public ConsoleApp, public std::enable_shared_from_this<SubsystemApp> {

 public:
    SubsystemApp(std::string appName, std::string cursor);
    ~SubsystemApp();

    void init(void);
    void init(std::vector<telux::common::SubsystemInfo> subsystems, bool nonInteractive = false);
    void registerListener(void);
    void deRegisterListener(void);
    void triggerMpssRestart(void);
    void setEdlConfigurations(void);
    void getEdlConfigurations(void);
    void triggerEdl(void);
    void getEdlState(void);
    static std::string edlStateToString(telux::platform::EdlState edlState);

    // KPI logging methods
    void setKpiLoggingEnabled(bool enable);
    bool isKpiLoggingEnabled() const;
    void logSubsystemStateChange(telux::common::SubsystemInfo subsystemInfo,
        telux::common::OperationalStatus newOperationalStatus);

 private:
    UserUtils userUtils_;
    std::shared_ptr<StateChangeListener> stateChangeListener_;
    std::shared_ptr<EdlListener> edlListener_;
    std::shared_ptr<telux::platform::ISubsystemManager> subsystemMgr_;
    std::vector<telux::common::SubsystemInfo> cliSubsystems_;
    bool nonInteractive_;
    bool kpiLoggingEnabled_;

    void getSubsystemsToMonitor(std::vector<telux::common::SubsystemInfo> &listOfSubsystems);
};

#endif  // SUBSYSTEMAPP_HPP
