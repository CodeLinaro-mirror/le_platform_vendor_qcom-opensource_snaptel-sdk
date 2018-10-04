# Request Voice Service State of the device

Please follow below steps to get voice service state notifications.

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

### 4. Check for radio state ###

If radio is in OFF state turn it to ON in order to perform any operations on the phone. Either wait for radio to be turned on or else pass the callback to receive the response for setRadioPower

   ~~~~~~{.cpp}
   RadioState radioState = phone->getRadioState();
   if(radioState == RadioState::RADIO_STATE_OFF){
      phone->setRadioPower(true);
   }
   ~~~~~~

### 5. Implement IVoiceServiceStateCallback interface ###

   ~~~~~~{.cpp}
   class MyVoiceServiceStateCallback : public telux::tel::IVoiceServiceStateCallback {
   public:
      void voiceServiceStateResponse(const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo, telux::common::ErrorCode error) override;
   };
   ~~~~~~

### 6. Instantiate MyVoiceServiceStateCallback ###

   ~~~~~~{.cpp}
   auto myVoiceServiceStateCallback = std::make_shared<MyVoiceServiceStateCallback>();
   ~~~~~~

### 7. Send voice service state request. ###

   ~~~~~~{.cpp}
   phone->requestVoiceServiceState(myVoiceServiceStateCallback);
   ~~~~~~

### 8. After receiving voiceServiceStateResponse in MyVoiceServiceStateCallback, the status of voice registration can be accessed by using the VoiceServiceInfo. ###
