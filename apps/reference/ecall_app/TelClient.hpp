/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021-2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef TELCLIENT_HPP
#define TELCLIENT_HPP

#include <telux/tel/CallManager.hpp>
#include <telux/tel/MultiSimManager.hpp>

using namespace telux::common;
using namespace telux::tel;

/**
 * Data structure to cache all the information when an ecall is initiated, when an emergency
 * network scan fail indication is reported, when high capability switch is required.
 */
struct ECallInfo {
   bool transmitMsd;          /**< Set to true if MSD needs to be transmitted*/
   ECallMsdData msdData;      /**< If the transmitMsd is true, msdData will holds all the details
                                   required to construct an MSD */
   bool isCustomNumber;       /**< Set to true if client is dialing*/
   std::string dialNumber;    /**< If isCustomNumber is true, dialNumber holds the number */
   ECallCategory category;    /**< ECall Category ie., automatic or normal */
   ECallVariant variant;      /**< ECall Variant ie., test or emergency or voice call */
   bool eCallNWScanFailed;   /**< Set to true if the emergency network scan fail indication is
                                   reported */
   bool triggerHighCapSwitch; /**< Set to true if high capability switch is required */
   ECallMsdTransmissionStatus msdTransmissionStatus;
                              /**< MSD transmission status */

};


/** Listener class that provides eCall call status updates */
class CallStatusListener {
public:
    /**
     * This function is called when the eCall is disconnected/ends
     */
    virtual void onCallDisconnect() {
    }

