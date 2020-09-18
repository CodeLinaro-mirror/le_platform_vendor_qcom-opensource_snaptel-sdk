/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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

#include "DataListener.hpp"
#include "DataUtils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void DataListener::initDataCallListResponseCb(
    const std::vector<std::shared_ptr<telux::data::IDataCall>> &dataCallList,
    telux::common::ErrorCode error) {

    if (telux::common::ErrorCode::SUCCESS == error) {
        for (auto dataCall:dataCallList) {
            updateDataCallMap(dataCall);
        }
    }
}

void DataListener::onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   logDataCallDetails(dataCall);
   updateDataCallMap(dataCall);
}

void DataListener::onServiceStatusChange(telux::common::ServiceStatus status) {
   PRINT_NOTIFICATION << " ** Data onServiceStatusChange **\n";
   switch(status) {
      case telux::common::ServiceStatus::SERVICE_AVAILABLE:
         std::cout << " SERVICE_AVAILABLE\n";
         break;
      case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
         std::cout << " SERVICE_UNAVAILABLE\n";
         break;
      default:
         std::cout << " Unknown service status \n";
         break;
   }
}
std::shared_ptr<telux::data::IDataCall> DataListener::getDataCall(int slotId, int profileId) {
   std::lock_guard<std::mutex> lk(mtx_);
   std::shared_ptr<telux::data::IDataCall> dataCall = nullptr;
   for (auto& dc : dataCallMap_) {
      //If datacall with same profile id and slot id is found and it is in connected state
      if ((dc.first == profileId) && (slotId == dc.second->getSlotId()) &&
          (telux::data::DataCallStatus::NET_CONNECTED == dc.second->getDataCallStatus())) {
          dataCall = dc.second;
      }
   }
   return dataCall;
}

void DataListener::updateDataCallMap(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   if(dataCall) {
      std::lock_guard<std::mutex> lk(mtx_);
      int profileId = dataCall->getProfileId();
      //Find if datacall object exist
      std::multimap<int, std::shared_ptr<telux::data::IDataCall>>::iterator dataCallMapItr =
          dataCallMap_.end();
      for (auto dcItr = dataCallMap_.begin(); dcItr != dataCallMap_.end(); ++dcItr) {
         if ((dcItr->first == profileId) && (dataCall->getSlotId() == dcItr->second->getSlotId())) {
            dataCallMapItr = dcItr;
            break;
         }
      }
      //If data call object not found
      if((dataCallMapItr == dataCallMap_.end())) {
         //If it is not disconnect notification
         if(telux::data::DataCallStatus::NET_NO_NET != dataCall->getDataCallStatus()) {
            //Add it to list
            dataCallMap_.emplace(profileId, dataCall);
         }
         else {
            // it is disconnect notification, ignore it
         }
      }
      else {
         //If it is not disconnect notification
         if(telux::data::DataCallStatus::NET_NO_NET != dataCall->getDataCallStatus()) {
            //Update data call object
            dataCallMapItr->second = dataCall;
         }
         else {
            // it is disconnect notification, remove it from list
            dataCallMap_.erase(dataCallMapItr);
         }
      }
   }
}

void DataListener::logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << " ** DataCall Details **\n";
   std::cout << " SlotID: " << dataCall->getSlotId()
             << "\n ProfileID: " << dataCall->getProfileId()
             << "\n InterfaceName: " << dataCall->getInterfaceName()
             << "\n DataCallStatus: " << DataUtils::dataCallStatusToString(dataCall->getDataCallStatus())
             << "\n DataCallEndReason:\n   Type: "
             << DataUtils::callEndReasonTypeToString(dataCall->getDataCallEndReason().type)
             << ", Code: " << DataUtils::callEndReasonCode(dataCall->getDataCallEndReason()) << std::endl;
   if (telux::data::IpFamilyType::IPV4V6 == dataCall->getIpFamilyType()) {
      std::cout << " IPv4 Status: " << DataUtils::dataCallStatusToString(
         dataCall->getIpv4Info().status) << std::endl;
      std::cout << " IPv6 Status: " << DataUtils::dataCallStatusToString(
         dataCall->getIpv6Info().status) << std::endl;
   }
   std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
   for(auto &it : ipAddrList) {
      std::cout << "\n ifAddress: " << it.ifAddress << "\n gwAddress: " << it.gwAddress
                << "\n primaryDnsAddress: " << it.primaryDnsAddress
                << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << '\n';
   }
   std::cout << " IpFamilyType: " << DataUtils::ipFamilyTypeToString(dataCall->getIpFamilyType()) << '\n';
   std::cout << " TechPreference: " << DataUtils::techPreferenceToString(dataCall->getTechPreference())
             << '\n';
   std::cout << " DataBearerTechnology: " << DataUtils::bearerTechToString(dataCall->getCurrentBearerTech())
             << '\n';
   std::cout << " OperationType: " << DataUtils::operationTypeToString(dataCall->getOperationType())
             << '\n';
}


