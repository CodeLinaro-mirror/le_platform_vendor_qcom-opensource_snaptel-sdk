# How to Get and Set Auto Config Selection Mode.

Please follow below steps to Get and Set Auto Config Selection Mode.

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

### 3. Set Auto Config Selection Mode
   ~~~~~~{.cpp}
    std::promise<bool> p;
    telux::config::AutoSelectionMode mode = telux::config::AutoSelectionMod::ENABLED;
    telux::common::Status status = modemConfigManager_->setAutoSelectionMode(mode,
           slotId_, [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to set selection mode" << std::endl;
                p.set_value(false);
            }
        });
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "set selection mode Request sent" << std::endl;
    } else {
        std::cout << "set selection mode Request failed" << std::endl;
    }

    if (p.get_future().get()) {
             std::cout << "set selection mode succeeded." << std::endl;
    }
   ~~~~~~

### 4. Get Auto Config Selection Mode
   ~~~~~~{.cpp}
    std::promise<bool> p;
    telux::config::AutoSelectionMode selMode;
    telux::common::Status status = modemConfigManager_->getAutoSelectionMode(
            [&p, &selMode](AutoSelectionMode selectionMode, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                selMode = selectionMode;
                p.set_value(true);
            } else {
                std::cout << "Failed to get selection mode" << std::endl;
                p.set_value(false);
            }
        }, slotId_);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Get selection mode Request sent" << std::endl;
    } else {
        std::cout << "Get selection mode Request failed" << std::endl;
    }

    if (p.get_future().get()) {
        if (selMode == telux::config::AutoSelectionMode::DISABLED) {
            std::cout << "DISABLED" << std::endl;
        } else {
            std::cout << "ENABLED" << std::endl;
        }
    }
   ~~~~~~