/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
#include <cstring>
#include <map>
#include <cstdlib>

#include "Cv2xTelux.hpp"
#include "Cv2xLog.hpp"

#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataFactory.hpp>

using telux::common::Status;
using telux::common::ErrorCode;
using telux::data::OperationType;

static std::map<ServiceStatus, std::string> convertServiceStatusToString = {
    {ServiceStatus::SERVICE_AVAILABLE, "Available"},
    {ServiceStatus::SERVICE_UNAVAILABLE, "Unavailable"},
};

Cv2xTelux::Cv2xTelux() {
    isPostSSRV2XDone_ = false;

    callInfo_[CV2X_DATA_CALL_IP].profileIndex = -1;
    callInfo_[CV2X_DATA_CALL_IP].callStatus = DataCallStatus::INVALID;
    callInfo_[CV2X_DATA_CALL_IP].apnName = APN_NAME_V2X_IP;

    callInfo_[CV2X_DATA_CALL_NON_IP].profileIndex = -1;
    callInfo_[CV2X_DATA_CALL_NON_IP].callStatus = DataCallStatus::INVALID;
    callInfo_[CV2X_DATA_CALL_NON_IP].apnName = APN_NAME_V2X_NON_IP;
}

void Cv2xTelux::onStatusChanged(Cv2xStatus status) {

    logStatusChanged(status);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isPostSSRV2XDone_ == false) {
            isPostSSRV2XDone_ = true;
            cv_.notify_one();
        }
    }

    bool startDataCalls = false;

    // Handle State Transition InActive to Active/Suspended
    if (((cv2xStatus_.txStatus ==  Cv2xStatusType::INACTIVE) &&
         (cv2xStatus_.rxStatus ==  Cv2xStatusType::INACTIVE)) &&
         ((status.txStatus !=  Cv2xStatusType::INACTIVE) &&
         (status.rxStatus !=  Cv2xStatusType::INACTIVE))) {
        LOGD("State Transition From Inactive to Active/Suspended\n");
        startDataCalls = true;
    }

    cv2xStatus_ = status;

    if (startDataCalls) {
        auto f = std::async(std::launch::async , [this]() {
            findProfilesAndStartDataCalls();
        });
    }
}

void Cv2xTelux::logStatusChanged(Cv2xStatus &status) {
    static uint8_t previousCbr = 255;

    if ((status.txStatus != Cv2xStatusType::UNKNOWN or
         status.rxStatus != Cv2xStatusType::UNKNOWN) and
        (cv2xStatus_.txStatus != status.txStatus or
         cv2xStatus_.rxStatus != status.rxStatus)) {

        if (status.txStatus == Cv2xStatusType::ACTIVE) {
            bootkpilog("cv2x-daemon: V2X TX status is active");
        } else if (status.txStatus == Cv2xStatusType::SUSPENDED) {
            bootkpilog("cv2x-daemon: V2X TX status is suspended");
        }
        if (status.rxStatus == Cv2xStatusType::ACTIVE) {
            bootkpilog("cv2x-daemon: V2X RX status is active");
        } else if (status.rxStatus == Cv2xStatusType::SUSPENDED) {
            bootkpilog("cv2x-daemon: V2X RX status is suspended");
        }

        LOGI("tx_status=%d, rx_status=%d, tx_cause=%d, rx_cause=%d\n",
            Cv2xUtils::convertStatus(status.txStatus),
            Cv2xUtils::convertStatus(status.rxStatus),
            Cv2xUtils::convertStatus(status.txCause),
            Cv2xUtils::convertStatus(status.rxCause));
    }

    if (status.cbrValueValid && previousCbr != status.cbrValue) {
        LOGD("cbr_value=%d\n", static_cast<int>(status.cbrValue));
        previousCbr = status.cbrValue;
    }
}

DataConnectionListener::DataConnectionListener(std::weak_ptr<Cv2xTelux> instance) {
    cv2xTelux_ = instance;
}

