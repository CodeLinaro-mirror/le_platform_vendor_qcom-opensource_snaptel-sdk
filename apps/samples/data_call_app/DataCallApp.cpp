/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <telux/data/DataFactory.hpp>
#include <thread>

/**
 * @file: DataCallApp.cpp
 *
 * @brief: Sample application that demonstrates the usage of SDK data libraries
 * to start data call and handle throttle indications
 */

static int profileId = 0;
#define MAX_START_DATA_CALL_RETRY 5
#define DATA_CALL_RETRY_TIMER 2

// Implementation of IDataConnectionListener
class DataConnectionManager
    : public telux::data::IDataConnectionListener,
      public std::enable_shared_from_this<telux::data::IDataConnectionListener> {
  public:
   bool init() {
      // [1] Get the DataFactory
      auto &dataFactory = telux::data::DataFactory::getInstance();
      dataConnMgr_ = dataFactory.getDataConnectionManager();

      // [2] Check if the data subsystem is ready
      bool subSystemStatus = dataConnMgr_->isSubsystemReady();

      // [2.1] If data subsystem is not ready, wait for it to be ready
      if (!subSystemStatus) {
         std::cout << "DATA subsystem is not ready" << std::endl;
         std::cout << "wait unconditionally for it to be ready " << std::endl;
         std::future<bool> f = dataConnMgr_->onSubsystemReady();
         // If we want to wait unconditionally for the data subsystem to be
         // ready
         subSystemStatus = f.get();
      }

      // [3] Exit the application, if SDK is unable to initialize data
      // subsystems
      if (subSystemStatus) {
         std::cout << " *** DATA Sub System is Ready *** " << std::endl;
      } else {
         std::cout << " *** ERROR - Unable to initialize data subsystem *** "
                   << std::endl;
         return false;
      }

      // [4] Register for Data listener
      if (dataConnMgr_->registerListener(shared_from_this()) !=
          telux::common::Status::SUCCESS) {
         return false;
      }
      return true;
   }

   void startDataCall() {
      // [5] attempt to start data call
      std::promise<telux::common::ErrorCode> prom = std::promise<telux::common::ErrorCode>();
      {
         telux::common::Status status;
         std::unique_lock<std::mutex> lock(dataCallUpdateMutex_);
         if (isDataCallRequestInProgress_ || isDataCallConnected_) {
            // data call is connected or data call request already in progress
            return;
         }
         if (++dataCallAttempt_ > MAX_START_DATA_CALL_RETRY) {
            // max data call retry attempt reached
            std::cout << "failed to start data call attempted "
                      << MAX_START_DATA_CALL_RETRY << " times" << std::endl;
            exit(1);
         }
         std::cout << "start data call attempt: " << dataCallAttempt_ << std::endl;
         telux::data::IpFamilyType ipFamilyType =
             telux::data::IpFamilyType::IPV4;
         status = dataConnMgr_->startDataCall(
             profileId, ipFamilyType,
             [this, &prom](
                 const std::shared_ptr<telux::data::IDataCall> &dataCall,
                 telux::common::ErrorCode errorCode) {
                // callback of start data call
                std::cout << "startCallResponse: errorCode: "
                          << static_cast<int>(errorCode) << std::endl;
                prom.set_value(errorCode);
             });

         if (status == telux::common::Status::SUCCESS &&
             prom.get_future().get() == telux::common::ErrorCode::SUCCESS) {
            isDataCallRequestInProgress_ = true;
            return;
         }
      }

      if (isProfileThrottled()) {
         //[5.1] data profile is throttled wait to get un-throttled @ref [8]
         std::cout << "\n data profile is throttled wait to get un-throttled";
      } else {
         std::this_thread::sleep_for(
             std::chrono::seconds(DATA_CALL_RETRY_TIMER));
         // retry start data call
         std::cout << "\n retry start data call ";
         startDataCall();
      }
   }

   // This function is called when there is a change in the data call
   void onDataCallInfoChanged(
       const std::shared_ptr<telux::data::IDataCall> &dataCall) override {
      // [6] Start data call update
      {
         std::unique_lock<std::mutex> lock(dataCallUpdateMutex_);
         std::cout << "\n onDataCallInfoChanged";
         logDataCallDetails(dataCall);
         telux::data::DataCallStatus status = dataCall->getDataCallStatus();
         if (status == telux::data::DataCallStatus::NET_CONNECTED) {
            // Start data call success
            std::cout << "\n onDataCallInfoChanged data call connected !!!";
            isDataCallConnected_ = true;
            isDataCallRequestInProgress_ = false;
            return;
         } else if (status == telux::data::DataCallStatus::NET_CONNECTING) {
            std::cout << "\n Trying to connect data call";
            return;
         } else {
            // start data call failed
            isDataCallConnected_ = false;
            isDataCallRequestInProgress_ = false;
         }
      }

      if (isProfileThrottled()) {
         //[6.1] data profile is throttled wait to get un-throttled @ref [8]
         std::cout << "\n data profile is throttled wait to get un-throttled";
         return;
      } else {
         std::this_thread::sleep_for(
             std::chrono::seconds(DATA_CALL_RETRY_TIMER));
         // retry start data call
         std::cout << "\n retry start data call ";
         startDataCall();
      }
   }

   bool isProfileThrottled() {
      // [7] Check if the data profile throttled
      std::promise<bool> prom = std::promise<bool>();
      telux::common::Status status = dataConnMgr_->requestThrottledApnInfo(
          [&](const std::vector<telux::data::APNThrottleInfo> &throttleInfoList,
              telux::common::ErrorCode error) {
             std::cout << "startCallResponse: errorCode: "
                       << static_cast<int>(error) << std::endl;
             logThrottledApnInfoChanged(throttleInfoList);
             bool isProfileThrottled = false;
             if (profileId) {
                for (auto throttleInfo : throttleInfoList) {
                   for (int tProfId : throttleInfo.profileIds) {
                      if (profileId == tProfId) {
                         isProfileThrottled = true;
                         break;
                      }
                   }
                }
             }
             prom.set_value(isProfileThrottled);
          });
      if (status == telux::common::Status::SUCCESS) {
         return prom.get_future().get();
      } else {
         std::cout
             << "Error: failed to trigger requestThrottledApnInfo; status: "
             << static_cast<int>(status) << std::endl;
         return false;
      }
   }

   // This function is called when the throttled state changes, such as when a
   // new APN is throttled or an existing throttled APN is no longer throttled
   // after the timeout. APNs that are not throttled anymore will not appear in
   // the list of throttled APNs.
   void onThrottledApnInfoChanged(
       const std::vector<telux::data::APNThrottleInfo> &throttleInfoList) {
      // [8] callback of start data call
      bool reTriggerStartDataCall = false;
      {
         std::unique_lock<std::mutex> lock(throttleUpdateMutex_);
         logThrottledApnInfoChanged(throttleInfoList);
         bool newThrottleState = false;
         if (profileId) {
            for (auto throttleInfo : throttleInfoList) {
               for (int tProfId : throttleInfo.profileIds) {
                  // absence of profile id in throttle info considered as the
                  // profile is not throttled
                  if (profileId == tProfId) {
                     newThrottleState = true;
                     if (throttleInfo.isBlocked) {
                        // APN blocked on all plmns
                        std::cout << "\n APN = " << throttleInfo.apn
                                  << "; is blocked on all plmns!!!\n";
                     }
                     break;
                  }
               }
            }
         }
         // profile was throttled before now as per the updated indication it's
         // not throttled. So retry to start the data call
         if (isProfileThrottled_ && !newThrottleState) {
            reTriggerStartDataCall = true;
         }
         isProfileThrottled_ = newThrottleState;
      }

      if (reTriggerStartDataCall) {
         startDataCall();
      }
   }

   void cleanUp() {
      // [9] deregister listener and loos data connection manager reference
      dataConnMgr_->deregisterListener(shared_from_this());
      dataConnMgr_ = nullptr;
   }

  private:
   void logDataCallDetails(
       const std::shared_ptr<telux::data::IDataCall> &dataCall) {
      std::cout << " ** DataCall Details **\n";
      std::cout << " SlotID: " << dataCall->getSlotId() << std::endl;
      std::cout << " ProfileID: " << dataCall->getProfileId() << std::endl;
      std::cout << " interfaceName: " << dataCall->getInterfaceName()
                << std::endl;
      std::cout << " DataCallStatus: " << (int)dataCall->getDataCallStatus()
                << std::endl;
      std::cout << " DataCallEndReason: Type = "
                << static_cast<int>(dataCall->getDataCallEndReason().type)
                << std::endl;
      std::list<telux::data::IpAddrInfo> ipAddrList =
          dataCall->getIpAddressInfo();
      for (auto &it : ipAddrList) {
         std::cout << "\n ifAddress: " << it.ifAddress
                   << "\n primaryDnsAddress: " << it.primaryDnsAddress
                   << "\n secondaryDnsAddress: " << it.secondaryDnsAddress
                   << '\n';
      }
      std::cout << " IpFamilyType: "
                << static_cast<int>(dataCall->getIpFamilyType()) << '\n';
      std::cout << " TechPreference: "
                << static_cast<int>(dataCall->getTechPreference()) << '\n';
      std::cout << " DataBearerTechnology: "
                << static_cast<int>(dataCall->getCurrentBearerTech()) << '\n';
   }

   void logThrottledApnInfoChanged(
       const std::vector<telux::data::APNThrottleInfo> &throttleInfoList) {
      std::cout << "** onThrottledApnInfoChanged **" << std::endl;
      std::cout << " Number of throttled APN: " << throttleInfoList.size()
                << std::endl;
      int index = 0;
      for (auto throttleInfo : throttleInfoList) {
         std::cout << " index = " << ++index << std::endl << " Profile IDs = ";
         for (int profileId : throttleInfo.profileIds) {
            std::cout << profileId << ", ";
         }
         std::cout << std::endl
                   << " APN: " << throttleInfo.apn << std::endl
                   << " ipv4Time (msec): " << throttleInfo.ipv4Time << std::endl
                   << " ipv6Time (msec): " << throttleInfo.ipv6Time << std::endl
                   << " isBlocked: "
                   << (throttleInfo.isBlocked ? "True" : "False") << std::endl
                   << " mcc: " << throttleInfo.mcc << std::endl
                   << " mnc: " << throttleInfo.mnc << std::endl
                   << std::endl;
      }
   }

   std::shared_ptr<telux::data::IDataConnectionManager> dataConnMgr_;
   int dataCallAttempt_ = 0;
   bool isProfileThrottled_ = false;
   std::mutex throttleUpdateMutex_;

   bool isDataCallConnected_ = false;
   bool isDataCallRequestInProgress_ = false;
   std::mutex dataCallUpdateMutex_;
};

int main(int argc, char *argv[]) {
   if (argc == 2) {
      profileId = std::atoi(argv[1]);
   } else {
      std::cout << "\n Invalid argument!!! \n\n";
      std::cout << "\n Sample command is: \n";
      std::cout << "\n\t ./data_app <profieId> \n";
      std::cout << "\n\t ./data_app 1    --> to start the data call on "
                   "profile Id 1\n";
      return 1;
   }

   // initialize data connection manager
   std::shared_ptr<DataConnectionManager> dataConnMgr = std::make_shared<DataConnectionManager>();
   if (dataConnMgr->init()) {
      // attempt to start data call
      dataConnMgr->startDataCall();
   } else {
      std::cout << "\n\nfailed to initialize data connection manager!!! \n\n";
      return 1;
   }

   // exit logic
   std::cout << "\n\nPress ENTER to exit!!! \n\n";
   std::cin.ignore();

   // Cleanup
   dataConnMgr->cleanUp();
   return 0;
}
