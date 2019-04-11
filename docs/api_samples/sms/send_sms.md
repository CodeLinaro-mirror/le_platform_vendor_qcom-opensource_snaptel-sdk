# Sending SMS

Please follow below steps to send an SMS to any mobile number.

### 1. Get the PhoneFactory and PhoneManager instances. ###

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();
   ~~~~~~

### 2. Check if telephony subsystem is ready ###

   ~~~~~~{.cpp}
   bool subSystemsStatus = phoneManager->isSubsystemReady();
   ~~~~~~

##### 2.1 If telephony subsystem is not ready, wait for it to be ready ###


Telephony subsystems is to make sure that device is ready for services like Phone, SMS
and others. if subsystems were not ready, wait for unconditionally.

   ~~~~~~{.cpp}
   if(!subSystemsStatus) {
      std::future<bool> f = phoneManager->onSubsystemReady();
      subSystemsStatus = f.get();
   }
   ~~~~~~

### 3. Instantiate SMS sent and delivery callback ###

   ~~~~~~{.cpp}
   auto smsSentCb = std::make_shared<SmsCallback>();
   auto smsDeliveryCb = std::make_shared<SmsDeliveryCallback>();
   ~~~~~~

### 3.1 Implement ICommandResponseCallback interface to know SMS sent and Delivery status ###

   ~~~~~~{.cpp}
   class SmsCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
   };

   void SmsCallback::commandResponse(ErrorCode error) {
      std::cout << "onSmsSent callback" << std::endl;
   }

   class SmsDeliveryCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
   };

   void SmsDeliveryCallback::commandResponse(ErrorCode error) {
      std::cout << "SMS Delivery callback" << std::endl;
   }
   ~~~~~~

### 4. Get default SMS Manager instance ###

   ~~~~~~{.cpp}
   std::shared_ptr<ISmsManager> smsManager = phoneFactory.getSmsManager();
   ~~~~~~

### 5. Send an SMS using ISmsManager by passing the text and receiver number along with required callback ###

   ~~~~~~{.cpp}
   if(smsManager) {
      std::string receiverAddress("+18989531755");
      std::string message("TEST message");
      smsManager->sendSms(message, receiverAddress, smsSentCb, smsDeliveryCb);
   }
   ~~~~~~

### 6. Receive responses for sendSms request ###
