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

/**
 * Data Connection Manager Sample Application using Telematics SDK
 * This is used to demonstrate data connection manager APIs like start/stop data
 * calls and profile management
 */

#ifndef DATAMENU_HPP
#define DATAMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataProfile.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/tel/PhoneManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"

#include "DataListener.hpp"
#include "DataResponseCallback.hpp"

class DataMenu : public ConsoleApp {
public:
   bool initializeSDK();

   // initialize menu and sdk
   void init();
   void startDataCall(std::vector<std::string> inputCommand);
   void stopDataCall(std::vector<std::string> inputCommand);
   void requestDataCallStatistics(std::vector<std::string> inputCommand);
   void resetDataCallStatistics(std::vector<std::string> inputCommand);
   // Profile Management APIs
   void requestProfileList(std::vector<std::string> inputCommand);
   void createProfile(std::vector<std::string> inputCommand);
   void deleteProfile(std::vector<std::string> inputCommand);
   void modifyProfile(std::vector<std::string> inputCommand);
   void queryProfile(std::vector<std::string> inputCommand);
   void requestProfileById(std::vector<std::string> inputCommand);

   DataMenu(std::string appName, std::string cursor);
   ~DataMenu();

private:
   std::shared_ptr<telux::tel::IPhoneManager> phoneManager_;
   std::shared_ptr<telux::data::IDataConnectionManager> dataConnectionManager_;
   std::shared_ptr<telux::data::IDataProfileManager> dataProfileManager_;
   telux::data::ProfileParams params_;

   std::shared_ptr<MyDataProfilesCallback> myDataProfileListCb_;
   std::shared_ptr<MyDataProfilesCallback> myDataProfileListCbForQuery_;
   std::shared_ptr<MyDataProfileCallback> myDataProfileCb_;
   std::shared_ptr<MyDataCreateProfileCallback> myDataCreateProfileCb_;
   std::shared_ptr<MyDataProfileCallback> myDataProfileCbForGetProfileById_;
   std::shared_ptr<MyDeleteProfileCallback> myDeleteProfileCb_;
   std::shared_ptr<MyModifyProfileCallback> myModifyProfileCb_;

   std::shared_ptr<DataListener> dataListener_;

   void getProfileParamsFromUser();
};
#endif
