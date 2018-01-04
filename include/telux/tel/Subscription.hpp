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
 * @file        Subscription.hpp
 * @brief       Subscription class provides the details about operator
 *              subscription pertaining to a SIM card and the network to
 *              which the SIM is connected.
 *
 * @note       Eval: This is a new API and is being evaluated. It is subject to
 *             change and could break backwards compatibility.
 */

#ifndef SUBSCRIPTION_HPP
#define SUBSCRIPTION_HPP

#include <string>

namespace telux {

namespace tel {

/** @addtogroup telematics_subscription
 * @{ */

/**
 *  @brief       Subscription returns information about network operator
 *               subscription details pertaining to a SIM card.
 *
 * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
 *         break backwards compatibility.
 */
class ISubscription {

public:
   /**
    * Retrieves the name of the carrier on which this subscription is made.
    *
    * @returns Name of the carrier.
    *
    * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
    *         break backwards compatibility.
    */
   virtual std::string getCarrierName() = 0;

   /**
    * Retrieves ISO code of the country where the subscription is made.
    *
    * @returns Country's ISO code.
    *
    * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
    *         break backwards compatibility.
    */
   virtual std::string getCountryISO() = 0;

   /**
    * Retrieves the SIM's ICCID (Integrated Chip ID) - i.e SIM Serial Number.
    *
    * @returns Integrated Chip Id.
    *
    * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
    *         break backwards compatibility.
    */
   virtual std::string getIccId() = 0;

   /**
    * Retrieves the mobile country code of the carrier to which the phone is
    * connected.
    *
    * @returns Mobile Country Code.
    *
    * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
    *         break backwards compatibility.
    */
   virtual int getMcc() = 0;

   /**
    * Retrieves the mobile network code of the carrier to which phone is
    * connected.
    *
    * @returns Mobile Network Code.
    *
    * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
    *         break backwards compatibility.
    */
   virtual int getMnc() = 0;

   /**
    * Retrieves the phone number for the SIM subscription.
    *
    * @returns PhoneNumber.
    *
    * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
    *         break backwards compatibility.
    */
   virtual std::string getPhoneNumber() = 0;

   /**
    * Retrieves SIM Slot index for the SIM pertaining to this subscription object.
    *
    * @returns SIM slotId.
    *
    * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
    *         break backwards compatibility.
    */
   virtual int getSlotId() = 0;

   /**
    * Retrieves IMSI (International Mobile Subscriber Identity) for the SIM.
    * This will have home network MCC and MNC values.
    *
    * @returns imsi.
    *
    * @note   Eval: This is a new API and is being evaluated.It is subject to change and could
    *         break backwards compatibility.
    */
   virtual std::string getImsi() = 0;

   virtual ~ISubscription(){};
};
/** @} */ /* end_addtogroup telematics_subscription */

}  // end namespace tel
}  // end namespace telux

#endif
