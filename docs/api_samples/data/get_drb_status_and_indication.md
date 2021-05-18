Get Dedicated Radio Bearer Status and Indication {#get_drb_status_and_indication}
=================================================================================

# How to get dedicated radio bearer status and indications

Please follow below steps to get dedicated radio bearer status and indication

### 1. Implement IServingSystemListener listener class

   ~~~~~~{.cpp}
    class ServingSystemListener : public telux::data::IServingSystemListener {
    public:
    ServingSystemListener(SlotId slotId) : slotId_(slotId) {}
    void onDrbStatusChanged(telux::data::DrbStatus status) override {
        std::cout << "\n onDrbStatusChanged on SlotId: "
                    << static_cast<int>(slotId_) << std::endl;
        switch(status) {
            case telux::data::DrbStatus::ACTIVE:
                std::cout << "Current Drb Status is Active" << std::endl;
                break;
            case telux::data::DrbStatus::DORMANT:
                std::cout << "Current Drb Status is Dormant" << std::endl;
                break;
            case telux::data::DrbStatus::UNKNOWN:
                std::cout << "Current Drb Status is Unknown" << std::endl;
                break;
            default:
                std::cout << "Error: Unexpected Drb Status is reported" << std::endl;
                break;
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

### 6. Get dedicated radio bearer Status

   ~~~~~~{.cpp}
    telux::data::DrbStatus drbStatus = dataServingSystemMgr->getDrbStatus();
    switch(drbStatus) {
        case telux::data::DrbStatus::ACTIVE:
        std::cout << "Current Drb Status is Active" << std::endl;
        break;
        case telux::data::DrbStatus::DORMANT:
        std::cout << "Current Drb Status is Dormant" << std::endl;
        break;
        case telux::data::DrbStatus::UNKNOWN:
        std::cout << "Current Drb Status is Unknown" << std::endl;
        break;
        default:
        std::cout << "Error: Unexpected Drb Status is reported" << std::endl;
        break;
    }
   ~~~~~~

### 7. Wait for dedicated radio bearer notifications
