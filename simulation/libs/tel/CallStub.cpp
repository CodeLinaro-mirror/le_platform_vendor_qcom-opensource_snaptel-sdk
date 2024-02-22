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

#include "CallStub.hpp"

using namespace telux::common;
using namespace telux::tel;
using namespace std;

CallStub::CallStub(int phoneId, CallInfo callInfo)
    : stub_(DialerService::NewStub(grpc::CreateChannel("localhost:8089",
    grpc::InsecureChannelCredentials())))
    , phoneId_(phoneId)
    , callInfo_(callInfo) {
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
}

telux::common::Status CallStub::answer(
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "answer()");
    telux::common::Status status = telux::common::Status::FAILED;
    if ((callInfo_.callState == CallState::CALL_INCOMING ) ||
        (callInfo_.callState == CallState::CALL_WAITING)) {
        ::telStub::AnswerRequest request;
        ::telStub::AnswerReply response;
        ClientContext context;
        request.set_phone_id(phoneId_);
        request.set_call_index(callInfo_.index);
        LOG(DEBUG, "Answer(), phoneId ", phoneId_, "CallIndex", callInfo_.index);
        grpc::Status reqstatus = stub_->Answer(&context, request, &response);
        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCommandCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    } else {
        LOG(ERROR, "call in wrong state:", (int)callInfo_.callState);
        return telux::common::Status::INVALIDSTATE;
    }
    return telux::common::Status::SUCCESS;
}

/**
 * Puts this call on hold
 */
telux::common::Status CallStub::hold(
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "hold()");
    telux::common::Status status = telux::common::Status::FAILED;
    if (callInfo_.callState == CallState::CALL_ACTIVE) {
        ::telStub::HoldRequest request;
        ::telStub::HoldReply response;
        ClientContext context;
        request.set_phone_id(phoneId_);
        request.set_call_index(callInfo_.index);
        LOG(DEBUG, "hold(), phoneId ", phoneId_, "CallIndex", callInfo_.index);
        grpc::Status reqstatus = stub_->Hold(&context, request, &response);
        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());

        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCommandCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    } else {
        LOG(ERROR, "call in wrong state:", (int)callInfo_.callState);
        return telux::common::Status::INVALIDSTATE;
    }
    return status;
}

/**
 * Resumes this call from on-hold state into active state
 */
telux::common::Status CallStub::resume(
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "resume()");
    telux::common::Status status = telux::common::Status::FAILED;
    if (callInfo_.callState == CallState::CALL_ON_HOLD) {
        ::telStub::ResumeRequest request;
        ::telStub::ResumeReply response;
        ClientContext context;
        request.set_phone_id(phoneId_);
        request.set_call_index(callInfo_.index);
        LOG(DEBUG, "resume(), phoneId ", phoneId_, "CallIndex", callInfo_.index);
        grpc::Status reqstatus = stub_->Resume(&context, request, &response);
        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCommandCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    } else {
        LOG(ERROR, "call in wrong state:", (int)callInfo_.callState);
        return telux::common::Status::INVALIDSTATE;
    }
    return status;
}

/**
 * Rejects the call. Only applicable for CallState::INCOMING and CallState::WAITING
 */
telux::common::Status CallStub::reject(
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "reject()");
    telux::common::Status status = telux::common::Status::FAILED;
    if (callInfo_.callState == CallState::CALL_INCOMING
        || callInfo_.callState == CallState::CALL_WAITING) {
        ::telStub::RejectRequest request;
        ::telStub::RejectReply response;
        ClientContext context;
        request.set_phone_id(phoneId_);
        request.set_call_index(callInfo_.index);
        LOG(DEBUG, "Reject(), phoneId ", phoneId_, "CallIndex", callInfo_.index);
        grpc::Status reqstatus = stub_->Reject(&context, request, &response);
        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCommandCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    } else {
        LOG(ERROR, "call in wrong state:", (int)callInfo_.callState);
        return telux::common::Status::INVALIDSTATE;
    }
    return status;
}

/**
 * Rejects the call, sends an SMS to caller. Only applicalbe for CallState::INCOMING and
 * CallState::WAITING
 */
