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

#include "PhoneStub.hpp"
#include <telux/common/DeviceConfig.hpp>

using namespace telux::common;
using namespace telux::tel;
using namespace std;

PhoneStub::PhoneStub(int phoneId) {

}

telux::common::Status PhoneStub::getPhoneId(int &phId) {
    return telux::common::Status::NOTSUPPORTED;
}

RadioState PhoneStub::getRadioState() {
    return telux::tel::RadioState::RADIO_STATE_ON;
}

void PhoneStub::setRadioState(RadioState radioState) {}

ServiceState PhoneStub::getServiceState() {
    return telux::tel::ServiceState::IN_SERVICE;
}

telux::common::Status PhoneStub::requestVoiceRadioTechnology(VoiceRadioTechResponseCb callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status
    PhoneStub::requestVoiceServiceState(std::weak_ptr<IVoiceServiceStateCallback> callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::setRadioPower(
    bool enable, std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::requestCellInfo(telux::tel::CellInfoCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::setCellInfoListRate(uint32_t timeInterval,
    common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::requestSignalStrength(
    std::shared_ptr<telux::tel::ISignalStrengthCallback> callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::setECallOperatingMode(ECallMode eCallMode,
    telux::common::ResponseCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::requestECallOperatingMode(ECallGetOperatingModeCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

bool PhoneStub::isSubsystemReady() {
    return true;
}

std::future<bool> PhoneStub::onReady() {
    LOG(DEBUG, __FUNCTION__);
    std::future<bool> ready_future;
    ready_future = std::async(std::launch::async,
    [this]() {
        while (!isSubsystemReady()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    return(isSubsystemReady());});
    return((ready_future));
}

telux::common::Status PhoneStub::requestOperatorName(OperatorNameCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::requestOperatorInfo(OperatorInfoCallback callback) {
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status PhoneStub::configureSignalStrength(
    std::vector<SignalStrengthConfig> signalStrengthConfig, telux::common::ResponseCallback
    callback) {
    return telux::common::Status::NOTSUPPORTED;
}