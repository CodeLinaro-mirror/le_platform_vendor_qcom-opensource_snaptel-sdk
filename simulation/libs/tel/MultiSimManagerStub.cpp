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

#include "MultiSimManagerStub.hpp"

#define FIRST_SIM_SLOT_ID 1
#define INIT_DELAY 100

namespace telux {

namespace tel {

MultiSimManagerStub::MultiSimManagerStub(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

MultiSimManagerStub::~MultiSimManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void MultiSimManagerStub::cleanup() {
   LOG(DEBUG, __FUNCTION__);
}

void MultiSimManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(callback) {
        auto f = std::async(std::launch::async, [this, callback]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(INIT_DELAY));
            if (callback) {
                callback(telux::common::ServiceStatus::SERVICE_AVAILABLE);
            }
        }).share();
        taskQ_->add(f);
    }
}

std::future<bool> MultiSimManagerStub::onSubsystemReady() {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
        [this]() {
            while (!isSubsystemReady()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(INIT_DELAY));
            }
            return(isSubsystemReady());});
    return((ready_future));
}

telux::common::ServiceStatus MultiSimManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    return telux::common::ServiceStatus::SERVICE_AVAILABLE;
}

telux::common::Status MultiSimManagerStub::registerListener(
    std::weak_ptr<IMultiSimListener> listener) {
    return telux::common::Status::SUCCESS;
}

telux::common::Status MultiSimManagerStub::deregisterListener(
    std::weak_ptr<IMultiSimListener> listener) {
    return telux::common::Status::SUCCESS;
}

bool MultiSimManagerStub::isSubsystemReady() {
    return true;
}

telux::common::Status MultiSimManagerStub::getSlotCount(int &count) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::requestHighCapability(HighCapabilityCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::setHighCapability(int slotId,
    common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::switchActiveSlot(SlotId slotId,
    common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status MultiSimManagerStub::requestSlotStatus(SlotStatusCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

void MultiSimManagerStub::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
}

} // end of namespace tel

} // end of namespace telux