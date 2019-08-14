# Using Thermal Shutdown Manager APIs to set Autoshutdown modes

Please follow below steps to set thermal autoshutdown modes

### 1. Get thermal factory and thermal shutdown manager instances

   ~~~~~~{.cpp}
   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();
   auto thermShutdownMgr_ = thermalFactory.getThermalShutdownManager();
   ~~~~~~

### 2. Check if thermal shutdown management service is ready.

   ~~~~~~{.cpp}
   bool subSystemsStatus = thermShutdownMgr_->isReady();
    if(subSystemsStatus) {
        std::cout << APP_NAME << " Thermal-Shutdown management service is ready."<< std::endl;
    } else {
        std::cout << APP_NAME << " Thermal-Shutdown management service is NOT ready."<< std::endl;
    }
   ~~~~~~

### 3 If Thermal shutdown management service is not ready, wait for it to be ready

   ~~~~~~{.cpp}
    std::future<bool> f = thermShutdownMgr_->onSubsystemReady();
    #if  //Timeout based wait
        if (f.wait_for(std::chrono::seconds(timeoutSec)) == std::future_status::timeout) {
            std::cout << "operation timed out." << std::endl;
        } else {
            subSystemsStatus = f.get();
            if (subSystemsStatus) {
                std::cout << " Thermal-Shutdown management service is ready." << std::endl;
            }
        }
    #else //Unconditional wait
        subSystemsStatus = f.get();
        if (subSystemsStatus) {
            std::cout << " Thermal-Shutdown management service is ready." << std::endl;
        } else {
            std::cout << " Thermal-Shutdown management service is NOT ready" << std::endl;
            return -1;
        }
    #endif
   ~~~~~~

### 3. Query the current thermal auto shutdown mode

   ~~~~~~{.cpp}
    // Callback which provides response to query operation
    void getStatusCallback(AutoShutdownMode mode)
    {
        if(mode == AutoShutdownMode::ENABLE) {
            std::cout << " Current auto shutdown mode is: Enable" << std::endl;
        } else if(mode == AutoShutdownMode::DISABLE) {
            std::cout << " Current auto shutdown mode is: Disable" << std::endl;
        } else {
            std::cout << " *** ERROR - Failed to send get auto-shutdown mode " << std::endl;
        }
    }
    // Send get themal auto Shutdown mode command
    auto status = thermShutdownMgr_->getAutoShutdownMode(getStatusCallback);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << "getShutdownMode command failed with error" << static_cast<int>(status) << std::endl;
    } else {
        std::cout << "Request to query thermal shutdown status sent" << std::endl;
    }
   ~~~~~~

### 4. Set thermal auto shutdown mode

   ~~~~~~{.cpp}
    // Callback which provides response to set thermal auto shutdown mode command
    void commandResponse(ErrorCode error)
    {
        if(error == ErrorCode::SUCCESS) {
        std::cout << " sent successfully" << std::endl;
        } else {
            std::cout << " failed\n errorCode: " << static_cast<int>(error) << std::endl;
        }
    }
    // Send set themal auto Shutdown mode command
    auto status = thermShutdownMgr_->setAutoShutdownMode(state, commandResponse);
    if(status != telux::common::Status::SUCCESS) {
        std::cout << "setShutdownMode command failed with error" << static_cast<int>(status) << std::endl;
    } else {
        std::cout << "Request to set thermal shutdown status sent" << std::endl;
    }
   ~~~~~~
