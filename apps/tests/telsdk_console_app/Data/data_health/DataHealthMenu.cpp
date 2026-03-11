/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

extern "C" {
#include "unistd.h"
}

#include "../../../../common/utils/Utils.hpp"

#include "DataHealthMenu.hpp"
#include "../DataUtils.hpp"

using namespace std;

// ============================================================================
// DataHealthListener Implementation
// ============================================================================

void DataHealthListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "CALLBACK: onServiceStatusChange" << std::endl;
    std::cout << "Service Status: " << static_cast<int>(status) << std::endl;

    switch (status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            std::cout << "Status: SERVICE_AVAILABLE" << std::endl;
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            std::cout << "Status: SERVICE_UNAVAILABLE" << std::endl;
            break;
        case telux::common::ServiceStatus::SERVICE_FAILED:
            std::cout << "Status: SERVICE_FAILED" << std::endl;
            break;
        default:
            std::cout << "Status: UNKNOWN" << std::endl;
            break;
    }
    std::cout << "========================================\n" << std::endl;
}

void DataHealthListener::onDataStallDetectionStateChanged(
    const DataStallDetectionState &state) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "CALLBACK: onDataStallDetectionStateChanged" << std::endl;
    std::cout << "Network Module: " << DataUtils::networkModuleToString(state.module)
              << std::endl;
    std::cout << "Detection Enabled: " << (state.enabled ? "Yes" : "No") << std::endl;

    if (!state.enabled) {
        std::cout << "Disablement Reason: "
        << DataUtils::disablementReasonToString(state.reason) << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}

void DataHealthListener::onDataStallDetected(const DataStallInfo &info) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "CALLBACK: onDataStallDetected" << std::endl;
    std::cout << "Network Module: " << DataUtils::networkModuleToString(info.module)
              << std::endl;
    std::cout << "Number of Instances: " << info.numOfInstances << std::endl;
    std::cout << "Recovery Action: " << DataUtils::recoveryActionToString(info.action)
              << std::endl;
    std::cout << "Stall Reason: " << DataUtils::stallReasonToString(info.reason)
              << std::endl;
    std::cout << "========================================\n" << std::endl;
}

void DataHealthListener::onDataStallRecoveryTriggered(const DataStallRecoveryStatus &status) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "CALLBACK: onDataStallRecoveryTriggered" << std::endl;
    std::cout << "Network Module: " << DataUtils::networkModuleToString(status.module)
              << std::endl;
    std::cout << "Recovery Action: "
              << DataUtils::recoveryActionToString(status.recoveryAction) << std::endl;
    std::cout << "Recovery Result: " << DataUtils::recoveryResultToString(status.status)
              << std::endl;
    std::cout << "Is Recoverable: " << (status.isRecoverable ? "Yes" : "No") << std::endl;

    if (!status.isRecoverable) {
        std::cout << "\n*** WARNING: Data stall cannot be automatically recovered ***"
                  << std::endl;
        std::cout << "*** OEM/Application intervention is required ***" << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}

void DataHealthListener::onDataStallRecoveryRestartTimerUpdate(
    const DataStallRecoveryRestartStatus &status) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "CALLBACK: onDataStallRecoveryRestartTimerUpdate" << std::endl;
    std::cout << "Network Module: " << DataUtils::networkModuleToString(status.module)
              << std::endl;
    std::cout << "Timer Status: " << DataUtils::restartTimerStatusToString(status.status)
              << std::endl;

    switch (status.status) {
        case DataStallRestartTimerStatus::STARTED:
            std::cout << "Grace period for manual intervention has started" << std::endl;
            break;
        case DataStallRestartTimerStatus::EXPIRED:
            std::cout << "Grace period has expired, automatic recovery will resume"
                      << std::endl;
            break;
        default:
            break;
    }
    std::cout << "========================================\n" << std::endl;
}

// ============================================================================
// DataHealthMenu Implementation
// ============================================================================

DataHealthMenu::DataHealthMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   subSystemStatusUpdated_ = false;
}

DataHealthMenu::~DataHealthMenu() {
    if (dataHealthManager_ && dataHealthListener_) {
        dataHealthManager_->deregisterListener(dataHealthListener_);
    }
    dataHealthManager_.reset();
    dataHealthListener_.reset();
}

