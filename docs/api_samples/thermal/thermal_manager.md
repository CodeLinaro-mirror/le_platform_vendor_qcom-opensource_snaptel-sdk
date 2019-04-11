# Using Thermal Manager APIs

Please follow below steps to get thermal zones and cooling devices

### 1. Get thermal factory and thermal manager instances

   ~~~~~~{.cpp}
   auto &thermalFactory = telux::therm::ThermalFactory::getInstance();
   auto thermalMgr = thermalFactory.getThermalManager();
   ~~~~~~

### 2. Send get thermal zones request using thermal manager object

   ~~~~~~{.cpp}
   if(thermalMgr != nullptr) {
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
   } else {
      std::cout << "Thermal manager is nullptr" << std::endl;
   }
   ~~~~~~

### 3. Send get cooling devices request using thermal manager object

   ~~~~~~{.cpp}
   if(thermalMgr != nullptr) {
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
   } else {
      std::cout << "Thermal manager is nullptr" << std::endl;
   }
   ~~~~~~