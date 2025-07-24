Get/Set autoshutdown modes {#send_thermal_shutdown_mode_commands}
=================================================================

This sample app demonstrates how to get/set thermal autoshutdown mode.

### 1. Get thermal factory instance

   ~~~~~~{.cpp}
   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();
   ~~~~~~

### 2. Get Thermal manager instance and Prepare initialization callback

   ~~~~~~{.cpp}
    std::promise<telux::common::ServiceStatus> prom;
    auto initCb = [&p](telux::common::ServiceStatus status) {
        prom.set_value(status);
    };

    thermShutdownMgr_ = thermalFactory.getThermalShutdownManager(initCb);
    if (thermShutdownMgr_ == NULL) {
        std::cout << " Failed to get manager instance" << std::endl;
        return;
    }
   ~~~~~~

### 3. Wait for the initialization callback and check the service status

   ~~~~~~{.cpp}
    // Check if thermal subsystem is ready
    // If thermal subsystem is not ready, wait for it to be ready
    telux::common::ServiceStatus subSystemStatus = thermShutdownMgr_->getServiceStatus();
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " Thermal-Shutdown management subsystem is not ready, Please wait"
            << std::endl;
        subSystemStatus = prom.get_future().get();
    }

    // Exit the application, if SDK is unable to initialize thermal subsystems
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " ERROR - Unable to initialize subSystem" << std::endl;
        return;
    }
   ~~~~~~

### 4. Query the current thermal auto shutdown mode

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

   // Send get themal auto shutdown mode command
   auto status = thermShutdownMgr_->getAutoShutdownMode(getStatusCallback);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "getShutdownMode command failed with error" << static_cast<int>(status) << std::endl;
   } else {
      std::cout << "Request to query thermal shutdown status sent" << std::endl;
   }
   ~~~~~~

### 5. Set thermal auto shutdown mode

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

   // Send set themal auto shutdown mode command
   auto status = thermShutdownMgr_->setAutoShutdownMode(state, commandResponse);
   if(status != telux::common::Status::SUCCESS) {
      std::cout << "setShutdownMode command failed with error" << static_cast<int>(status) << std::endl;
   } else {
      std::cout << "Request to set thermal shutdown status sent" << std::endl;
   }
   ~~~~~~