void DataConnectionListener::onDataCallInfoChanged(const std::shared_ptr<IDataCall> &dataCall) {
    if (!dataCall) {
        return;
    }

    auto iface = dataCall->getInterfaceName();
    auto status = Cv2xUtils::DataCallStatusToStr(dataCall->getDataCallStatus());
    auto reason = Cv2xUtils::DataCallEndReasonToInt(dataCall->getDataCallEndReason());
    auto ip_type = Cv2xUtils::IpFamilyTypeToStr(dataCall->getIpFamilyType());
    auto profile_id = dataCall->getProfileId();
    DataCallStatus callStatus = dataCall->getDataCallStatus();

    if (iface == "") {
        iface = "unknown";
    }

    if (status != "NET_NO_NET") {
        reason = 0;
    }

    LOGI("iface=%s, status=%s, reason=%d, ip_type=%s, profile_id=%d\n",
        iface.c_str(), status.c_str(), reason, ip_type.c_str(), profile_id);

    auto sp = cv2xTelux_.lock();
    if (sp) {
        if (sp->isIpDataCall(profile_id)) {
            if (callStatus == DataCallStatus::NET_CONNECTED) {
                bootkpilog("cv2x-daemon: V2X IP call is online");
            }
            sp->setIpCallStatus(callStatus);
        } else if (sp->isNonIpDataCall(profile_id)) {
            if (callStatus == DataCallStatus::NET_CONNECTED) {
                bootkpilog("cv2x-daemon: V2X Non-IP call is online");
            }
            sp->setNonipCallStatus(callStatus);
        } else {
            LOGE("unknown profile ID %d.\n", profile_id);
            return;
        }
    }
}

void DataConnectionListener::onServiceStatusChange(ServiceStatus status) {
    Status res = Status::FAILED;

    LOGI("DataConnectionListener Service Status changed to %s\n",
            convertServiceStatusToString[status].c_str() );
    if (status == ServiceStatus::SERVICE_AVAILABLE){

        auto sp = cv2xTelux_.lock();
        if(sp) {
            LOGD("Waiting for CV2xRadio to come back ONLINE\n");

            std::unique_lock<std::mutex> cvLock(sp->mutex_);
            while(sp->isPostSSRV2XDone_ == false) {
                sp->cv_.wait(cvLock);
            }
            LOGD("CV2xRadio back ONLINE\n");

            res = sp->findProfilesAndStartDataCalls();
            if (res != Status::SUCCESS) {
                LOGE("Failed to start data call\n");
                return;
            }
        }
    }
}

bool Cv2xTelux::isIpDataCall(uint8_t profileID) {
    return (profileID == callInfo_[CV2X_DATA_CALL_IP].profileIndex);
}

bool Cv2xTelux::isNonIpDataCall(uint8_t profileID) {
    return (profileID == callInfo_[CV2X_DATA_CALL_NON_IP].profileIndex);
}

void Cv2xTelux::onServiceStatusChange(ServiceStatus status) {
    Status res = Status::FAILED;

    LOGI("Cv2xTelux Service Status changed to %s\n",
         convertServiceStatusToString[status].c_str());

    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        isPostSSRV2XDone_ = false;
    } else if (status == ServiceStatus::SERVICE_AVAILABLE){
        res = startV2xRadio();
        if (res!= Status::SUCCESS) {
            LOGE("Failed to start v2x mode\n");
            return;
        }
    }
}

QueryProfileCallback::QueryProfileCallback(std::shared_ptr<std::promise<ProfileIds>> prom) {
    prom_ = prom;
}

void QueryProfileCallback::onProfileListResponse(
    const std::vector<std::shared_ptr<DataProfile>> &profiles, ErrorCode error) {
    ProfileIds profileIds = { -1, -1};

    if (error == ErrorCode::SUCCESS) {
        for (auto it : profiles) {
            if (0 == APN_NAME_V2X_IP.compare(it->getApn()) &&
                profileIds.ip == -1) {
                profileIds.ip = it->getId();
            } else if (0 == APN_NAME_V2X_NON_IP.compare(it->getApn()) &&
                     profileIds.nonIp == -1) {
                profileIds.nonIp = it->getId();
            }
            if (profileIds.ip != -1 && profileIds.nonIp != -1) {
                break;
            }
        }
    }
    prom_->set_value(profileIds);
}

