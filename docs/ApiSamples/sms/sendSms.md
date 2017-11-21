# Sending SMS

*Quick steps:* Please follow below steps to send an SMS to any mobile number.

### 1. Implement ICommandResponseCallback interface to know SMS sent status ###

   ~~~~~~{.cpp}
   class SmsCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
   };

   void SmsCallback::commandResponse(ErrorCode error) {
      if(error == ErrorCode::SUCCESS) {
         std::cout << "onSmsSent successfully" << std::endl;
      } else {
         std::cout << "onSmsSent failed" << std::endl;
      }
      std::cout << "onSmsSent error = " << (int)error << std::endl;
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

##### 3.1 If telephony subsystem is not ready, wait for it to be ready ###

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready" << std::endl;
      std::cout << "wait uncondotionally for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemsStatus = f.get();
   }
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

### 4. Exit the application, if SDK is unable to initialize telephony subsystems ###

   ~~~~~~{.cpp}
   if(subSystemStatus) {
      std::cout << " *** Subsystem Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }
   ~~~~~~

### 5. Instantiate ICommandResponseCallback ###

   ~~~~~~{.cpp}
   auto smsCb = std::make_shared<SmsCallback>();
   ~~~~~~

### 6. Get default SMS Manager instance ###

   ~~~~~~{.cpp}
   std::shared_ptr<ISmsManager> smsManager = phoneFactory.getSmsManager();
   ~~~~~~

### 7. Send an SMS from the SMS Manager by passing the text and receiver number ###

   ~~~~~~{.cpp}
   if(smsManager != nullptr) {
      std::string message("Test Message");
      std::string receiverAddress("+18588451326");
      smsManager->sendSms(message, receiverAddress, smsCb);
   }
   ~~~~~~
   
### 8. Receive command( i.e. send SMS) response in ICommandResponseCallback ###