bool DataHealthMenu::init() {
    bool dhmSubSystemStatus = initHealthManagerAndListener();

    std::shared_ptr<ConsoleAppCommand> configureDetection =
        std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("1",
                              "register_for_data_stall_detection_indications",
                              {},
                              std::bind(&DataHealthMenu::configureDataStallDetection,
                                        this,
                                        std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> setDataStallCfgCmd =
        std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("2",
                              "set_data_stall_config",
                              {},
                              std::bind(&DataHealthMenu::setDataStallConfig,
                                        this,
                                        std::placeholders::_1)));

    std::shared_ptr<ConsoleAppCommand> getDataStallCfgCmd =
        std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("3",
                              "get_data_stall_config",
                              {},
                              std::bind(&DataHealthMenu::getDataStallConfig,
                                        this,
                                        std::placeholders::_1)));
    std::shared_ptr<ConsoleAppCommand> getRegisteredModulesCmd =
        std::make_shared<ConsoleAppCommand>(
            ConsoleAppCommand("4",
                              "get_enabled_data_stall_modules",
                              {},
                              std::bind(&DataHealthMenu::getEnabledDataStallModules,
                                        this,
                                        std::placeholders::_1)));


    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList = {
        configureDetection,
        setDataStallCfgCmd,
        getDataStallCfgCmd,
        getRegisteredModulesCmd,
    };

    addCommands(commandsList);
    return dhmSubSystemStatus;
}

bool DataHealthMenu::displayMenu() {
    bool retVal = true;
    if (dataHealthManager_ &&
        (telux::common::ServiceStatus::SERVICE_AVAILABLE ==
        dataHealthManager_->getServiceStatus())) {
        std::cout << "\nData Health Manager is ready" << std::endl;
    } else {
        std::cout << "\nData Health Manager is not ready" << std::endl;
        retVal = false;
    }
    ConsoleApp::displayMenu();
    return retVal;
}

bool DataHealthMenu::initHealthManagerAndListener() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    bool retValue = false;
    subSystemStatusUpdated_ = false;

    auto initCb = std::bind(&DataHealthMenu::onInitCompleted, this, std::placeholders::_1);

    // Get the DataFactory instance
    auto &dataFactory = telux::data::DataFactory::getInstance();
    dataHealthManager_ = dataFactory.getDataHealthManager(initCb);

    if (dataHealthManager_) {
        // Create and register listener
        dataHealthListener_ = std::make_shared<DataHealthListener>();

        telux::common::Status status =
            dataHealthManager_->registerListener(dataHealthListener_);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "ERROR - Failed to register listener. Status: "
                      << static_cast<int>(status) << std::endl;
            return false;
        }

        // Initialize data health manager
        std::cout << "\n\nInitializing Data Health Manager subsystem, Please wait ..."
                  << std::endl;
        std::unique_lock<std::mutex> lck(mtx_);

        // Set init wait timeout to 60 sec
        constexpr auto timeout = std::chrono::seconds(60);
        if (!cv_.wait_for(lck, timeout, [this]{return this->subSystemStatusUpdated_;})) {
            std::cout << "\nERROR - Data Health Manager initialization timed out" << std::endl;
            return false;
        }

        subSystemStatus = dataHealthManager_->getServiceStatus();

        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "\nData Health Manager is ready" << std::endl;
            retValue = true;
        } else {
            std::cout << "\nData Health Manager is not ready" << std::endl;
            return false;
        }
    } else {
        std::cout << "Data Health Manager failed to initialize" << std::endl;
    }
    return retValue;
}

void DataHealthMenu::onInitCompleted(telux::common::ServiceStatus status) {
    std::lock_guard<std::mutex> lock(mtx_);
    subSystemStatusUpdated_ = true;
    cv_.notify_all();
}

DataStallNetworkModule DataHealthMenu::getNetworkModuleFromUser() {
    int moduleChoice;
    std::cout << "\nSelect Network Module:" << std::endl;
    std::cout << "0 - ETHERNET" << std::endl;
    std::cout << "1 - IPA" << std::endl;
    std::cout << "2 - WWAN" << std::endl;
    std::cout << "3 - WLAN" << std::endl;
    std::cout << "Enter choice: ";
    std::cin >> moduleChoice;
    Utils::validateInput(moduleChoice, {0, 1, 2, 3});

    return static_cast<DataStallNetworkModule>(moduleChoice);
}

