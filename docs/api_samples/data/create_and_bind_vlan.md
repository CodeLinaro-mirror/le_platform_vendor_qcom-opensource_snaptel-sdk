# Create Vlan And Bind It To PDN

Please follow below steps to create Vlan and bind it to PDN

### 1. Get the DataFactory and VlanManager instances

   ~~~~~~{.cpp}
    auto &dataFactory = telux::data::DataFactory::getInstance();
    auto dataVlanMgr  = dataFactory.getVlanManager(opType);
   ~~~~~~

### 2. Check if data subsystem is ready

   ~~~~~~{.cpp}
   bool subSystemsStatus = dataVlanMgr->isSubsystemReady();
   ~~~~~~

### 2.1 If data subsystem is not ready, wait for it to be ready

Data subsystems is to make sure that device is ready for services like create and remove Vlan.
if subsystems were not ready, wait for unconditionally.

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::future<bool> f = dataVlanMgr->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Implement callback for create Vlan ###

   ~~~~~~{.cpp}
   auto respCbCreate = [](bool isAccelerated, telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                  << "createVlan Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error);
      std::cout << " Acceleration " << (isAccelerated ? "is allowed" : "is not allowed") << "\n";
   };
   ~~~~~~

### 4. Create Vlan based on interface type, acceleration, and assigned id ###

   ~~~~~~{.cpp}
   telux::data::VlanConfig config;
   config.iface = infType;
   config.vlanId = vlanId;
   config.isAccelerated = isAccelerated;
   dataVlanMgr->createVlan(config, respCbCreate);
   ~~~~~~

### 5. Response callback will be called for the createVlan response ###

### 6. Implement callback for bindWithprofile reponse ###

   ~~~~~~{.cpp}
   auto respCbBind = [](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                  << "bindWithProfile Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error) << std::endl;
   };
   ~~~~~~

### 7. Bind created Vlan with user provided profile id ###

   ~~~~~~{.cpp}
   dataVlanMgr->bindWithProfile(profileId, vlanId, respCbBind);
   ~~~~~~

### 8. Response callback will be called for the bindWithProfile response ###
