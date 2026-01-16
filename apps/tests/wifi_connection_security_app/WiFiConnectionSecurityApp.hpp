/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CELLULARCONNECTIONSECURITYAPP_HPP
#define CELLULARCONNECTIONSECURITYAPP_HPP

#include <mutex>
#include <iostream>
#include <memory>
#include <future>
#include <condition_variable>

#include <telux/sec/ConnectionSecurityFactory.hpp>

#include "common/console_app_framework/ConsoleApp.hpp"

using namespace telux::sec;
using namespace telux::common;

class WiFiSecurityReportListener : public telux::sec::IWiFiReportListener {

 public:
    void isTrustedAP(telux::sec::ApInfo apInfo, bool &isTrusted) override;
    void onReportAvailable(telux::sec::WiFiSecurityReport report) override;
    void onDeauthenticationAttack(telux::sec::DeauthenticationInfo deauthenticationInfo) override;

    void setTrustAPSelection(bool trust);

    bool trustGivenAP_{false};
    bool trustAPSelectionMade_{false};
    std::mutex trustMutex_;
    std::condition_variable trustCV_;

 private:
    bool promptUserForTrustingAP_{false};
};

class WiFiConnectionSecurityApp : public ConsoleApp,
                                  public IServiceStatusListener,
                                  public std::enable_shared_from_this<WiFiConnectionSecurityApp> {

 public:
    WiFiConnectionSecurityApp(std::string appName, std::string cursor);
    ~WiFiConnectionSecurityApp();

    void initConsole();
    void onServiceStatusChange(ServiceStatus status) override;

    void init(void);
    void registerListener(void);
    void deregisterListener(void);
    void getTrustAPSelection(void);
    void getTrustedApList(void);
    void removeApFromTrustedList(void);

 private:
    std::shared_ptr<telux::sec::IWiFiSecurityManager> wifiConSecMgr_;
    std::shared_ptr<WiFiSecurityReportListener> reportListener_;

    void getStringFromUser(const std::string promptToDisplay, std::string &userData);
};

#endif  // CELLULARCONNECTIONSECURITYAPP_HPP
