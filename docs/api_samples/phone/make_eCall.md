# Making eCall (Emergency E112)

Please follow below steps to make an emergency call(eCall).

### 1. Get the PhoneFactory and PhoneManager instances.

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();
   ~~~~~~

### 2. Check if telephony subsystem is ready

   ~~~~~~{.cpp}
   bool subSystemsStatus = phoneManager->isSubsystemReady();
   ~~~~~~

### 2.1 If telephony subsystem is not ready, wait for it to be ready

Telephony subsystems is to make sure that device is ready for services like Phone, SMS
and others. if subsystems were not ready, wait for unconditionally.

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::future<bool> f = phoneManager->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Instantiate Phone and call manager

   ~~~~~~{.cpp}
   auto phone = phoneManager->getPhone();
   std::shared_ptr<ICallManager> callManager = phoneFactory.getCallManager();
   ~~~~~~


### 5. Initialize phoneId with default value

   ~~~~~~{.cpp}
   int phoneId = DEFAULT_PHONE_ID;
   ~~~~~~

### 6. Instantiate dial callback instance - this is optional

   ~~~~~~{.cpp}
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback> ();
   ~~~~~~


##### 6.1. implement IMakeCallCallback interface to receive response for the dial request - optional

   ~~~~~~{.cpp}
   class DialCallback : public IMakeCallCallback {
   public:
      void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) override;
   };

   void DialCallback::makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
      // will be invoked with response of makeECall operation
   }
   ~~~~~~


### 7. Initialize the data required for eCall such as eCallMsdData,emergencyCategory and eCallVariant

   ~~~~~~{.cpp}
   ECallCategory emergencyCategory = ECallCategory::VOICE_EMER_CAT_AUTO_ECALL;
   ECallVariant eCallVariant = ECallVariant::ECALL_TEST;
   // Instantiate ECallMsdData structure and populate it with valid information
   // such as Latitude, Longitude etc.
   // Parameter values mentioned here are for illustrative purposes only.
   ECallMsdData eCallMsdData;
   eCallMsdData.msdData.messageIdentifier = 1; // Each MSD message should bear a unique id
   eCallMsdData.optionals.recentVehicleLocationN1Present = true;
   eCallMsdData.optionals.recentVehicleLocationN2Present = true;
   eCallMsdData.optionals.numberOfPassengersPresent = 2;
   eCallMsdData.msdData.control.automaticAvtivation = true;
   eCallMsdData.control.testCall = true;
   eCallMsdData.control.positionCanBeTrusted = true;
   eCallMsdData.control.vehicleType = ECallVehicleType::PASSENGER_VEHICLE_CLASS_M1;
   eCallMsdData.msdData.vehicleIdentificationNumber.isowmi = "ECA";
   eCallMsdData.msdData.vehicleIdentificationNumber.isovds = "LLEXAM";
   eCallMsdData.msdData.vehicleIdentificationNumber.isovisModelyear = "P";
   eCallMsdData.msdData.vehicleIdentificationNumber.isovisSeqPlant = "LE02013";
   eCallMsdData.msdData.vehiclePropulsionStorage.gasolineTankPresent = true;
   eCallMsdData.msdData.vehiclePropulsionStorage.dieselTankPresent = false;
   eCallMsdData.vehiclePropulsionStorage.compressedNaturalGas = false;
   eCallMsdData.vehiclePropulsionStorage.liquidPropaneGas = false;
   eCallMsdData.vehiclePropulsionStorage.electricEnergyStorage = false;
   eCallMsdData.vehiclePropulsionStorage.hydrogenStorage = false;
   eCallMsdData.vehiclePropulsionStorage.otherStorage = false;
   eCallMsdData.timestamp = 1367878452;
   eCallMsdData.vehicleLocation.positionLatitude = 123;
   eCallMsdData.vehicleLocation.positionLongitude = 1234;
   eCallMsdData.msdData.vehicleDirection = 4;
   eCallMsdData.recentVehicleLocationN1.latitudeDelta = false;
   eCallMsdData.recentVehicleLocationN1.longitudeDelta = 0;
   eCallMsdData.recentVehicleLocationN2.latitudeDelta = true;
   eCallMsdData.recentVehicleLocationN2.
   ~~~~~~

### 8. Send a eCall request

   ~~~~~~{.cpp}
   if(callManager) {
      auto makeCallStatus = callManager->makeECall(phoneId, eCallMsdData, emergencyCategory,
                                                   eCallVariant, dialCb);
      std::cout << "Dial ECall Status:" << (int)makeCallStatus << std::endl;
   }
   ~~~~~~