Status Cv2xTelux::initV2xLibrary() {
    auto &cv2xFactory = Cv2xFactory::getInstance();
    cv2xRadioMgr_ = cv2xFactory.getCv2xRadioManager();

    /* Check that V2X radio is initialized */
    if (not cv2xRadioMgr_->isReady()) {
        if (not cv2xRadioMgr_->onReady().get()) {
            LOGE("V2X cv2xRadioMgr initialization failed\n");
            return Status::FAILED;
        }
    }
    return Status::SUCCESS;
}

Status Cv2xTelux::initDataLibrary() {

    auto &dataFactory = DataFactory::getInstance();
    dataConnectionMgr_ = dataFactory.getDataConnectionManager();
    if (not dataConnectionMgr_->isSubsystemReady()) {
        if (not dataConnectionMgr_->onSubsystemReady().get()) {
            LOGE("dataConnectionMgr initialization failed\n");
            return Status::FAILED;
        }
    }

    dataProfileMgr_ = dataFactory.getDataProfileManager();
    if (not dataProfileMgr_->isSubsystemReady()) {
        if (not dataProfileMgr_->onSubsystemReady().get()) {
            LOGE("dataProfileMgr initialization failed\n");
            return Status::FAILED;
        }
    }

    return Status::SUCCESS;
}

Status Cv2xTelux::deinitV2xLibrary() {
    if (dataConnectionMgr_ != nullptr) {
        dataConnectionMgr_->deregisterListener(dataConnectionListener_);
    } else {
        LOGE("Failed to Register DataConnection Listener\n");
        return Status::FAILED;
    }

    if (cv2xRadioMgr_ != nullptr) {
        cv2xRadioMgr_->deregisterListener(shared_from_this());
    } else {
        LOGE("cv2xRadioMgr Instance invalid\n");
        return Status::FAILED;
    }

    return Status::SUCCESS;
}

Status Cv2xTelux::getV2xRadioStatus(Cv2xStatus &status) {
    std::promise<Cv2xStatus> prom;

    auto res = cv2xRadioMgr_->requestCv2xStatus(
    [&prom](Cv2xStatus status, ErrorCode code) {
        prom.set_value(status);
    });

    if (res != Status::SUCCESS) {
        return res;
    }
    status = prom.get_future().get();

    return Status::SUCCESS;
}

Status Cv2xTelux::startV2xRadio() {

    LOGI("Starting V2X radio\n");

    std::promise<ErrorCode> prom;
    cv2xRadioMgr_->startCv2x([&prom](ErrorCode code) {
        if (code == ErrorCode::SUCCESS) {
            LOGI("Started V2X radio\n");
            bootkpilog("cv2x-daemon: V2X mode started");
        } else {
            LOGE("Failed to start the V2X radio\n");
        }
        prom.set_value(code);
    });

    auto res = prom.get_future().get();

    if (res != ErrorCode::SUCCESS) {
        return Status::FAILED;
    }

    return Status::SUCCESS;
}

Status Cv2xTelux::stopV2xRadio() {
    std::promise<ErrorCode> prom;
    cv2xRadioMgr_->stopCv2x([&prom](ErrorCode code) {
        if (code == ErrorCode::SUCCESS) {
            LOGI("Stopped V2X radio\n");
            bootkpilog("cv2x-daemon: V2X mode stopped");
        } else {
            LOGE("Failed to stop the V2X radio\n");
        }
        prom.set_value(code);
    });

    auto res = prom.get_future().get();

    if (res != ErrorCode::SUCCESS) {
        return Status::FAILED;
    }

    return Status::SUCCESS;
}

Status Cv2xTelux::registerListeners() {
    Status ret = Status::FAILED;

    cv2xStatus_.rxStatus = Cv2xStatusType::UNKNOWN;
    cv2xStatus_.txStatus = Cv2xStatusType::UNKNOWN;

    ret = cv2xRadioMgr_->registerListener(shared_from_this());
    if (ret != Status::SUCCESS) {
        LOGE("Failed to register cv2xRadioMgr listener\n");
        return ret;
    }
    return Status::SUCCESS;
}

