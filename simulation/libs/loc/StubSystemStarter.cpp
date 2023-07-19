/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include "StubSystemStarter.hpp"
#include "StubHelper.hpp"

using namespace telux::common;

static std::atomic<bool> locManagerReadyBool(false);
static std::atomic<bool> configuratorSystemReadyBool(false);
static std::atomic<bool> dgnssSystemReadyBool(false);
static std::atomic<ServiceStatus> locManagerReadyStatus(ServiceStatus::SERVICE_FAILED);
static std::atomic<ServiceStatus> configuratorSystemReadyStatus(ServiceStatus::SERVICE_FAILED);
static std::atomic<ServiceStatus> dgnssSystemReadyStatus(ServiceStatus::SERVICE_FAILED);
static std::atomic<bool> locManagerReadyBoolUpdated(false);
static std::atomic<bool> configuratorSystemReadyBoolUpdated(false);
static std::atomic<bool> dgnssSystemReadyBoolUpdated(false);

void mapBooleanToServiceStatus(bool sysStatus, ServiceStatus &status) {
    if (sysStatus) {
        status = ServiceStatus::SERVICE_AVAILABLE;
    } else {
        status = ServiceStatus::SERVICE_FAILED;
    } // SERVICE_UNAVAILABLE is ignored as there is no SSR handling in SDK.
}

static void systemThread(int delay, bool sysStatus, SubSystemType sysType, InitResponseCb
        callback) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
    ServiceStatus servStatus = ServiceStatus::SERVICE_FAILED;
    mapBooleanToServiceStatus(sysStatus, servStatus);
    switch (sysType) {
        case SubSystemType::LOCATION_MANAGER :
            locManagerReadyBool.store(sysStatus);
            locManagerReadyStatus.store(servStatus);
            locManagerReadyBoolUpdated.store(true);
            break;
        case SubSystemType::LOCATION_CONFIGURATOR :
            configuratorSystemReadyBool.store(sysStatus);
            configuratorSystemReadyStatus.store(servStatus);
            configuratorSystemReadyBoolUpdated.store(true);
            break;
        case SubSystemType::DGNSS_MANAGER :
            dgnssSystemReadyBool.store(sysStatus);
            dgnssSystemReadyStatus.store(servStatus);
            dgnssSystemReadyBoolUpdated.store(true);
            break;
        default:
            break;
    }
    if (callback) {
        callback(servStatus);
    }
}

bool StubSystemStarter::isStatusUpdated(SubSystemType sysType) {
    bool retVal=false;
    switch (sysType) {
        case SubSystemType::LOCATION_MANAGER :
            retVal = locManagerReadyBoolUpdated.load();
            break;
        case SubSystemType::LOCATION_CONFIGURATOR :
            retVal = configuratorSystemReadyBoolUpdated.load();
            break;
        case SubSystemType::DGNSS_MANAGER :
            retVal = dgnssSystemReadyBoolUpdated.load();
            break;
        default:
            break;
    }
    return(retVal);
}

bool StubSystemStarter::getStatus(SubSystemType sysType) {
    bool retVal=false;
    switch (sysType) {
        case SubSystemType::LOCATION_MANAGER :
            retVal = locManagerReadyBool.load();
            break;
        case SubSystemType::LOCATION_CONFIGURATOR :
            retVal = configuratorSystemReadyBool.load();
            break;
        case SubSystemType::DGNSS_MANAGER :
            retVal = dgnssSystemReadyBool.load();
            break;
        default:
            break;
    }
    return(retVal);
}

ServiceStatus StubSystemStarter::getServiceStatus(SubSystemType sysType) {
    ServiceStatus retVal = ServiceStatus::SERVICE_FAILED;
    switch (sysType) {
        case SubSystemType::LOCATION_MANAGER :
            retVal = locManagerReadyStatus.load();
            break;
        case SubSystemType::LOCATION_CONFIGURATOR :
            retVal = configuratorSystemReadyStatus.load();
            break;
        case SubSystemType::DGNSS_MANAGER :
            retVal = dgnssSystemReadyStatus.load();
            break;
        default:
            break;
    }
    return(retVal);
}

StubSystemStarter::StubSystemStarter(SubSystemType sysType, telux::common::InitResponseCb
        callback) {
    std::string delayStr;
    std::string statusStr;
    int delayMin, delayMax, delayDefault;
    switch (sysType) {
        case SubSystemType::LOCATION_MANAGER :
            delayStr = "LOCATION_MANAGER_SYSTEM_READY_DELAY";
            statusStr = "LOCATION_MANAGER_SYSTEM_READY_STATUS";
            delayMin = LOC_MANAGER_START_MIN_VALUE;
            delayMax = LOC_MANAGER_START_MAX_VALUE;
            delayDefault = LOC_MANAGER_START_DEFAULT_VALUE;
            break;
        case SubSystemType::LOCATION_CONFIGURATOR :
            delayStr = "LOCATION_CONFIGURATOR_SYSTEM_READY_DELAY";
            statusStr = "LOCATION_CONFIGURATOR_SYSTEM_READY_STATUS";
            delayMin = LOC_CONFIGURATOR_START_MIN_VALUE;
            delayMax = LOC_CONFIGURATOR_START_MAX_VALUE;
            delayDefault = LOC_CONFIGURATOR_START_DEFAULT_VALUE;
            break;
        case SubSystemType::DGNSS_MANAGER :
            delayStr = "LOCATION_DGNSS_SYSTEM_READY_DELAY";
            statusStr = "LOCATION_DGNSS_SYSTEM_READY_STATUS";
            delayMin = DGNSS_START_MIN_VALUE;
            delayMax = DGNSS_START_MAX_VALUE;
            delayDefault = DGNSS_START_DEFAULT_VALUE;
            break;
        default:
            break;
    }
    auto &s_stubbed =  StubHelper::getInstance();
    std::string strDelay = s_stubbed.configValue(delayStr);
    std::string status = s_stubbed.configValue(statusStr);
    int delay = (strDelay.size())? (std::stoi(strDelay)) : delayDefault;
    if (delay < delayMin) {
        delay = delayMin;
    } else if (delay > delayMax) {
        delay = delayMax;
    }

    bool sysStatus = true;
    if (status.compare(0, 4, "FAIL", 4) == 0) {
        sysStatus = false;
    }
    std::thread t(std::move(systemThread), delay, sysStatus, sysType, callback);
    t.detach();
}


std::future<bool> StubSystemStarter::onSubSystemReady(SubSystemType sysType) {
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
        [](SubSystemType sysType) {
            while (!StubSystemStarter::isStatusUpdated(sysType)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return(StubSystemStarter::getStatus(sysType));},
            sysType);
    return((ready_future));
}

bool StubSystemStarter::isSubSystemReady(SubSystemType sysType) {
    return(StubSystemStarter::getStatus(sysType));
}

StubSystemStarter::~StubSystemStarter() {}
