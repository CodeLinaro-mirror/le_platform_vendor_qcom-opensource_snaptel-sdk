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
 * @file       CellBroadcastManagerStub.hpp
 *
 * @brief      Implementation of CellBroadcastManager
 *
 */

#ifndef CELLBROADCAST_MANAGER_STUB_HPP
#define CELLBROADCAST_MANAGER_STUB_HPP

#include "common/event-manager/ClientEventManager.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "common/ListenerManager.hpp"
#include <telux/tel/CellBroadcastDefines.hpp>
#include <telux/tel/CellBroadcastManager.hpp>
#include <telux/common/CommonDefines.hpp>
#include "TelDefinesStub.hpp"

namespace telux {
namespace tel {

class CellBroadcastManagerStub : public ICellBroadcastManager,
                                 public IEventListener,
                                 public std::enable_shared_from_this<CellBroadcastManagerStub> {
public:

    CellBroadcastManagerStub(int phoneId, telux::common::InitResponseCb clientCallback);
    bool isSubsystemReady();
    std::future<bool> onSubsystemReady();
    telux::common::ServiceStatus getServiceStatus();
    SlotId getSlotId();
    telux::common::Status updateMessageFilters(std::vector<CellBroadcastFilter> filters,
        telux::common::ResponseCallback callback = nullptr);
    telux::common::Status requestMessageFilters(
        RequestFiltersResponseCallback callback);
    telux::common::Status setActivationStatus(bool activate,
        telux::common::ResponseCallback callback = nullptr);
    telux::common::Status requestActivationStatus(
        RequestActivationStatusResponseCallback callback);
    telux::common::Status registerListener(std::weak_ptr<ICellBroadcastListener> listener);
    telux::common::Status deregisterListener(std::weak_ptr<ICellBroadcastListener> listener);

private:
    void initSync(telux::common::InitResponseCb callback);
    void invokeCallback(telux::common::ResponseCallback callback,
        telux::common::ErrorCode error);
    void invokeCallback(RequestFiltersResponseCallback callback,
        telux::common::ErrorCode error, std::vector<CellBroadcastFilter> filters);
    void invokeCallback(RequestActivationStatusResponseCallback callback,
        telux::common::ErrorCode error, bool isActivated);
    void invokeInitResponseCallback(telux::common::ServiceStatus cbStatus,
        telux::common::InitResponseCb callback);
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
        telux::common::InitResponseCb initCb_;
    std::mutex mutex_;
    std::shared_ptr<telux::common::ListenerManager<ICellBroadcastListener>> listenerMgr_;
};

} // end of namespace tel

} // end of namespace telux

#endif // CELLBROADCAST_MANAGER_STUB_HPP