# Using Request Network Selection Mode API

Please follow below steps to request network selection mode

### 1. Get phone factory and network selection manager instances

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto networkMgr
      = phoneFactory.getNetworkSelectionManager(DEFAULT_SLOT_ID);
   ~~~~~~

### 2. Wait for the network selection subsystem initialization ###
   ~~~~~~{.cpp}
   bool subSystemStatus = networkMgr->isSubsystemReady();
   ~~~~~~

### 2.1 If network selection subsystem is not ready, wait for it to be ready ###
   ~~~~~~{.cpp}
   if(!subSystemStatus) {
      std::cout << "network selection subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = networkMgr->onSubsystemReady();
      // If we want to wait unconditionally for network selection subsystem to be ready
      subSystemStatus = f.get();
   }
   ~~~~~~

### 3. Exit the application, if SDK is unable to initialize network selection subsystem ###
   ~~~~~~{.cpp}
   if(subSystemsStatus) {
      std::cout << " *** Network selection subsystem ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize network selection subsystem" << std::endl;
      return 1;
   }
   ~~~~~~

### 4. Implement response callback to receive response for request network selection mode ###

   ~~~~~~{.cpp}
   class SelectionModeResponseCallback {
   public:
      void selectionModeResponse(
         telux::tel::NetworkSelectionMode networkSelectionMode,
         telux::common::ErrorCode errorCode) {
         if(errorCode == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Network selection mode: "
                      << static_cast<int>(networkSelectionMode)
                      << std::endl;
         } else {
            std::cout << "\n requestNetworkSelectionMode failed, ErrorCode: "
                      << static_cast<int>(errorCode)
                      << std::endl;
         }
      }
   };
   ~~~~~~

### 5. Send requestNetworkSelectionMode along with required function object ###

   ~~~~~~{.cpp}
   if(networkMgr) {
      auto status = networkMgr->requestNetworkSelectionMode(
         SelectionModeResponseCallback::selectionModeResponse);
      std::cout << static_cast<int>(status) <<std::endl;
      }
   }
   ~~~~~~

### 6. Receive callback for get network selection mode request in selectionModeResponse function ###

