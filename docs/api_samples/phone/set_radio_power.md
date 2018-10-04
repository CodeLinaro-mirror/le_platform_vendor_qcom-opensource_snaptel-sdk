# Set radio power of the device

Please follow below steps to Radio Power state notifications.

### 1. Get the PhoneFactory and PhoneManager instances ###

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();
   ~~~~~~

### 2. Check if telephony subsystem is ready ###

   ~~~~~~{.cpp}
   bool subSystemsStatus = phoneManager->isSubsystemReady();
   ~~~~~~

### 2.1 If telephony subsystem is not ready, wait for it to be ready ###

   If subsystem is not ready, wait unconditionally.

   ~~~~~~{.cpp}
   if (!subSystemsStatus) {
      std::future<bool> f = phoneManager->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Instantiate Phone ###

   ~~~~~~{.cpp}
   auto phone = phoneManager->getPhone();
   ~~~~~~

### 4. Implement IPhoneListener interface to receive service state change notifications ###

   ~~~~~~{.cpp}
   class MyPhoneListener : public telux::tel::IPhoneListener {
   public:
       void onRadioStateChanged(int phoneId, telux::tel::RadioState radiostate) {
       }
      ~MyPhoneListener() {
      }
   }
   ~~~~~~

### 4.1 Instantiate MyPhoneListener ###

   ~~~~~~{.cpp}
   auto myPhoneListener = std::make_shared<MyPhoneListener>();
   ~~~~~~

### 5. Register for phone info updates ###

   ~~~~~~{.cpp}
   phoneManager->registerListener(myPhoneListener);
   ~~~~~~

### 6. Implement ICommandResponseCallback to receive the status of setRadioPower API call ###

   ~~~~~~{.cpp}
   class MyPhoneCommandResponseCallback : public ICommandResponseCallback {
   public:
      MyPhoneCommandResponseCallback() {
      }
      void commandResponse(ErrorCode error) override;
   };
   ~~~~~~

### 7. Instantiate MyPhoneCommandResponseCallback ###

   ~~~~~~{.cpp}
   auto myPhoneCommandCb = std::make_shared<MyPhoneCommandResponseCallback>();
   ~~~~~~

### 8. Set the Radio power ON/OFF. ###

   ~~~~~~{.cpp}
   phone->setRadioPower(true, myPhoneCommandCb);
   ~~~~~~

### 9. Command response callback is invoked with error code indicating SUCCESS or FAILURE of the operation. ###

   ~~~~~~{.cpp}
   MyPhoneCommandResponseCallback::commandResponse(ErrorCode error) {
      if(error == ErrorCode::SUCCESS) {
         std::cout << "Set Radio Power On request is successful ";
      } else {
         std::cout << "Set Radio Power On request failed with error " << static_cast<int>(error);
      }
   }
   ~~~~~~
