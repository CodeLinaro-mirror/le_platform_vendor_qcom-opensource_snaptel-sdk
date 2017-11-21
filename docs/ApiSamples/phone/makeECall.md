# Making eCall (Emergency E112)

*Quick steps:* Please follow below steps to make an emergency call(eCall).

### 1. Implement IPhoneListener interface ###

   ~~~~~~{.cpp}
   #define print_notification std::cout << "NOTIFICATION: "
   class MyPhoneListener : public IPhoneListener {
   public:
      void onECallMsdTransmissionStatus(std::shared_ptr<IPhone> phone, MsdStatus status, ErrorCode error);
   };

   void MyPhoneListener::onECallMsdTransmissionStatus(std::shared_ptr<IPhone> phone, MsdStatus status, ErrorCode error){
      print_notification << "MyPhoneListener::onECallMsdTransmissionStatus" << std::endl;
      print_notification << "Msd Status:" << (int)status << std::endl;
      print_notification << "Ecall Msd Transmission status:" << (int)error <<std::endl;
   }
   ~~~~~~


### 2. Get the PhoneFactory and PhoneManager instances ###

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();
   ~~~~~~

### 3. Check if telephony subsystem is ready ###

   ~~~~~~{.cpp}
   bool subSystemsStatus = phoneManager->isSubsystemReady();
   ~~~~~~

### 4. If telephony subsystem is not ready, wait for it to be ready ###

   ~~~~~~{.cpp}
   std::future<bool> f = phoneManager->onSubSystemReady();
    // If we want to wait unconditionally for telephony subsystem to be ready
   subSystemsStatus = f.get();
   ~~~~~~
   or

   ~~~~~~{.cpp}
   // If we want to wait with a timeout
   std::future_status status;
   status = f.wait_for(std::chrono::seconds(5));
   if (status != std::future_status::ready) {
     // Error out
   }
   subSystemStatus = f.get();
   ~~~~~~

### 5. Exit the application, if SDK is unable to initialize telephony subsystems ###

   ~~~~~~{.cpp}
   if(subSystemStatus) {
      std::cout << " *** Subsystem Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }
   ~~~~~~

### 6. Get Phone instance ###

   ~~~~~~{.cpp}
   auto defaultPhone = phoneManager->getPhone());
  ~~~~~~

### 7. Register listener to eCall MSD transmission status by instantiating global IPhoneListener  ###

   ~~~~~~{.cpp}
   auto myPhoneListener = std::make_shared<MyPhoneListener>();
   phoneManager->registerListener(ListenType::ECALL_STATE, myPhoneListener);	
   ~~~~~~

### 8. Initialize the Minimum Set of Data(MSD) data as shown ###

   ~~~~~~{.cpp}
   // eCall MSD data
   #define MSD_VERSION 2
   #define MESSAGE_IDENTIFIER 60
   #define AUTOMATIC_ACTIVATION 1
   #define TEST_CALL 0
   #define POSITION_CAN_BE_TRUSTED 1
   #define VEHICLE_TYPE 0
   #define ISO_WMI "ECA"
   #define ISO_VDS "LLEXAM"
   #define ISO_VIS_MODEL_YEAR "P"
   #define ISO_VIS_SEQ_PLANT "LE02013"
   #define GASOLINE_TANK_PRESENT 1
   #define DIESEL_TANK_PRESENT 0
   #define COMPRESSED_NATURALGAS 0
   #define LIQUID_PROPANE_GAS 0
   #define ELECTRIC_ENERGY_STORAGE 0
   #define HYDROGEN_STORAGE 0
   #define OTHER_STORAGE 0
   #define TIMESTAMP 1367878452
   #define VEHICLE_POSITION_LATITUDE 123
   #define VEHICLE_POSITION_LONGITUDE 1234
   #define VEHICLE_DIRECTION 4
   #define RECENT_LOCATION_N1_PRESENT 1
   #define RECENT_N1_LATITUDE_DELTA -1
   #define RECENT_N1_LONGITUDE_DELTA -10
   #define RECENT_LOCATION_N2_PRESENT 1
   #define RECENT_N2_LATITUDE_DELTA -1
   #define RECENT_N2_LONGITUDE_DELTA -30
   #define NUMBER_OF_PASSENGERS_PRESENT 1
   #define NUMBER_OF_PASSENGERS 2
   #define VIN "ECALLEXAMPLE02013"
   #define OPTIONAL_DATA_PRESENT 1
   #define OPTIONALS_OPTIONAL_DATA_TYPE 1
   ~~~~~~
