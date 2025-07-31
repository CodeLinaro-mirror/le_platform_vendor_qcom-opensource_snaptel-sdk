Get/Set data filter mode {#get_and_set_data_filter_mode}
========================================================

This sample application demonstrates how to get and set data filter mode.

### 1. Get the DataFactory, DataConnectionManager and DataFilterManager instances

   ~~~~~~{.cpp}
   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dataConnMgr_ = dataFactory.getDataConnectionManager();
   auto dataFilterMgr_ = dataFactory.getDataFilterManager();
   ~~~~~~

### 2. Wait for the Data Connection Manager and Data Filter Manager sub system initialization

   ~~~~~~{.cpp}
   bool dataConnectionSubSystemStatus = false;
   std::condition_variable initCv;
   std::mutex mtx;
   auto   initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      dataConnectionSubSystemStatus = true;
      initCv.notify_all();
   };

   dataConnMgr_ = dataFactory.getDataConnectionManager(initCb);
   {
      std::unique_lock<std::mutex> lck(mtx);
      initCv.wait(lck, [&]{return dataConnectionSubSystemStatus;});
   }

   // Exit the application, if SDK is unable to initialize data manager subsystems
   if(dataConnMgr_->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return EXIT_FAILURE;
   }

   bool dataFilterSubSystemStatus = false;
   initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      dataFilterSubSystemStatus = true;
      initCv.notify_all();
   };

   dataFilterMgr_ = dataFactory.getDataFilterManager(initCb);
   {
      std::unique_lock<std::mutex> lck(mtx);
      initCv.wait(lck, [&]{return dataFilterSubSystemStatus;});
   }

   // Exit the application, if SDK is unable to initialize data manager subsystems
   if(dataFilterMgr_->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
       return EXIT_FAILURE;
   }
   ~~~~~~

### 3. Set data filter mode to enable

   ~~~~~~{.cpp}
   std::promise<bool> p;
   int profileId = 2;
   telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;

   std::promise<bool> dataCallConnectedPromise;
   auto dataCallConnectedFuture = dataCallConnectedPromise.get_future();
   auto responseCallback = [&dataCallConnectedPromise](
      const std::shared_ptr<telux::data::IDataCall> &dataCall,
      telux::common::ErrorCode error) {
         if (error == telux::common::ErrorCode::SUCCESS &&
            dataCall->getDataCallStatus() == telux::data::DataCallStatus::NET_CONNECTED) {
            dataCallConnectedPromise.set_value(true);
         } else {
            dataCallConnectedPromise.set_value(false);
            std::cout << "*** ERROR - Data call failed to connect ***" << std::endl;
         }
   };

   telux::common::Status startDataCallStatus =
      dataConnMgr->startDataCall(profileId, ipFamilyType, responseCallback);

   if (startDataCallStatus == telux::common::Status::SUCCESS) {
      if (dataCallConnectedFuture.wait_for(std::chrono::seconds(5)) ==
         std::future_status::timeout) {
            std::cout << "*** ERROR - Data call connection timed out ***" << std::endl;
            return 1;
      }

      bool isConnected = dataCallConnectedFuture.get();
      if (!isConnected) {
         std::cout << "*** ERROR - Data call failed to connect ***" << std::endl;
         return 1;
      }
   } else {
      std::cout << "*** ERROR - start data call request failed ***" << std::endl;
      return 1;
   }

   telux::data::DataRestrictMode enableMode;
   enableMode.filterAutoExit = telux::data::DataRestrictModeType::DISABLE;
   enableMode.filterMode = telux::data::DataRestrictModeType::ENABLE;

   auto status = dataFilterMgr_->setDataRestrictMode(enableMode,
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to set data filter mode" << std::endl;
                p.set_value(false);
            }
        });
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Set data filter mode Request sent" << std::endl;
   } else {
      std::cout << "Set data filter mode Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "Set data filter mode succeeded." << std::endl;
   }
   ~~~~~~

### 4. Get data filter mode

   ~~~~~~{.cpp}
   std::promise<bool> p;
   std::string interfaceName = "rmnet_data0";

   auto status = dataFilterMgr_->requestDataRestrictMode(interfaceName, configType_,
             [&p](telux::data::DataRestrictMode mode, telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
                if (mode.filterMode == DataRestrictModeType::DISABLE) {
                    std::cout << " DataRestrictMode Disabled" << std::endl;
                } else if (mode.filterMode == DataRestrictModeType::ENABLE) {
                    std::cout << " DataRestrictMode Enabled" << std::endl;
                } else {
                    std::cout << " Invalid DataRestrictMode" << std::endl;
                }
            } else {
                std::cout << "Failed to get data filter mode" << std::endl;
                p.set_value(false);
            }
   });
   
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Get data filter mode Request sent" << std::endl;
   } else {
      std::cout << "Get data filter mode Request failed" << std::endl;
   }

   if (p.get_future().get()) {
       std::cout << "Get data filter mode succeeded." << std::endl;
   }
   ~~~~~~
