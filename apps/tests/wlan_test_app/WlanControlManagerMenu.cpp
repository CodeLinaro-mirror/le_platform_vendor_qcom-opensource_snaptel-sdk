/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanControlManagerMenu.hpp"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

WlanControlManagerMenu::WlanControlManagerMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
    menuOptionsAdded_       = false;
    subSystemStatusUpdated_ = false;
}

WlanControlManagerMenu::~WlanControlManagerMenu() {
    if (wlanControlManager_) {
        wlanControlManager_ = nullptr;
    }
}

bool WlanControlManagerMenu::isSubSystemReady() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    subSystemStatusUpdated_                      = false;
    if (wlanControlManager_ == nullptr) {
        auto initCb
            = std::bind(&WlanControlManagerMenu::onInitComplete, this, std::placeholders::_1);
        auto &wlanFactory   = telux::wlan::WlanFactory::getInstance();
        wlanControlManager_ = wlanFactory.getWlanControlManager(initCb);
        if (wlanControlManager_ == nullptr) {
            std::cout << "\nError encountered in initializing Wlan Control Manager" << std::endl;
            return false;
        }
        wlanControlManager_->registerListener(shared_from_this());
    }
    {
        std::unique_lock<std::mutex> lck(mtx_);
        std::cout << "\nInitializing WlanControlManager, Please wait ..." << std::endl;
        int const DEFAULT_TIMEOUT_SECONDS = 60;
        bool ready = cv_.wait_for(lck, std::chrono::seconds(DEFAULT_TIMEOUT_SECONDS),
            [this] { return this->subSystemStatusUpdated_; });
        if (!ready) {
            std::cout << "\nTimed out waiting for Wlan Control Manager initialization" << std::endl;
            wlanControlManager_ = nullptr;
            return false;
        }
        subSystemStatus = wlanControlManager_->getServiceStatus();
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nWlan Control Manager is Ready" << std::endl;
        } else {
            wlanControlManager_ = nullptr;
            return false;
        }
    }
    return true;
}

