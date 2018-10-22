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

#include <telux/data/DataFactory.hpp>
#include <Utils.hpp>

#include "DataMenu.hpp"
#include "DataResponseCallback.hpp"

DataMenu::DataMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DataMenu::~DataMenu() {
   if(dataConnectionManager_) {
      dataConnectionManager_->deregisterListener(dataListener_);
   }
}

bool DataMenu::initializeSDK() {
   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();
   //  Get the DataFactory instances.
   auto &dataFactory = telux::data::DataFactory::getInstance();

   dataConnectionManager_ = telux::data::DataFactory::getInstance().getDataConnectionManager();

   //  Check if telephony subsystem is ready
   bool subSystemStatus = dataConnectionManager_->isSubsystemReady();

   //  If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "\n\nData subsystem is not ready, Please wait!!!..." << std::endl;
      std::future<bool> f = dataConnectionManager_->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemStatus = f.get();
   }

   //  Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemStatus) {
      endTime = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      exit(0);
   }

   if(subSystemStatus) {
      dataListener_ = std::make_shared<DataListener>();
      dataConnectionManager_->registerListener(dataListener_);
   }

   dataProfileManager_ = dataFactory.getDataProfileManager();
   myDataProfileListCb_ = std::make_shared<MyDataProfilesCallback>();
   myDataProfileListCbForQuery_ = std::make_shared<MyDataProfilesCallback>();
   myDataCreateProfileCb_ = std::make_shared<MyDataCreateProfileCallback>();
   myDataProfileCb_ = std::make_shared<MyDataProfileCallback>();
   myDeleteProfileCb_ = std::make_shared<MyDeleteProfileCallback>();
   myModifyProfileCb_ = std::make_shared<MyModifyProfileCallback>();
   myDataProfileCbForGetProfileById_ = std::make_shared<MyDataProfileCallback>();

   return true;
}