void DataHealthMenu::configureDataStallDetection(std::vector<std::string> inputCommand) {
    std::cout << "\nConfigure Data Stall Detection" << std::endl;

    if (!dataHealthManager_) {
        std::cout << "Data Health Manager is not initialized" << std::endl;
        return;
    }

    // Get enable/disable choice
    int enableChoice;
    std::cout << "\nSelect Action:" << std::endl;
    std::cout << "0 - Disable" << std::endl;
    std::cout << "1 - Enable" << std::endl;
    std::cout << "Enter choice: ";
    std::cin >> enableChoice;
    Utils::validateInput(enableChoice, {0, 1});

    bool enable = (enableChoice == 1);

    // Get network module
    DataStallNetworkModule module = getNetworkModuleFromUser();

    std::cout << "\n" << (enable ? "Enabling" : "Disabling")
              << " data stall detection for "
              << DataUtils::networkModuleToString(module) << " module..." << std::endl;

    telux::common::ErrorCode retCode =
       dataHealthManager_->enableDataStallDetection(enable, module);

    if (retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Successfully " << (enable ? "enabled" : "disabled")
                  << " data stall detection" << std::endl;
    } else {
        std::cout << "Failed to " << (enable ? "enable" : "disable")
                  << " data stall detection. ErrorCode: "
                  << static_cast<int>(retCode)
                  << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    }
}

void DataHealthMenu::setDataStallConfig(std::vector<std::string> inputCommand) {
    std::cout << "\nSet Data Stall Config" << std::endl;

    if (!dataHealthManager_) {
        std::cout << "Data Health Manager is not initialized" << std::endl;
        return;
    }

    // Get network module
    DataStallNetworkModule module = getNetworkModuleFromUser();

    DataStallConfig config;
    config.module = module;

    // For WWAN, get slotId and profileId
    if (module == DataStallNetworkModule::WWAN) {
        config.slotId = DEFAULT_SLOT_ID;

        int profileId;
        std::cout << "Enter Profile Id : ";
        std::cin >> profileId;
        if (profileId != -1) {
            config.profileId = profileId;
        }
    }

    // Get enableRecovery flag
    int enableRecovery;
    std::cout << "Enable Recovery? (0-No, 1-Yes): ";
    std::cin >> enableRecovery;
    Utils::validateInput(enableRecovery, {0, 1});
    config.enableRecovery = (enableRecovery == 1);

    // Get enableRecoveryRestart flag
    int enableRecoveryRestart;
    std::cout << "Enable Recovery Restart? (0-No, 1-Yes): ";
    std::cin >> enableRecoveryRestart;
    Utils::validateInput(enableRecoveryRestart, {0, 1});
    config.enableRecoveryRestart = (enableRecoveryRestart == 1);

    // If enableRecoveryRestart is true, get recoveryRestartTimeDuration
    if (config.enableRecoveryRestart) {
        uint32_t recoveryRestartTimeDuration;
        std::cout << "Enter Recovery Restart Time Duration (seconds): ";
        std::cin >> recoveryRestartTimeDuration;
        config.recoveryRestartTimeDuration = recoveryRestartTimeDuration;
    }

    // Get packetStatsTimerInterval
    uint32_t packetStatsTimerInterval;
    std::cout << "Enter Packet Stats Timer Interval (minimum 20 seconds): ";
    std::cin >> packetStatsTimerInterval;
    // Validate minimum value
    if (packetStatsTimerInterval < 20) {
        std::cout << "Warning: Minimum accepted value is 20 seconds. Using 20 instead." << std::endl;
        packetStatsTimerInterval = 20;
    }
    config.packetStatsTimerInterval = packetStatsTimerInterval;

    // For WWAN, get extended detection config
    if (module == DataStallNetworkModule::WWAN) {
        int activeConnCheckEnable;
        std::cout << "Enable Active Connectivity Check? (0-No, 1-Yes): ";
        std::cin >> activeConnCheckEnable;
        Utils::validateInput(activeConnCheckEnable, {0, 1});
        config.extendedConfig.activeConnCheckEnable = (activeConnCheckEnable == 1);

        if (config.extendedConfig.activeConnCheckEnable) {
            std::string pingServerAddressV4;
            std::cout << "Enter IPv4 Ping Server Address (or press Enter to skip): ";
            std::cin.ignore();
            std::getline(std::cin, pingServerAddressV4);
            if (!pingServerAddressV4.empty()) {
                config.extendedConfig.pingServerAddressV4 = pingServerAddressV4;
            }

            std::string pingServerAddressV6;
            std::cout << "Enter IPv6 Ping Server Address (or press Enter to skip): ";
            std::getline(std::cin, pingServerAddressV6);
            if (!pingServerAddressV6.empty()) {
                config.extendedConfig.pingServerAddressV6 = pingServerAddressV6;
            }
        }

        int txThresholdInput;
        std::cout << "Enter TX Threshold (1-255): ";
        std::cin >> txThresholdInput;
        if (txThresholdInput < 1 || txThresholdInput > 255) {
            std::cout << "Warning: TX Threshold must be between 1 and 255. Using 1 instead." << std::endl;
            config.extendedConfig.txThreshold = 1;
        } else {
            config.extendedConfig.txThreshold = static_cast<std::uint8_t>(txThresholdInput);
        }

        int rxThresholdInput;
        std::cout << "Enter RX Threshold (1-255): ";
        std::cin >> rxThresholdInput;
        if (rxThresholdInput < 1 || rxThresholdInput > 255) {
            std::cout << "Warning: RX Threshold must be between 1 and 255. Using 1 instead." << std::endl;
            config.extendedConfig.rxThreshold = 1;
        } else {
            config.extendedConfig.rxThreshold = static_cast<std::uint8_t>(rxThresholdInput);
        }
    }

    std::cout << "\nSetting data stall config for "
              << DataUtils::networkModuleToString(module) << " module..." << std::endl;

    telux::common::ErrorCode retCode = dataHealthManager_->setDataStallConfig(config);

    if (retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Successfully set data stall config" << std::endl;
        DataUtils::printDataStallConfig(config);
    } else {
        std::cout << "Failed to set data stall config. ErrorCode: "
                  << static_cast<int>(retCode)
                  << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    }
}

