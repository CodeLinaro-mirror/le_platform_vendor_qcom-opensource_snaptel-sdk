# Enable/Disable Socks

Please follow below steps to Enable/Disable Socks

### 1.Get the DataFactory and Socks Manager instance ###

   ~~~~~~{.cpp}
   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dataSocksMgr  = dataFactory.getSocksManager(opType);
   ~~~~~~

### 2. Check if data subsystem is ready ###

   ~~~~~~{.cpp}
   bool subSystemsStatus = dataSocksMgr->isSubsystemReady();
   ~~~~~~

### 2.1 If data subsystem is not ready, wait for it to be ready ###

Data subsystems is to make sure that device is ready for services.
if subsystems were not ready, wait for unconditionally.

   ~~~~~~{.cpp}
   if(!subSystemStatus) {
      std::future<bool> f = dataSocksMgr->onSubsystemReady();
      // If we want to wait unconditionally for data subsystem to be ready
      subSystemStatus = f.get();
   }
   ~~~~~~

### 3. Instantiate enable Socks callback instance - this is optional ###

   ~~~~~~{.cpp}
   auto respCb = [](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                  << "enableSocks Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error) << "\n";
      promise.set_value(1);
   };
   ~~~~~~

### 4. enable/disable Socks ###

   ~~~~~~{.cpp}
    dataSocksMgr->enableSocks(enable, respCb);
   ~~~~~~

### 5. Response callback will be called for the setFirewall response ###
