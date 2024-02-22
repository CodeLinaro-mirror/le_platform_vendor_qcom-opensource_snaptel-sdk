/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       MultiSimManagerStub.hpp
 *
 * @brief      Implementation of IMultiSimManager
 *
 */

#ifndef MULTISIM_MANAGER_STUB_HPP
#define MULTISIM_MANAGER_STUB_HPP

#include <telux/tel/MultiSimManager.hpp>
#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include "common/event-manager/ClientEventManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"

namespace telux {
namespace tel {

class MultiSimManagerStub : public IMultiSimManager,
                                public IEventListener,
                                public std::enable_shared_from_this<MultiSimManagerStub>  {
public:
    MultiSimManagerStub(telux::common::InitResponseCb clientCallback);
    ~MultiSimManagerStub();
    bool isSubsystemReady() override;
    std::future<bool> onSubsystemReady() override;
    telux::common::ServiceStatus getServiceStatus() override;
    telux::common::Status registerListener(std::weak_ptr<IMultiSimListener>) override;
    telux::common::Status deregisterListener(std::weak_ptr<IMultiSimListener>) override;
    telux::common::Status getSlotCount(int &count) override;
    telux::common::Status requestHighCapability(HighCapabilityCallback callback) override;
    telux::common::Status setHighCapability(int slotId,
        common::ResponseCallback callback) override;
    telux::common::Status switchActiveSlot(SlotId slotId,
        common::ResponseCallback callback) override;
    telux::common::Status requestSlotStatus(SlotStatusCallback callback) override;
    void onEventUpdate(google::protobuf::Any event);
    void cleanup();
private:
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    void initSync(telux::common::InitResponseCb callback);
};

} // end of namespace tel

} // end of namespace telux

#endif // MULTISIM_MANAGER_STUB_HPP