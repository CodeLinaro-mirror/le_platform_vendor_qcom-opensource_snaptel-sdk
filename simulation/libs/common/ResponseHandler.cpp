/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <future>
#include "ResponseHandler.hpp"

#define DEFAULT_DELAY 100
#define DEFAULT_SUBSYSTEM_STATUS 1
#define DEFAULT_ERROR_CODE 0
#define SUBSYSTEM_DELAY "IsSubsystemReadyDelay"
#define SUBSYSTEM_READINESS "IsSubsystemReady"
#define SERVICE_STATUS "getServiceStatus"

namespace telux {
namespace common {

ResponseHandler::ResponseHandler(Json::Value& obj)
: rootObj_(obj)
{
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

ResponseHandler::~ResponseHandler() {
    taskQ_ = nullptr;
}

telux::common::Status ResponseHandler::initResponseHandler(std::string subsystem,
    telux::common::InitResponseCb callback) {
    int cbDelay = rootObj_[subsystem].get(SUBSYSTEM_DELAY, DEFAULT_DELAY).asInt();
    int cbStatus =
    rootObj_[subsystem].get(SUBSYSTEM_READINESS, DEFAULT_SUBSYSTEM_STATUS).asInt();

    LOG(DEBUG, __FUNCTION__, " subsystem::", subsystem);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
    auto f = std::async(std::launch::async, [this, cbDelay, cbStatus, callback]() {
        this->invokeInitResponseCallback(cbDelay, cbStatus, callback);
    }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

telux::common::Status ResponseHandler::asyncResponseHandler(Json::Value& rootObj,
    std::string publicInterface, telux::common::ResponseCallback callback) {
    int cbDelay = rootObj[publicInterface].get("callbackDelay", DEFAULT_DELAY).asInt();
    int cbErrorCode = rootObj[publicInterface].get("error", DEFAULT_ERROR_CODE).asInt();

    LOG(DEBUG, __FUNCTION__, " publicInterface::", publicInterface);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbErrorCode::", cbErrorCode);
    auto f = std::async(std::launch::async, [this, cbDelay, cbErrorCode, callback]() {
        this->invokeResponseCallback(cbDelay, cbErrorCode, callback);
    }).share();
    taskQ_->add(f);

    return telux::common::Status::SUCCESS;
}

void ResponseHandler::invokeInitResponseCallback(int cbDelay, int cbStatus,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    telux::common::ServiceStatus servStatus = static_cast<telux::common::ServiceStatus>(cbStatus);

    if (callback) {
        callback(servStatus);
    }
}

void ResponseHandler::invokeResponseCallback(int cbDelay, int cbErrorCode,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    telux::common::ErrorCode errCode = static_cast<telux::common::ErrorCode>(cbErrorCode);

    if (callback) {
        callback(errCode);
    }
}

void ResponseHandler::mapServiceStatus(std::string subsystem,
    ServiceStatus &status) {
    LOG(DEBUG, __FUNCTION__);

    std::string srvStatus = rootObj_[subsystem][SERVICE_STATUS].asString();
    if (srvStatus == "SERVICE_FAILED") {
        status = ServiceStatus::SERVICE_FAILED;
    } else if (srvStatus == "SERVICE_UNAVAILABLE") {
        status = ServiceStatus::SERVICE_UNAVAILABLE;
    } else {
        status = ServiceStatus::SERVICE_AVAILABLE;
    }
}

telux::common::ServiceStatus ResponseHandler::getServiceStatus(std::string subsystem) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ServiceStatus servStatus;
    mapServiceStatus(subsystem, servStatus);

    return servStatus;
}

std::future<bool> ResponseHandler::onSubSystemReady(std::string subsystem) {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
        [this](std::string subsystem) {
            while (!this->isSubSystemReady(subsystem)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            return(this->isSubSystemReady(subsystem));},
            subsystem);
    return((ready_future));
}

bool ResponseHandler::isSubSystemReady(std::string subsystem) {
    bool status =
    rootObj_[subsystem].get(SUBSYSTEM_READINESS, DEFAULT_SUBSYSTEM_STATUS).asBool();
    LOG(DEBUG, __FUNCTION__, " subsystem::", subsystem, " Ready::", status);

    return status;
}

}
}