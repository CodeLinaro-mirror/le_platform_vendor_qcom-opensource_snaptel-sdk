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

#include "DgnssManagerStub.hpp"
#include "Logger/Logger.hpp"

namespace telux {

namespace loc {

DgnssManagerStub::DgnssManagerStub(telux::common::InitResponseCb callback) {
    Debug(__FILE__,__func__);
    static bool created = false;
    if (created == false) {
        created = true;
        systemStarter_ = std::make_shared<StubSystemStarter>(SubSystemType::DGNSS_MANAGER,
            callback);
    }
}

std::future<bool> DgnssManagerStub::onSubsystemReady() {
    Debug(__FILE__,__func__);
    return(systemStarter_->onSubSystemReady(SubSystemType::DGNSS_MANAGER));
}

bool DgnssManagerStub::isSubsystemReady() {
    Debug(__FILE__,__func__);
    return(systemStarter_->isSubSystemReady(SubSystemType::DGNSS_MANAGER));
}

telux::common::ServiceStatus DgnssManagerStub::getServiceStatus() {
    Debug(__FILE__,__func__);
    return(systemStarter_->getServiceStatus(SubSystemType::DGNSS_MANAGER));
}

telux::common::Status DgnssManagerStub::registerListener(
    std::weak_ptr<IDgnssStatusListener> listener) {
    Debug(__FILE__,__func__);
    if (statusListener_.lock()) {
        Error(__func__, "Listener Already Registered");
        return telux::common::Status::INVALIDSTATE;
    }
    if (listener.lock()!=nullptr) {
        Info(__func__, "Listener Registered");
        statusListener_ = listener;
        return telux::common::Status::SUCCESS;
    } else {
        Error(__func__, "Listener Paramater Invalid");
        return telux::common::Status::INVALIDPARAM;
    }
}

telux::common::Status DgnssManagerStub::deRegisterListener(void) {
    Debug(__FILE__,__func__);
    if (statusListener_.lock()) {
        statusListener_.reset();
        Info(__func__, "Listener Deregistered");
        return telux::common::Status::SUCCESS;
    } else {
        Error(__func__, "No Listener Registered");
        return telux::common::Status::NOSUBSCRIPTION;
    }
}

telux::common::Status DgnssManagerStub::createSource(DgnssDataFormat dataFormat) {
    Debug(__FILE__,__func__);
    if (dataSource_ == nullptr) {
        dataSource_ = std::make_shared<std::string> ("Data Source");
        dataFormat_ = dataFormat;
        Info(__func__, "Source Created");
        return telux::common::Status::SUCCESS;
    } else {
        //user must call releaseSource() before calling this function
        Error(__func__, "INVALID STATE", "call releaseSource() before calling this function");
        return telux::common::Status::INVALIDSTATE;
    }
}

telux::common::Status DgnssManagerStub::releaseSource(void) {
    Debug(__FILE__,__func__);
    if (dataSource_ != nullptr) {
        dataFormat_ = DgnssDataFormat::DATA_FORMAT_UNKNOWN;
        dataSource_.reset();
        Info(__func__, "Resource Released");
        return telux::common::Status::SUCCESS;
    } else {
        Error(__func__, "INVALID STATE");
        return telux::common::Status::INVALIDSTATE;
    }
}

telux::common::Status DgnssManagerStub::injectCorrectionData(const uint8_t* buffer,
        uint32_t bufferSize) {
    Debug(__FILE__,__func__);
    if( dataSource_ != nullptr) {
        //inject data
        Info(__func__, "Injecting Data");
        return telux::common::Status::SUCCESS;
    } else {
        Error(__func__, "INVALID STATE");
        return telux::common::Status::INVALIDSTATE;
    }
}

DgnssManagerStub::~DgnssManagerStub() {}

}

}
