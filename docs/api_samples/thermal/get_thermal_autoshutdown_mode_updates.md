# Using Thermal Shutdown Manager APIs to get Thermal autoshutdown mode updates

The below steps need to be followed by applications to listen to thermal auto-shutdown mode updates.

### 1. Implement IThermalShutdownListener interface ###
   ~~~~~~{.cpp}
    class MyThermalShutdownModeListener : public IThermalShutdownListener {
    public:
        void onShutdownEnabled() override;
        void onShutdownDisabled() override;
        void onImminentShutdownEnablement(uint32_t imminentDuration) override;
        void onServiceStatusChange(ServiceStatus status) override;
    };
   ~~~~~~

### 2. Get thermal factory and thermal shutdown manager instances

   ~~~~~~{.cpp}
   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();
   auto thermShutdownMgr_ = thermalFactory.getThermalShutdownManager();
   ~~~~~~

### 3. Check if thermal shutdown management service is ready.

   ~~~~~~{.cpp}
   bool subSystemsStatus = thermShutdownMgr_->isReady();
    if(subSystemsStatus) {
        std::cout << " Thermal-Shutdown management service is ready."<< std::endl;
    } else {
        std::cout << " Thermal-Shutdown management service is NOT ready."<< std::endl;
    }
   ~~~~~~

### 4. If Thermal shutdown management service is not ready, wait for it to be ready

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
            std::cout << " Thermal-Shutdown management service is NOT ready." << std::endl;
            return -1;
        }
    #endif
   ~~~~~~

### 5. Instantiate MyThermalStateListener ###
   ~~~~~~{.cpp}
    auto myThermalModeListener = std::make_shared<MyThermalShutdownModeListener>();
   ~~~~~~

### 6. Register for updates on thermal autoshutdown mode and its management service status ###
   ~~~~~~{.cpp}
    thermShutdownMgr_->registerListener(myThermalModeListener);
   ~~~~~~

### 7. Wait for the Thermal auto shutdown mode updates ###
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

### 8. When the Thermal shutdown management service goes down, this API is invoked with status UNAVAILABLE. All Thermal auto shutdown mode notifications will be stopped until the status becomes AVAILABLE again ###
   ~~~~~~{.cpp}
    void MyThermalShutdownModeListener::onServiceStatusChange(ServiceStatus status) {
        std::cout << std::endl << "**** Thermal-Shutdown management service status update ****" << std::endl;
        // Avoid long blocking calls when handling notifications
    }
   ~~~~~~
