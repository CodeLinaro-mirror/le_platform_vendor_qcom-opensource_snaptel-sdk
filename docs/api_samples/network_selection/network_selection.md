Request network selection mode {#network_selection}
===================================================

This sample application demonstrates how to request current network selection mode.

### 1. Get the phone factory instance

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   ~~~~~~

### 2. Get NetworkSelectionManager instance and Wait for sub system initialization

    ~~~~~~{.cpp}
    std::promise<telux::common::ServiceStatus> prom{};
    auto networkMgr = phoneFactory.getNetworkSelectionManager(
         [&prom](telux::common::ServiceStatus status) {
             prom.set_value(status);
    });

    if (!networkMgr) {
        std::cout << "Failed to get network selection manager" << std::endl;
        return;
    }

    // Check if network selection subsystem is ready
    // If network selection subsystem is not ready, wait for it to be ready
    telux::common::ServiceStatus managerStatus = networkMgr->getServiceStatus();
    if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nNetwork selection manager is not ready, Please wait ..." << std::endl
        managerStatus = prom.get_future().get();
    }

   ~~~~~~

### 3. Exit the application, if network selection subsystem can not be initialzed

   ~~~~~~{.cpp}
   if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return;
   }
   ~~~~~~

### 4. Implement response callback to receive response for request network selection mode

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

### 5. Send requestNetworkSelectionMode along with response callback

   ~~~~~~{.cpp}
   if(networkMgr) {
      auto status = networkMgr->requestNetworkSelectionMode(
         SelectionModeResponseCallback::selectionModeResponse);
      std::cout << static_cast<int>(status) <<std::endl;
      }
   }
   ~~~~~~

Now, selectionModeResponse() callback gets invoked with current network selection mode information.
