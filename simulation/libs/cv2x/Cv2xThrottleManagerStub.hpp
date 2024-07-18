/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CV2X_THROTTLE_MANAGER_STUB_HPP
#define CV2X_THROTTLE_MANAGER_STUB_HPP

#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xThrottleManager.hpp>

#include "protos/proto-src/cv2x_simulation.grpc.pb.h"

namespace telux {
namespace cv2x {

class Cv2xThrottleManagerStub : public ICv2xThrottleManager {
 public:
    Cv2xThrottleManagerStub();
    ~Cv2xThrottleManagerStub();

    telux::common::Status init(telux::common::InitResponseCb callback);
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(
        std::weak_ptr<ICv2xThrottleManagerListener> listener) override;
    telux::common::Status deregisterListener(
        std::weak_ptr<ICv2xThrottleManagerListener> listener) override;
    telux::common::Status setVerificationLoad(int load, setVerificationLoadCallback cb) override;

 private:
    std::unique_ptr<::cv2xStub::Cv2xManagerService::Stub> stub_;
    std::atomic<telux::common::ServiceStatus> serviceStatus_{
        telux::common::ServiceStatus::SERVICE_UNAVAILABLE};

    void initSync(telux::common::InitResponseCb callback);
};

}  // namespace cv2x
}  // namespace telux

#endif  // CV2X_THROTTLE_MANAGER_STUB_HPP
