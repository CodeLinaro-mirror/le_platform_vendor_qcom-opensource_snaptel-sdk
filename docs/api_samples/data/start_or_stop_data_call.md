# Cellular Data Call - Start/Stop

Please follow below steps to start or stop cellular data call

### 1. Get the DataFactory and DataConnectionManager instances

   ~~~~~~{.cpp}
   auto &dataFactory = DataFactory::getInstance();
   auto dataConnectionManager = dataFactory.getDataConnectionManager();
   ~~~~~~

### 2. Check if data subsystem is ready

   ~~~~~~{.cpp}
   bool subSystemsStatus = dataConnectionManager->isSubsystemReady();
   ~~~~~~

### 2.1 If data subsystem is not ready, wait for it to be ready

Data subsystems is to make sure that device is ready for services like bring-up or tear-down cellular data call.
if subsystems were not ready, wait for unconditionally.

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::future<bool> f = dataConnectionManager->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Implement DataCallResponseCb callback for startDatacall###

   ~~~~~~{.cpp}
   void startDataCallResponseCallBack(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                                 telux::common::ErrorCode error) {
      std::cout<< "Received callback for startDataCall" << std::endl;
      if(error == telux::common::ErrorCode::SUCCESS) {
         std::cout<< "Request sent successfully" << std::endl;
      } else {
         std::cout<< "Request failed with errorCode: " << static_cast<int>(error) << std::endl;
      }
   }
   ~~~~~~

### 4. Send a start data call request with profile ID, IpFamily type along with required callback function ###

   ~~~~~~{.cpp}
   dataConnectionManager->startDataCall(profileId, telux::data::IpFamilyType::IPV4V6,
                                        startDataCallResponseCallBack);
   ~~~~~~

### 5. Response callback will be called for the startDataCall response ###

### 6. Implement DataCallResponseCb callback for stopDatacall###

   ~~~~~~{.cpp}
   void stopDataCallResponseCallBack(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                                telux::common::ErrorCode error) {
   std::cout << "Received callback for stopDataCall" << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      std::cout << "Request sent successfully" << std::endl;
   } else {
      std::cout << "Request failed with errorCode: " << static_cast<int>(error) << std::endl;
   }
   ~~~~~~

### 7. Send a stop data call request with profile ID, IpFamily type along with required callback function ###

   ~~~~~~{.cpp}
   dataConnectionManager->stopDataCall(profileId, telux::data::IpFamilyType::IPV4V6,
                                       stopDataCallResponseCallBack);
   ~~~~~~

### 8. Response callback will be called for the stopDataCall response ###