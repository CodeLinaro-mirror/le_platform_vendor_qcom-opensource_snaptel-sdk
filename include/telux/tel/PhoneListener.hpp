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
 * @file       PhoneListener.hpp
 * @brief      Interface for Phone listener object. Client needs to implement this interface
 *             to get access to Telephony subsystem notifications like service state and
 *             signal strength.
 *
 *             The methods in listener can be invoked from multiple different threads.
 *             The implementation should be thread-safe.
 */

#ifndef PHONELISTENER_HPP
#define PHONELISTENER_HPP

#include <vector>
#include <memory>

#include <telux/tel/Call.hpp>
#include <telux/tel/SmsManager.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/SignalStrength.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_phone
 * @{ */

class IPhone;

/**
 * These are all the notifications that can be listened to global telephony state.
 * They are part of a bitmask and can be ANDed together to provide to
 * PhoneManager::RegisterListener() method.
 */
enum class ListenType {
   SERVICE_STATE = 0x0001,   /**< Listen for changes to network service state */
   SIGNAL_STRENGTH = 0x0002, /**< Listen for changes to the network signal strength */
};

/**
 * @brief A listener class for monitoring changes in specific telephony states on the device,
 * including service state and signal strength.
 * Override the methods for the state that you wish to receive updates for.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 */
class IPhoneListener {
public:
   /**
    * This function is called when device service state changes.
    *
    * @param [out] phone    Pointer to existing IPhone instance created by Phone Manager
    * @param [out] state    @ref ServiceState
    */
   virtual void onServiceStateChanged(std::shared_ptr<IPhone> phone, ServiceState state) {
   }

   /**
    * This function is called when network signal strength changes.
    *
    * @param [out] phone              Pointer to existing IPhone instance created by Phone Manager
    * @param [out] signalStrength     Pointer to signal strength object
    */
   virtual void onSignalStrengthChanged(std::shared_ptr<IPhone> phone,
                                        std::shared_ptr<SignalStrength> signalStrength) {
   }

   virtual ~IPhoneListener() {
   }
};
/** @} */ /* end_addtogroup telematics_phone */

}  // End of namespace tel

}  // End namespace telux

#endif  // PHONELISTENER_HPP
