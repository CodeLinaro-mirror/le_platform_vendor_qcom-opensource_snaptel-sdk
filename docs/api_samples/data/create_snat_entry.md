# Create Static NAT Entry

Please follow below steps to create static NAT entry

### 1. Get the DataFactory and NatManager instances

   ~~~~~~{.cpp}
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto dataSnatMgr  = dataFactory.getNatManager(opType);
   ~~~~~~

### 2. Check if data subsystem is ready

   ~~~~~~{.cpp}
   bool subSystemsStatus = dataSnatMgr->isSubsystemReady();
   ~~~~~~

### 2.1 If data subsystem is not ready, wait for it to be ready

Data subsystems is to make sure that device is ready for services like create and remove nat entries.
if subsystems were not ready, wait for unconditionally.

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::future<bool> f = dataSnatMgr->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Implement callback for create Snat entry ###

   ~~~~~~{.cpp}
   auto respCb = [](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                  << "addStaticNatEntry"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
   };
   ~~~~~~

### 4. Create Snat entry based on profile id, local ip, local port, global port, and protocol ###

   ~~~~~~{.cpp}
   natConfig.addr = ipAddr;
   natConfig.port = (uint16_t)localIpPort;
   natConfig.globalPort = (uint16_t)globalIpPort;
   natConfig.proto = (uint8_t)proto;
   dataSnatMgr->addStaticNatEntry(profileId, natConfig, respCb);
   ~~~~~~

### 5. Response callback will be called for the addStaticNatEntry response ###
