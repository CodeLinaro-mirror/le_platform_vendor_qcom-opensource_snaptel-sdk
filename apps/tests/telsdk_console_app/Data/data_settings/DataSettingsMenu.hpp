/*
 *  Copyright (c) 2021,2023 Qualcomm Innovation Center, Inc. All rights reserved.
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


/**
 * This is Data Settings Manager Sample Application using Telematics SDK.
 * It is used to demonstrate APIs to interface with Settings applicable to Data Subsystem
 */

#ifndef DATASETTINGSMENU_HPP
#define DATASETTINGSMENU_HPP

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <map>


#include "console_app_framework/ConsoleApp.hpp"

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>

using namespace telux::data;
using namespace telux::common;

class DataSettingsMenu : public ConsoleApp ,
                         public IDataSettingsListener,
                         public std::enable_shared_from_this<DataSettingsMenu> {
 public:
    // initialize menu
    bool init();

    // Data Settings Manager APIs
    void setBackhaulPref(std::vector<std::string> inputCommand);
    void requestBackhaulPref(std::vector<std::string> inputCommand);
    void setBandInterferenceConfig(std::vector<std::string> inputCommand);
    void requestBandInterferenceConfig(std::vector<std::string> inputCommand);

    void requestDdsSwitch(std::vector<std::string> inputCommand);
    void requestCurrentDds(std::vector<std::string> inputCommand);
    void setWwanConnectivityConfig(std::vector<std::string> inputCommand);
    void requestWwanConnectivityConfig(std::vector<std::string> inputCommand);
    void onWwanConnectivityConfigChange(SlotId slotId, bool isConnectivityAllowed) override;
    //Initialization callback
    void onInitComplete(telux::common::ServiceStatus status);

    DataSettingsMenu(std::string appName, std::string cursor);
    ~DataSettingsMenu();
 private:
    bool menuOptionsAdded_;
    bool subSystemStatusUpdated_;
    std::map<telux::data::OperationType,
      std::shared_ptr<telux::data::IDataSettingsManager>> dataSettingsManagerMap_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool initDataSettingsManager(telux::data::OperationType opType);
};
#endif