void DataHealthMenu::getDataStallConfig(std::vector<std::string> inputCommand) {
    std::cout << "\nGet Data Stall Config" << std::endl;

    if (!dataHealthManager_) {
        std::cout << "Data Health Manager is not initialized" << std::endl;
        return;
    }

    // Get network module
    DataStallNetworkModule module = getNetworkModuleFromUser();

    DataStallConfig config;
    config.module = module;

    // For WWAN, get profileId (optional)
    if (module == DataStallNetworkModule::WWAN) {
        int profileId;
        std::cout << "Enter Profile Id : ";
        std::cin >> profileId;
        if (profileId != -1) {
            config.profileId = profileId;
        }
    }

    std::cout << "\nGetting data stall config for "
              << DataUtils::networkModuleToString(module) << " module..." << std::endl;

    telux::common::ErrorCode retCode = dataHealthManager_->getDataStallConfig(config);

    if (retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Successfully retrieved data stall config" << std::endl;
        DataUtils::printDataStallConfig(config);
    } else {
        std::cout << "Failed to get data stall config. ErrorCode: "
                  << static_cast<int>(retCode)
                  << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    }
}

void DataHealthMenu::getEnabledDataStallModules(std::vector<std::string> inputCommand) {
    std::cout << "\nGet Registered Data Stall Modules" << std::endl;

    if (!dataHealthManager_) {
        std::cout << "Data Health Manager is not initialized" << std::endl;
        return;
    }

    std::vector<DataStallNetworkModule> modules;
    telux::common::ErrorCode retCode =
        dataHealthManager_->getEnabledDataStallModules(modules);

    if (retCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Successfully retrieved registered data stall modules" << std::endl;
        std::cout << "Registered modules:" << std::endl;

        if (modules.empty()) {
            std::cout << "  No modules registered for data stall detection" << std::endl;
        } else {
            for (const auto& module : modules) {
                std::cout << "  - " << DataUtils::networkModuleToString(module) << std::endl;
            }
        }
    } else {
        std::cout << "Failed to get registered data stall modules. ErrorCode: "
                  << static_cast<int>(retCode)
                  << ", description: " << Utils::getErrorCodeAsString(retCode) << std::endl;
    }
}