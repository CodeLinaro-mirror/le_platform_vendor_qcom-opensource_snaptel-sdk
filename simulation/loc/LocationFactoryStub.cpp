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

#include <telux/loc/LocationFactory.hpp>
#include "LocationManagerStub.hpp"
#include "LocationConfiguratorStub.hpp"
#include "DgnssManagerStub.hpp"
#include "Logger/Logger.hpp"

using namespace telux::common;

namespace telux {

namespace loc {

LocationFactory::LocationFactory() {
    Debug(__FILE__,__func__);
}

LocationFactory::~LocationFactory() {}

LocationFactory & LocationFactory::getInstance() {
    Debug(__FILE__,__func__);
    static LocationFactory myFactory;
    return(myFactory);
}

std::shared_ptr<ILocationManager> LocationFactory::getLocationManager(telux::common::InitResponseCb
        callback) {
    Debug(__FILE__,__func__);
    auto locationManager = std::make_shared<LocationManagerStub>(callback);
    return (locationManager);
}

std::shared_ptr<ILocationConfigurator> LocationFactory::getLocationConfigurator(telux::common::InitResponseCb
        callback) {
    Debug(__FILE__,__func__);
    static bool created = false;
    if (created == false) {
        created = true;
        locConfigurator_ = std::make_shared<LocationConfiguratorStub>(callback);
    } else {
        std::future<bool> f = locConfigurator_->onSubsystemReady();
        auto status = f.get();
        if (status) {
            std::thread appCallback(callback, ServiceStatus::SERVICE_AVAILABLE);
            appCallback.detach();
        } else {
            std::thread appCallback(callback, ServiceStatus::SERVICE_FAILED);
            appCallback.detach();
        }
    }
    return (locConfigurator_);
}

std::shared_ptr<IDgnssManager> LocationFactory::getDgnssManager (DgnssDataFormat dataFormat,
    telux::common::InitResponseCb callback) {
    Debug(__FILE__,__func__);
    static bool created_dgnss = false;
    if (created_dgnss == false) {
        created_dgnss = true;
        dgnssManager_ = std::make_shared<DgnssManagerStub>(callback);
    } else {
        std::future<bool> f = dgnssManager_->onSubsystemReady();
        auto status = f.get();
        if (status) {
            std::thread appCallback(callback, ServiceStatus::SERVICE_AVAILABLE);
            appCallback.detach();
        } else {
            std::thread appCallback(callback, ServiceStatus::SERVICE_FAILED);
            appCallback.detach();
        }
    }
    return (dgnssManager_);
}


} //namespace loc

} //namespace telux
