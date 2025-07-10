Request voice service state updates {#request_voice_service_state}
==========================================================

This sample application demonstrates how to request voice service state of the device.

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

   ~~~~~~{.cpp}
   if (!subSystemsStatus) {
      std::future<bool> f = phoneManager->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Instantiate Phone

   ~~~~~~{.cpp}
   auto phone = phoneManager->getPhone();
   ~~~~~~

### 4. Check for operating mode

If operating mode is not ONLINE turn it to ON in order to perform any operations on the phone. Either wait for operating mode to be on or set operating mode to ONLINE.

   ~~~~~~{.cpp}
   OperatingMode opMode = phoneManager->requestOperatingMode();
   if (opMode != OperatingMode::ONLINE){
      phone->setOperatingMode(OperatingMode::ONLINE);
   }
   ~~~~~~

### 5. Implement IVoiceServiceStateCallback interface

   ~~~~~~{.cpp}
   class MyVoiceServiceStateCallback : public telux::tel::IVoiceServiceStateCallback {
   public:
      void voiceServiceStateResponse(const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo, telux::common::ErrorCode error) override;
   };
   ~~~~~~

### 6. Instantiate MyVoiceServiceStateCallback

   ~~~~~~{.cpp}
   auto myVoiceServiceStateCallback = std::make_shared<MyVoiceServiceStateCallback>();
   ~~~~~~

### 7. Send voice service state request

   ~~~~~~{.cpp}
   phone->requestVoiceServiceState(myVoiceServiceStateCallback);
   ~~~~~~

After receiving voiceServiceStateResponse in MyVoiceServiceStateCallback, the status of voice registration can be accessed by using the VoiceServiceInfo.
