/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <string>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>

class MySubscriptionListener : public telux::tel::ISubscriptionListener {
public:
   void onSubscriptionInfoChanged(std::shared_ptr<telux::tel::ISubscription> subscription) override {
      if(subscription) {
         std::cout << "\n**onSubscriptionInfoChanged -- Subscription Details**" << std::endl;
         std::cout << " CarrierName : " << subscription->getCarrierName() << std::endl;
         std::cout << " PhoneNumber : " << subscription->getPhoneNumber() << std::endl;
         std::cout << " IccId : " << subscription->getIccId() << std::endl;
         std::cout << " Mcc : " << subscription->getMcc() << std::endl;
         std::cout << " Mnc : " << subscription->getMnc() << std::endl;
         std::cout << " SlotId : " << subscription->getSlotId() << std::endl;
         std::cout << " Imsi: " << subscription->getImsi() << std::endl;
      } else {
         std::cout << " \n Subscription is empty" << std::endl;
      }
   }
};

/**
 * Main routine
 */
int main(int argc, char *argv[]) {

   // [1] Get the PhoneFactory and SubscriptionManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   auto subMgr = phoneFactory.getSubscriptionManager();

   // [2] Check if telephony subsystem is ready
   bool subSystemStatus = subMgr->isSubsystemReady();

   // [2.1] If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "Subscription subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = subMgr->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemStatus) {
      std::cout << " *** Subscription Sub System is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }

   // [3] Register listener with Subscription Manager for the notification
   auto mySubscriptionListener = std::make_shared<MySubscriptionListener>();
   subMgr->registerListener(mySubscriptionListener);

   // [4] Get the default subscription instance
   std::shared_ptr<telux::tel::ISubscription> subscription = subMgr->getSubscription();
   if(subscription) {
      std::cout << "**Subscription Details**" << std::endl;
      std::cout << " CarrierName : " << subscription->getCarrierName() << std::endl;
      std::cout << " PhoneNumber : " << subscription->getPhoneNumber() << std::endl;
      std::cout << " IccId : " << subscription->getIccId() << std::endl;
      std::cout << " Mcc : " << subscription->getMcc() << std::endl;
      std::cout << " Mnc : " << subscription->getMnc() << std::endl;
      std::cout << " SlotId : " << subscription->getSlotId() << std::endl;
      std::cout << " Imsi : " << subscription->getImsi() << std::endl;
   } else {
      std::cout << "Subscription is empty" << std::endl;
   }

   // [5] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [6] Cleanup
   subMgr->removeListener(mySubscriptionListener);
   return 0;
}
