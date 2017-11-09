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
 * @file       Phone.hpp
 * @brief      Phone class is the primary interface that provides telephony services
 *             like makeCall, get phoneInfo, radio state, service state.
 */

#ifndef PHONE_HPP
#define PHONE_HPP

#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Call.hpp>
#include <telux/tel/ECallDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneManager.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_phone
 * @{ */
class PhoneListener;
class ISignalStrengthCallback;

/**
 * @brief This class allows making phone calls, getting system information and registering
 *        for system events. Each Phone instance is associated with a single SIM.
 *        So on a dual SIM device you would have 2 Phone instances.
 */
class IPhone {
public:
   /**
    * Get the Phone ID corresponding to phone.
    *
    * @param [out] phoneId - Unique identifier for the phone
    *
    * @returns Status of getPhoneId i.e. success or suitable error code.
    */
   virtual telux::common::Status getPhoneId(int &phId) = 0;

   /**
    * Get Radio state of device.
    *
    * @returns @ref RadioState
    *
    */
   virtual RadioState getRadioState() = 0;

   /**
    * Get service state of the phone.
    *
    * @returns @ref ServiceState
    */
   virtual ServiceState getServiceState() = 0;

   /**
    * Get current signal strength of the associated network.
    *
    * @param [in] callback - Optional callback pointer to get the response of signal strength
    * request
    *
    * @returns Status of requestSignalStrength i.e. success or suitable error code.
    */
   virtual telux::common::Status
      requestSignalStrength(std::shared_ptr<ISignalStrengthCallback> callback = nullptr)
      = 0;

   virtual ~IPhone(){};
};

/**
 * @brief Interface for Signal strength callback object.
 * Client needs to implement this interface to get single shot responses for commands like get
 * signal strength.
 *
 * The methods in callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 */
class ISignalStrengthCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to requestSignalStrength API.
    *
    * @param [out] signalStrength - Pointer to signal strength object
    * @param [out] error - Return code for whether the operation succeeded or failed
    *        - @ref SUCCESS
    *        - @ref RADIO_NOT_AVAILABLE
    */
   virtual void signalStrengthResponse(std::shared_ptr<SignalStrength> signalStrength,
                                       telux::common::ErrorCode error) {
   }
};
/** @} */ /* end_addtogroup telematics_phone */

}  // End of namespace tel

}  // End of namespace telux

#endif  // PHONE_HPP
