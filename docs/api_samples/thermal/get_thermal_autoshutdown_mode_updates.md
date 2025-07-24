Get thermal autoshutdown mode updates {#get_thermal_autoshutdown_mode_updates}
==============================================================================

This sample app demonstrates how to get thermal autoshutdown mode updates.

### 1. Implement IThermalShutdownListener interface

   ~~~~~~{.cpp}
   class MyThermalShutdownModeListener : public IThermalShutdownListener {
   public:
       void onShutdownEnabled() override;
       void onShutdownDisabled() override;
       void onImminentShutdownEnablement(uint32_t imminentDuration) override;
       void onServiceStatusChange(ServiceStatus status) override;
   };
   ~~~~~~

### 2. Get thermal factory instance

   ~~~~~~{.cpp}
   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();
   ~~~~~~

### 3. Get Thermal manager instance and Prepare initialization callback

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

### 4. Wait for the initialization callback and check the service status

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

### 5. Instantiate MyThermalStateListener

   ~~~~~~{.cpp}
   auto myThermalModeListener = std::make_shared<MyThermalShutdownModeListener>();
   ~~~~~~

### 6. Register for updates on thermal autoshutdown mode and its management service status

   ~~~~~~{.cpp}
   thermShutdownMgr_->registerListener(myThermalModeListener);
   ~~~~~~

### 7. Wait for the Thermal auto shutdown mode updates

   ~~~~~~{.cpp}
   // Avoid long blocking calls when handling notifications
   void MyThermalShutdownModeListener::onShutdownEnabled() {
        std::cout << std::endl << "**** Thermal auto shutdown mode enabled ****" << std::endl;
   }
   void MyThermalShutdownModeListener::onShutdownDisabled() {
        std::cout << std::endl << "**** Thermal auto shutdown mode disabled ****" << std::endl;
   }
   void MyThermalShutdownModeListener::onImminentShutdownEnablement(uint32_t imminentDuration) {
        std::cout << std::endl << "**** Thermal auto shutdown mode will be enabled in "
            << imminentDuration << " seconds ****" << std::endl;
   }
   ~~~~~~

### 8. Implement onServiceStatusChange callback to know when thermal shutdown management service goes down

   ~~~~~~{.cpp}
   // When the thermal management service goes down, this API is invoked
   // with status UNAVAILABLE. All thermal auto shutdown mode notifications
   // stopped until the status becomes AVAILABLE again.
   void MyThermalShutdownModeListener::onServiceStatusChange(ServiceStatus status) {
        std::cout << std::endl << "**** Thermal-Shutdown management service status update ****" << std::endl;
        // Avoid long blocking calls when handling notifications
   }
   ~~~~~~
