/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <future>
#include <chrono>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <map>

#include <telux/config/ConfigFactory.hpp>
#include <telux/config/ConfigManager.hpp>

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::config;
using namespace telux::common;

class ConfigListener : public IConfigListener {
  public:
    void onConfigUpdate(std::string key, std::string value) override;
    void onServiceStatusChange(ServiceStatus status) override;
    ~ConfigListener() {}
};

void ConfigListener::onConfigUpdate(std::string key, std::string value) {
    PRINT_NOTIFICATION << "\n*********** CONFIGURATION UPDATE *********************\n";
    std::cout << "Updated Key: " << key << " New Value: " << value << "\n";
}

void ConfigListener::onServiceStatusChange(ServiceStatus status) {
    PRINT_NOTIFICATION << "\n*********** SERVICE STATUS UPDATE *********************\n";
    switch(status) {
        case ServiceStatus::SERVICE_UNAVAILABLE : std::cout << "Service Unavailable \n";
                                                  break;
        case ServiceStatus::SERVICE_AVAILABLE   : std::cout << "Service Available \n";
                                                  break;
        case ServiceStatus::SERVICE_FAILED      : std::cout << "Service Failed \n";
                                                  break;
    }
}

int main(int argc, char **argv) {
    //Initialize Config manager and Config Listener.
    std::shared_ptr<IConfigManager> configManager = nullptr;
    std::shared_ptr<ConfigListener> configListener = nullptr;

    //Get the Config Factory instance to retrieve the Config manager.
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    auto &configFactory = ConfigFactory::getInstance();
    configManager = configFactory.getConfigManager([&](ServiceStatus status) {
        if (status == ServiceStatus::SERVICE_AVAILABLE) {
                prom.set_value(ServiceStatus::SERVICE_AVAILABLE);
            } else {
                prom.set_value(ServiceStatus::SERVICE_FAILED);
            }
        });
    std::chrono::time_point<std::chrono::steady_clock> startTime, endTime;
    startTime = std::chrono::steady_clock::now();
    ServiceStatus configMgrStatus = configManager->getServiceStatus();
    if(configMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Apps Config subsystem is not ready, Please wait" << std::endl;
    }
    //Wait for the subsystem to be available.
    configMgrStatus = prom.get_future().get();
    if(configMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
        endTime = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsedTime = endTime - startTime;
        std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count()
            << "s\n" << std::endl;
    } else {
        std::cout << "ERROR - Unable to initialize Apps Config subsystem" << std::endl;
        return EXIT_FAILURE;
    }

    //Register Listener since Manager is now available.
    configListener = std::make_shared<ConfigListener>();
    telux::common::Status status = configManager->registerListener(configListener);
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Reg Listener Request Failed" << std::endl;
    }

    //Retrieving all the configurations.
    auto configMap = configManager->getAllConfigs();
    std::cout << "Current config List - \n";
    for(auto itr: configMap) {
        std::cout << itr.first << " : " << itr.second << "\n";
    }

    //Setting a configuration. For example, setting FILE_LOG_LEVEL tp DEBUG.
    std::string key = "FILE_LOG_LEVEL";
    std::string value = "DEBUG";
    auto ret = configManager->setConfig(key, value);
    if(ret == Status::SUCCESS) {
        std::cout << "Success in setting config \n";
        //If we successfully set the configuration, we can just retrieve the value of the key set.
        std::string val = configManager->getConfig(key);
        std::cout << "Corresponding Value: " << val << "\n";
    } else {
        std::cout << "Failed to set config \n";
    }

    return 0;
}