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


/**
 * @file       PhoneManagerStub.hpp
 *
 * @brief      Implementation of PhoneManager
 *
 */

#ifndef PHONE_MANAGER_STUB_HPP
#define PHONE_MANAGER_STUB_HPP

#include "common/Logger.hpp"
#include "common/AsyncTaskQueue.hpp"
#include <telux/tel/PhoneManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include "PhoneStub.hpp"
#include <map>

namespace telux {
namespace tel {

class PhoneManagerStub : public IPhoneManager,
                         public std::enable_shared_from_this<PhoneManagerStub> {
public:

    PhoneManagerStub(telux::common::InitResponseCb clientCallback);

    telux::common::ServiceStatus getServiceStatus() override;
    ~PhoneManagerStub();
    telux::common::Status getPhoneIds(std::vector<int> &phoneIds);
    int getPhoneIdFromSlotId(int slotId);
    int getSlotIdFromPhoneId(int phoneId);
    std::shared_ptr<IPhone> getPhone(int phoneId);
    telux::common::Status registerListener(std::weak_ptr<IPhoneListener> listener);
    telux::common::Status removeListener(std::weak_ptr<IPhoneListener> listener);
    telux::common::Status requestCellularCapabilityInfo(
      std::shared_ptr<ICellularCapabilityCallback> callback = nullptr) override;
    telux::common::Status setOperatingMode(OperatingMode operatingMode,
                                          telux::common::ResponseCallback callback
                                          = nullptr) override;
    telux::common::Status requestOperatingMode(std::shared_ptr<IOperatingModeCallback> callback
                                              = nullptr) override;
    telux::common::Status resetWwan(telux::common::ResponseCallback callback
                                          = nullptr) override;
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;

private:
    std::vector<int> phoneIds_;
    std::map<int, std::shared_ptr<PhoneStub>> phoneMap_;
    void initSync(telux::common::InitResponseCb callback);
    void invokeInitResponseCallback(int cbDelay, telux::common::ServiceStatus cbStatus,
    telux::common::InitResponseCb callback);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::mutex phoneManagerMutex_;
    std::vector<std::weak_ptr<IPhoneListener>> listeners_;
    void invokeOperatingModeCallback(telux::tel::OperatingMode operatingMode,
        telux::common::ErrorCode error, std::shared_ptr<IOperatingModeCallback> callback);
};

} // end of namespace tel

} // end of namespace telux

#endif // PHONE_MANAGER_STUB_HPP