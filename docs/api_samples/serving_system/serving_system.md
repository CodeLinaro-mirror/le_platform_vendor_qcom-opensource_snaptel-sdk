Request service domain preference {#serving_system}
===================================================

This sample application demonstrates how to request current service domain preference.

### 1. Get the phone factory instance

   ~~~~~~{.cpp}
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   ~~~~~~

### 2. Get serving system manager instance and wait for sub system initialization

   ~~~~~~{.cpp}
   std::promise<telux::common::ServiceStatus> prom{};
   auto servingSystemMgr = phoneFactory.getServingSystemManager(
       [&prom](telux::common::ServiceStatus status) {
           prom.set_value(status);
   });

   if (!servingSystemMgr) {
       std::cout << "Failed to get serving system manager" << std::endl;
       return;
   }

   // Check if serving system subsystem is ready
   // If serving system subsystem is not ready, wait for it to be ready
   telux::common::ServiceStatus managerStatus = servingSystemMgr->getServiceStatus();
   if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "\nServing system manager is not ready, Please wait ..." << std::endl
       managerStatus = prom.get_future().get();
   }

   ~~~~~~

### 3. Exit the application, if serving subsystem can not be initialized

   ~~~~~~{.cpp}
   if (managerStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return;
   }
   ~~~~~~

### 6. Implement response callback to receive response for request service domain preference

   ~~~~~~{.cpp}
   class ServiceDomainResponseCallback {
   public:
      void serviceDomainResponse(telux::tel::ServiceDomainPreference preference,
                                 telux::common::ErrorCode errorCode) {
         if(errorCode == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Service domain preference: "
                      << static_cast<int>(preference)
                      << std::endl;
         } else {
            std::cout << "\n setServiceDomainPreference failed, ErrorCode: "
                      << static_cast<int>(errorCode)
                      << std::endl;
         }
      }
   };
   ~~~~~~

### 7. Send request service domain preference request along with required function object

   ~~~~~~{.cpp}
   if(servingSystemMgr) {
      servingSystemMgr->requestServiceDomainPreference(
         ServiceDomainResponseCallback::serviceDomainResponse);
      std::cout << static_cast<int>(status) <<std::endl;
   }
   ~~~~~~
   
Now serviceDomainResponse() method will be invoked with current service domain preference.
