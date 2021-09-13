Create Vlan And Bind It To PDN {#create_and_bind_vlan}
======================================================

# Create Vlan And Bind It To PDN

Please follow below steps to create Vlan and bind it to PDN

### 1. Implement initialization callback and get the DataFactory instance
Optionally initialization callback can be provided with get manager instance.
Data factory will call callback when manager initialization is complete.

   ~~~~~~{.cpp}
   auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };
    auto &dataFactory = telux::data::DataFactory::getInstance();
   ~~~~~~

### 2. Get the VlanManager instances

   ~~~~~~{.cpp}
    std::unique_lock<std::mutex> lck(mtx);
    auto dataVlanMgr  = dataFactory.getVlanManager(opType, initCb);
   ~~~~~~

### 3. Wait for VlanManager initialization to be complete

   ~~~~~~{.cpp}
   initCv.wait(lck);
   ~~~~~~

### 3.1 Check VlanManager initialization state

If VlanManager initialization failed, new initialization attempt can be accomplished
by calling step 2. If VlanManager initialization succeed, proceed to step 4

   ~~~~~~{.cpp}
   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      //Go to step 2 for another initialization attempt
   }
   ~~~~~~

### 4. Implement callback for create Vlan ###

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

### 5. Create Vlan based on interface type, acceleration, and assigned id ###

   ~~~~~~{.cpp}
   telux::data::VlanConfig config;
   config.iface = infType;
   config.vlanId = vlanId;
   config.isAccelerated = isAccelerated;
   dataVlanMgr->createVlan(config, respCbCreate);
   ~~~~~~

### 6. Response callback will be called for the createVlan response ###

### 7. Implement callback for bindWithprofile reponse ###

   ~~~~~~{.cpp}
   auto respCbBind = [](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                  << "bindWithProfile Response"
                  << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                  << ". ErrorCode: " << static_cast<int>(error) << std::endl;
   };
   ~~~~~~

### 8. Bind created Vlan with user provided profile id ###

   ~~~~~~{.cpp}
   dataVlanMgr->bindWithProfile(profileId, vlanId, respCbBind);
   ~~~~~~

### 9. Response callback will be called for the bindWithProfile response ###
