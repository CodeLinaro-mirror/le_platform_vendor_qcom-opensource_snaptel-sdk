/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <grpcpp/grpcpp.h>

#include "Cv2xThrottleManagerStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"
#include "common/SimulationConfigParser.hpp"
#include "common/event-manager/ClientEventManager.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

#define DEFAULT_DELAY 100

namespace telux {
namespace cv2x {

Cv2xThrottleManagerStub::Cv2xThrottleManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

Cv2xThrottleManagerStub::~Cv2xThrottleManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status Cv2xThrottleManagerStub::init(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async,
                     [callback]() {
                         if (callback) {
                             std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_DELAY));
                             callback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
                         }
                     }).share();
    taskQ_.add(f);
    return telux::common::Status::SUCCESS;
}

telux::common::ServiceStatus Cv2xThrottleManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::Status Cv2xThrottleManagerStub::registerListener(
    std::weak_ptr<ICv2xThrottleManagerListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::Status::SUCCESS;
}
telux::common::Status Cv2xThrottleManagerStub::deregisterListener(
    std::weak_ptr<ICv2xThrottleManagerListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::Status::SUCCESS;
}

telux::common::Status Cv2xThrottleManagerStub::setVerificationLoad(
    int load, setVerificationLoadCallback cb) {
    LOG(DEBUG, __FUNCTION__);
    auto f
        = std::async(std::launch::async,
                     [cb]() { cb(telux::common::ErrorCode::SUCCESS); }).share();

    taskQ_.add(f);

    return telux::common::Status::SUCCESS;
}

}  // namespace cv2x
}  // namespace telux
