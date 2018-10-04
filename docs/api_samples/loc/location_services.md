# Using Location Service APIs

Please follow below steps to get Location, Satellite Vehicle (SV) Info reports

### 1. Implement ICommandResponseCallback interface for receiving notifications ###
   ~~~~~~{.cpp}
   class MyLocationCommandResponseCallback : public ICommandResponseCallback {
   public:
      MyLocationCommandResponseCallback() {
      }

      void commandResponse(ErrorCode error) override;
   };
   ~~~~~~

### 2. Implement ILocationListener interface ###
   ~~~~~~{.cpp}
   class MyLocationListener : public ILocationListener {
   public:
      void onLocationUpdate(const std::shared_ptr<ILocationInfo> &locationInfo) override;

      void onGnssSVInfo(const std::shared_ptr<IGnssSVInfo> &gnssSVInfo) override;
   };
   ~~~~~~

### 3. Get the LocationFactory instance ###
   ~~~~~~{.cpp}
   auto &locationFactory = LocationFactory::getInstance();
   ~~~~~~

### 4. Get LocationManager instance ###
   ~~~~~~{.cpp}
   auto locationManager = locationFactory.getLocationManager();
   ~~~~~~

### 5. Wait for the location subsystem initialization ###
   ~~~~~~{.cpp}
   bool subSystemsStatus = locationManager->isSubsystemReady();
   if(!subSystemsStatus) {
      std::cout << "Location subsystem is not ready" << std::endl;
      std::cout << "wait uncondotionally for it to be ready " << std::endl;
      std::future<bool> f = locationManager->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 6. Exit the application, if SDK is unable to initialize location subsystems ###
   ~~~~~~{.cpp}
   if(subSystemsStatus) {
      std::cout << " *** Sub Systems Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize location subsystem" << std::endl;
      return 1;
   }
   ~~~~~~

### 7. Instantiate MyLocationListener ###
   ~~~~~~{.cpp}
   auto myLocationListener = std::make_shared<MyLocationListener>();
   ~~~~~~

### 8. Register for Location and SV info updates ###
   ~~~~~~{.cpp}
   locationManager->registerListener(myLocationListener);
   ~~~~~~

### 9. Wait for Location fix and SV info ###

Following configuration values will be used by default which can be changed through configuration APIs
- HORIZONTAL_ACCURACY_LEVEL = 1 // Low
- FINAL_REPORT_INTERVAL = 1000 // 1 second
- POSITION_REPORT_TIMEOUT = 255000  // 255 seconds

   ~~~~~~{.cpp}
   void MyLocationListener::onLocationUpdate(const std::shared_ptr<ILocationInfo> &locationInfo) {
      std::cout << std::endl;
      std::cout << "*********************** Location Report *********************" << std::endl;
      time_t realtime;
      realtime = (time_t)(locationInfo->getTimeStamp());
      std::cout << "Timestamp : " << ctime(&realtime) << std::endl;
      std::cout << "Latitude : " << locationInfo->getLatitude() << std::endl;
      std::cout << "Longitude : " << locationInfo->getLongitude() << std::endl;
   }

   void MyLocationListener::onGnssSVInfo(const std::shared_ptr<IGnssSVInfo> &gnssSVInfo) {
      std::cout << std::endl;
      std::cout << "**************** Satellite Vehicle Infomation ***************" << std::endl;
      std::cout << "Altitude type : " << static_cast<int>(gnssSVInfo->getAltitudeType())
                         << std::endl;
      for(auto svInfo : gnssSVInfo->getSVInfoList()) {
         std::cout << "Constellation : " << static_cast<int>(svInfo->getConstellation())
                            << std::endl;
         std::cout << "Gnss SV Id : " << svInfo->getId() << std::endl;
         std::cout << "SV health htatus : " << static_cast<int>(svInfo->getSVHealthStatus())
                            << std::endl;
         std::cout << "SV status : " << static_cast<int>(svInfo->getStatus()) << std::endl;
         std::cout << "Has ephemeris : " << static_cast<int>(svInfo->getHasEphemeris())
                            << std::endl;
         std::cout << "Has almanac : " << static_cast<int>(svInfo->getHasAlmanac())
                            << std::endl;
         std::cout << "Elevation : " << svInfo->getElevation() << std::endl;
         std::cout << "Azimuth : " << svInfo->getAzimuth() << std::endl;
         std::cout << "SNR : " << svInfo->getSnr() << std::endl;
      }

      std::cout << "*************************************************************" << std::endl;
   }
   ~~~~~~

### 10. Instantiate MyLocationCommandResponseCallback ###
   ~~~~~~{.cpp}
   auto myLocationCommandCb = std::make_shared<MyLocationCommandResponseCallback>();
   ~~~~~~

### 11. Set minimum interval for reports ###
   ~~~~~~{.cpp}
   uint32_t minIntervalForReports = 1500; // Default is 1000 milli seconds.
   locationManager->setMinIntervalForReports(minIntervalForReports, myLocationCommandCb);
   ~~~~~~

### 12. Command response callback is invoked with error code indicating SUCCESS or FAILURE of the operation. ###
   ~~~~~~{.cpp}
   MyLocationCommandResponseCallback::commandResponse(ErrorCode error) {
      if(error == ErrorCode::SUCCESS) {
         std::cout << "Set min interval for reports request is successful ";
      } else {
         std::cout << "Set min interval for reports request failed with error " << static_cast<int>(error);
      }
   }
   ~~~~~~

### 13. Set position timeout ###
   ~~~~~~{.cpp}
   uint32_t timeout = 200000;
   locationManager->setPositionReportTimeout(timeout, myLocationCommandCb);
   ~~~~~~

### 14. Command response callback is invoked with error code indicating SUCCESS or FAILURE of the operation. ###
   ~~~~~~{.cpp}
   MyLocationCommandResponseCallback::commandResponse(ErrorCode error) {
      if(error == ErrorCode::SUCCESS) {
         std::cout << "Set position report timeout request is successful ";
      } else {
         std::cout << "Set position report timeout reports request failed with error " << static_cast<int>(error);
      }
   }
   ~~~~~~

### 15. Configuring horizontal accuracy level ###
   ~~~~~~{.cpp}
   HorizontalAccuracyLevel accuracy = HorizontalAccuracyLevel::MEDIUM;
   locationManager->setHorizontalAccuracyLevel(accuracy, myLocationCommandCb);
   ~~~~~~

### 16. Command response callback is invoked with error code indicating SUCCESS or FAILURE of the operation. ###
   ~~~~~~{.cpp}
   MyLocationCommandResponseCallback::commandResponse(ErrorCode error) {
      if(error == ErrorCode::SUCCESS) {
         std::cout << "Set horizontal accuracy level request is successful ";
      } else {
         std::cout << "Set horizontal accuracy request failed with error " << static_cast<int>(error);
      }
   }
   ~~~~~~

### 17. Observe that changes in the configuration parameters have corresponding effect on how the Location and SV Info reports are received. ###
