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
#include <cstring>
#include <map>

#include "Cv2xTelux.hpp"

#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataFactory.hpp>

using telux::common::Status;
using telux::common::ErrorCode;

static std::map<ServiceStatus, std::string> convertServiceStatusToString = {
        {ServiceStatus::SERVICE_AVAILABLE, "Available"},
        {ServiceStatus::SERVICE_UNAVAILABLE, "Unavailable"},
};

void Cv2xTelux::onStatusChanged(Cv2xStatus status)
{
    if ((status.txStatus != Cv2xStatusType::UNKNOWN ||
        status.rxStatus != Cv2xStatusType::UNKNOWN)) {
        LOGI("tx_status=%d, rx_status=%d, tx_cause=%d, rx_cause=%d\n",
            Cv2xUtils::convertStatus(status.txStatus),
            Cv2xUtils::convertStatus(status.rxStatus),
            Cv2xUtils::convertStatus(status.txCause),
            Cv2xUtils::convertStatus(status.rxCause));
    }

    // Trigger post SSR event to start data call
    bool triggetPostSSRV2XReady = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (isPostSSRV2XDone_ == false) {
            isPostSSRV2XDone_ = true;
            cv_.notify_one();
            triggetPostSSRV2XReady = true;
        }
    }

    if (triggetPostSSRV2XReady) {
        LOGD("Triggered post ssr event to start data calls\n");
    }

    // Handle State Transition InActive to Active/Suspended
    if (((cv2xStatus_.txStatus ==  Cv2xStatusType::INACTIVE) &&
         (cv2xStatus_.rxStatus ==  Cv2xStatusType::INACTIVE)) &&
         ((status.txStatus !=  Cv2xStatusType::INACTIVE) &&
         (status.rxStatus !=  Cv2xStatusType::INACTIVE))) {

        LOGD("State Transition From Inactive to Active/Suspended\n");

        // Checks if data calls were ever started before.
        // If not, then this state change is a result of daemon starting up and not
        // because of state transitions from inactive to active.
        //
        // So don't start data calls during daemon startup here, it will be done elsewhere.
        bool startDataCalls = false;
        {
            std::lock_guard<std::mutex> lock(dcMutex_);
            if (isInitializationDone_) {
                startDataCalls = true;
            }
        }

        if (startDataCalls) {
            createProfileAndStartDataCalls();
        }
    }

    cv2xStatus_ = status;
}

void Cv2xTelux::onL2AddrChanged(uint32_t newL2Address)
{
    LOGI("Received QMI_WDS_V2X_SRC_L2_INFO_IND_V01\n");
    LOGI("l2_addr=0x%x\n", newL2Address);
}

DataConnectionListener::DataConnectionListener(std::weak_ptr<Cv2xTelux> instance){
    cv2xTelux_ = instance;
}

void DataConnectionListener::onDataCallInfoChanged(const std::shared_ptr<IDataCall> &dataCall)
{
    if (!dataCall) {
        return;
    }

    auto iface = dataCall->getInterfaceName();
    auto status = Cv2xUtils::DataCallStatusToStr(dataCall->getDataCallStatus());
    auto reason = Cv2xUtils::DataCallEndReasonToInt(dataCall->getDataCallEndReason());
    auto ip_type = Cv2xUtils::IpFamilyTypeToStr(dataCall->getIpFamilyType());
    auto profile_id = dataCall->getProfileId();

    if (iface == "") {
        iface = "unknown";
    }

    if (status != "NET_NO_NET") {
        reason = 0;
    }

    LOGD("onDataCallInfoChanged: iface=%s, status=%s, reason=%d, ip_type=%s, profile_id=%d\n",
            iface.c_str(), status.c_str(), reason, ip_type.c_str(), profile_id);;
}

