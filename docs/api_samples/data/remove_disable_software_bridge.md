# Remove a software bridge and Disable software bridge management

Please follow below steps to remove a software bridge and and disable software bridge management

### 1. Get the DataFactory and BridgeManager instances ###

   ~~~~~~{.cpp}
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto dataBridgeMgr  = dataFactory.getBridgeManager();
   ~~~~~~

### 2. Check if data subsystem is ready ###

   ~~~~~~{.cpp}
   bool subSystemsStatus = dataBridgeMgr->isSubsystemReady();
   ~~~~~~

### 2.1 If data subsystem is not ready, wait for it to be ready ###

Data subsystem needs to be ready to remove a software bridge and disable the software bridge
management in the system. If the subsystem is not ready, wait for it unconditionally.

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::future<bool> f = dataBridgeMgr->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Implement callbacks to get, remove software bridge and disable the software bridge management ###

   ~~~~~~{.cpp}
   auto respCbGet = [](const std::vector<BridgeInfo> &configs, telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Get software bridge info request"
                 << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
       for (auto c : configs) {
           std::cout << "Iface name: " << c.ifaceName << ", ifaceType: " << (int)c.ifaceType
                     << ", bandwidth: " << c.bandwidth << std::endl;
       }
   };
   auto respCbRemove = [](telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Remove software bridge request"
                 << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   };
   auto respCbDisable = [](telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Disable software bridge management request is"
                 << (error == telux::common::ErrorCode::SUCCESS ? " successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   };
   ~~~~~~

### 4. Get the list of software bridges configured in the system ###

   ~~~~~~{.cpp}
   dataBridgeMgr->requestBridgeInfo(respCbGet);
   ~~~~~~

### 5. Response callback will be invoked with the list of software bridges ###

### 6. Remove the software bridge from the configured bridges, based on the interface name ###

   ~~~~~~{.cpp}
   dataBridgeMgr->removeBridge(ifaceName, respCbRemove);
   ~~~~~~

### 5. Response callback will be invoked with the removeBridge response ###

### 6. Disable the software bridge management if required ###

   ~~~~~~{.cpp}
   bool enable = false;
   dataBridgeMgr->enableBridge(enable, respCbDisable);
   ~~~~~~

### 7. Response callback will be invoked with the enableBridge response ###
