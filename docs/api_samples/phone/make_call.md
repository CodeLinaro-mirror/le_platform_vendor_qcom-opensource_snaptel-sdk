# Making a Voice Call

Please follow below steps to make a voice call

### 1. Get the PhoneFactory and PhoneManager instances

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

### 4. Initialize phoneId with default value

   ~~~~~~{.cpp}
   int phoneId = DEFAULT_PHONE_ID;
   ~~~~~~

### 5. Instantiate dial call instance - this is optional

   ~~~~~~{.cpp}
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback> ();
   ~~~~~~

### 5.1 Implement IMakeCallCallback interface to receive response for the dial request optional

   ~~~~~~{.cpp}
   class DialCallback : public IMakeCallCallback {
   public:
      void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) override;
   };

   void DialCallback::makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
      // will be invoked with response of makeCall operation
   }
   ~~~~~~

### 6. Send a dial request

   ~~~~~~{.cpp}
   if(callManager) {
      std::string phoneNumber("+18989531755");
      auto makeCallStatus = callManager->makeCall(phoneId, phoneNumber, dialCb);
      std::cout << "Dial Call Status:" << (int)makeCallStatus << std::endl;
   }
   ~~~~~~
