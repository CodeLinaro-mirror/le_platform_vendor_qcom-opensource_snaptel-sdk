/*
 *  Copyright (c) 2017,2021 The Linux Foundation. All rights reserved.
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

/**
 * @file       Call.hpp
 * @brief      Phone Call class is the primary interface to process Call requests.
 */

#ifndef CALL_HPP
#define CALL_HPP

#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_call
 * @{ */

class IPhone;

/**
 * @brief ICall represents a call in progress. An ICall cannot be directly created by the client,
 *        rather it  is returned as a result of instantiating a call or from the PhoneListener
 *        when receiving an incoming call.
 */
class ICall {
public:
   /**
    * Allows the client to answer the call. This is only applicable for CallState::INCOMING and
    * CallState::WAITING calls.
    * If a Waiting call is being answered and the existing call is Active, then existing call
    * will move to Hold state.If the existing call is on Hold already, then it will remain on Hold.
    * The waiting call state transition from Waiting to Active.
    *
    * @param [in] callback - optional callback pointer to get the response of answer request
    * below are possible error codes for callback response
    *        - @ref SUCCESS
    *        - @ref RADIO_NOT_AVAILABLE
    *        - @ref NO_MEMORY
    *        - @ref MODEM_ERR
    *        - @ref INTERNAL_ERR
    *        - @ref INVALID_STATE
    *        - @ref INVALID_CALL_ID
    *        - @ref INVALID_ARGUMENTS
    *        - @ref OPERATION_NOT_ALLOWED
    *        - @ref GENERIC_FAILURE
    *
    * @returns Status of hold function i.e. success or suitable error code.
    */
   virtual telux::common::Status
      answer(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Puts the ongoing call on hold.
    *
    * @param [in] callback - optional callback pointer to get the response of hold request
    * below are possible error codes for callback response
    *        - @ref SUCCESS
    *        - @ref RADIO_NOT_AVAILABLE
    *        - @ref NO_MEMORY
    *        - @ref MODEM_ERR
    *        - @ref INTERNAL_ERR
    *        - @ref INVALID_STATE
    *        - @ref INVALID_CALL_ID
    *        - @ref INVALID_ARGUMENTS
    *        - @ref OPERATION_NOT_ALLOWED
    *        - @ref GENERIC_FAILURE
    *
    *  @returns Status of hold function i.e. success or suitable error code.
    */
   virtual telux::common::Status
      hold(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Resumes this call from on-hold state to active state
    *
    * @param [in] callback - optional callback pointer to get the response of resume request
    * below are possible error codes for callback response
    *        - @ref SUCCESS
    *        - @ref RADIO_NOT_AVAILABLE
    *        - @ref NO_MEMORY
    *        - @ref MODEM_ERR
    *        - @ref INTERNAL_ERR
    *        - @ref INVALID_STATE
    *        - @ref INVALID_CALL_ID
    *        - @ref INVALID_ARGUMENTS
    *        - @ref OPERATION_NOT_ALLOWED
    *        - @ref GENERIC_FAILURE
    *
    *  @returns Status of resume function i.e. success or suitable error code.
    */
   virtual telux::common::Status
      resume(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Reject the incoming/waiting call. Only applicable for CallState::INCOMING and
    * CallState::WAITING calls.
    *
    * @param [in] callback - optional callback pointer to get the response of reject request
    * below are possible error codes for callback response
    *        - @ref SUCCESS
    *        - @ref RADIO_NOT_AVAILABLE
    *        - @ref NO_MEMORY
    *        - @ref MODEM_ERR
    *        - @ref INTERNAL_ERR
    *        - @ref INVALID_STATE
    *        - @ref INVALID_CALL_ID
    *        - @ref INVALID_ARGUMENTS
    *        - @ref OPERATION_NOT_ALLOWED
    *        - @ref GENERIC_FAILURE
    *
    * @returns Status of reject function i.e. success or suitable error code.
    */
   virtual telux::common::Status
      reject(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Reject the call and  send an SMS to caller. Only applicable for CallState::INCOMING
    * and CallState::WAITING calls.
    *
    * @param [in] rejectSMS SMS string used to send in response to a call rejection.
    * @param [in] callback - optional callback pointer to get the response of rejectwithSMS request
    * below are possible error codes for callback response
    *        - @ref SUCCESS
    *        - @ref RADIO_NOT_AVAILABLE
    *        - @ref NO_MEMORY
    *        - @ref MODEM_ERR
    *        - @ref INTERNAL_ERR
    *        - @ref INVALID_STATE
    *        - @ref INVALID_CALL_ID
    *        - @ref INVALID_ARGUMENTS
    *        - @ref OPERATION_NOT_ALLOWED
    *        - @ref GENERIC_FAILURE
    *
    *  @deprecated This API not being supported
    *
    *  @returns Status of success for call reject() or suitable error code.
    *
    */
   virtual telux::common::Status
      reject(const std::string &rejectSMS,
             std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Hangup the call if the call state is either active, hold, dialing, waiting or alerting.
    *
    * @param [in] callback - optional callback pointer to get the response of hangup request
    * below are possible error codes for callback response
    *        - @ref SUCCESS
    *        - @ref RADIO_NOT_AVAILABLE
    *        - @ref NO_MEMORY
    *        - @ref MODEM_ERR
    *        - @ref INTERNAL_ERR
    *        - @ref INVALID_STATE
    *        - @ref INVALID_CALL_ID
    *        - @ref INVALID_ARGUMENTS
    *        - @ref OPERATION_NOT_ALLOWED
    *        - @ref GENERIC_FAILURE
    *
    * @returns Status of hangup i.e. success or suitable error code.
    */
   virtual telux::common::Status
      hangup(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Play a DTMF tone and stop it. The interval for which the tone is played is dependent on the
    * system implementation. If continuous DTMF tone is playing, it will be stopped.
    * This API is used to play DTMF tone on TX path so that it is heard on far end. For DTMF
    * playback on local device on the RX path use @ref telux::audio::IAudioVoiceStream::playDtmfTone
    *
    * @param [in] tone - a single character with one of 12 values: 0-9, *, #.
    *
    * @param [in] callback - Optional callback pointer to get the result of
    * playDtmfTones function
    *
    * @returns Status of playDtmfTones i.e. success or suitable error code.
    */
   virtual telux::common::Status playDtmfTone(
      char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Starts a continuous DTMF tone. To terminate the continous DTMF tone,stopDtmfTone API needs to
    * be invoked explicitly. This API is used to play DTMF tone on TX path so that it is heard on
    * far end. For DTMF playback on local device on the RX path use
    * @ref telux::audio::IAudioVoiceStream::playDtmfTone
    *
    * @param [in] tone - a single character with one of 12 values: 0-9, *, #.
    * @param [in] callback - Optional callback pointer to get the result of
    * startDtmfTone function.
    *
    * @returns Status of startDtmfTone i.e. success or suitable error code.
    */
   virtual telux::common::Status startDtmfTone(
      char tone, std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Stop the currently playing continuous DTMF tone.
    * @param [in] callback - Optional callback pointer to get the result of
    * stopDtmfTone function.
    *
    * @returns Status of stopDtmfTone i.e. success or suitable error code.
    */
   virtual telux::common::Status
      stopDtmfTone(std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Get the current state of the call, such as ringing, in progress etc.
    *
    * @returns CallState - enumeration representing call State
    */
   virtual CallState getCallState() = 0;

   /**
    * Get the unique index of the call assigned by Telephony subsystem
    *
    * @returns Call Index
    */
   virtual int getCallIndex() = 0;

   /**
    * Get the direction of the call
    *
    * @returns CallDirection - enumeration representing call direction
    *                          i.e. INCOMING/ OUTGOING
    */
   virtual CallDirection getCallDirection() = 0;

   /**
    * Get the dailing number
    *
    * @returns Phone Number to which the call was dialed out
    *          Empty string in case of INCOMING call direction
    */
   virtual std::string getRemotePartyNumber() = 0;

   /**
    * Get the cause of the termination of the call.
    *
    * @returns Enum representing call end cause.
    */
   virtual CallEndCause getCallEndCause() = 0;

   /**
    * Get id of the phone object which represents the network/SIM on which
    * the call is in progress.
    *
    * @returns Phone Id.
    */
   virtual int getPhoneId() = 0;

   /**
    *  To check if call is in multi party call(conference) or not
    *
    * @returns True if call is in conference otherwise false.
    *
    * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
    *           break backwards compatibility.
    */
   virtual bool isMultiPartyCall() = 0;


   virtual ~ICall() {
   }
};
/** @} */ /* end_addtogroup telematics_call */

}  // End of namespace tel

}  // End of namespace telux

#endif  // PHONE_CALL_HPP
