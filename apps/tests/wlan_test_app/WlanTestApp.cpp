/*
 * Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "WlanTestApp.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanTestApp::WlanTestApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

WlanTestApp::~WlanTestApp() {
    wlanDeviceManagerMenu_ = nullptr;
    wlanApInterfaceManagerMenu_ = nullptr;
}

bool WlanTestApp::initWlan() {
    if (wlanDeviceManagerMenu_ == nullptr) {
        wlanDeviceManagerMenu_ =
            std::make_shared<WlanDeviceManagerMenu>("Device Manager Menu", "device> ");
    }
    if (wlanDeviceManagerMenu_->isSubSystemReady()) {
        std::cout << "Wlan Subsystem is Ready" << std::endl;
        wlanApInterfaceManagerMenu_ =
            std::make_shared<WlanApInterfaceManagerMenu>("Ap Interface Manager Menu", "ap> ");
        if(wlanApInterfaceManagerMenu_) {
            wlanApInterfaceManagerMenu_->init() ;
        }
        wlanStaInterfaceManagerMenu_ =
            std::make_shared<WlanStaInterfaceManagerMenu>("Station Interface Manager Menu", "sta> ");
        if(wlanStaInterfaceManagerMenu_) {
            wlanStaInterfaceManagerMenu_->init();
        }
        return true;
    }
    return false;
}

bool WlanTestApp::init() {
    if (initWlan()) {
        std::shared_ptr<ConsoleAppCommand> wlanDeviceManagerMenu =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "device_manager_Menu",
            {}, std::bind(&WlanTestApp::wlanDeviceManagerMenu, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> wlanApInterfaceManagerMenu =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "ap_interface_manager_menu",
            {}, std::bind(&WlanTestApp::wlanApInterfaceManagerMenu, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> wlanStaInterfaceManagerMenu =
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "sta_interface_manager_menu",
            {}, std::bind(&WlanTestApp::wlanStaInterfaceManagerMenu, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {wlanDeviceManagerMenu,
            wlanApInterfaceManagerMenu, wlanStaInterfaceManagerMenu};

        addCommands(commandsList);
        ConsoleApp::displayMenu();
        return true;
    }
    else {
        return false;
    }
}

void WlanTestApp::wlanDeviceManagerMenu(std::vector<std::string> inputCommand) {
    if (wlanDeviceManagerMenu_->init()) {
        wlanDeviceManagerMenu_->mainLoop();
    }
    ConsoleApp::displayMenu();
}

void WlanTestApp::wlanApInterfaceManagerMenu(std::vector<std::string> inputCommand) {
    if (wlanApInterfaceManagerMenu_) {
        wlanApInterfaceManagerMenu_->showMenu();
        wlanApInterfaceManagerMenu_->mainLoop();
    }
    ConsoleApp::displayMenu();
}

void WlanTestApp::wlanStaInterfaceManagerMenu(std::vector<std::string> inputCommand) {
    if (wlanStaInterfaceManagerMenu_) {
        wlanStaInterfaceManagerMenu_->showMenu();
        wlanStaInterfaceManagerMenu_->mainLoop();
    }
    ConsoleApp::displayMenu();
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {
    auto sdkVersion = telux::common::Version::getSdkVersion();
    std::string appName = "Wlan Test App v" + std::to_string(sdkVersion.major) + "."
                          + std::to_string(sdkVersion.minor) + "."
                          + std::to_string(sdkVersion.patch);
    WlanTestApp wlanTestApp(appName, "Wlan> ");
    if (wlanTestApp.init()) {       // initialize commands and display
        wlanTestApp.mainLoop();     // Main loop to continuously read and execute commands
    }
    return 0;
}
