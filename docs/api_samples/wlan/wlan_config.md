# How to configure and enable wlan {#wlan_config}

Please follow below steps to configure wlan

### 1. Create IWlanListener class
   ~~~~~~{.cpp}
    class NotificationListener: public telux::wlan::IWlanListener {
    public:
    void onEnableChanged(bool enable) {
        promise_.set_value(enable);
    }

    bool getEnableStatus() {
        return promise_.get_future().get();
    }

    void resetPromise() {
        promise_ = std::promise<bool>();
    }
    private:
    std::promise<bool> promise_;
    };
   ~~~~~~

### 2. Create initialization callback object to be notified when the object has completed initialization and is ready for use
   ~~~~~~{.cpp}
    auto initCb = [&](telux::common::ServiceStatus status) {
        initPromise.set_value(status);
    };
   ~~~~~~

### 3. Get the WlanFactory and Device Manager instance
   ~~~~~~{.cpp}
    auto &wlanFactory = telux::wlan::WlanFactory::getInstance();
    wlanDevMgr  = wlanFactory.getWlanDeviceManager(initCb);
   ~~~~~~

### 4. Wait for object to complete initialization
   ~~~~~~{.cpp}
    telux::common::ServiceStatus = subSystemStatus = initPromise.get_future().get();
    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** Wlan SubSystem is Ready *** " << std::endl;
    }
    else {
        std::cout << " *** Unable to initialize Wlan subsystem *** " << std::endl;
    }
   ~~~~~~

### 5. If WLAN is enabled, disable it
   ~~~~~~{.cpp}
    bool enableStat = false;
    std::vector<telux::wlan::InterfaceStatus> ifStatus;
    if(telux::common::ErrorCode::SUCCESS != wlanDevMgr->getStatus(enableStat, ifStatus)) {
        std::cout << "Failed to retrieve Wlan status" <<std::endl;
    }
    if(enableStat) {
        //Disable Wlan before changing configuration...
        wlanDevMgr->registerListener(listener);
        //Ensure callback returns SUCCESS and indication for Wlan disablement is received
        if((telux::common::ErrorCode::SUCCESS != wlanDevMgr->enable(false)) ||
            (true == listener->getEnableStatus())) {
            std::cout << "Failed to disable Wlan " <<std::endl;
            break;
        }
    }
   ~~~~~~

### 6. Set desired WLAN configurations
   ~~~~~~{.cpp}
    if(telux::common::ErrorCode::SUCCESS != wlanDevMgr->setMode(numAp, numSta)) {
        std::cout << "Failed to set Wlan configuration" <<std::endl;
    }
   ~~~~~~

### 7. Enable WLAN for new configurations to take effect
   ~~~~~~{.cpp}
    listener->resetPromise();
    //Ensure callback returns SUCCESS and indication for Wlan enablement is received
    if((telux::common::ErrorCode::SUCCESS != wlanDevMgr->enable(true)) ||
       (false == listener->getEnableStatus())) {
        std::cout << "Failed to enable Wlan " <<std::endl;
    }
   ~~~~~~