Status Cv2xTelux::registerDataListeners() {
    Status ret = Status::FAILED;
    dataConnectionListener_ = std::make_shared<DataConnectionListener>(shared_from_this());

    ret = dataConnectionMgr_->registerListener(dataConnectionListener_);
    if (ret != Status::SUCCESS) {
        LOGE("Failed to register data connection listener\n");
        return ret;
    }

    return Status::SUCCESS;
}

Status Cv2xTelux::startDataCall(DataCallType callType, IpFamilyType ipFamilyType) {
    Status res = Status::SUCCESS;
    std::promise<bool> response;
    std::string apnName;

    if (callType >= CV2X_DATA_CALL_MAX) {
        return Status::FAILED;
    }
    {
        std::lock_guard<std::mutex> lock(dcMutex_);
        if (!(DataCallStatus::NET_NO_NET == callInfo_[callType].callStatus ||
            DataCallStatus::INVALID == callInfo_[callType].callStatus)) {
            LOGE("abort due to call status %d", static_cast<int>(callInfo_[callType].callStatus));
            return res;
        }
        callInfo_[callType].callStatus = DataCallStatus::NET_CONNECTING;
    }
    apnName = callInfo_[callType].apnName;
    std::string kpiStr = "cv2x-daemon: Start Data Call ";
    kpiStr.append(apnName);
    bootkpilog(kpiStr.c_str());

    res = dataConnectionMgr_->startDataCall(callInfo_[callType].profileIndex, ipFamilyType,
    [&response,apnName](const std::shared_ptr<IDataCall> &data, ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            response.set_value(true);
        } else {
            response.set_value(false);
            LOGE("Failed start data call operation (%s ret=%d)\n",
                 apnName.c_str(), static_cast<int>(error));
        }
    },OperationType::DATA_LOCAL);

    if (res == Status::SUCCESS) {
        if (response.get_future().get()) {
            LOGI("start cv2x data call for:%s in progress\n", apnName.c_str());
        } else {
            res = Status::FAILED;
        }
    } else {
        LOGE("start cv2x data call for:%s failed\n", apnName.c_str());
        res = Status::FAILED;
    }

    if (res != Status::SUCCESS) {
        std::lock_guard<std::mutex> lock(dcMutex_);
        if (callInfo_[callType].callStatus == DataCallStatus::NET_CONNECTING) {
            callInfo_[callType].callStatus = DataCallStatus::NET_NO_NET;
        }
    }

    return res;
}


Status Cv2xTelux::findProfiles() {
    ProfileIds profileIds = {-1, -1};

    if (-1 != callInfo_[CV2X_DATA_CALL_IP].profileIndex &&
        -1 != callInfo_[CV2X_DATA_CALL_NON_IP].profileIndex) {
        /*already got cv2x data calls related profiles info., they are constant during run time*/
        return Status::SUCCESS;
    }
    auto prom = std::make_shared<std::promise<ProfileIds>>();
    auto cb = std::make_shared<QueryProfileCallback>(prom);

    auto status = dataProfileMgr_->requestProfileList(cb);
    if (status == Status::SUCCESS) {
        profileIds = prom->get_future().get();
    }

    // check IP Data Profile
    if (profileIds.ip != -1) {
        callInfo_[CV2X_DATA_CALL_IP].profileIndex = profileIds.ip;
        LOGI("Found V2X_IP profile, idx=%d\n", profileIds.ip);
    } else {
        LOGE("Failed to find V2X_IP profile\n");
        return Status::FAILED;
    }

    // check Non-IP Data Profile
    if (profileIds.nonIp != -1) {
        callInfo_[CV2X_DATA_CALL_NON_IP].profileIndex = profileIds.nonIp;
        LOGI("Found V2X_NON_IP profile, idx=%d\n", profileIds.nonIp);
    } else {
        LOGE("Failed to find V2X_NON_IP profile\n");
        return Status::FAILED;
    }

    return Status::SUCCESS;
}

