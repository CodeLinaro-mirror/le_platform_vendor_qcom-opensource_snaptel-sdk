Using Sensor APIs to control sensor features {#sensor_feature_control}
=============================================================

# Using Sensor APIs to control sensor features

Please follow below steps as a guide to control sensor features

### 1. Get sensor factory ###

   ~~~~~~{.cpp}
   auto &sensorFactory = telux::sensor::SensorFactory::getInstance();
   ~~~~~~

### 2. Prepare a callback that is invoked when the sensor sub-system initialization is complete ###

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
      std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
      p.set_value(status);
   };
   ~~~~~~

### 3. Get the sensor feature manager. If initialization fails, perform necessary error handling ###

   ~~~~~~{.cpp}
   std::shared_ptr<telux::sensor::ISensorFeatureManager> sensorFeatureManager
       = sensorFactory.getSensorFeatureManager(initCb);
   if (sensorFeatureManager == nullptr) {
      std::cout << "sensor feature manager is nullptr" << std::endl;
      exit(1);
   }
   std::cout << "obtained sensor feature manager" << std::endl;
   ~~~~~~

### 4. Wait until initialization is complete ###

   ~~~~~~{.cpp}
   p.get_future().get();
   if (sensorManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Sensor service not available" << std::endl;
      exit(1);
   }
   ~~~~~~

### 5. Get information regarding available sensor features ###

   ~~~~~~{.cpp}
   std::cout << "Sensor feature service is now available" << std::endl;
   std::vector<telux::sensor::SensorFeature> sensorFeatures;
   telux::common::Status status = sensorFeatureManager->getAvailableFeatures(sensorFeatures);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to get information on available features" << static_cast<int>(status)
                << std::endl;
      exit(1);
   }
   std::cout << "Received sensor features" << std::endl;
   for (auto feature : sensorFeatures) {
      printSensorFeatureInfo(feature);
   }
   ~~~~~~

### 6. Create and register a sensor feature event listener ###

   ~~~~~~{.cpp}
   std::shared_ptr<SensorFeatureEventListener> sensorFeatureEventListener
       = std::make_shared<SensorFeatureEventListener>();
   sensorFeatureManager->registerListener(sensorFeatureEventListener);
   ~~~~~~

### 7. Enable the required features ###

   ~~~~~~{.cpp}
   for (auto feature : sensorFeatures) {
      status = sensorFeatureManager->enableFeature(feature.name);
      if (status != telux::common::Status::SUCCESS) {
         std::cout << "Failed to enable feature: " << feature.name << std::endl;
      }
   }
   ~~~~~~

### 8. Receive sensor feature events with the registered listener ###

   ~~~~~~{.cpp}
   virtual void onEvent(telux::sensor::SensorFeatureEvent event) override {
      printSensorFeatureEvent(event);
   }
   ~~~~~~

### 9. When the sensor feature(s) are no longer necessary, disable them ###

   ~~~~~~{.cpp}
   for (auto feature : sensorFeatures) {
      status = sensorFeatureManager->disableFeature(feature.name);
      if (status != telux::common::Status::SUCCESS) {
         std::cout << "Failed to disable feature: " << feature.name << std::endl;
      }
   }
   ~~~~~~

### 10. Release the instance of ISensorFeatureManager to cleanup resources ###

   ~~~~~~{.cpp}
   sensorFeatureManager = nullptr;
   ~~~~~~