    /** This function is called when the eCall connection is in progress i.e, during redial from
     * application or modem
     */
    virtual void onCallConnect(int phoneId) {
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
     * This function requests to stop T10 eCall High Level Application Protocol(HLAP) timer
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     *
     * @returns Status of stopT10Timer i.e success or suitable status code.
     *
     */
    telux::common::Status stopT10Timer(int phoneId);

    /**
     * This function requests to set the value of eCall High Level Application Protocol(HLAP)
     * timer
     *
     * @param [in] phoneId       Represents phone corresponding to which the operation will be
     *                           performed
     * @param [in] type          @ref HlapTimerType
     * @param [in] timeDuration  Represents the time duration.
     *
     *
     * @returns Status of setHlapTimer i.e success or suitable status code.
     *
     */
    telux::common::Status setHlapTimer(int phoneId, HlapTimerType type, uint32_t timeDuration);

    /**
     * This function requests to get the value of eCall High Level Application Protocol(HLAP)
     * timer
     *
     * @param [in] phoneId  Represents phone corresponding to which the operation will be performed
     * @param [in] type     @ref HlapTimerType
     * @returns Status of getHlapTimer i.e success or suitable status code.
     *
     */
    telux::common::Status getHlapTimer(int phoneId, HlapTimerType type);

    /**
     * Get various configuration parameters related to eCall
     *
     * @returns Status of getECallConfig i.e success or suitable status code.
     *
     */
    telux::common::Status getECallConfig();

    /**
     * Set various configuration parameters related to eCall
     *
     * @param [in] config configuration to be written
     *
     * @returns Status of setECallConfig i.e success or suitable status code.
     *
     */
    telux::common::Status setECallConfig(EcallConfig config);

    /**
     * Gets encoded optional additional data content for eCall MSD.
     *
     * @param [in] optionalAdditionalDataConteny  Euro NCAP optional additional data content.
     * @param [out] data                          Encoded vector of bytes.
     *
     * @returns Status of getEncodedOptionalAdditionalDataContent i.e success or suitable
     * status code.
     *
     */
    telux::common::Status getEncodedOptionalAdditionalDataContent(ECallOptionalEuroNcapData
        optionalAdditionalDataContent, std::vector<uint8_t> &data);

    /**
     * Gets encoded eCall MSD payload.
     *
     * @param [in] eCallMsd    ECall MSD.
     * @param [out] msdPdu     Encoded vector of bytes.
     *
     * @returns Error code for getECallMsdPayload i.e success or suitable status code.
     *
     */
    telux::common::ErrorCode getECallMsdPayload(ECallMsdData eCallMsd,
        std::vector<uint8_t> &msdPdu);

    /**
     * This function provides the eCall progress state.
     *
     * @returns True if an eCall is in progress, otherwise false.
     *
     */
    bool isECallInProgress();

    /**
     * This function caches latest MSD recieved after location update.
     *
     * @param [in] MSD data
     *
     */
    void setECallMsd(ECallMsdData& msdData_);

    void onIncomingCall(std::shared_ptr<ICall> call) override;
    void onCallInfoChange(std::shared_ptr<ICall> call) override;
    void onECallMsdTransmissionStatus(int phoneId, ErrorCode errorCode) override;
    void onECallMsdTransmissionStatus(int phoneId,
                    ECallMsdTransmissionStatus msdTransmissionStatus) override;
    void OnMsdUpdateRequest(int phoneId) override;
    void onECallHlapTimerEvent(int phoneId, ECallHlapTimerEvents timerEvents) override;
    void makeCallResponse(telux::common::ErrorCode error,
                                    std::shared_ptr<telux::tel::ICall>) override;
    void hlapTimerStatusResponse(telux::common::ErrorCode error, int phoneId,
                                 ECallHlapTimerStatus timersStatus);
    void stopT10TimerResponse(telux::common::ErrorCode error);
    void setHlapTimerResponse(telux::common::ErrorCode error);
    void getHlapTimerResponse(telux::common::ErrorCode error, uint32_t timeDuration);
    void onServiceStatusChange(ServiceStatus status) override;

    TelClient();
    ~TelClient();

private:
    void setECallProgressState(bool state);

    class AnswerCommandCallback : public telux::common::ICommandResponseCallback {
    public:
        void commandResponse(telux::common::ErrorCode error) override;
        AnswerCommandCallback(std::weak_ptr<TelClient> telClient);
    private:
        std::weak_ptr<TelClient> eCallTelClient_;
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

    //Map to hold the ongoing eCall Info w.r.t phoneId
    std::map<int, ECallInfo> eCallDataMap_;
    ECallMsdData msdData_;

    class EcallScanFailHandler :  public ICallListener,
                                  public std::enable_shared_from_this<EcallScanFailHandler> {
    public:
        telux::common::Status init();
       /**
        * This function is called whenever there is a scan failure after one round of network scan
        * during origination of emergency call or at any time during the emergency call.
        *
        * During origination of an ecall or in between an ongoing ecall, if the UE is in an area of
        * no/poor coverage and loses service, the modem will perform network scan and try toi
        * register on any available network.
        * If the scan completes successfully and the device finds a suitable cell, the ecall will be
        * placed and the call state changes to the active state.
        * If the network scan fails then this function will be invoked after one round of network
        * scan.
        *
        * @param [in] phoneId - Unique Id of phone on which network scan failure reported.
        *
        */
        void onEmergencyNetworkScanFail(int phoneId) override;
        EcallScanFailHandler(std::weak_ptr<TelClient> telClient);
        ~EcallScanFailHandler();
    private:
        telux::common::Status setHighCapability(int phoneId);
        telux::common::Status requestHighCapability();

        void setHighCapabilityResponse(telux::common::ErrorCode error);
        void requestHighCapabilityResponse(int slotId, telux::common::ErrorCode error);

        void onCallInfoChange(std::shared_ptr<ICall> call);
        /** Member variable to hold MultiSimManager object */
        std::shared_ptr<telux::tel::IMultiSimManager> multiSimMgr_ = nullptr;
        std::weak_ptr<TelClient> eCallTelClient_;
    };

    std::shared_ptr<EcallScanFailHandler> eCallScanFailHdlrInstance_;
};

#endif  // TELCLIENT_HPP
