Add data filter {#add_data_filter}
==================================

This sample application demonstrates how to add a data filter.

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

### 3. Start datacall & set data filter mode to enable

   ~~~~~~{.cpp}
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

   std::promise<bool> p;
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

### 4. Add data filter

   ~~~~~~{.cpp}
   std::promise<bool> p;
   std::string ipAddr = std::string("168.128.91.1");
   int port = 8888;
   telux::data::IPv4Info ipv4Info_ = {};
   ipv4Info_.srcAddr = ipAddr;

   telux::data::PortInfo srcPort;
   srcPort.port = port;
   srcPort.range = 0;
   telux::data::UdpInfo udpInfo_ = {};
   udpInfo_.src = srcPort;

   // IpProtocol for UDP
   int PROTO_UDP =  17;
   // create a filter of UDP type, and set source IP and port.
   std::shared_ptr<telux::data::IIpFilter> dataFilter =
   dataFactory.getNewIpFilter(PROTO_UDP);
   dataFilter->setIPv4Info(ipv4Info_);

   auto udpRestrictFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(dataFilter);
   udpRestrictFilter->setUdpInfo(udpInfo_);

   auto status = dataFilterMgr_->addDataRestrictFilter(dataFilter,
             [&p](telux::common::ErrorCode error) {
            if (error == telux::common::ErrorCode::SUCCESS) {
                p.set_value(true);
            } else {
                std::cout << "Failed to add data filter" << std::endl;
                p.set_value(false);
            }
        });
   if(status == telux::common::Status::SUCCESS) {
        std::cout << "Add data filter Request sent" << std::endl;
        if (p.get_future().get()) {
             std::cout << "Add data filter succeeded." << std::endl;
        }
   } else {
        std::cout << "Add data filter Request failed" << std::endl;
   }
   
   ~~~~~~
