Get thermal zones and cooling devices {#thermal_manager}
=======================================================

This sample app demonstrates how to get thermal zones and cooling devices.

### 1. Get thermal factory instance

   ~~~~~~{.cpp}
   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();
   ~~~~~~

### 2. Prepare initialization callback

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> p;
   auto initCb = [&p](telux::common::ServiceStatus status) {
      std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
      p.set_value(status);
   };
   ~~~~~~

### 3. Get thermal shutdown manager instance

   ~~~~~~{.cpp}
   std::shared_ptr<telux::therm::IThermalManager> thermalMgr
      = thermalFactory.getThermalManager(initCb);
   if (!thermalMgr) {
      std::cout << "Thermal manager is nullptr" << std::endl;
      return -1;
   }
   ~~~~~~

### 4. Wait for the initialization callback and check the service status

   ~~~~~~{.cpp}
   telux::common::ServiceStatus serviceStatus = p.get_future().get();
   if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      std::cout << "Thermal manager initialization failed" << std::endl;
      return -2;
   }
   ~~~~~~

### 5. Send get thermal zones request using thermal manager object

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

### 6. Send get cooling devices request using thermal manager instance

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