void DataMenu::init() {

   std::shared_ptr<ConsoleAppCommand> startDataCall = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", "start_data_call", {},
                        std::bind(&DataMenu::startDataCall, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> stopDataCall = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("2", "stop_data_call", {},
                        std::bind(&DataMenu::stopDataCall, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> reqDataCallStats
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "request_datacall_statistics\n", {},
         std::bind(&DataMenu::requestDataCallStatistics, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> reqProfile = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("100", "request_profile_list", {},
                        std::bind(&DataMenu::requestProfileList, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> createProfileMenu = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("101", "create_profile", {},
                        std::bind(&DataMenu::createProfile, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> deleteProfileMenu = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("102", "delete_profile", {"profileId", "techPref (0-3GPP, 1-3GPP2)"},
                        std::bind(&DataMenu::deleteProfile, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> modifyProfileMenu = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("103", "modify_profile", {},
                        std::bind(&DataMenu::modifyProfile, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> queryProfileMenu = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("104", "query_profile", {},
                        std::bind(&DataMenu::queryProfile, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> requestProfileByIdMenu = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("105", "request_profile_by_id", {"profileId", "techPref (0-3GPP, 1-3GPP2)"},
                        std::bind(&DataMenu::requestProfileById, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
      = {startDataCall,     stopDataCall,      reqDataCallStats,
         reqProfile,        createProfileMenu, deleteProfileMenu,
         modifyProfileMenu, queryProfileMenu,  requestProfileByIdMenu};

   addCommands(commandsList);

   if(DataMenu::initializeSDK()) {
      ConsoleApp::displayMenu();
   }
}

void DataMenu::startDataCall(std::vector<std::string> inputCommand) {
   std::cout << "\nStart data call" << std::endl;
   int profileId;
   std::cout << "Enter Profile Id : ";
   std::cin >> profileId;
   Utils<int>::validateInput(profileId);

   int ipFamilyType;
   std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
   std::cin >> ipFamilyType;
   Utils<int>::validateInput(ipFamilyType);

   telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
   dataConnectionManager_->startDataCall(profileId, ipFamType,
                                         MyDataCallResponseCallback::startDataCallResponseCallBack);
}

void DataMenu::stopDataCall(std::vector<std::string> inputCommand) {
   std::cout << "\nStop data call" << std::endl;
   int profileId;
   std::cout << "Enter Profile Id : ";
   std::cin >> profileId;
   Utils<int>::validateInput(profileId);

   int ipFamilyType;
   std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
   std::cin >> ipFamilyType;
   Utils<int>::validateInput(ipFamilyType);

   telux::data::IpFamilyType ipFamType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
   dataConnectionManager_->stopDataCall(profileId, ipFamType,
                                        MyDataCallResponseCallback::stopDataCallResponseCallBack);
}

void DataMenu::requestDataCallStatistics(std::vector<std::string> inputCommand) {
   std::cout << "\nRequest DataCall Statistics" << std::endl;

   int profileId;
   std::cout << "Enter Profile Id : ";
   std::cin >> profileId;
   Utils<int>::validateInput(profileId);

   dataConnectionManager_->requestDataCallStatistics(
      profileId, DataCallStatisticsResponseCb::requestStatisticsResponse);
}

void DataMenu::getProfileParamsFromUser() {
   char delimiter = '\n';
   int techPref;
   std::cout << "Enter Tech Preference (0-3GPP, 1-3GPP2): ";
   std::cin >> techPref;
   Utils<int>::validateInput(techPref);

   std::cin.get();
   std::string profileName;
   std::cout << "Enter profileName : ";
   std::getline(std::cin, profileName, delimiter);

   std::string apnName;
   std::cout << "Enter APN : ";
   std::getline(std::cin, apnName, delimiter);

   std::string username;
   std::cout << "Enter userName : ";
   std::getline(std::cin, username, delimiter);

   std::string password;
   std::cout << "Enter password : ";
   std::getline(std::cin, password, delimiter);

   int authType;
   std::cout << "Enter Authentication Protocol Type : \n0-None \n1-PAP \n2-CHAP"
                "\n3-PAP_CHAP\n";
   std::cin >> authType;
   Utils<int>::validateInput(authType);

   int ipFamilyType;
   std::cout << "Enter Ip Family (4-IPv4, 6-IPv6, 10-IPv4V6): ";
   std::cin >> ipFamilyType;
   Utils<int>::validateInput(ipFamilyType);

   params_.profileName = profileName;
   params_.techPref = static_cast<telux::data::TechPreference>(techPref);
   params_.authType = static_cast<telux::data::AuthProtocolType>(authType);
   params_.ipFamilyType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
   params_.apn = apnName;
   params_.userName = username;
   params_.password = password;
}

void DataMenu::requestProfileList(std::vector<std::string> inputCommand) {
   telux::common::Status status = dataProfileManager_->requestProfileList(myDataProfileListCb_);

   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Request profile list sent successfully" << std::endl;
   } else {
      std::cout << "Request profile list failed, status:" << int(status) << std::endl;
   }
}

void DataMenu::createProfile(std::vector<std::string> inputCommand) {
   getProfileParamsFromUser();

   telux::common::Status status
      = dataProfileManager_->createProfile(params_, myDataCreateProfileCb_);

   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Create profile request sent successfully" << std::endl;
   } else {
      std::cout << "Failed to send create profile request, Status:" << int(status) << std::endl;
   }
}

void DataMenu::deleteProfile(std::vector<std::string> inputCommand) {
   int profileId, techPrefId;
   try {
      profileId = std::stoi(inputCommand[1]);
      techPrefId = std::stoi(inputCommand[2]);
   } catch(const std::exception &e) {
      std::cout << "ERROR: Invalid input, please enter numerical values " << std::endl;
      return;
   }
   telux::data::TechPreference tp = telux::data::TechPreference::UNKNOWN;
   if(techPrefId == 1) {
      tp = telux::data::TechPreference::TP_3GPP;
   } else if(techPrefId == 2) {
      tp = telux::data::TechPreference::TP_3GPP2;
   }
   telux::common::Status status
      = dataProfileManager_->deleteProfile(profileId, tp, myDeleteProfileCb_);
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Delete profile request sent successfully" << std::endl;
   } else {
      std::cout << "Failed to send delete profile request, Status:" << int(status) << std::endl;
   }
}

void DataMenu::modifyProfile(std::vector<std::string> inputCommand) {
   int profileId;
   std::cout << "Enter profile Id to Modify : ";
   std::cin >> profileId;
   Utils<int>::validateInput(profileId);

   getProfileParamsFromUser();

   telux::common::Status status
      = dataProfileManager_->modifyProfile(profileId, params_, myModifyProfileCb_);
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Modify profile request sent successfully" << std::endl;
   } else {
      std::cout << "Failed to send Modify profile request, Status:" << int(status) << std::endl;
   }
}

void DataMenu::queryProfile(std::vector<std::string> inputCommand) {
   char delimiter = '\n';
   int techPref;
   std::cout << "Enter Tech Preference (0-3GPP, 1-3GPP2): ";
   std::cin >> techPref;
   Utils<int>::validateInput(techPref);

   std::cin.get();
   std::string profileName;
   std::cout << "Enter profileName: ";
   std::getline(std::cin, profileName, delimiter);

   std::string apnName;
   std::cout << "Enter APN: ";
   std::getline(std::cin, apnName, delimiter);

   std::string username;
   std::cout << "Enter username: ";
   std::getline(std::cin, username, delimiter);

   std::string password;
   std::cout << "Enter password: ";
   std::getline(std::cin, password, delimiter);

   int authType;
   std::cout << "Enter Authentication Protocol Type : \n0-None \n1-PAP"
                "\n2-CHAP \n3-PAP_CHAP\n";
   std::cin >> authType;
   Utils<int>::validateInput(authType);

   int ipFamilyType;
   std::cout << "Enter Ip Family (4-IPv4, 6-IPV6, 10-IPV4V6): ";
   std::cin >> ipFamilyType;
   Utils<int>::validateInput(ipFamilyType);

   params_.profileName = profileName;
   params_.techPref = static_cast<telux::data::TechPreference>(techPref);
   params_.authType = static_cast<telux::data::AuthProtocolType>(authType);
   params_.ipFamilyType = static_cast<telux::data::IpFamilyType>(ipFamilyType);
   params_.apn = apnName;
   params_.userName = username;
   params_.password = password;

   telux::common::Status status
      = dataProfileManager_->queryProfile(params_, myDataProfileListCbForQuery_);
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Query profile request sent successfully" << std::endl;
   } else {
      std::cout << "Failed to send Query profile request, Status:" << int(status) << std::endl;
   }
}

void DataMenu::requestProfileById(std::vector<std::string> inputCommand) {
   int profileId, techPrefId;
   try {
      profileId = std::stoi(inputCommand[1]);
      techPrefId = std::stoi(inputCommand[2]);
   } catch(const std::exception &e) {
      std::cout << "ERROR: Invalid input, please enter numerical values " << std::endl;
      return;
   }

   telux::data::TechPreference tp = telux::data::TechPreference::UNKNOWN;
   if(techPrefId == 1) {
      tp = telux::data::TechPreference::TP_3GPP;
   } else if(techPrefId == 2) {
      tp = telux::data::TechPreference::TP_3GPP2;
   }
   telux::common::Status status
      = dataProfileManager_->requestProfile(profileId, tp, myDataProfileCbForGetProfileById_);
   if(status == telux::common::Status::SUCCESS) {
      std::cout << "Request profile by ID request sent successfully" << std::endl;
   } else {
      std::cout << "Failed to send Request profile by ID request, Status:" << int(status)
                << std::endl;
   }
}
