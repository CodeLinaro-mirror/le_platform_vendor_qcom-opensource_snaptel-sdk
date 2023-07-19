/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>

#include <telux/common/Version.hpp>

#include "common/utils/Utils.hpp"

#include "WiFiConnectionSecurityApp.hpp"

WiFiConnectionSecurityApp::WiFiConnectionSecurityApp(
        std::string appName, std::string cursor) : ConsoleApp(appName, cursor) {
}

WiFiConnectionSecurityApp::~WiFiConnectionSecurityApp() {
}

/*
 *  Listener to receive ML analysis result.
 */
void WiFiSecurityReportListener::onReportAvailable(
        telux::sec::WiFiSecurityReport report) {

    std::cout << "ssid             : " << report.ssid << std::endl;
    std::cout << "bssid            : " << report.bssid << std::endl;
    std::cout << "is connected     : " << report.isConnectedToAP << std::endl;
    std::cout << "is open          : " << report.isOpenAP << std::endl;
    std::cout << "ml threat score  : " <<
        report.mlAlgorithmAnalysis.threatScore << std::endl;
    std::cout << "ml result        : " <<
        static_cast<int>(report.mlAlgorithmAnalysis.result) << std::endl;
    std::cout << "summoning result : " <<
        static_cast<int>(report.summoningAnalysis.result) << std::endl;
}

/*
 *  Listener to receive deauthentication attack info.
 */
void WiFiSecurityReportListener::onDeauthenticationAttack(
        telux::sec::DeauthenticationInfo deauthenticationInfo) {

    std::cout << "disconnect reason : " <<
        deauthenticationInfo.deauthenticationReason << std::endl;
    std::cout << "did AP initiated  : " <<
        deauthenticationInfo.didAPInitiateDisconnect << std::endl;
    std::cout << "threat score      : " <<
        deauthenticationInfo.threatScore << std::endl;
}

/*
 *  Sets the value based on selection made by user with the help of UI previously.
 */
void WiFiSecurityReportListener::isTrustedAP(std::string ssid, bool& isTrusted) {

    std::cout << "Please press 3 to trust/distrust AP " << ssid << std::endl;

    /* Wait until user makes confirms to trust or distrust AP */
    std::unique_lock<std::mutex> lock(trustMutex_);
    promptUserForTrustingAP_ = true;
    trustCV_.wait(lock, [this]{return trustAPSelectionMade_;});

    /* Return user selection */
    isTrusted = trustGivenAP_;

    /* Reset local state */
    trustAPSelectionMade_ = false;
    trustGivenAP_ = false;
    promptUserForTrustingAP_ = false;
}

/*
 *  Save the user selection for trusting the AP.
 */
void WiFiSecurityReportListener::setTrustAPSelection(bool trust) {

    std::lock_guard<std::mutex> lock(trustMutex_);
    if (!promptUserForTrustingAP_) {
        /* User exercised option without prompting, don't do anything */
        return;
    }

    trustGivenAP_ = trust;
    trustAPSelectionMade_ = true;

    /* Pass the user selection to the waiter thread */
    trustCV_.notify_all();
}

/*
 *  Define whether to trust the AP or not when device connects to an AP and
 *  user selection listener is invoked.
 */
void WiFiConnectionSecurityApp::getTrustAPSelection() {

    std::string usrInput = "";
    size_t choiceLength;

    if (!reportListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    while(1) {
        std::cout << "do you trust this AP (yes/no): ";

        if ((!std::getline(std::cin, usrInput)) || usrInput.empty()) {
            std::cout << "invalid input " << usrInput << std::endl;
            continue;
        }

        choiceLength = usrInput.length();
        for (size_t x = 0; x < choiceLength; x++) {
            usrInput[x] = tolower(usrInput[x]);
        }

        if (!usrInput.compare("no")) {
            reportListener_->setTrustAPSelection(false);
            return;
        } else if (!usrInput.compare("yes")) {
            reportListener_->setTrustAPSelection(true);
            return;
        } else {
            std::cout << "invalid input " << usrInput << std::endl;
        }
    }
}

/*
 *  Register listener to start listening for security analysis reports.
 */
void WiFiConnectionSecurityApp::registerListener() {

    telux::common::ErrorCode ec;

    if (reportListener_) {
        std::cout << "Listener exist" << std::endl;
        return;
    }

    try {
        reportListener_ = std::make_shared<WiFiSecurityReportListener>();
    } catch (const std::exception& e) {
        std::cout << "can't allocate WiFiReportListener" << std::endl;
    }

    ec = wifiConSecMgr_->registerListener(reportListener_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't register listener, err " << static_cast<int>(ec) << std::endl;
        reportListener_ = nullptr;
        return;
    }

    std::cout << "Listener registered" << std::endl;
}

/*
 *  Deregister listener to stop receiving security reports.
 */
void WiFiConnectionSecurityApp::deregisterListener() {

    telux::common::ErrorCode ec;

    if (!reportListener_) {
        std::cout << "Listener doesn't exist" << std::endl;
        return;
    }

    ec = wifiConSecMgr_->deRegisterListener(reportListener_);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't register listener, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    reportListener_ = nullptr;
    std::cout << "Listener deregistered" << std::endl;
}

/*
 *  Prepare the menu and display it on the console.
 */
void WiFiConnectionSecurityApp::init() {

    telux::common::ErrorCode ec;

    auto &wifiConSecFact = telux::sec::ConnectionSecurityFactory::getInstance();

    wifiConSecMgr_ = wifiConSecFact.getWiFiSecurityManager(ec);
    if (!wifiConSecMgr_) {
        std::cout <<
         "can't get IWiFiSecurityManager, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    std::shared_ptr<ConsoleAppCommand> regListener = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("1", "Start listening to security reports", {},
        std::bind(&WiFiConnectionSecurityApp::registerListener, this)));

    std::shared_ptr<ConsoleAppCommand> deregListener = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("2", "Stop listening to security reports", {},
        std::bind(&WiFiConnectionSecurityApp::deregisterListener, this)));

    std::shared_ptr<ConsoleAppCommand> getTrustAPSelection = std::make_shared<
        ConsoleAppCommand>(ConsoleAppCommand("3", "Trust the AP (yes/no)", {},
        std::bind(&WiFiConnectionSecurityApp::getTrustAPSelection, this)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> mainCmds = {
        regListener, deregListener, getTrustAPSelection };

    ConsoleApp::addCommands(mainCmds);
    ConsoleApp::displayMenu();
}

int main(int argc, char **argv) {

    auto sdkVersion = telux::common::Version::getSdkVersion();

    std::string sdkReleaseName = telux::common::Version::getReleaseName();

    std::string appName = "WiFi connection security console app - SDK v"
                            + std::to_string(sdkVersion.major) + "."
                            + std::to_string(sdkVersion.minor) + "."
                            + std::to_string(sdkVersion.patch) + "\n"
                            + "Release name: " + sdkReleaseName;

    auto wcsApp = std::make_shared<WiFiConnectionSecurityApp>(appName, "wificonsec> ");

    std::vector<std::string> supplementaryGrps{"system", "diag", "gps"};

    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc < 0) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }

    wcsApp->init();

    return wcsApp->mainLoop();
}