Status Cv2xTelux::startDataCalls() {

    auto f = std::async(std::launch::async , [this]()
    {
        Status ret = Status::FAILED;
        LOGI("Start Data Call IP\n");
        ret = startDataCall(CV2X_DATA_CALL_IP, IpFamilyType::IPV6);
        if(ret != Status::SUCCESS) {
            LOGE("Failed Starting IP Data Call\n");
        }
        return ret;
    });

    LOGI("Start Data Call NON-IP\n");
    Status resNonIP = startDataCall(CV2X_DATA_CALL_NON_IP, IpFamilyType::IPV6);
    if(resNonIP != Status::SUCCESS) {
        LOGE("Failed Starting NON-IP Data Call\n");
    }

    Status resIP  = f.get();
    if(resIP != Status::SUCCESS || resNonIP != Status::SUCCESS) {
        bootkpilog("cv2x-daemon: Failed Starting Data Call");
        return Status::FAILED;
    }
    return Status::SUCCESS;
}

Status Cv2xTelux::findProfilesAndStartDataCalls() {
    Status res = Status::FAILED;
    if (nullptr == dataProfileMgr_ || nullptr == dataConnectionMgr_) {
        return Status::FAILED;
    }
    LOGD("Check dataProfileMgr_ Subsystem Ready\n");

    if (not dataProfileMgr_->isSubsystemReady()) {
        if (not dataProfileMgr_->onSubsystemReady().get()) {
            LOGE("dataProfileMgr initialization failed\n");
            return Status::FAILED;
        }
    }
    LOGD("Check dataConnectionMgr Subsystem Ready\n");

    if (not dataConnectionMgr_->isSubsystemReady()) {
        if (not dataConnectionMgr_->onSubsystemReady().get()) {
            LOGE("dataConnectionMgr initialization failed\n");
            return Status::FAILED;
        }
    }

    res = findProfiles();
    if(res != Status::SUCCESS) {
        LOGE("Error finding data profiles\n");
        return res;
    }

    if ((cv2xStatus_.txStatus ==  Cv2xStatusType::INACTIVE) &&
        (cv2xStatus_.rxStatus ==  Cv2xStatusType::INACTIVE)) {
        // will re-start data calls on v2x status change
        LOGI("not start data calls if V2X status is inactive\n");
    } else {
        res = startDataCalls();
        if(res != Status::SUCCESS) {
            LOGE("Error starting data calls\n");
            return res;
        }
    }

    return Status::SUCCESS;
}

void Cv2xTelux::setIpCallStatus(DataCallStatus newStatus) {
    std::lock_guard<std::mutex> lock(dcMutex_);
    callInfo_[CV2X_DATA_CALL_IP].callStatus = newStatus;
}

void Cv2xTelux::setNonipCallStatus(DataCallStatus newStatus) {
    std::lock_guard<std::mutex> lock(dcMutex_);
    callInfo_[CV2X_DATA_CALL_NON_IP].callStatus = newStatus;;
}

int Cv2xTelux::stopDataCall(DataCallType callType, IpFamilyType ipFamilyType) {
    std::promise<bool> prom;
    int res = -1;

    if (callType >= CV2X_DATA_CALL_MAX) {
        return res;
    }
    std::string apnName = callInfo_[callType].apnName;

    // Stop IP Data Call
    Status status = dataConnectionMgr_->stopDataCall(callInfo_[callType].profileIndex,
        ipFamilyType,
    [&prom,&apnName,this](const std::shared_ptr<IDataCall> &data, ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            prom.set_value(true);
            LOGD("Stop data call succeeded (type=%s, ret=%d)\n",
                apnName.c_str(), static_cast<int>(error));

        } else {
            prom.set_value(false);
            LOGE("Stop data call failed (type=%s ret=%d)\n",
                apnName.c_str(), static_cast<int>(error));

        }
    });

    if (status != Status::SUCCESS || !prom.get_future().get()) {
        LOGE("Failed stop data call operation type=%d\n", callType);
        return -EINVAL;
    }

    return 0;
}

int Cv2xTelux::stopV2xDataCalls() {
    int res=0;

    // Stop IP Data Call
    res = stopDataCall(CV2X_DATA_CALL_IP, IpFamilyType::IPV6);
    if(res) {
        LOGE("Failed Stop IP Data Call\n");
    }

    // Stop NON-IP Data Call
    res = stopDataCall(CV2X_DATA_CALL_NON_IP, IpFamilyType::IPV6);
    if(res) {
        LOGE("Failed Stop NON-IP Data Call\n");
    }

    return res;
}