telux::common::Status CallStub::reject(const std::string &rejectSMS,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "rejectSms()");
    telux::common::Status status = telux::common::Status::FAILED;
    if (callInfo_.callState == CallState::CALL_INCOMING
        || callInfo_.callState == CallState::CALL_WAITING) {
        ::telStub::RejectWithSMSRequest request;
        ::telStub::RejectWithSMSReply response;
        ClientContext context;
        request.set_phone_id(phoneId_);
        request.set_call_index(callInfo_.index);
        LOG(DEBUG, "Reject(), phoneId ", phoneId_, "CallIndex", callInfo_.index);
        grpc::Status reqstatus = stub_->RejectWithSMS(&context, request, &response);
        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCommandCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    } else {
        LOG(ERROR, "call in wrong state:", (int)callInfo_.callState);
        return telux::common::Status::INVALIDSTATE;
    }
    return status;
}

/**
 * Hangup the call if the call state is either active, hold, dialing, waiting or alerting
 */
telux::common::Status CallStub::hangup(
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
     telux::common::Status status = telux::common::Status::FAILED;
    if (callInfo_.callState == CallState::CALL_ON_HOLD
        || callInfo_.callState == CallState::CALL_WAITING
        || callInfo_.callState == CallState::CALL_ACTIVE
        || callInfo_.callState == CallState::CALL_DIALING
        || callInfo_.callState == CallState::CALL_ALERTING) {
        ::telStub::HangupRequest request;
        ::telStub::HangupReply response;
        ClientContext context;
        request.set_phone_id(phoneId_);
        request.set_call_index(callInfo_.index);
        LOG(DEBUG, "hangup(), phoneId ", phoneId_, "CallIndex", callInfo_.index);
        grpc::Status reqstatus = stub_->Hangup(&context, request, &response);
        if (!reqstatus.ok()) {
            return telux::common::Status::FAILED;
        }
        telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
        status = static_cast<telux::common::Status>(response.status());
        bool isCallbackNeeded = static_cast<bool>(response.iscallback());
        int delay = static_cast<int>(response.delay());
        if ((status == telux::common::Status::SUCCESS )&& (isCallbackNeeded)) {
            auto f1 = std::async(std::launch::async,
                [this, error, callback, delay]() {
                    this->invokeCommandCallback(callback, error, delay);
                }).share();
            taskQ_->add(f1);
        }
    } else {
        LOG(ERROR, "call in wrong state:", (int)callInfo_.callState);
        return telux::common::Status::INVALIDSTATE;
    }
    return status;
}

void CallStub::invokeCommandCallback(std::shared_ptr<ICommandResponseCallback> callback,
    ErrorCode error, int cbDelay) {
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f = std::async(std::launch::async,
        [this, error , callback]() {
            callback->commandResponse(error);
        }).share();
    taskQ_->add(f);
}

/**
 * Get the current state of this call
 */
CallState CallStub::getCallState() {
    return callInfo_.callState;
}

int CallStub::getCallIndex() {
    return callInfo_.index;
}

/**
 * Get the cause of call termination
 */
CallEndCause CallStub::getCallEndCause() {
    return callInfo_.callEndCause;
}

/**
 * Get the call type - incoming, outgoing etc
 */
CallDirection CallStub::getCallDirection() {
    return callInfo_.callDirection;
}

/**
 * Get the dialed number of this call
 */
std::string CallStub::getRemotePartyNumber() {
    return callInfo_.remotePartyNumber;
}

int CallStub::getPhoneId() {
    return phoneId_;
}

telux::common::Status CallStub::stopDtmfTone(
    std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "Phone = ", __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

telux::common::Status CallStub::startDtmfTone(
    char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "Phone = ", __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

bool CallStub::isMultiPartyCall() {
    return callInfo_.isMpty;
}

telux::common::Status CallStub::playDtmfTone(
    char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback) {
    LOG(DEBUG, "Phone = ", __FUNCTION__);
    return telux::common::Status::NOTSUPPORTED;
}

void CallStub::updateCallState(CallState callState) {
    LOG(DEBUG, "Phone = ", __FUNCTION__);
    callInfo_.callState = callState;
}

void CallStub::updateCallDirection(CallDirection callDirection) {
    LOG(DEBUG, "Phone = ", __FUNCTION__);
    callInfo_.callDirection = callDirection;
}

void CallStub::setCallIndex(int index) {
    callInfo_.index = index;
}

