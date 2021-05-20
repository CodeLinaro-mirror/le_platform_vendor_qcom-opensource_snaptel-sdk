Using Sensor APIs to configure and acquire sensor data {#sensor_data_acquisition}
=============================================================

# Using Sensor APIs to configure and acquire sensor data

Please follow below steps as a guide to configure and acquire sensor data

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

### 3. Get the sensor manager. If initialization fails, perform necessary error handling ###

   ~~~~~~{.cpp}
   std::shared_ptr<telux::sensor::ISensorManager> sensorManager
      = sensorFactory.getSensorManager(initCb);
   if (sensorManager == nullptr) {
      std::cout << "sensor manager is nullptr" << std::endl;
      exit(1);
   }
   std::cout << "obtained sensor manager" << std::endl;
   ~~~~~~

### 4. Wait until initialization is complete ###

   ~~~~~~{.cpp}
   p.get_future().get();
   if (sensorManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Sensor service not available" << std::endl;
      exit(1);
   }
   ~~~~~~

### 5. Get information regarding available sensors in the system ###

   ~~~~~~{.cpp}
   std::cout << "Sensor service is now available" << std::endl;
   std::vector<telux::sensor::SensorInfo> sensorInfo;
   telux::common::Status status = sensorManager->getAvailableSensorInfo(sensorInfo);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to get information on available sensors" << static_cast<int>(status)
                << std::endl;
      exit(1);
   }
   std::cout << "Received sensor information" << std::endl;
   for (auto info : sensorInfo) {
      printSensorInfo(info);
   }
   ~~~~~~

### 6. Request the ISensorManager for the desired sensor ###

   ~~~~~~{.cpp}
   std::shared_ptr<telux::sensor::ISensor> sensor;
   std::cout << "Getting sensor: " << name << std::endl;
   status = sensorManager->getSensor(sensor, name);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to get sensor: " << name << std::endl;
      exit(1);
   }
   ~~~~~~

### 7. Create and register a sensor event listener for configuration updates and sensor events ###

###### The event listener extends the ISensorEventListener to receive notification about configuration and sensor events.

   ~~~~~~{.cpp}
   class SensorEventListener : public telux::sensor::ISensorEventListener {
   public:
      SensorEventListener(telux::sensor::SensorInfo info)
         : info_(info) {
      }

      virtual void onEvent(std::shared_ptr<std::vector<telux::sensor::SensorEvent>> events) override {
         PRINT_NOTIFICATION << ": Received " << events->size()
                            << " events from sensor: " << info_.name << std::endl;
         for (telux::sensor::SensorEvent s : *(events.get())) {
               printSensorEvent(s);
         }
      }

      virtual void onConfigurationUpdate(telux::sensor::SensorConfiguration configuration) override {
         PRINT_NOTIFICATION << ": Received configuration update from sensor: " << info_.name << ": ["
                              << configuration.samplingRate << ", " << configuration.batchCount << " ]"
                              << std::endl;
      }

   private:
      bool isUncalibratedSensor(telux::sensor::SensorType type) {
         return ((type == telux::sensor::SensorType::GYROSCOPE_UNCALIBRATED)
                  || (type == telux::sensor::SensorType::ACCELEROMETER_UNCALIBRATED));
      }
      void printSensorEvent(telux::sensor::SensorEvent &s) {
         if (isUncalibratedSensor(info_.type)) {
               PRINT_NOTIFICATION << ": " << info_.name << ": " << s.timestamp << ", "
                                 << s.uncalibrated.data.x << ", " << s.uncalibrated.data.y << ", "
                                 << s.uncalibrated.data.z << ", " << s.uncalibrated.bias.x << ", "
                                 << s.uncalibrated.bias.y << ", " << s.uncalibrated.bias.z
                                 << std::endl;
         } else {
               PRINT_NOTIFICATION << ": " << info_.name << ": " << s.timestamp << ", "
                                 << s.calibrated.x << ", " << s.calibrated.y << ", " << s.calibrated.z
                                 << std::endl;
         }
      }
      telux::sensor::SensorInfo info_;
   };
   ~~~~~~

###### Create a event listener and register it with the sensor.

   ~~~~~~{.cpp}
   std::shared_ptr<SensorEventListener> sensorEventListener
      = std::make_shared<SensorEventListener>(sensor->getSensorInfo());
   sensor->registerListener(sensorEventListener);
   ~~~~~~

### 8. Configure the sensor with required configuration setting the necessary validityMask ###

   ~~~~~~{.cpp}
   telux::sensor::SensorConfiguration config;
   config.samplingRate = getMinimumSamplingRate(sensor->getSensorInfo());
   config.batchCount = sensor->getSensorInfo().maxBatchCountSupported;
   std::cout << "Configuring sensor with samplingRate, batchCount [" << config.samplingRate << ", "
           << config.batchCount << "]" << std::endl;
   config.validityMask.set(telux::sensor::SensorConfigParams::SAMPLING_RATE);
   config.validityMask.set(telux::sensor::SensorConfigParams::BATCH_COUNT);
   status = sensor->configure(config);
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to configure sensor: " << name << std::endl;
      exit(1);
   }
   ~~~~~~

### 9. Receive updates on sensor configuration ###

   ~~~~~~{.cpp}
   virtual void onConfigurationUpdate(telux::sensor::SensorConfiguration configuration) override {
      PRINT_NOTIFICATION << ": Received configuration update from sensor: " << info_.name << ": ["
                         << configuration.samplingRate << ", " << configuration.batchCount << " ]"
                         << std::endl;
   }
   ~~~~~~

### 10. Activate the sensor to receive sensor data ###

   ~~~~~~{.cpp}
   status = sensor->activate();
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to activate sensor: " << name << std::endl;
      exit(1);
   }
   ~~~~~~

### 11. Receive sensor data with the registered listener ###

   ~~~~~~{.cpp}
   virtual void onEvent(std::shared_ptr<std::vector<telux::sensor::SensorEvent>> events) override {
      PRINT_NOTIFICATION << ": Received " << events->size()
                         << " events from sensor: " << info_.name << std::endl;
      for (telux::sensor::SensorEvent s : *(events.get())) {
         printSensorEvent(s);
      }
   }
   ~~~~~~

### 12. When data acquisition is no longer necessary, deactivate the sensor ###

   ~~~~~~{.cpp}
   status = sensor->deactivate();
   if (status != telux::common::Status::SUCCESS) {
      std::cout << "Failed to deactivate sensor: " << name << std::endl;
      exit(1);
   }
   ~~~~~~

### 13. Release the instance of ISensor if no longer required ###

   ~~~~~~{.cpp}
   sensor = nullptr;
   ~~~~~~

### 14. Release the instance of ISensorManager to cleanup resources ###

   ~~~~~~{.cpp}
   sensorManager = nullptr;
   ~~~~~~