void DataConnectionListener::onServiceStatusChange(ServiceStatus status)
{
    Status res = Status::FAILED;

    LOGD("DataConnectionListener Service Status changed to %s\n",
            convertServiceStatusToString[status].c_str() );
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
    } else if (status == ServiceStatus::SERVICE_AVAILABLE){

        auto sp = cv2xTelux_.lock();
        if(sp) {
            LOGD("Waiting for CV2xRadio to come back ONLINE\n");

            std::unique_lock<std::mutex> cvLock(sp->mutex_);
            while(sp->isPostSSRV2XDone_ == false) {
                sp->cv_.wait(cvLock);
            }
            LOGD("CV2xRadio back ONLINE\n");

            res = sp->createProfileAndStartDataCalls();
            if (res != Status::SUCCESS) {
                LOGE("Failed to start data call\n");
                return;
            }
        }
    }
}

void Cv2xTelux::onServiceStatusChange(ServiceStatus status)
{
    Status res = Status::FAILED;

    LOGD("Cv2xTelux Service Status changed to %s\n",
            convertServiceStatusToString[status].c_str() );

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

QueryProfileCallback::QueryProfileCallback(std::shared_ptr<std::promise<ProfileIds>> prom)
{
    prom_ = prom;
}

void QueryProfileCallback::onProfileListResponse(
    const std::vector<std::shared_ptr<DataProfile>> &profiles, ErrorCode error)
{
    ProfileIds profileIds = { -1, -1};

    if (error == ErrorCode::SUCCESS) {
        for (auto it : profiles) {
            if (it->getApn().compare(APN_NAME_V2X_IP) == 0) {
                profileIds.ip = it->getId();
            }
            if (it->getApn().compare(APN_NAME_V2X_NON_IP) == 0) {
                profileIds.nonIp = it->getId();
            }
        }
    }
    prom_->set_value(profileIds);
}

CreateProfileCallback::CreateProfileCallback(std::shared_ptr<std::promise<int>> prom)
{
    prom_ = prom;
}

void CreateProfileCallback::onResponse(int profileId, ErrorCode error)
{
    if (error == ErrorCode::SUCCESS) {
        prom_->set_value(profileId);
    } else {
        prom_->set_value(-1);
    }
}

Status Cv2xTelux::initV2xLibrary()
{
    auto &cv2xFactory = Cv2xFactory::getInstance();
    cv2xRadioMgr_ = cv2xFactory.getCv2xRadioManager();

    auto &dataFactory = DataFactory::getInstance();
    dataProfileMgr_ = dataFactory.getDataProfileManager();
    dataConnectionMgr_ = dataFactory.getDataConnectionManager();

    /* Check that V2X radio is initialized */
    if (not cv2xRadioMgr_->isReady()) {
        if (not cv2xRadioMgr_->onReady().get()) {
            LOGE("V2X cv2xRadioMgr initialization failed\n");
            return Status::FAILED;
        }
    }

    if (not dataProfileMgr_->isSubsystemReady()) {
        if (not dataProfileMgr_->onSubsystemReady().get()) {
            LOGE("dataProfileMgr initialization failed\n");
            return Status::FAILED;
        }
    }

    if (not dataConnectionMgr_->isSubsystemReady()) {
        if (not dataConnectionMgr_->onSubsystemReady().get()) {
            LOGE("dataConnectionMgr initialization failed\n");
            return Status::FAILED;
        }
    }

    dcInfoIP_ = nullptr;
    dcInfoNonIP_ = nullptr;
    isInitializationDone_ = false;
    isPostSSRV2XDone_ = false;

    return Status::SUCCESS;
}

Status Cv2xTelux::deinitV2xLibrary()
{
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

Status Cv2xTelux::getV2xRadioStatus(Cv2xStatus &status)
{
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

Status Cv2xTelux::startV2xRadio()
{
    std::promise<ErrorCode> prom;
    cv2xRadioMgr_->startCv2x([&prom](ErrorCode code) {
        if (code == ErrorCode::SUCCESS) {
            LOGD("Started V2X radio\n");
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

Status Cv2xTelux::stopV2xRadio()
{
    std::promise<ErrorCode> prom;
    cv2xRadioMgr_->stopCv2x([&prom](ErrorCode code) {
        if (code == ErrorCode::SUCCESS) {
            LOGD("Stopped V2X radio\n");
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

Status Cv2xTelux::registerListeners()
{
    Status ret = Status::FAILED;
    dataConnectionListener_ = std::make_shared<DataConnectionListener>(shared_from_this());

    cv2xStatus_.rxStatus = Cv2xStatusType::UNKNOWN;
    cv2xStatus_.txStatus = Cv2xStatusType::UNKNOWN;

    ret = cv2xRadioMgr_->registerListener(shared_from_this());
    if (ret != Status::SUCCESS) {
        LOGE("Failed to register cv2xRadioMgr listener\n");
        return ret;
    }

    ret = dataConnectionMgr_->registerListener(dataConnectionListener_);
    if (ret != Status::SUCCESS) {
        LOGE("Failed to register data connection listener\n");
        return ret;
    }

    return Status::SUCCESS;
}

static bool createV2xProfile(std::shared_ptr<IDataProfileManager> dataProfileMgr,
                               std::shared_ptr<DataCallInfo> dataCall,
                               std::string apnName)
{
    ProfileParams params;
    memset(&params, 0, sizeof(params));
    params.profileName = apnName;
    params.apn = apnName;
    params.techPref = TechPreference::TP_3GPP;
    params.ipFamilyType = IpFamilyType::IPV6;

    auto prom = std::make_shared<std::promise<int>>();
    auto cb = std::make_shared<CreateProfileCallback>(prom);
    auto status = dataProfileMgr->createProfile(params, cb);

    if (status == Status::SUCCESS) {
        dataCall->profileIndex = prom->get_future().get();
        LOGD("Created profile for APN=%s, profile_id=%d\n", apnName.c_str(),
             dataCall->profileIndex);
        return true;
    } else {
        LOGE("Create profile failed for APN=%s with error code %d\n", apnName.c_str(),
             static_cast<int>(status));
        return false;
    }

    return true;
}

Status Cv2xTelux::startDataCall(std::shared_ptr<DataCallInfo> dataCall, IpFamilyType ipFamilyType)
{
    Status res = Status::FAILED;
    std::promise<bool> response;

    dataConnectionMgr_->startDataCall(dataCall->profileIndex,
            IpFamilyType::IPV6,
    [&response,&dataCall,this](const std::shared_ptr<IDataCall> &data, ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            response.set_value(true);
        } else {
            response.set_value(false);
            LOGE("Failed start data call operation (type=%d ret=%d)\n",
                 dataCall->type, static_cast<int>(error));
        }
    });

    if (response.get_future().get()) {
        LOGI("Received DSI_EVT_NET_IS_CONN: network_type=%d is online\n", dataCall->type);
        res = Status::SUCCESS;
    } else {
        LOGI("Received DSI_EVT_NET_IS_CONN: network_type=%d is offline\n", dataCall->type);
        res = Status::FAILED;
    }
    return res;
}


Status Cv2xTelux::createProfile() {
    dcInfoIP_->type = CV2X_DATA_CALL_IP;
    dcInfoNonIP_->type = CV2X_DATA_CALL_NON_IP;

    ProfileIds profileIds = { -1, -1};

    auto prom = std::make_shared<std::promise<ProfileIds>>();
    auto cb = std::make_shared<QueryProfileCallback>(prom);

    auto status = dataProfileMgr_->requestProfileList(cb);
    if (status == Status::SUCCESS) {
        profileIds = prom->get_future().get();
    }

    // check IP Data Profile
    if (profileIds.ip != -1) {
        dcInfoIP_->profileIndex = profileIds.ip;
        LOGI("Found V2X_IP profile, idx=%d\n", profileIds.ip);
    } else {
        if (!createV2xProfile(dataProfileMgr_, dcInfoIP_,
                    APN_NAME_V2X_IP)) {
            LOGE("Failed to create V2X_IP profile\n");
            return Status::FAILED;
        } else {
            LOGI("Created V2X_IP profile, idx=%d\n",
                    dcInfoIP_->profileIndex);
        }
    }

    // check Non-IP Data Profile
    if (profileIds.nonIp != -1) {
        dcInfoNonIP_->profileIndex = profileIds.nonIp;
        LOGI("Found V2X_NON_IP profile, idx=%d\n", profileIds.nonIp);
    } else {
        if (!createV2xProfile(dataProfileMgr_, dcInfoNonIP_,
                    APN_NAME_V2X_NON_IP)) {
            LOGE("Failed to create V2X_NON_IP profile\n");
            return Status::FAILED;
        } else {
            LOGI("Created V2X_NON_IP profile, idx=%d\n",
                    dcInfoNonIP_->profileIndex);
        }
    }

    return Status::SUCCESS;
}

Status Cv2xTelux::startDataCalls() {
    Status res = Status::FAILED;

    LOGI("Start Data Call IP\n");
    res = startDataCall(dcInfoIP_,IpFamilyType::IPV6);
    if(res != Status::SUCCESS) {
        LOGE("Failed Starting IP Data Call\n");
        return res;
    }

    LOGI("Start Data Call NON-IP\n");
    res = startDataCall(dcInfoNonIP_,IpFamilyType::IPV6);
    if(res != Status::SUCCESS) {
        LOGE("Failed Starting NON-IP Data Call\n");
        return res;
    }

    return Status::SUCCESS;
}

Status Cv2xTelux::createProfileAndStartDataCalls()
{
    Status res = Status::FAILED;

    if ((dcInfoIP_ == nullptr) && (dcInfoNonIP_ == nullptr)) {
        dcInfoIP_ = std::make_shared<DataCallInfo>();
        dcInfoNonIP_ = std::make_shared<DataCallInfo>();
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

    res = createProfile();
    if(res != Status::SUCCESS) {
        LOGE("Error creating data profiles\n");
        return res;
    }
    LOGI("APN profiles setup done\n");

    res = startDataCalls();
    if(res != Status::SUCCESS) {
        LOGE("Error starting data calls\n");
        return res;
    }

    std::lock_guard<std::mutex> lock(dcMutex_);
    // Set flag to indicate that the data calls have been started successfully
    // as part of first startup.
    if (not isInitializationDone_) {
        isInitializationDone_  = true;
    }

    return Status::SUCCESS;
}

int Cv2xTelux::stopDataCall(std::shared_ptr<DataCallInfo> dataCall,
        IpFamilyType ipFamilyType)
{
    std::promise<bool> prom;

    // Stop IP Data Call
    Status status = dataConnectionMgr_->stopDataCall(dataCall->profileIndex, IpFamilyType::IPV6,
    [&prom,&dataCall,this](const std::shared_ptr<IDataCall> &data, ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            prom.set_value(true);
            LOGD("Stop data call succeeded (type=%d, ret=%d)\n",
                dataCall->type, static_cast<int>(error));

        } else {
            prom.set_value(false);
            LOGE("Stop data call failed (type=%d ret=%d)\n",
                dataCall->type, static_cast<int>(error));

        }
    });

    if (status != Status::SUCCESS || !prom.get_future().get()) {
        LOGE("Failed stop data call operation type=%d\n", dataCall->type);
        return -EINVAL;
    }

    return 0;
}

int Cv2xTelux::stopV2xDataCalls()
{
    int res=0;

    // Stop IP Data Call
    res = stopDataCall(dcInfoIP_, IpFamilyType::IPV6);
    if(res) {
        LOGE("Failed Stop IP Data Call\n");
    }

    // Stop NON-IP Data Call
    res = stopDataCall(dcInfoNonIP_, IpFamilyType::IPV6);
    if(res) {
        LOGE("Failed Stop NON-IP Data Call\n");
    }

    return res;
}
