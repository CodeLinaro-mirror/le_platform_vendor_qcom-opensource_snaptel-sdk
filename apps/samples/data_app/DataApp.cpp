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
#include <memory>
#include <cstdlib>

#include <telux/data/DataFactory.hpp>

/**
 * @file: DataApp.cpp
 *
 * @brief: Simple application to start the data call on the given profile id
 */

// Response callback for start or stop dataCall
void responseCallback(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                      telux::common::ErrorCode errorCode) {
   std::cout << "startCallResponse: errorCode: " << static_cast<int>(errorCode) << std::endl;
}

// Implementation of IDataConnectionListener
class DataConnectionListener : public telux::data::IDataConnectionListener {
public:
   void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) override {
      std::cout << "\n onDataCallInfoChanged";
      logDataCallDetails(dataCall);
   }

private:
   void logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
      std::cout << " ** DataCall Details **\n";
      std::cout << " SlotID: " << dataCall->getSlotId() << std::endl;
      std::cout << " ProfileID: " << dataCall->getProfileId() << std::endl;
      std::cout << " interfaceName: " << dataCall->getInterfaceName() << std::endl;
      std::cout << " DataCallStatus: " << (int)dataCall->getDataCallStatus() << std::endl;
      std::cout
         << " DataCallEndReason: Type = " << static_cast<int>(dataCall->getDataCallEndReason().type)
         << std::endl;
      std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
      for(auto &it : ipAddrList) {
         std::cout << "\n ifAddress: " << it.ifAddress
                   << "\n primaryDnsAddress: " << it.primaryDnsAddress
                   << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << '\n';
      }
      std::cout << " IpFamilyType: " << static_cast<int>(dataCall->getIpFamilyType()) << '\n';
      std::cout << " TechPreference: " << static_cast<int>(dataCall->getTechPreference()) << '\n';
      std::cout << " DataBearerTechnology: " << static_cast<int>(dataCall->getCurrentBearerTech())
                << '\n';
   }
};

int main(int argc, char *argv[]) {
   // [1] Get the DataFactory
   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dataConnMgr = dataFactory.getDataConnectionManager();

   // [2] Check if data subsystem is ready
   bool subSystemStatus = dataConnMgr->isSubsystemReady();

   // [2.1] If data subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "DATA subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = dataConnMgr->onSubsystemReady();
      // If we want to wait unconditionally for data subsystem to be ready
      subSystemStatus = f.get();
   }

   // [3] Exit the application, if SDK is unable to initialize data subsystems
   if(subSystemStatus) {
      std::cout << " *** DATA Sub System is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize data subsystem *** " << std::endl;
      return 1;
   }

   // [4] Register for Data listener
   std::shared_ptr<telux::data::IDataConnectionListener> dataListener
      = std::make_shared<DataConnectionListener>();
   dataConnMgr->registerListener(dataListener);

   // [5] Start data call on the mentioned profile id
   if(argc == 2) {
      int profileId = std::atoi(argv[1]);
      telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;
      dataConnMgr->startDataCall(profileId, ipFamilyType, responseCallback);

   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_app <profieId> \n";
      std::cout << "\n\t ./data_app 1    --> to start the data call on profile Id 1\n";
   }

   // [6] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [7] Cleanup
   dataConnMgr->deregisterListener(dataListener);
   dataConnMgr = nullptr;
   return 0;
}
