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
#include <memory>
#include <iomanip>

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataProfile.hpp>
#include <telux/data/DataProfileManager.hpp>

// 3. Implement ICommandCallback interface to know status of requestProfileList
class MyDataProfilesCallback : public telux::data::IDataProfileListCallback {
   // 5. Receive responses for requestProfileList request
   void onProfileListResponse(const std::vector<std::shared_ptr<telux::data::DataProfile>> &profiles,
                              telux::common::ErrorCode error) {
      static std::vector<std::shared_ptr<telux::data::DataProfile>> profiles_;

      profiles_.clear();
      profiles_ = profiles;
      std::cout << std::endl << std::endl;
      std::cout << " ** onProfileListResponse **" << std::endl;
      std::cout << std::setw(2)
                << "+-----------------------------------------------------------------+"
                << std::endl;
      std::cout << std::setw(14) << "| Profile # | " << std::setw(11) << "TechPref | "
                << std::setw(15) << "      APN      " << std::setw(17) << "|  ProfileName  |"
                << std::setw(10) << " IP Type |" << std::endl;
      std::cout << std::setw(2)
                << "+-----------------------------------------------------------------+"
                << std::endl;
      for(auto it : profiles) {
         std::cout << std::left << std::setw(4) << "  " << std::setw(10) << it->getId()
                   << std::setw(11) << techPreferenceToString(it->getTechPreference())
                   << std::setw(15) << it->getApn() << std::setw(17) << it->getName()
                   << std::setw(10) << ipFamilyTypeToString(it->getIpFamilyType()) << std::endl;
      }
      std::cout << "ErrorCode:" << (int)error << std::endl;
      std::cout << std::endl << std::endl;
   }

   std::string techPreferenceToString(telux::data::TechPreference techPref) {
      switch(techPref) {
         case telux::data::TechPreference::TP_3GPP:
            return "3gpp";
         case telux::data::TechPreference::TP_3GPP2:
            return "3gpp2";
         case telux::data::TechPreference::TP_ANY:
         default:
            return "Any";
      }
   }

   std::string ipFamilyTypeToString(telux::data::IpFamilyType ipType) {
      switch(ipType) {
         case telux::data::IpFamilyType::IPV4:
            return "IPv4";
         case telux::data::IpFamilyType::IPV6:
            return "IPv6";
         case telux::data::IpFamilyType::IPV4V6:
            return "IPv4v6";
         case telux::data::IpFamilyType::UNKNOWN:
         default:
            return "NA";
      }
   }
};

int main(int argc, char *argv[]) {
   // 1. Get the DataFactory and DataProfileManager instances
   auto &dataFactory = telux::data::DataFactory::getInstance();
   std::shared_ptr<telux::data::IDataProfileManager> dataProfileMgr
      = dataFactory.getDataProfileManager();

   // [2] Check if data subsystem is ready
   bool subSystemStatus = dataProfileMgr->isSubsystemReady();

   // [2.1] If data subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "DATA Profile subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = dataProfileMgr->onSubsystemReady();
      // If we want to wait unconditionally for data subsystem to be ready
      subSystemStatus = f.get();
   }

   // [3] Exit the application, if SDK is unable to initialize data subsystems
   if(subSystemStatus) {
      std::cout << " *** DATA Profile Subsystem is Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize data subsystem *** " << std::endl;
      return 1;
   }
   // 2. Instantiate requestProfileList callback
   std::shared_ptr<MyDataProfilesCallback> myDataProfileListCb_
      = std::make_shared<MyDataProfilesCallback>();
   // 4. Send a requestProfileList along with required callback function
   if(dataProfileMgr) {
      dataProfileMgr->requestProfileList(myDataProfileListCb_);
   }
   // 6. Exit logic for the application
   std::cout << "\n\nPress ENTER to exit \n\n";
   std::cin.ignore();
   return 0;
}