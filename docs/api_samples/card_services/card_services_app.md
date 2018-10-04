# Using Card Service APIs

Please follow below steps to use Card Service APIs to transmit APDU

### 1. Get the PhoneFactory and CardManager instances. ###
   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   std::shared_ptr<ICardManager> cardManager = phoneFactory.getCardManager();
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


### 3. Get SlotCount, SlotIds and Card instance ###
   ~~~~~~{.cpp}
   int slotCount;
   cardManager->getSlotCount(slotCount);
   std::cout << "Slots Count is :" << slotCount << std::endl;
   std::vector<int> slotIds;
   cardManager->getSlotIds(slotIds);
   std::cout << "Slot Ids are : { ";
   for(auto id : slotIds) {
      std::cout << id << " ";
   }
   std::cout << "}" << std::endl;
   std::shared_ptr<ICard> cardImpl = cardManager->getCard(slotIds.front());
   ~~~~~~

### 4. Get supported applications from the card ###
   ~~~~~~{.cpp}
   std::vector<std::shared_ptr<ICardApp>> applications;
   if(cardImpl) {
      std::cout << "\nApplications available are : " << std::endl;
      applications = cardImpl->getApplications();
      for(auto cardApp : applications) {
         std::cout << "AppId : " << cardApp->getAppId() << std::endl;
      }
   }
   ~~~~~~


### 5. Instantiate optional IOpenLogicalChannelCallback, ICommandResponseCallback and ITransmitApduResponseCallback ###

   ~~~~~~{.cpp}
   auto myOpenLogicalCb = std::make_shared<MyOpenLogicalChannelCallback>();
   auto myCloseLogicalCb = std::make_shared<MyCloseLogicalChannelCallback>();
   auto myTransmitApduResponseCb = std::make_shared<MyTransmitApduResponseCallback>();
   ~~~~~~

###### 5.1 Implementation of ICardChannelCallback interface for receiving notifications on card event like open logical channel ######

   ~~~~~~{.cpp}
   class MyOpenLogicalChannelCallback : public ICardChannelCallback {
   public:
      void onChannelResponse(int channel, IccResult result, ErrorCode error) override;
   };

   void MyOpenLogicalChannelCallback::onChannelResponse(int channel, IccResult result,
                                                        ErrorCode error) {
      std::cout << "onChannelResponse, error: " << (int)error << std::endl;
      std::unique_lock<std::mutex> lock(eventMutex);
      errorCode = error;
      openChannel = channel;
      std::cout << "onChannelResponse: " << result.toString() << std::endl;
      if(cardEventExpected == CardEvent::OPEN_LOGICAL_CHANNEL) {
         std::cout << "Card Event OPEN_LOGICAL_CHANNEL found with code :" << int(error) << std::endl;
         eventCV.notify_one();
      }
   }
   ~~~~~~

###### 5.2. Implementation of ICommandResponseCallback interface for receiving notifications on card event like close logical channel ######

   ~~~~~~{.cpp}
   class MyCloseLogicalChannelCallback : public ICommandResponseCallback {
   public:
      void commandResponse(ErrorCode error) override;
   };

   void MyCloseLogicalChannelCallback::commandResponse(ErrorCode error) {
      std::cout << "commandResponse, error: " << (int)error << std::endl;
      std::unique_lock<std::mutex> lock(eventMutex);
      errorCode = error;
      if(cardEventExpected == CardEvent::CLOSE_LOGICAL_CHANNEL) {
         std::cout << "Card Event CLOSE_LOGICAL_CHANNEL found with code :" << int(error) << std::endl;
         eventCV.notify_one();
      }
   }
   ~~~~~~

###### 5.3. Implementation of ICardCommandCallback interface for receiving notifications on card event like transmit apdu logical channel and transmit apdu basic channel ######

   ~~~~~~{.cpp}
   class MyTransmitApduResponseCallback : public ICardCommandCallback {
   public:
      void onResponse(IccResult result, ErrorCode error) override;
   };

   void MyTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
      std::cout << "onResponse, error: " << (int)error << std::endl;
      std::unique_lock<std::mutex> lock(eventMutex);
      errorCode = error;
      std::cout << "onResponse:  " << result.toString() << std::endl;
      if(cardEventExpected == CardEvent::TRANSMIT_APDU_CHANNEL) {
         std::cout << "Card Event TRANSMIT_APDU_CHANNEL found with code :" << int(error) << std::endl;
         eventCV.notify_one();
      }
   }
   ~~~~~~

### 6. Open Logical Channel and wait for request to complete ###

   ~~~~~~{.cpp}
   std::string aid;
   for(auto app : applications) {
      if(app->getAppType() == APPTYPE_USIM) {
         aid = app->getAppId();
         break;
      }
   }
   cardImpl->openLogicalChannel(aid, myOpenLogicalCb);
   std::cout << "Opening Logical Channel to Transmit the APDU..." << std::endl;
   ~~~~~~

### 7. Transmit Apdu on Logical Channel, wait for request to complete ###

   ~~~~~~{.cpp}
   cardImpl->transmitApduLogicalChannel(openChannel, CLA, INSTRUCTION, P1, P2, P3, DATA,
                                        myTransmitApduResponseCb);
   std::cout << "Transmit APDU request made..." << std::endl;
   ~~~~~~

### 8. Close the opened logical channel and wait for the completion ###

   ~~~~~~{.cpp}
   cardImpl->closeLogicalChannel(openChannel, myCloseLogicalCb);
   std::cout << "Close the Logical Channel..." << std::endl;
   ~~~~~~

### 9. Transmit Apdu on Basic Channel and wait for completion ###

   ~~~~~~{.cpp}
   cardImpl->transmitApduBasicChannel(CLA, INSTRUCTION, P1, P2, P3, DATA, myTransmitApduResponseCb);
   std::cout << "Transmit APDU request on Basic channel made..." << std::endl;
   ~~~~~~
