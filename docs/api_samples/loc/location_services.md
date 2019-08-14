# Using Location Service APIs

Please follow below steps to get Location, Satellite Vehicle (SV) and Jammer Info reports

### 1. Implement a command response function ###
   ~~~~~~{.cpp}
    void CmdResponse(ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            std::cout << " Command executed successfully" << std::endl;
        }
        else {
            std::cout << " Command failed\n errorCode: " << static_cast<int>(error) << std::endl;
        }
    }
   ~~~~~~

### 2. Implement ILocationListener interface ###
   ~~~~~~{.cpp}
    class MyLocationListener : public ILocationListener {
        public:
            void onBasicLocationUpdate(const std::shared_ptr<ILocationInfoBase> &locationInfo)
              override;
            void onDetailedLocationUpdate(const std::shared_ptr<ILocationInfoEx> &locationInfo)
              override;
            void onGnssSVInfo(const std::shared_ptr<IGnssSVInfo> &gnssSVInfo) override;
            void onGnssSignalInfo(const std::shared_ptr<IGnssSignalInfo> &gnssDatainfo) override;
    };
   ~~~~~~

### 3. Get the LocationFactory instance ###
   ~~~~~~{.cpp}
    auto &locationFactory = LocationFactory::getInstance();
   ~~~~~~

### 4. Get LocationManager instance ###
   ~~~~~~{.cpp}
    auto locationManager_ = locationFactory.getLocationManager();
   ~~~~~~

### 5. Wait for the location subsystem initialization ###
   ~~~~~~{.cpp}
    bool subSystemsStatus = locationManager_->isSubsystemReady();
    if (!subSystemsStatus) {
        std::cout << "Location subsystem is not ready, Please wait!!!... "<< std::endl;
        std::future<bool> f = locationManager_->onSubsystemReady();
        subSystemsStatus = f.get();
    }
   ~~~~~~

### 6. Exit the application, if SDK is unable to initialize location subsystems ###
   ~~~~~~{.cpp}
    if (subSystemsStatus) {
        std::cout<< "Subsystem is ready" << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize Location subsystem"<< std::endl;
        return -1;
    }
   ~~~~~~

### 7. Instantiate MyLocationListener ###
   ~~~~~~{.cpp}
    auto myLocationListener = std::make_shared<MyLocationListener>();
   ~~~~~~

### 8. Register for Location, SV and Jammer info updates ###
   ~~~~~~{.cpp}
    locationManager_->registerListenerEx(myLocationListener);
   ~~~~~~

### 9. Start Location reports with Detailed information ###
   ~~~~~~{.cpp}
    uint32_t minIntervalInput = 2000; // Default is 1000 milli seconds.
    locationManager_->startDetailedReports(minIntervalInput, CmdResponse);
   ~~~~~~

### 10. Command response callback is invoked with error code indicating SUCCESS or FAILURE of the operation. ###

### 11. Wait for Location fix, SV and Jammer info ###
   ~~~~~~{.cpp}
    void MyLocationListener::onDetailedLocationUpdate(
      const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
        std::cout << "New detailed Location info received" << std::endl;
    }

    void MyLocationListener::onGnssSVInfo(
      const std::shared_ptr<telux::loc::IGnssSVInfo> &gnssSVInfo) {
        std::cout << "Gnss Satellite Vehicle info received" << std::endl;
    }

    void MyLocationListener::onGnssSignalInfo(
      const std::shared_ptr<telux::loc::IGnssSignalInfo> &gnssDatainfo) {
        std::cout << "Gnss Signal info received" << std::endl;
    }
   ~~~~~~

### 12. Observe that changes in the configuration parameters have corresponding effect on how the Location, Satellite Vehicle (SV) and Jammer reports are received. The configuration will be same across all the location client applications and the least value of the parameter from all client applications will take effect finally. ###