bool WlanControlManagerMenu::init() {
    if (menuOptionsAdded_ == false) {
        menuOptionsAdded_ = true;
        int stepID        = 1;

        std::shared_ptr<ConsoleAppCommand> getInterfaceStatus = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand(std::to_string(stepID++), "get_interface_status", {},
                std::bind(
                    &WlanControlManagerMenu::getInterfaceStatus, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setStaIpConfig = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand(std::to_string(stepID++), "set_sta_ip_config", {},
                std::bind(&WlanControlManagerMenu::setStaIpConfig, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> getStaIpConfig = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand(std::to_string(stepID++), "get_sta_ip_config", {},
                std::bind(&WlanControlManagerMenu::getStaIpConfig, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> setApInterworking = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand(std::to_string(stepID++), "set_ap_interworking", {},
                std::bind(
                    &WlanControlManagerMenu::setApInterworking, this, std::placeholders::_1)));

        std::shared_ptr<ConsoleAppCommand> getApInterworking = std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand(std::to_string(stepID++), "get_ap_interworking", {},
                std::bind(
                    &WlanControlManagerMenu::getApInterworking, this, std::placeholders::_1)));

        std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {getInterfaceStatus,
            setStaIpConfig, getStaIpConfig, setApInterworking, getApInterworking};
        addCommands(commandsList);
    }
    ConsoleApp::displayMenu();
    return true;
}

void WlanControlManagerMenu::showMenu() {
    ConsoleApp::displayMenu();
}

// ---------------------------------------------------------------------------
// Initialization callback
// ---------------------------------------------------------------------------

void WlanControlManagerMenu::onInitComplete(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

// ---------------------------------------------------------------------------
// Menu command handlers
// ---------------------------------------------------------------------------

void WlanControlManagerMenu::getInterfaceStatus(std::vector<std::string> userInput) {
    std::cout << "Request WLAN Control Interface Status" << std::endl;

    std::vector<telux::wlan::InterfaceStatus> status;
    telux::common::ErrorCode retCode = wlanControlManager_->getInterfaceStatus(status);

    std::cout << "\nGet Interface Status Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;

    if (retCode == telux::common::ErrorCode::SUCCESS) {
        if (status.size() > 0) {
            for (auto &ifStatus : status) {
                std::cout << "------------------------------------------" << std::endl;
                std::cout
                    << "Device             : " << WlanUtils::getWlanDeviceName(ifStatus.device)
                    << std::endl;
                WlanUtils::printAPStatus(ifStatus.apStatus);
                WlanUtils::printStaStatus(ifStatus.staStatus);
            }
        } else {
            std::cout << "No active WLAN interfaces found" << std::endl;
        }
    }
}

void WlanControlManagerMenu::setStaIpConfig(std::vector<std::string> userInput) {
    std::cout << "Set Station IP Configuration (Control Manager)" << std::endl;

    int staId = 1;
    std::cout << "Enter Station Id (1-PRIMARY, 2-SECONDARY): ";
    std::cin >> staId;
    WlanUtils::validateInput(staId, {1, 2});
    std::cout << std::endl;

    int ipConfig = 1;
    std::cout << "Select Station IP Type (1-Dynamic IP, 2-Static IP): ";
    std::cin >> ipConfig;
    WlanUtils::validateInput(ipConfig, {1, 2});
    std::cout << std::endl;

    telux::wlan::StaIpConfig staIpConfig          = {};
    telux::wlan::StaStaticIpConfig staticIpConfig = {};

    if (ipConfig == 2) {
        staIpConfig = telux::wlan::StaIpConfig::STATIC_IP;
        std::string input{};

        std::cout << "Enter IPv4 Address: ";
        std::cin >> input;
        Utils::validateInput(input);
        staticIpConfig.ipAddr = input;
        std::cout << std::endl;

        std::cout << "Enter Gateway IPv4 Address: ";
        std::cin >> input;
        Utils::validateInput(input);
        staticIpConfig.gwIpAddr = input;
        std::cout << std::endl;

        std::cout << "Enter Subnet Mask: ";
        std::cin >> input;
        Utils::validateInput(input);
        staticIpConfig.netMask = input;
        std::cout << std::endl;

        std::cout << "Enter DNS IPv4 Address: ";
        std::cin >> input;
        Utils::validateInput(input);
        staticIpConfig.dnsAddr = input;
        std::cout << std::endl;
    } else {
        staIpConfig = telux::wlan::StaIpConfig::DYNAMIC_IP;
    }

    telux::common::ErrorCode retCode = wlanControlManager_->setStaIpConfig(
        static_cast<telux::wlan::Id>(staId), staIpConfig, staticIpConfig);

    std::cout << "\nSet Station IP Configuration Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanControlManagerMenu::getStaIpConfig(std::vector<std::string> userInput) {
    std::cout << "Get Station IP Configuration (Control Manager)" << std::endl;

    int staId = 1;
    std::cout << "Enter Station Id (1-PRIMARY, 2-SECONDARY): ";
    std::cin >> staId;
    WlanUtils::validateInput(staId, {1, 2});
    std::cout << std::endl;

    telux::wlan::StaIpConfig ipConfig             = {};
    telux::wlan::StaStaticIpConfig staticIpConfig = {};

    telux::common::ErrorCode retCode = wlanControlManager_->getStaIpConfig(
        static_cast<telux::wlan::Id>(staId), ipConfig, staticIpConfig);

    std::cout << "\nGet Station IP Configuration Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;

    if (retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "IP Config: "
                  << (ipConfig == telux::wlan::StaIpConfig::STATIC_IP ? "Static IP" : "Dynamic IP")
                  << std::endl;
        if (ipConfig == telux::wlan::StaIpConfig::STATIC_IP) {
            std::cout << "IPv4 Address : " << staticIpConfig.ipAddr << std::endl;
            std::cout << "Gateway      : " << staticIpConfig.gwIpAddr << std::endl;
            std::cout << "Subnet Mask  : " << staticIpConfig.netMask << std::endl;
            std::cout << "DNS Address  : " << staticIpConfig.dnsAddr << std::endl;
        }
    }
}

void WlanControlManagerMenu::setApInterworking(std::vector<std::string> userInput) {
    std::cout << "Set AP Interworking" << std::endl;

    int apId = 0;
    std::cout << "Enter AP Id (1-PRIMARY, 2-SECONDARY, 3-TERTIARY, 4-QUATERNARY): ";
    std::cin >> apId;
    WlanUtils::validateInput(apId, {1, 2, 3, 4});
    std::cout << std::endl;

    int interworking = 0;
    std::cout << "Select AP Interworking (0-INTERNET_ACCESS, 1-FULL_ACCESS): ";
    std::cin >> interworking;
    WlanUtils::validateInput(interworking, {0, 1});
    std::cout << std::endl;

    telux::common::ErrorCode retCode
        = wlanControlManager_->setApInterworking(WlanUtils::convertIntToWlanId(apId),
            static_cast<telux::wlan::ApInterworking>(interworking));

    std::cout << "\nSet AP Interworking Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
}

void WlanControlManagerMenu::getApInterworking(std::vector<std::string> userInput) {
    std::cout << "Get AP Interworking" << std::endl;

    int apId = 0;
    std::cout << "Enter AP Id (1-PRIMARY, 2-SECONDARY, 3-TERTIARY, 4-QUATERNARY): ";
    std::cin >> apId;
    WlanUtils::validateInput(apId, {1, 2, 3, 4});
    std::cout << std::endl;

    telux::wlan::ApInterworking interworking;
    telux::common::ErrorCode retCode
        = wlanControlManager_->getApInterworking(WlanUtils::convertIntToWlanId(apId), interworking);

    std::cout << "\nGet AP Interworking Response"
              << (retCode == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
              << ". ErrorCode: " << static_cast<int>(retCode)
              << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;

    if (retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout
            << "AP Interworking: "
            << (interworking == telux::wlan::ApInterworking::INTERNET_ACCESS ? "INTERNET_ACCESS"
                                                                             : "FULL_ACCESS")
            << std::endl;
    }
}

// ---------------------------------------------------------------------------
// IWlanControlListener callbacks
// ---------------------------------------------------------------------------

void WlanControlManagerMenu::onServiceStatusChange(telux::common::ServiceStatus status) {
    PRINT_NOTIFICATION << " ** WlanControl onServiceStatusChange **\n";
    switch (status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            std::cout << " SERVICE_AVAILABLE\n";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            std::cout << " SERVICE_UNAVAILABLE\n";
            break;
        default:
            std::cout << " Unknown service status\n";
            break;
    }
}

void WlanControlManagerMenu::onApStatusChanged(const std::vector<telux::wlan::ApStatus> &status) {
    PRINT_NOTIFICATION << " ** WlanControl onApStatusChanged **\n";
    std::vector<telux::wlan::ApStatus> statusCopy(status);
    WlanUtils::printAPStatus(statusCopy);
}

void WlanControlManagerMenu::onStationStatusChanged(
    const std::vector<telux::wlan::StaStatus> &staStatus) {
    PRINT_NOTIFICATION << " ** WlanControl onStationStatusChanged **\n";
    std::vector<telux::wlan::StaStatus> statusCopy(staStatus);
    WlanUtils::printStaStatus(statusCopy);
}
