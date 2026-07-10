/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       CallStub.hpp
 *
 * @brief      Implementation of Call
 *
 */

#ifndef CALL_STUB_HPP
#define CALL_STUB_HPP

#include <telux/tel/CallManager.hpp>
#include <telux/tel/CallListener.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/common/CommonDefines.hpp>
#include "../common/AsyncTaskQueue.hpp"
#include <grpcpp/grpcpp.h>
#include "../../protos/proto-src/tel_simulation.grpc.pb.h"

#define INVALID -1

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using telStub::DialerService;

namespace telux {
namespace tel {

enum class AecsCallEndReason {
    DROPPED     = 2, /* AECS call connected and failed unexpectedly */
    ORIG_FAILED = 4, /* AECS call origination fails */
    FAILED      = 5, /* AECS call failed permanently */
    COMPLETED   = 6, /* AECS call ended or disconnected */
    UNSPECIFIED = 0xffff, /* AECS call fail reason is not available */
};

struct CallInfo {
    int index                     = INVALID;  // Connection Index
    CallDirection callDirection   = CallDirection::NONE;  // enumeration for MO / MT call
    std::string remotePartyNumber = "";  // Remote party number
    bool transmitMsd              = false;
    CallState callState           = CallState::CALL_IDLE;
    CallEndCause callEndCause     = CallEndCause::NORMAL;
    int sipErrorCode              = 0;
    int rawCauseCode              = 0;
    bool isMultiPartyCall         = false;
    bool isMpty                   = false;
    RttMode mode                  = RttMode::DISABLED;  // RTT mode of the call
    RttMode localRttCapability    = RttMode::DISABLED;  // RTT capability of local device
    RttMode peerRttCapability     = RttMode::DISABLED;  // RTT capability of peer device
    CallType callType             = CallType::UNKNOWN;
    std::string callReason        = "";
    NetworkMode networkMode       = NetworkMode::UNKNOWN;
    bool isAecsCallDrop           = false;
    RedialState redialState       = RedialState::UNKNOWN;
};

class CallStub : public ICall {
 public:
    CallStub(int phoneId, CallInfo callInfo);

    telux::common::Status answer(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr,
        RttMode mode                                                      = RttMode::DISABLED);

    telux::common::Status hold(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);

    telux::common::Status resume(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);

    telux::common::Status reject(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);

    telux::common::Status reject(const std::string &rejectSMS,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status hangup(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status playDtmfTone(
        char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status startDtmfTone(
        char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    telux::common::Status stopDtmfTone(
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    RttMode getRttMode();
    RttMode getLocalRttCapability();
    RttMode getPeerRttCapability();
    CallType getCallType();
    telux::common::Status modify(
        RttMode mode, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    NetworkMode getNetworkMode();
    telux::common::Status respondToModifyRequest(bool modifyResponseType,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr);
    CallState getCallState();
    int getCallIndex();
    CallEndCause getCallEndCause();
    int getSipErrorCode();
    int getDetailedCauseCode();
    CallDirection getCallDirection();
    std::string getRemotePartyNumber();
    std::string getCallReason();
    int getPhoneId();
    bool isMultiPartyCall();
    void updateCallState(CallState callState);
    void updateCallDirection(CallDirection callDirection);
    void setCallIndex(int index);
    void setCallState(CallState callState);
    void setCallReason(std::string reason);
    bool match(std::shared_ptr<CallStub> &ci);
    bool isInfoStale(const std::shared_ptr<CallStub> &ci);
    telux::common::Status updateCallInfo(std::shared_ptr<CallStub> &callInfo);
    void logCallDetails();
    bool isAecsCallDrop();
    telux::tel::RedialState getRedialState();
    /**
     * Set the cause of this call
     */
    void setCallEndCause(CallEndCause causeCode);

    /**
     * Set the call end reason for AECS call failure.
     */
    void setAecsCallEndReason(AecsCallEndReason reason);

 private:
    std::unique_ptr<::telStub::DialerService::Stub> stub_;
    int phoneId_;
    CallInfo callInfo_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    void invokeCommandCallback(
        std::shared_ptr<ICommandResponseCallback> callback, ErrorCode error, int cbDelay);
    telux::common::Status modifyOrRespondToModifyCall(RttMode mode, std::string api,
        std::shared_ptr<telux::common::ICommandResponseCallback> callback);
};

}  // end of namespace tel

}  // end of namespace telux

#endif  // CALL_STUB_HPP
