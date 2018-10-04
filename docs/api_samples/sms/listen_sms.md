# Listening to Incoming SMS

Please follow below steps to listen for incoming SMS

### 1. Implement ISmsListener interface to receive incoming SMS ###
   ~~~~~~{.cpp}
   class MySmsListener : public ISmsListener {
   public:
      void onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> message) override;
   };

   void MySmsListener::onIncomingSms(int phoneId, std::shared_ptr<SmsMessage> smsMsg) {
      std::cout << "MySmsListener::onIncomingSms from PhoneId : " << phoneId << std::endl;
      std::cout << "smsReceived: From : " << smsMsg->toString() << std::endl;
   }
   ~~~~~~

### 2. Get the PhoneFactory and PhoneManager instances ###

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();
   ~~~~~~

### 3. Check if telephony subsystem is ready ###

   ~~~~~~{.cpp}
   bool subSystemStatus = phoneManager->isSubsystemReady();
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

### 5. Instantiate global ISmsListener ###

   ~~~~~~{.cpp}
   auto myPhoneListener = std::make_shared<MyPhoneListener>();
   ~~~~~~

### 6. Get default SMS Manager instance ###

   ~~~~~~{.cpp}
   std::shared_ptr<ISmsManager> smsMgr = phoneFactory.getSmsManager();
   ~~~~~~

### 7. Register for incoming SMS ###

   ~~~~~~{.cpp}
   if(smsMgr) {
      smsMgr->registerListener(mySmsListener);
   }
   ~~~~~~

### 8. Wait for incoming SMS ###

   ~~~~~~{.cpp}
   std::cout << " *** wait for MyPhoneListener::onIncomingSms() to be triggered*** " << std::endl;
   std::cout << " *** Press enter to exit the application *** " << std::endl;
   std::string input;
   std::getline(std::cin, input);
   return 0;
   ~~~~~~
