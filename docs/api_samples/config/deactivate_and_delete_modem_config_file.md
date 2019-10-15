# How to deactivate and delete modem config file

Please follow below steps to deactivate and delete modem config file

### 1. Get the ConfigFactory and ModemConfigManager instances
   ~~~~~~{.cpp}
    auto &configFactory = telux::config::ConfigFactory::getInstance();
    auto modemConfigManager = configFactory.getModemConfigManager();
   ~~~~~~

### 2. Wait for the Modem Config sub system initialization.
   ~~~~~~{.cpp}
    bool subSystemStatus = modemConfigManager->isSubsystemReady();
    if (!subSystemStatus) {
        std::cout << "Modem Config subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = modemConfigManager_->onSubsystemReady();
        // Wait unconditionally for modem config subsystem to be ready
        subSystemStatus = f.get();
    }
    // Exit the application, if SDK is unable to initialize modem config subsystems
    if (!subSystemStatus) {
        std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
        return EXIT_FAILURE;
    }
   ~~~~~~

### 3. Deactivate a modem config file.
   ~~~~~~{.cpp}
    std::promise<bool> p;
    telux::common::Status status = modemConfigManager_->deactivateConfig(configType_,
           slotId_,[&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to deactivate config" << std::endl;
                p.set_value(false);
            }
        });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Deactivate Request sent" << std::endl;
    } else {
        std::cout << "Deactivate Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "deactivate config succeeded." << std::endl;
    }
   ~~~~~~

### 4. Request Config List from modem's storage.
   ~~~~~~{.cpp}
    // configList_ is member variable to store config list.
    std::promise<bool> p;
    telux::common::Status status = modemConfigManager_->requestConfigList(
        [&p, this](std::vector<telux::config::ConfigInfo> configList,
                                        telux::common::ErrorCode errCode) {
        if (errCode == telux::common::ErrorCode::SUCCESS) {
            configList_ = configList;
            p.set_value(true);
        } else {
            std::cout << "Failed to get config list" << std::endl;
            p.set_value(false);
        }
    });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Get Config List Request sent" << std::endl;
    } else {
        std::cout << "Get Config List Request failed" << std::endl;
    }

    if (p.get_future().get()) {
        std::cout << "Config List Updated !!" << std::endl;
    }
   ~~~~~~

### 5. Delete a modem config file from modem's storage.
   ~~~~~~{.cpp}
    std::promise<bool> p;
    // configNo denotes the index of config file in config list
    configId = configList_[configNo].id;
    telux::common::Status status = modemConfigManager_->deleteConfig(configType_,
          configId, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to delete config" << std::endl;
                p.set_value(false);
            }
        });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Delete Request sent successfully" << std::endl;
    } else {
        std::cout << "Delete Request failed" << std::endl;
    }
    if (p.get_future().get()) {
             std::cout << "Delete config succeeded." << std::endl;
    }
   ~~~~~~