# How to load and activate modem config file

Please follow below steps to load and activate a modem config file

### 1. Get the ConfigFactory and ModemConfigManager instances

   ~~~~~~{.cpp}
    auto &configFactory = telux::config::ConfigFactory::getInstance();
    auto modemConfigManager = configFactory.getModemConfigManager();
   ~~~~~~

### 2. Wait for the Modem Config sub system initialization
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

### 3. Load a modem config file from file path
   ~~~~~~{.cpp}
    std::promise<bool> p;
    auto status = modemConfigManager_->loadConfigFile(filePath, configType_,
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to load config" << std::endl;
                p.set_value(false);
            }
        });
    if(status == telux::common::Status::SUCCESS) {
        std::cout << "Load config Request sent" << std::endl;
    } else {
        std::cout << "Load config Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "Load config succeeded." << std::endl;
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

### 5. Activate modem config File.
   ~~~~~~{.cpp}
    // configNo denotes index of the modem config file in in config List.
    ConfigId configId;
    configId = configList_[configNo].id;
    telux::common::Status status = modemConfigManager_->activateConfig(configType_, configId,
            slotId_, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to activate config" << std::endl;
                p.set_value(false);
            }
        });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Activate Request sent " << std::endl;
    } else {
        std::cout << "Activate Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "config Activated !!" << std::endl;
    }
   ~~~~~~
