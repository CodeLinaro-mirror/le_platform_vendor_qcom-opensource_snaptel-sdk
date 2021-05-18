Get Roaming Status and Indication {#get_roaming_status_and_indication}
======================================================================

# How to get roaming status and indications

Please follow below steps to request roaming status and indication

### 1. Implement IServingSystemListener listener class

   ~~~~~~{.cpp}
    class ServingSystemListener : public telux::data::IServingSystemListener {
    public:
    ServingSystemListener(SlotId slotId) : slotId_(slotId) {}
    void onRoamingStatusChanged(telux::data::RoamingStatus status) {
        std::cout << "\n onRoamingStatusChanged on SlotId: "
                    << static_cast<int>(slotId_) << std::endl;
        bool isRoaming = status.isRoaming;
        if(isRoaming) {
            std::cout << "System is in Roaming State" << std::endl;
        } else {
            std::cout << "System is not in Roaming State" << std::endl;
        }
    }
    private:
    SlotId slotId_;
    };
   ~~~~~~

### 2. Instantiate initialization callback - this is optional

   ~~~~~~{.cpp}
    auto initCb = [&](telux::common::ServiceStatus status) {
        subSystemStatus = status;
        subSystemStatusUpdated = true;
        cv_.notify_all();
    };
   ~~~~~~

### 3. Get the DataFactory and data Serving System Manager instance

   ~~~~~~{.cpp}
    auto &dataFactory = telux::data::DataFactory::getInstance();
    do {
        subSystemStatusUpdated = false;
        std::unique_lock<std::mutex> lck(mtx_);
        dataServingSystemMgr = dataFactory.getServingSystemManager(slotId, initCb);
   ~~~~~~
### 4. Check if data Serving System manager is ready

   ~~~~~~{.cpp}
        if (dataServingSystemMgr) {
            std::cout << "\n\nInitializing Data Serving System manager subsystem on slot " <<
                slotId << ", Please wait ..." << std::endl;
            cv_.wait(lck, [&]{return subSystemStatusUpdated;});
            subSystemStatus = dataServingSystemMgr->getServiceStatus();
        }
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << " *** DATA Serving System is Ready *** " << std::endl;
            break;
        }
        else {
            std::cout << " *** Unable to initialize data Serving System *** " << std::endl;
        }
    } while (1);
   ~~~~~~

### 5. Register for Serving System listener

   ~~~~~~{.cpp}
      dataServingSystemMgr->registerListener(dataListener);
   ~~~~~~

### 6. Get current Service Status

   ~~~~~~{.cpp}
      // Callback
      auto respCb = [&slotId](
                     telux::data::RoamingStatus roamingStatus, telux::common::ErrorCode error) {
         std::cout << std::endl << std::endl;
         std::cout << "CALLBACK: "
                     << "requestRoamingStatus Response on slotid " << static_cast<int>(slotId);
         if(error == telux::common::ErrorCode::SUCCESS) {
            std::cout << " is successful" << std::endl;
            logRoamingStatusDetails(roamingStatus);
         }
         else {
            std::cout << " failed"
                      << ". ErrorCode: " << static_cast<int>(error) << std::endl;
         }
      };
      dataServingSystemMgr->requestRoamingStatus(respCb);
   ~~~~~~

### 7. Wait for request response and notifications
