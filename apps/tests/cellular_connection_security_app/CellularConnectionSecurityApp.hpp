/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CELLULARCONNECTIONSECURITYAPP_HPP
#define CELLULARCONNECTIONSECURITYAPP_HPP

#include <telux/sec/ConnectionSecurityFactory.hpp>

#include "common/console_app_framework/ConsoleApp.hpp"

class CellSecurityReportListener : public telux::sec::ICellularScanReportListener {

 public:
    void onScanReportAvailable(
        telux::sec::CellularSecurityReport report, telux::sec::EnvironmentInfo envInfo) override;

    void onServiceStatusChange(telux::common::ServiceStatus newStatus) override;
};

class CellularConnectionSecurityApp : public ConsoleApp {

 public:
    CellularConnectionSecurityApp(std::string appName, std::string cursor);
    ~CellularConnectionSecurityApp();

    void init(void);
    void registerListener(void);
    void deregisterListener(void);
    void getSessionStats(void);

 private:
    std::shared_ptr<telux::sec::ICellularSecurityManager> cellConSecMgr_;
    std::shared_ptr<CellSecurityReportListener> reportListener_;
};

#endif  // CELLULARCONNECTIONSECURITYAPP_HPP
