/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
#include <iomanip>

#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataProfile.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/tel/PhoneManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"

#include "DataListener.hpp"
#include "DataResponseCallback.hpp"
#include "MyProfileListener.hpp"
#include "bridge/BridgeMenu.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataFilterManager.hpp>
#include <telux/data/DataFilterListener.hpp>
#include <telux/data/DataFilterListener.hpp>

#include "MyDataFilterListener.hpp"
#include "ConfigParser.hpp"

using namespace telux::data;
using namespace telux::common;
using namespace telux::data::net;

class DataMenu : public IDataFilterListener, public ConsoleApp {
 public:
    bool initializeSDK();

    // initialize menu and sdk
    void init();
    void startDataCall(std::vector<std::string> inputCommand);
    void stopDataCall(std::vector<std::string> inputCommand);
    void requestDataCallStatistics(std::vector<std::string> inputCommand);
    void resetDataCallStatistics(std::vector<std::string> inputCommand);
    void requestDataCallList();
    void setDefaultProfile();

    // Data Filter APIs
    void sendSetDataRestrictMode(DataRestrictMode mode);
    void getFilterMode();
    void addFilter();
    void removeAllFilter();

    IpProtocol getTypeOfFilter(ConfigParser instance, std::map<std::string, std::string> filter);
    void addIPParameters(std::shared_ptr<telux::data::IIpFilter> &dataFilter, ConfigParser instance,
        std::map<std::string, std::string> filterMap);
    ResponseCallback responseCb;
    void commandCallback(ErrorCode errorCode);

    // Profile Management APIs
    void requestProfileList(std::vector<std::string> inputCommand);
    void createProfile(std::vector<std::string> inputCommand);
    void deleteProfile(std::vector<std::string> inputCommand);
    void modifyProfile(std::vector<std::string> inputCommand);
    void queryProfile(std::vector<std::string> inputCommand);
    void requestProfileById(std::vector<std::string> inputCommand);

    DataMenu(std::string appName, std::string cursor);
    ~DataMenu();

    void addStaticNatEntry(std::vector<std::string> inputCommand);
    void removeStaticNatEntry(std::vector<std::string> inputCommand);
    void requestStaticNatEntries(std::vector<std::string> inputCommand);
    void setFirewall(std::vector<std::string> inputCommand);
    void requestFirewallStatus(std::vector<std::string> inputCommand);
    void addFirewallEntry(std::vector<std::string> inputCommand);
    void requestFirewallEntry(std::vector<std::string> inputCommand);
    void removeFirewallEntry(std::vector<std::string> inputCommand);
    void enableDmz(std::vector<std::string> inputCommand);
    void disableDmz(std::vector<std::string> inputCommand);
    void requestDmzEntry(std::vector<std::string> inputCommand);

    void createVlan(std::vector<std::string> inputCommand);
    void removeVlan(std::vector<std::string> inputCommand);
    void queryVlanInfo(std::vector<std::string> inputCommand);
    void bindWithProfile(std::vector<std::string> inputCommand);
    void unbindFromProfile(std::vector<std::string> inputCommand);
    void queryVlanMappingList(std::vector<std::string> inputCommand);

    void enableSocks(std::vector<std::string> inputCommand);
    void bridgeMenu(std::vector<std::string> inputCommand);
 private:
    void requestDataCallList(OperationType operationType, DataCallListResponseCb cb);

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
    std::shared_ptr<MyProfileListener> profileListener_;

    std::shared_ptr<DataListener> dataListener_;

    std::shared_ptr<telux::data::IDataFilterManager> dataFilterMgr_;
    std::shared_ptr<MyDataFilterListener> dataFilterListener_;

    std::map<std::string, telux::data::IpProtocol> protoMap_;
    std::vector<std::shared_ptr<IFirewallEntry>> fwEntries_;

    telux::data::IpProtocol getProtcol(std::string protoStr);
    void parseProtoInfo(std::shared_ptr<IIpFilter> filter, telux::data::IpProtocol protocol,
        int &srcPort, int &destPort, int &srcPortRange, int &dstPortRange, std::string &protoStr);
    void displayFirewallEntry();
    void getProfileParamsFromUser();
};
#endif