### 9. Create details required to make emergency call(eCall) like eCallMsdData , emergencyCategory and eCallVariant ###

   ~~~~~~{.cpp}
   int emergencyCategory = 64;
   ECallMsdData eCallMsdData; 
   int eCallVariant = 1;
   int msdVersion_ = MSD_VERSION;
   eCallMsdData.optionals.recentVehicleLocationN1Present = RECENT_LOCATION_N1_PRESENT;
   eCallMsdData.optionals.recentVehicleLocationN2Present = RECENT_LOCATION_N2_PRESENT;
   eCallMsdData.optionals.numberOfPassengersPresent = NUMBER_OF_PASSENGERS_PRESENT;
   eCallMsdData.messageIdentifier = MESSAGE_IDENTIFIER;
   eCallMsdData.control.automaticAvtivation = AUTOMATIC_ACTIVATION;
   eCallMsdData.control.testCall = TEST_CALL;
   eCallMsdData.control.positionCanBeTrusted = POSITION_CAN_BE_TRUSTED;
   eCallMsdData.control.vehicleType = static_cast<VehicleType>(VEHICLE_TYPE);
   eCallMsdData.vehicleIdentificationNumber.isowmi = ISO_WMI;
   eCallMsdData.vehicleIdentificationNumber.isovds = ISO_VDS;
   eCallMsdData.vehicleIdentificationNumber.isovisModelyear = ISO_VIS_MODEL_YEAR;
   eCallMsdData.vehicleIdentificationNumber.isovisSeqPlant = ISO_VIS_SEQ_PLANT;
   eCallMsdData.vehiclePropulsionStorage.gasolineTankPresent = GASOLINE_TANK_PRESENT;
   eCallMsdData.vehiclePropulsionStorage.dieselTankPresent = DIESEL_TANK_PRESENT;
   eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas = COMPRESSED_NATURALGAS;
   eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas = LIQUID_PROPANE_GAS;
   eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage = ELECTRIC_ENERGY_STORAGE;
   eCallMsdData.vehiclePropulsionStorage.hydrogenStorage = HYDROGEN_STORAGE;
   eCallMsdData.vehiclePropulsionStorage.otherStorage = OTHER_STORAGE;
   eCallMsdData.timestamp = TIMESTAMP;
   eCallMsdData.vehicleLocation.positionLatitude = VEHICLE_POSITION_LATITUDE;
   eCallMsdData.vehicleLocation.positionLongitude = VEHICLE_POSITION_LONGITUDE;
   eCallMsdData.vehicleDirection = VEHICLE_DIRECTION;
   eCallMsdData.optionals.optionalDataType = (OptionalDataType)OPTIONALS_OPTIONAL_DATA_TYPE;
   eCallMsdData.optionals.optionalDataPresent = OPTIONAL_DATA_PRESENT;
   eCallMsdData.recentVehicleLocationN1.latitudeDelta = RECENT_N1_LATITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN1.longitudeDelta = RECENT_N1_LONGITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN2.latitudeDelta = RECENT_N2_LATITUDE_DELTA;
   eCallMsdData.recentVehicleLocationN2.longitudeDelta = RECENT_N2_LONGITUDE_DELTA;
   eCallMsdData.numberOfPassengers = NUMBER_OF_PASSENGERS;
   ~~~~~~

### 10.  Implement callback for Make ECall ###

   ~~~~~{.cpp}
    class MyPhoneCallback: public ICommandResponseCallback {
    public:
        MyPhoneCallback() { }
        ~MyPhoneCallback() { }
        void commandResponse(ErrorCode error) {
            LOG(DEBUG, "MyCallCommandCallback: " __FUNCTION__);
            LOG(INFO, "ErrorCode: ", int(error));
        }
    };
   ~~~~~

### 11. Invoke Emergency Call API using Phone instance ###

   ~~~~~~{.cpp}
   auto callbackObj = std::make_shared<MyPhoneCallback>();
   auto eCallWithCallback = defaultPhone->makeECall(eCallMsdData, emergencyCategory, eCallVariant, callbackObj);
   ~~~~~~
