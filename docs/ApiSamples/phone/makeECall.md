# Making eCall (Emergency E112)

*Quick steps:* Please follow below steps to make an emergency call(eCall).

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


### 5. Get unique id of the phone

   ~~~~~~{.cpp}
   int phoneId;
   phone->getPhoneId(phoneId);
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


### 7. Initialize the Minimum Set of Data(MSD) data as shown

Initialize macros to create MSD data, like MSD_VERSION, LATITUDE, LONGITUDE and others which
is required for populate eCallMsdData

### 8. Create details required to make emergency call(eCall) like eCallMsdData, emergencyCategory and eCallVariant

   ~~~~~~{.cpp}
   int emergencyCategory = 64;
   ECallMsdData eCallMsdData; 
   int eCallVariant = 1;
   int msdVersion_ = MSD_VERSION;
   // Populate eCallMsdData with valid information
   ~~~~~~


### 9. Send a eCall request

   ~~~~~~{.cpp}
   if(callManager) {
      auto makeCallStatus = callManager->makeECall(phoneId, eCallMsdData, emergencyCategory,
                                                   eCallVariant, dialCb);
      std::cout << "Dial ECall Status:" << (int)makeCallStatus << std::endl;
   }
   ~~~~~~
