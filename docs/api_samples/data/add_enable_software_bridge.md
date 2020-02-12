# Add a software bridge and Enable software bridge management

Please follow below steps to add a software bridge and enable software bridge management

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

Data subsystem needs to be ready to add a software bridge and enable the software bridge management
in the system. If the subsystem is not ready, wait for it unconditionally.

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::future<bool> f = dataBridgeMgr->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Implement callbacks for adding a software bridge and enabling the software bridge management ###

   ~~~~~~{.cpp}
   auto respCbAdd = [](telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Add software bridge request"
                 << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   };
   auto respCbEnable = [](telux::common::ErrorCode error) {
       std::cout << "CALLBACK: "
                 << "Enable software bridge management request is"
                 << (error == telux::common::ErrorCode::SUCCESS ? " successful" : " failed")
                 << ". ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   };
   ~~~~~~

### 4. Add software bridge based on the interface name, interface type and bandwidth required ###

   ~~~~~~{.cpp}
   telux::data::net::BridgeInfo config;
   config.ifaceName = infName;
   config.ifaceType = infType;
   config.bandwidth = bridgeBw;
   dataBridgeMgr->addBridge(config, respCbAdd);
   ~~~~~~

### 5. Response callback will be invoked with the addBridge response ###

### 6. Enable the software bridge management if not enabled already ###

   ~~~~~~{.cpp}
   bool enable = true;
   dataBridgeMgr->enableBridge(enable, respCbEnable);
   ~~~~~~

### 7. Response callback will be invoked with the enableBridge response ###
