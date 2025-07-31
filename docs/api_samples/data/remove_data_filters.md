Remove data filter mode {#remove_data_filter}
=============================================

This sample application demonstrates how to remove all data filter mode.

### 1. Get the DataFactory, DataConnectionManager and DataFilterManager instances

   ~~~~~~{.cpp}
   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dataConnMgr_ = dataFactory.getDataConnectionManager();
   auto dataFilterMgr_ = dataFactory.getDataFilterManager();
   ~~~~~~

### 2. Wait for the data connection and data filter manager sub system initialization

   ~~~~~~{.cpp}
   bool dataConnectionSubSystemStatus = false;
   std::condition_variable initCv;
   std::mutex mtx;
   auto initCb = [&](telux::common::ServiceStatus status) {
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

### 3. Remove all data filter

   ~~~~~~{.cpp}
   std::promise<bool> p;
   int profileId = 2;
   telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;

   auto status = dataFilterMgr_->removeAllDataRestrictFilters(
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to remove all filter" << std::endl;
                p.set_value(false);
            }
        });
   if(status == telux::common::Status::SUCCESS) {
        std::cout << "Remove all data filter Request sent" << std::endl;
   } else {
        std::cout << "Remove all data filter Request failed" << std::endl;
   }

   if (p.get_future().get()) {
             std::cout << "Remove all data filter succeeded." << std::endl;
   }
   ~~~~~~
