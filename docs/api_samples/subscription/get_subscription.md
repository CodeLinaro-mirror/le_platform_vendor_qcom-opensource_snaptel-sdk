# Using Subscription Manager APIs

Please follow below steps to use Subscription Manager APIs to get Subscription Information.

##### 1. Get the PhoneFactory and SubscriptionManager instances

   ~~~~~~{.cpp}
   auto &phoneFactory = PhoneFactory::getInstance();
   std::shared_ptr<ISubscriptionManager> subscriptionMgr = phoneFactory.getSubscriptionManager();
   ~~~~~~

##### 2. Wait for the Subscription subsystem to ready

   ~~~~~~{.cpp}
   if(!subscriptionMgr->isSubsystemReady()) {
      auto subSystemStatus = subscriptionMgr->onSubsystemReady().get();
      if(!subSystemStatus){
         // if Subscription subsystem fails, then exit the application.
         exit(1);
      }
   }
   ~~~~~~

##### 3. Get the Subscription information

   ~~~~~~{.cpp}
   std::shared_ptr<ISubscription> subscription = subscriptionMgr->getSubscription();

   if(subscription != nullptr) {
      std::cout << "Subscription Details" << std::endl;
      std::cout << " CarrierName : " << subscription->getCarrierName() << std::endl;
      std::cout << " CountryISO : " << subscription->getCountryISO() << std::endl;
      std::cout << " PhoneNumber : " << subscription->getPhoneNumber() << std::endl;
      std::cout << " IccId : " << subscription->getIccId() << std::endl;
      std::cout << " Mcc : " << subscription->getMcc() << std::endl;
      std::cout << " Mnc : " << subscription->getMnc() << std::endl;
      std::cout << " SlotId : " << subscription->getSlotId() << std::endl;
      std::cout << " SubscriptionId : " << subscription->getSubscriptionId() << std::endl;
   }
   ~~~~~~
