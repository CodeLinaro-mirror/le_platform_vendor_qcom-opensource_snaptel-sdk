/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
#define PROTO_TCP 6
#define PROTO_UDP 17
/**
 * @file: DataFilterApp.cpp
 *
 * @brief: Simple application to start the data call on the given profile id
 */

// Response callback for start or stop dataCall
void responseCallback(const std::shared_ptr<telux::data::IDataCall> &dataCall,
                      telux::common::ErrorCode errorCode) {
   std::cout << "startCallResponse: errorCode: " << static_cast<int>(errorCode) << std::endl;
}

// Response callback for data filter API
void filterResponseCallback(telux::common::ErrorCode errorCode) {
   std::cout << "Data Filter Callback errorCode: " << static_cast<int>(errorCode) << std::endl;
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

// Implementation of IDataFilterListener
class DataFilterListener : public telux::data::IDataFilterListener {
public:
   void onDataRestrictModeChange(telux::data::DataRestrictMode mode) override {
      std::cout << "\n onDataRestrictModeChange";
      logDataFilterModeDetails(mode);
   }

private:
   void logDataFilterModeDetails(const telux::data::DataRestrictMode mode) {
      std::cout << " ** DataRestrictMode Details **\n";
      if (mode.filterMode == telux::data::DataRestrictModeType::ENABLE) {
         std::cout << "\n Data Filter Mode : Enable\n" << std::endl;
      } else if (mode.filterMode == telux::data::DataRestrictModeType::DISABLE) {
         std::cout << "\n Data Filter Mode : Disable\n" << std::endl;
      } else {
         std::cout << " ERROR: Invalid Data Filter mode notified" << std::endl;
      }
   }
};

int main(int argc, char *argv[]) {
   // [1] Get the DataFactory
   auto &dataFactory = telux::data::DataFactory::getInstance();
   // [1.1] Get data connection manager object
   auto dataConnMgr = dataFactory.getDataConnectionManager();
   // [1.2] Get data filter manager object
   auto dataFilterMgr = dataFactory.getDataFilterManager();

   // [2] Check if data connection subsystem is ready
   bool dataConnectionSubSystemStatus = dataConnMgr->isSubsystemReady();

   // [2.1] If data connection subsystem is not ready, wait for it to be ready
   if(!dataConnectionSubSystemStatus) {
      std::cout << "DATA connection subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = dataConnMgr->onSubsystemReady();
      // If we want to wait unconditionally for data subsystem to be ready
      dataConnectionSubSystemStatus = f.get();
   }

   // [2.2] Exit the application, if SDK is unable to initialize data subsystems
   if(dataConnectionSubSystemStatus) {
      std::cout << " *** DATA connection subsystem is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize data subsystem *** " << std::endl;
      return 1;
   }

   // [3] Check if data filter subsystem is ready
   bool dataFilterSubSystemStatus = dataFilterMgr->isReady();

   // [3.1] If data filter subsystem is not ready, wait for it to be ready
   if(!dataFilterSubSystemStatus) {
      std::cout << "DATA filter subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = dataFilterMgr->onReady();
      // If we want to wait unconditionally for data subsystem to be ready
      dataFilterSubSystemStatus = f.get();
   }

   // [3.2] Exit the application, if SDK is unable to initialize data subsystems
   if(dataFilterSubSystemStatus) {
      std::cout << " *** DATA filter subsystem is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize data subsystem *** " << std::endl;
      return 1;
   }

   // [4] Register for Data connection listener
   std::shared_ptr<telux::data::IDataConnectionListener> dataConnectionListener
      = std::make_shared<DataConnectionListener>();
   dataConnMgr->registerListener(dataConnectionListener);

   // [5] Register for Data filter listener
   std::shared_ptr<telux::data::IDataFilterListener> dataFilterListener
      = std::make_shared<DataFilterListener>();
   dataFilterMgr->registerListener(dataFilterListener);

   // [6] Start data call on the mentioned profile id, enable data filter mode and install an UDP
   // filter on give IP:Port combination.
   if(argc == 4) {
      int profileId = std::atoi(argv[1]);
      telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;
      dataConnMgr->startDataCall(profileId, ipFamilyType, responseCallback);

      telux::data::DataRestrictMode enableMode;
      enableMode.filterAutoExit = telux::data::DataRestrictModeType::DISABLE;
      enableMode.filterMode = telux::data::DataRestrictModeType::ENABLE;
      dataFilterMgr->setDataRestrictMode(enableMode, filterResponseCallback, profileId, ipFamilyType);

      std::string ipAddr = std::string(argv[2]);
      int port = std::atoi(argv[3]);
      telux::data::IPv4Info ipv4Info_ = {};
      ipv4Info_.srcAddr = ipAddr;

      telux::data::PortInfo srcPort;
      srcPort.port = port;
      srcPort.range = 0;
      telux::data::UdpInfo udpInfo_ = {};
      udpInfo_.src = srcPort;

      // create a filter of UDP type, and set source IP and port.
      std::shared_ptr<telux::data::IIpFilter> dataFilter = dataFactory.getNewIpFilter(PROTO_UDP);
      dataFilter->setIPv4Info(ipv4Info_);

      auto udpRestrictFilter = std::dynamic_pointer_cast<telux::data::IUdpFilter>(dataFilter);
      udpRestrictFilter->setUdpInfo(udpInfo_);

      dataFilterMgr->addDataRestrictFilter(dataFilter, filterResponseCallback,
                                           profileId, ipFamilyType);

   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_filter_app <profieId> <IP> <Port>\n";
      std::cout << "\n\t ./data_filter_app 1  158.2.3.4 8000  --> to start the data call on \
      profile Id 1 and install filter for Incoming packet matching IP 158.2.3.4 and port 8000 \n";
   }

   // [7] Exit logic for the application
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // [8] Cleanup
   dataConnMgr->deregisterListener(dataConnectionListener);
   dataFilterMgr->deregisterListener(dataFilterListener);
   dataConnMgr = nullptr;
   dataFilterMgr = nullptr;
   return 0;
}
