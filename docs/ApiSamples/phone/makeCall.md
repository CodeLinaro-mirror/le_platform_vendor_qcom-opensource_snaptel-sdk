# Making a Voice Call

*Quick steps:* Please follow below steps to make a voice call

### 1. Get the PhoneFactory and PhoneManager instances ###
   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();
   ~~~~~~

### 2. Check if telephony subsystem is ready ###

   ~~~~~~{.cpp}
   bool subSystemsStatus = phoneManager->isSubsystemReady();
   ~~~~~~

### 3. If telephony subsystem is not ready, wait for it to be ready ###

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
   subSystemsStatus = f.get();
   ~~~~~~

### 4. Get Phone identifiers of the phones that are present on the device ###
   ~~~~~~{.cpp}
   if(subSystemsStatus) {
      std::vector<int> phoneIds;
      auto status=phoneManager->getPhoneIds(&phoneIds);
   }
   ~~~~~~

### 5. Get Phone instance ###

   ~~~~~~{.cpp}
   auto defaultPhone = phoneManager->getPhone();
   ~~~~~~
   or get  phone with a particular identifier

   ~~~~~~{.cpp}
   auto phoneObj = phoneManager->getPhone(phoneIds[0])
   ~~~~~~

### 6. Implement callback for Make call ###

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

### 7. Make call from the phone instance by passing the dial number(call params), status and callback object ###

   ~~~~~~{.cpp}
   std::string dialNumber("+18588451326");
   const CallParams phCallParams = {dialNumber};
   Status *status = SUCCESS;
   auto callbackObj = std::make_shared<MyPhoneCallback>();
   std::shared_ptr<ICall> callObjWithCallback =  defaultPhone->makeCall(phCallParams, &status, callbackObj);
   ~~~~~~
