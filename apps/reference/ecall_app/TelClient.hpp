/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TELCLIENT_HPP
#define TELCLIENT_HPP

#include <telux/tel/CallManager.hpp>

using namespace telux::common;
using namespace telux::tel;

/** Listener class that provides eCall call status updates */
class CallStatusListener {
public:
    /**
     * This function is called when the eCall is disconnected/ends
     */
    virtual void onCallDisconnect() {
    }

    /**
     * Destructor of CallStatusListener
     */
    virtual ~CallStatusListener() {
    }
};

/** TelClient class provides methods to trigger an eCall, update MSD, answer/hangup a call */
class TelClient : public ICallListener,
                  public IMakeCallCallback,
                  public std::enable_shared_from_this<TelClient> {
public:
    /**
     * Initialize telephony subsystem
     */
    telux::common::Status init();

    /**
     * This function starts a standard eCall procedure(eg.112).
     * This is typically invoked when an eCall is triggered.
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     * @param [in] msdData      MSD data to be used
     * @param [in] category     ECallCategory
     * @param [in] variant      ECallVariant
     * @param [in] transmitMsd  Configures MSD transmission at MO call connect
     * @param [in] callListener pointer to CallStatusListener to notify call status changes
     *
     * @returns Status of startECall i.e success or suitable status code.
     *
     */
    telux::common::Status startECall(int phoneId, ECallMsdData msdData, ECallCategory category,
                    ECallVariant variant, bool transmitMsd,
                    std::shared_ptr<CallStatusListener> callListener);

    /**
     * This function starts a voice eCall procedure to the specified phone number.
     * This is typically invoked when a TPS eCall is triggered.
     *
     * @param [in] phoneId      Represents phone corresponding to which eCall operation is performed
     * @param [in] msdData      MSD data to be used
     * @param [in] category     ECallCategory
     * @param [in] dialNumber   phone number to be dialed
     * @param [in] transmitMsd  Configures MSD transmission at MO call connect
     * @param [in] callListener pointer to CallStatusListener to notify call status changes
     *
     * @returns Status of startECall i.e success or suitable status code.
     *
     */
    telux::common::Status startECall(int phoneId, ECallMsdData msdData, ECallCategory category,
                    const std::string dialNumber, bool transmitMsd,
                    std::shared_ptr<CallStatusListener> callListener);

    /**
     * This function updates the cached MSD data stored in Modem, which would be used in MSD pull
     * operation.
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     * @param [in] msdData  MSD data to be updated
     *
     * @returns Status of updateECallMSD i.e success or suitable status code.
     *
     */
    telux::common::Status updateECallMSD(int phoneId, ECallMsdData msdData);

    /**
     * This function is used to answer an incoming call
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     * @param [in] callListener pointer to CallStatusListener to notify call status changes
     *
     * @returns Status of answer i.e success or suitable status code.
     *
     */
    telux::common::Status answer(int phoneId, std::shared_ptr<CallStatusListener> callListener);

    /**
     * This function is used to hangup an ongoing call
     *
     * @param [in] phoneId    Represents phone corresponding to which the operation is performed
     * @param [in] callIndex  Represents the call on which the operation is performed
     *
     * @returns Status of hangup i.e success or suitable status code.
     *
     */
    telux::common::Status hangup(int phoneId, int callIndex);

    /**
     * This function dumps the list of calls in progress
     *
     * @returns Status of getCurrentCalls i.e success or suitable status code.
     */
    telux::common::Status getCurrentCalls();

    /**
     * This function requests status of various eCall HLAP timers
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     *
     * @returns Status of requestECallHlapTimerStatus i.e success or suitable status code.
     *
     */
    telux::common::Status requestECallHlapTimerStatus(int phoneId);

    /**
     * This function provides the eCall progress state.
     *
     * @returns True if an eCall is in progress, otherwise false.
     *
     */
    bool isECallInProgress();

    void onIncomingCall(std::shared_ptr<ICall> call) override;
    void onCallInfoChange(std::shared_ptr<ICall> call) override;
    void onECallMsdTransmissionStatus(int phoneId, ErrorCode errorCode) override;
    void onECallMsdTransmissionStatus(int phoneId,
                    ECallMsdTransmissionStatus msdTransmissionStatus) override;
    void onECallHlapTimerEvent(int phoneId, ECallHlapTimerEvents timerEvents) override;
    void makeCallResponse(telux::common::ErrorCode error,
                                    std::shared_ptr<telux::tel::ICall>) override;
    void hlapTimerStatusResponse(telux::common::ErrorCode error, int phoneId,
                                 ECallHlapTimerStatus timersStatus);

    TelClient();
    ~TelClient();

private:
    void setECallProgressState(bool state);

    class AnswerCommandCallback : public telux::common::ICommandResponseCallback {
    public:
        void commandResponse(telux::common::ErrorCode error) override;
        std::shared_ptr<TelClient> eCallTelClient_;
    };
    std::shared_ptr<AnswerCommandCallback> answerCommandCallback_;

    class HangupCommandCallback : public telux::common::ICommandResponseCallback {
    public:
        void commandResponse(telux::common::ErrorCode error) override;
    };
    std::shared_ptr<HangupCommandCallback> hangupCommandCallback_;

    class UpdateMsdCommandCallback : public telux::common::ICommandResponseCallback {
    public:
        void commandResponse(telux::common::ErrorCode error) override;
    };
    std::shared_ptr<UpdateMsdCommandCallback> updateMsdCommandCallback_;

    /** Member variable to hold Telephony manager object */
    std::shared_ptr<ICallManager> callMgr_;

    /** Call info related to eCall */
    std::shared_ptr<telux::tel::ICall> eCall_;

    /** Represents eCall status */
    bool eCallInprogress_;
    std::mutex mutex_;
    std::shared_ptr<CallStatusListener> callListener_;
};

#endif  // TELCLIENT_HPP
