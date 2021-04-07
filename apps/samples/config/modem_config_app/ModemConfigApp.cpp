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
#include <future>

#include <telux/config/ConfigFactory.hpp>
#include <telux/config/ModemConfigManager.hpp>

std::promise<telux::common::ErrorCode> gCallbackPromise;

// Resets the global callback promise variable
static inline void resetCallbackPromise(void) {
    gCallbackPromise = std::promise<telux::common::ErrorCode>();
}

static void configListCb(std::vector<telux::config::ConfigInfo> configList,
                                    telux::common::ErrorCode errCode) {
    if (errCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Total Configs in Modem " << configList.size() << std::endl;
        int count = 0;
        for (auto &config : configList) {
            std::cout << "Config No  : " << count << std::endl;
            std::cout << "Id         : " << config.id << std::endl;
            std::string type;
            if (config.type == telux::config::ConfigType::HARDWARE) {
                type = "HARDWARE";
            } else if(config.type == telux::config::ConfigType::SOFTWARE) {
                type = "SOFTWARE";
            }
        std::cout << "Type        : " << type << std::endl;
        std::cout << "Size        : " << static_cast<uint32_t>(config.size) << std::endl;
        std::cout << "Version     : " << static_cast<uint32_t>(config.version) << std::endl;
        std::cout << "Description : " << (config.desc) << std::endl << std::endl;
        count++;
        }
    }
    gCallbackPromise.set_value(errCode);
}

static void getActiveConfigCb(telux::config::ConfigInfo configInfo,
                                                    telux::common::ErrorCode errCode) {
    if (errCode == telux::common::ErrorCode::SUCCESS) {
        std::cout << "Active Config Details" << std::endl;
        std::cout << "Id         : " << configInfo.id << std::endl;
        std::string type;
        if (configInfo.type == telux::config::ConfigType::HARDWARE) {
            type = "HARDWARE";
        } else if(configInfo.type == telux::config::ConfigType::SOFTWARE) {
            type = "SOFTWARE";
        }
        std::cout << "Type        :" << type << std::endl;
        std::cout << "Size        :" << static_cast<uint32_t>(configInfo.size) << std::endl;
        std::cout << "Version     :" << static_cast<uint32_t>(configInfo.version) << std::endl;
        std::cout << "Description :" << configInfo.desc << std::endl;
    }
    gCallbackPromise.set_value(errCode);
}

static void getAutoSelectionModeCb(telux::config::AutoSelectionMode selectionMode,
                                                            telux::common::ErrorCode errCode) {
    if (errCode == telux::common::ErrorCode::SUCCESS) {
        if (selectionMode == telux::config::AutoSelectionMode::DISABLED) {
            std::cout << "Auto selection is disabled" << std::endl;
        } else {
            std::cout << "Auto selection is enabled" << std::endl;
        }
    }
    gCallbackPromise.set_value(errCode);
}

int main(int argc, char **argv) {

    // ### 1. Get the ConfigFactory and ModemConfigManager instances.
    auto &configFactory = telux::config::ConfigFactory::getInstance();
    auto modemConfigManager_ = configFactory.getModemConfigManager();

    // ### 2. Requesting to get modem config subsystem state
    bool subSystemStatus = modemConfigManager_->isSubsystemReady();

    // #### 2.1  If modem config subsystem is not ready, wait for it to be ready
    if (!subSystemStatus) {
        std::cout << "Modem Config subsystem is not ready, Please wait" << std::endl;
        std::future<bool> f = modemConfigManager_->onSubsystemReady();
        // Wait unconditionally for modem config subsystem to be ready
        subSystemStatus = f.get();
    }

    // Exit the application, if SDK is unable to initialize modem config subsystems
    if (!subSystemStatus) {
        std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
        return EXIT_FAILURE;
    }

    // Setting up default Parameters
    telux::config::ConfigType configType_ = telux::config::ConfigType::SOFTWARE;
    int slotId_ = 1;

    // ### 3. Requesting all configs present in the modem's storage
    auto status = modemConfigManager_->requestConfigList(configListCb);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "Get Config List Request sent" << std::endl;
        if (telux::common::ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            std::cout << "Error : failed to get config list." << std::endl;
        }
    } else {
        std::cout << "Get Config List Request failed" << std::endl;
    }

    resetCallbackPromise();

    // ### 4. Requesting selection mode of configs
    status = modemConfigManager_->getAutoSelectionMode(getAutoSelectionModeCb, slotId_);
     if (status == telux::common::Status::SUCCESS) {
        std::cout << "Get selection mode Request sent" << std::endl;
        if (telux::common::ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            std::cout << "Error : failed to get selection mode" << std::endl;
        }
    } else {
        std::cout << "Get selection mode Request failed" << std::endl;
    }

    resetCallbackPromise();

    // ### 5. Requesting active config info, will error out if only default configs active
    status = modemConfigManager_->getActiveConfig(configType_,getActiveConfigCb, slotId_);
    if (status == telux::common::Status::SUCCESS) {
        std::cout << "get active config request sent" << std::endl;
        if (telux::common::ErrorCode::SUCCESS != gCallbackPromise.get_future().get()) {
            std::cout << "Error : failed to get active config info" << std::endl;
        }
    } else {
        std::cout << "get active config request failed" << std::endl;
    }

    return EXIT_SUCCESS;
}