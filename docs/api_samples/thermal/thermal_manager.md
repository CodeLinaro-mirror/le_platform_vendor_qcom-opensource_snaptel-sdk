Get thermal zones and cooling devices {#thermal_manager}
=======================================================

This sample app demonstrates how to get thermal zones and cooling devices.

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

    thermalMgr = thermalFactory.getThermalManager(initCb);
    if (thermalMgr == NULL) {
        std::cout << " Failed to get manager instance" << std::endl;
        return;
    }
   ~~~~~~

### 3. Wait for the initialization callback and check the service status

   ~~~~~~{.cpp}
    // Check if thermal subsystem is ready
    // If thermal subsystem is not ready, wait for it to be ready
    telux::common::ServiceStatus subSystemStatus = thermalMgr->getServiceStatus();
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " Thermal management subsystem is not ready, Please wait"
            << std::endl;
        subSystemStatus = prom.get_future().get();
    }

    // Exit the application, if SDK is unable to initialize thermal subsystems
    if (subSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " ERROR - Unable to initialize subSystem" << std::endl;
        return;
    }
   ~~~~~~

### 4. Send get thermal zones request using thermal manager object

   ~~~~~~{.cpp}
   std::vector<std::shared_ptr<telux::therm::IThermalZone>> zoneInfo
      = thermalMgr->getThermalZones();
   if(zoneInfo.size() > 0) {
      for(auto index = 0; index < zoneInfo.size(); index++) {
         std::cout << "Thermal zone Id: " << zoneInfo(index)->getId() << "Description: "
                     << zoneInfo(index)->getDescription() << "Current temp: "
                     << zoneInfo(index)->getCurrentTemp() << std::endl;
         std::cout << std::endl;
      }
   } else {
      std::cout << "No thermal zones found!" << std::endl;
   }
   ~~~~~~

### 5. Send get cooling devices request using thermal manager instance

   ~~~~~~{.cpp}
   std::vector<std::shared_ptr<telux::therm::ICoolingDevice>> coolingDevice
      = thermalMgr->getCoolingDevices();
   if(coolingDevice.size() > 0) {
      for(auto index = 0; index < coolingDevice.size(); index++) {
         std::cout << "Cooling device Id: " << coolingDevice(index)->getId()
                     << "Description: " << coolingDevice(index)->getDescription()
                     << "Max cooling level: " << coolingDevice(index)->getMaxCoolingLevel()
                     << "Current cooling level: " << coolingDevice(index)->getCurrentCoolingLevel()
                     << std::endl;
         std::cout << std::endl;
      }
   } else {
      std::cout << "No cooling devices found!" << std::endl;
   }
   ~~~~~~
