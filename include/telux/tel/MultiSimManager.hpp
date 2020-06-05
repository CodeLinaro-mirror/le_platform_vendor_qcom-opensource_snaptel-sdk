/*
 *  Copyright (c) 2020 The Linux Foundation. All rights reserved.
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
 * @file       MultiSimManager.hpp
 * @brief      MultiSimManager allows operations pertaining to devices which have
 *             more than one SIM/UICC card. For example allow high capability
 *             switch on either of the slot which is associated with SIM. It also
 *             allows clients to register for notification of system events like
 *             high capability change.
 *
 */

#ifndef MULTISIMMANAGER_HPP
#define MULTISIMMANAGER_HPP

#include <future>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace tel {

// Forward declaration
class IMultiSimListener;

/**
 * This function is called in the response to requestHighCapability API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] slotId       SIM corresponding to slot identifier has high capability.
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not @ref ErrorCode
 */
using HighCapabilityCallback
   = std::function<void(int slotId, telux::common::ErrorCode error)>;

/** @addtogroup telematics_multi_sim
 * @{ */

/**
 *@brief       MultiSimManager allows to perform operation pertaining to devices which have
 *             more than one SIM/UICC card. Clients should check if the subsystem
 *             is ready before invoking any of the APIs as follows
 *
 *             bool isReady = MultiSimManager->isSubsystemReady();
 *
 */
class IMultiSimManager {
public:
   /**
    * Checks the status of Multi SIM subsystem and returns the result.
    *
    * @returns If true MultiSimManager is ready.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual bool isSubsystemReady() = 0;

   /**
    * Wait for Multi SIM subsystem to be ready.
    *
    * @returns A future that caller can wait on to be notified when Multi SIM
    * subsystem is ready.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * Get SIM slot count. The count can be used to determine whether the device supports
    * multi SIM.
    *
    * @param [out] count    Slot count.
    *
    * @returns Status of getSlotCount i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status getSlotCount(int &count) = 0;

   /**
    * Request to find out which SIM/slot is allowed to use advance Radio Technology like
    * 5G at a time. For example SIM/slot with high capability may allowed to use RAT
    * capabilities like 5G/4G/3G/2G while the SIM/slot with low capability may be allowed
    * to use RAT capabilities like 4G/2G.
    *
    * @param [in] callback    Callback function to get the response of request
    *                         high capability.
    *
    * @returns Status of requestHighCapability i.e. success or suitable
    *          error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status requestHighCapability(HighCapabilityCallback callback) = 0;

   /**
    * Set SIM/slot with high capability asynchronously. On dual SIM devices, only one SIM
    * may be allowed to use advanced Radio technology like 5G at a time. This API sets
    * the SIM/slot that should be allowed the highest RAT capability. The other SIM/slot
    * will be given lower RAT capabilities. For example, SIM in slot1 will be allowed
    * 2G/3G/4G/5G and the SIM in slot2 will be allowed only 2G/4G.
    *
    * @param [in] slotId       Slot set with high capablity.
    *
    * @param [in] callback     Callback function to get the response of set
    *                          high capability request.
    *
    * @returns Status of setHighCapability i.e. success or suitable
    *          error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status setHighCapability(int slotId,
      common::ResponseCallback callback = nullptr) = 0;

   /**
    * Register a listener for specific events in the Multi SIM subsystem.
    *
    * @param [in] listener  Pointer to IMultiSimListener object that processes the
    *                       notification
    *
    * @returns Status of registerListener i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<IMultiSimListener> listener) = 0;

   /**
    * Deregister the previously added listener.
    *
    * @param [in] listener    Pointer to IMultiSimListener object that needs to be
    *                         deregistered.
    *
    * @returns Status of deregisterListener i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated. It is subject to change
    *          and could break backwards compatibility.
    */
   virtual telux::common::Status deregisterListener(std::weak_ptr<IMultiSimListener> listener) = 0;

   virtual ~IMultiSimManager(){};
};

/**
 * @brief Listener class for getting high capability change notification.
 *        The listener method can be invoked from multiple different threads.
 *        Client needs to make sure that implementation is thread-safe.
 */
class IMultiSimListener : public common::IServiceStatusListener{
public:

   /**
    * This function is called whenever there is change in high capability for SIM/slot.
    *
    * @param [in] slotId       SIM corresponding to slot identifier has high capability now.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change
    *          and could break backwards compatibility.
    */
   virtual void onHighCapabilityChanged(int slotId) {
   }

   /**
    * Destructor of IMultiSimListener
    */
   virtual ~IMultiSimListener() {
   }
};

/** @} */ /* end_addtogroup telematics_multi_sim */
}
}

#endif
