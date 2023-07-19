/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "DataSubSystemStub.hpp"

DataSubSystemStub::DataSubSystemStub() {
    isInitialized_ = false;
    settings_ = std::make_shared<ConfigParser>(
                DEFAULT_DATA_STUB_CONFIG_FILE_NAME, DEFAULT_DATA_STUB_CONFIG_FILE_PATH);
    if(settings_) {
        //Read subsystem manager's readiness status
        std::string readyStatus = settings_->getValue("DATA_MANAGERS_READY_STATUS");
        //If subsystem readiness parameter exits and set to Fail
        if((!readyStatus.empty()) && (readyStatus.compare("FAIL") == 0)) {
            status_ = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
        } else {
            //else assume data ready status is pass
            status_ = telux::common::ServiceStatus::SERVICE_AVAILABLE;
        }
        //Read subsytem manager's callback delay
        std::string initCbDelay = settings_->getValue("DATA_MANAGERS_INIT_CB_DELAY_MS");
        //If subsystem init callback parameter exist, set to configured parameter
        if(!initCbDelay.empty()) {
            initCbDelay_ = atol(initCbDelay.c_str());
        } else {
            //Set to default value 2000 msec
            initCbDelay_ = 2000;
        }
    } else {
        LOG(ERROR, __FUNCTION__, "Data SubSystem Stub Conf file not found");
    }
}

DataSubSystemStub::~DataSubSystemStub() {
    if(settings_) {
        settings_ = nullptr;
    }
}

const std::shared_ptr<DataSubSystemStub> DataSubSystemStub::getInstance() {
    static std::shared_ptr<DataSubSystemStub> instancePtr(new DataSubSystemStub);
    return instancePtr;
}

telux::common::ServiceStatus DataSubSystemStub::init() {
    //If subsystem already initialized, return status
    if(!isInitialized_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(initCbDelay_));
        isInitialized_ = true;
    }
    return status_;
}

telux::common::ServiceStatus DataSubSystemStub::getServiceStatus() {
    if(!isInitialized_) {
        //If called before initialization is completed, it should return status unavailable
        return telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
    } else {
        //else it should return configured status
        return status_;
    }
}

std::string DataSubSystemStub::getSettingValue(std::string key) {
    return settings_->getValue(key);
}
