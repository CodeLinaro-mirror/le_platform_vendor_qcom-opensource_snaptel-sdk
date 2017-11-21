/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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
 * @file       CallManager.hpp
 * @brief Call Manager does aggregate state management for in-progress calls
 *        It provides APIs for conferencing or swapping active and on-hold calls
 *        and to get the list of in-progress calls
 *
 */

#ifndef CALLMANAGER_HPP
#define CALLMANAGER_HPP

#include <memory>
#include <vector>
#include <string>

#include <telux/tel/Call.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/ECallDefines.hpp>
#include <telux/tel/CallListener.hpp>

#include <telux/common/CommonDefines.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_call
 * @{ */

class IMakeCallCallback;

/**
 * @brief Call Manager does aggregate state management for in-progress calls
 *        It provides APIs for makeCall, makeECall, conferencing or swapping active and on-hold
 *        calls
 *        and to get the list of in-progress calls
 */
class ICallManager {
public:
   /**
    * Initiate a voice call.
    *
    * @param [in] phoneId - Represents phone corresponding to which on make call operation
    * is performed
    * @param [in] dialNumber - String representing the dialing number
    * @param [in] callback - Optional callback pointer to get the response of makeCall request.
    * Possible error codes for callback response
    *        - @ref SUCCESS
    *        - @ref RADIO_NOT_AVAILABLE
    *        - @ref DIAL_MODIFIED_TO_USSD
    *        - @ref DIAL_MODIFIED_TO_SS
    *        - @ref DIAL_MODIFIED_TO_DIAL
    *        - @ref INVALID_ARGUMENTS
    *        - @ref NO_MEMORY
    *        - @ref INVALID_STATE
    *        - @ref NO_RESOURCES
    *        - @ref INTERNAL_ERR
    *        - @ref FDN_CHECK_FAILURE
    *        - @ref MODEM_ERR
    *        - @ref NO_SUBSCRIPTION
    *        - @ref NO_NETWORK_FOUND
    *        - @ref INVALID_CALL_ID
    *        - @ref DEVICE_IN_USE
    *        - @ref MODE_NOT_SUPPORTED
    *        - @ref ABORTED
    *        - @ref GENERIC_FAILURE
    *
    *@returns Status of makeCall i.e. success or suitable status code.
    */
   virtual telux::common::Status makeCall(int phoneId, const std::string &dialNumber,
                                          std::shared_ptr<IMakeCallCallback> callback = nullptr)
      = 0;

   /**
    * Initiate an ecall.
    *
    * @param [in] phoneId - Represents phone corresponding to which on make ecall operation
    * is performed
    * @param [in] eCallMsdData - The structure containing required fields to
    * create eCall Minimum Set of Data (MSD)
    * @param [in] emergencyCategory - Denotes the eCall category
    *     - 0x20 or 32 (VOICE_EMER_CAT_MANUAL)
    *     - 0x40 or 64 (VOICE_EMER_CAT_AUTO_ECALL)
    * @param [in] eCallVariant - Denotes the call variant enum which can take following values:
    *     - 0x01 -- Test ECALL (Originate test eCall to the mobile number configured
    *               in NV params), This is the default value.
    *     - 0x02 -- Emergency ECALL (Originate EMERGENCY eCall i.e. call to 112)
    * @param [in] callback - Optional callback pointer to get the response of makeECall request.
    * Possible error codes for callback response
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
    * @returns Status of makeECall i.e. success or suitable status code.
    */
   virtual telux::common::Status makeECall(int phoneId, const ECallMsdData &eCallMsdData,
                                           int emergencyCategory, int eCallVariant,
                                           std::shared_ptr<IMakeCallCallback> callback = nullptr)
      = 0;

   /**
    * Update the eCall MSD in modem to be sent to Public Safety Answering Point (PSAP) when
    * requested.
    * @param [in] phoneId - Represents phone corresponding to which updateECallMsd operation
    * is performed
    * @param [in] eCallMsd - The data structure represents the Minimum Set of Data (MSD)
    *
    * @returns Status of updateECallMsd i.e. success or suitable error code.
    */
   virtual telux::common::Status
      updateECallMsd(int phoneId, const ECallMsdData &eCallMsd,
                     std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Get a vector of all currently in-progress calls in the system
    *
    * @returns List of active calls.
    */
   virtual std::vector<std::shared_ptr<ICall>> getInProgressCalls() = 0;

   /**
    * Join two calls in a conference
    *
    * @param [in] call1 - Call object to conference.
    * @param [in] call2 - Call object to conference.
    * @param [in] callback - Optional callback pointer to get the result of conference function
    *
    * @returns Status of conference i.e. success or suitable error code.
    */
   virtual telux::common::Status
      conference(std::shared_ptr<ICall> call1, std::shared_ptr<ICall> call2,
                 std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Swap two calls - make one active and put the other on hold.
    *
    * @param [in] callToHold - Active call object to swap to hold state.
    * @param [in] callToActivate - Hold call object to swap to active state.
    * @param [in] callback - Optional callback pointer to get the result of swap function
    *
    * @returns Status of swap i.e. success or suitable error code.
    */
   virtual telux::common::Status
      swap(std::shared_ptr<ICall> callToHold, std::shared_ptr<ICall> callToActivate,
           std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Add a listener for Call events.
    * This listener will only get called for state changes on the Call objects.
    * @param [in] listener - Pointer to ICallListener object which receives event corresponding
    * to phone
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    */
   virtual telux::common::Status
      registerListener(std::shared_ptr<telux::tel::ICallListener> listener)
      = 0;

   /**
    * Remove a previously added listener.
    * @param [in] listener - Pointer to ICallListener object which receives event corresponding
    * to call
    *
    * @returns Status of removeListener i.e. success or suitable error code.
    */
   virtual telux::common::Status removeListener(std::shared_ptr<telux::tel::ICallListener> listener)
      = 0;

   virtual ~ICallManager(){};
};

/**
 * @brief Interface for Make Call callback object.
 * Client needs to implement this interface to get single shot responses for commands like
 * make call.
 *
 * The methods in callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 */
class IMakeCallCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to makeCall API.
    *
    * @param [out] error - @ref ErrorCode
    * @param [out] call - Pointer to Call object or nullptr in case of failure
    */
   // TODO: How to get call object if dial request handler is in CallManager?
   virtual void makeCallResponse(telux::common::ErrorCode error,
                                 std::shared_ptr<ICall> call = nullptr) {
   }
};

/** @} */ /* end_addtogroup telematics_call */

}  // End  of namespace tel

}  // End  of namespace telux

#endif  // CALLMANAGER_HPP
