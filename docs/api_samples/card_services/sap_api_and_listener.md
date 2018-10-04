# Using SAP APIs

Please follow below steps to use SAP APIs to send APDU and listen to SAP events


### 1. Get the PhoneFactory and PhoneManager instances. ###

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();
   ~~~~~~

### 2. Wait for the telephony subsystem initialization. ###

   ~~~~~~{.cpp}
   bool subSystemsStatus = cardManager->isSubsystemReady();
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready, wait for it to be ready " << std::endl;
      std::future<bool> f = cardManager->onSubsystemReady();
      auto status = f.wait_for(std::chrono::seconds(5));
      if(status == std::future_status::ready) {
         subSystemsStatus = true;
      }
   }
   ~~~~~~


### 3. Get default Sap Card Manager instance ###

   ~~~~~~{.cpp}
   std::shared_ptr<ISapCardManager> sapCardMgr = phoneFactory.getSapCardManager();
   ~~~~~~


### 4. Instantiate ICommandResponseCallback, IAtrResponseCallback and ISapCardCommandCallback ###

   ~~~~~~{.cpp}
   auto mySapCmdResponseCb = std::make_shared<MySapCommandResponseCallback>();
   auto myAtrCb = std::make_shared<MyAtrResponseCallback>();
   auto myTransmitApduResponseCb = std::make_shared<MySapTransmitApduResponseCallback>();
   ~~~~~~


######  4.1 Implementation of ICommandResponseCallback interface for receiving notifications on sap events like open connection and close connection ######

   ~~~~~~{.cpp}
   class MySapCommandResponseCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error);
   };

   void MySapCommandResponseCallback::commandResponse(ErrorCode error) {
      std::cout << "commandResponse, error: " << (int)error << std::endl;
   }
   ~~~~~~


###### 4.2 Implementation of IAtrResponseCallback interface for receiving notification on sap event like request answer to reset(ATR) ######

   ~~~~~~{.cpp}
   class MyAtrResponseCallback : public IAtrResponseCallback {
   public:
      void atrResponse(std::vector<int> responseAtr, ErrorCode error);
   };

   void MyAtrResponseCallback::atrResponse(std::vector<int> responseAtr, ErrorCode error) {
      std::cout << "atrResponse, error: " << (int)error << std::endl;
   }
   ~~~~~~


###### 4.3 Implementation of ISapCardCommandCallback interface for receiving notification on sap event like transmit apdu. ######

   ~~~~~~{.cpp}
   class MySapTransmitApduResponseCallback : public ISapCardCommandCallback {
   public:
      void onResponse(IccResult result, ErrorCode error);
   };

   void MySapTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
      std::cout << "transmitApduResponse, error: " << (int)error << std::endl;
   }
   ~~~~~~

### 5. Open Sap connection and wait for request to complete ###

   ~~~~~~{.cpp}
   sapCardMgr->openConnection(SapCondition::SAP_CONDITION_BLOCK_VOICE_OR_DATA, mySapCmdResponseCb);
   std::cout << "Opening SAP connection to Transmit the APDU..." << std::endl;
   ~~~~~~

### 6. request sap ATR and wait for complete ###

   ~~~~~~{.cpp}
   sapCardMgr->requestAtr(myAtrCb);
   ~~~~~~

### 7. send sap apdu and wait for the request to complete ###

   ~~~~~~{.cpp}
   std::cout << "Transmit Sap APDU request made..." << std::endl;
   Status ret = sapCardMgr->transmitApdu(CLA, INSTRUCTION, P1, P2, LC, DATA, 0,
                                         myTransmitApduResponseCb);
   ~~~~~~

### 8. close sap connection and wait for the request to complete ###

   ~~~~~~{.cpp}
   sapCardMgr->closeConnection(mySapCmdResponseCb);
   ~~~~~~

