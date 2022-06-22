/*
 *  Copyright (c) 2017-2021, The Linux Foundation. All rights reserved.
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


/**
 * @file       PhoneFactory.hpp
 * @brief      PhoneFactory is the central factory to create all Telephony SDK
 *             classes and services.
 */

#ifndef PHONEFACTORY_HPP
#define PHONEFACTORY_HPP

#include <memory>

#include <telux/tel/CallManager.hpp>
#include <telux/tel/CellBroadcastManager.hpp>
#include <telux/tel/CardManager.hpp>
#include <telux/tel/NetworkSelectionManager.hpp>
#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneManager.hpp>
#include <telux/tel/RemoteSimManager.hpp>
#include <telux/tel/SapCardManager.hpp>
#include <telux/tel/ServingSystemManager.hpp>
#include <telux/tel/SmsManager.hpp>
#include <telux/tel/SubscriptionManager.hpp>
#include <telux/tel/MultiSimManager.hpp>
#include <telux/tel/SimProfileManager.hpp>
#include <telux/tel/ImsSettingsManager.hpp>
#include <telux/tel/HttpTransactionManager.hpp>
#include <telux/tel/ImsServingSystemManager.hpp>
#include <telux/tel/SuppServicesManager.hpp>

namespace telux {

namespace tel {

/** @addtogroup telematics_phone
 * @{ */

/**
 * @brief PhoneFactory is the central factory to create all Telephony SDK Classes
 *        and services
 */
class PhoneFactory {
 public:
   /**
    * Get Phone Factory instance.
    */
   static PhoneFactory &getInstance();

   /**
    * Get Phone Manager instance. Phone Manager is the main entry point into the
    * telephony subsystem.
    *
    * @returns Pointer of IPhoneManager object.
    */
   virtual std::shared_ptr<IPhoneManager> getPhoneManager() = 0;

   /**
    * Get SMS Manager instance for Phone ID. SMSManager used to send and receive
    * SMS messages.
    *
    * @param [in] phoneId   Unique identifier for the phone
    * @param [in] callback  Optional callback pointer to get response of SMS Manager initialization.
    *                       It will be invoked when initialization is either succeeded or failed.
    *                       In case of failure response, the provided SMS Manager object will no
    *                       more be a valid object.
    *
    * @returns Pointer of ISmsManager object or nullptr in case of failure.
    */
   virtual std::shared_ptr<ISmsManager> getSmsManager(int phoneId = DEFAULT_PHONE_ID,
      telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Call Manager instance to determine state of active calls and perform
    * other functions like dial, conference, swap call.
    *
    * @returns Pointer of ICallManager object.
    */
   virtual std::shared_ptr<ICallManager> getCallManager() = 0;

   /**
    * Get Card Manager instance to handle services such as transmitting APDU,
    * SIM IO and more.
    *
    * @returns Pointer of ICardManager object.
    */
   virtual std::shared_ptr<ICardManager> getCardManager() = 0;

   /**
    * Get Sap Card Manager instance associated with the provided slot id. This
    * object will handle services in SAP mode such as APDU, SIM Power On/Off
    * and SIM reset.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_SAP permission to
    * invoke this API successfully.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    *
    * @returns Pointer of ISapCardManager object.
    */
   virtual std::shared_ptr<ISapCardManager> getSapCardManager(int slotId = DEFAULT_SLOT_ID) = 0;

   /**
    * Get Subscription Manager instance to get device subscription details
    *
    * @returns Pointer of ISubscriptionManager object.
    */
   virtual std::shared_ptr<ISubscriptionManager> getSubscriptionManager() = 0;

   /**
    * Get Serving System Manager instance to get and set preferred network type.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IServingSystemManager object.
    */
   virtual std::shared_ptr<IServingSystemManager> getServingSystemManager(
      int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Network Selection Manager instance to get and set selection mode, get
    * and set preferred networks and scan available networks.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of INetworkSelectionManager object.
    */
   virtual std::shared_ptr<INetworkSelectionManager> getNetworkSelectionManager(
      int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr) = 0;

   /**
    * Get Remote SIM Manager instance to handle services like exchanging APDU,
    * SIM Power On/Off, etc.
    *
    * On platforms with access control enabled, caller needs to have TELUX_TEL_REMOTE_SIM permission
    * to invoke this API successfully.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    *
    * @returns Pointer of IRemoteSimManager object.
    */
   virtual std::shared_ptr<IRemoteSimManager> getRemoteSimManager(int slotId = DEFAULT_SLOT_ID) = 0;

   /**
    * Get Multi SIM Manager instance to handle operations like high capabilty
    * switch.
    *
    * @returns Pointer of IMultiSimManager object.
    */
   virtual std::shared_ptr<IMultiSimManager> getMultiSimManager() = 0;

   /**
    * Get CellBroadcast Manager instance for Slot ID. CellBroadcast manager used to receive
    * broacast messages and configure broadcast messages.
    *
    * @param [in] slotId   @ref telux::common::SlotId
    *
    * @returns Pointer of ICellBroadcastManager object or nullptr in case of failure.
    */
   virtual std::shared_ptr<ICellBroadcastManager> getCellBroadcastManager(
      SlotId slotId = DEFAULT_SLOT_ID) = 0;

   /**
    * Get SimProfileManager. SimProfileManager is a primary interface for remote
    * eUICC(eSIM) provisioning and local profile assistance.
    *
    * @returns Pointer of ISimProfileManager object or nullptr in case of failure.
    *
    */
   virtual std::shared_ptr<ISimProfileManager> getSimProfileManager() = 0;

   /**
    * Get Ims Settings Manager instance to handle IMS service enable configuation parameters like
    * enable/disable voIMS.
    *
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IImsSettingsManager object.
    *
    */
   virtual std::shared_ptr<IImsSettingsManager> getImsSettingsManager(
       telux::common::InitResponseCb  callback = nullptr) = 0;

   /**
    * Get HttpTransactionManager instance to handle HTTP related requests
    * from the modem for SIM profile update related operations.
    *
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IHttpTransactionManager object or nullptr in case of failure.
    *
    */
   virtual std::shared_ptr<IHttpTransactionManager> getHttpTransactionManager(
      telux::common::InitResponseCb  callback = nullptr) = 0;

   /**
    * Get IMS Serving System Manager instance to query IMS registration status
    *
    * @returns Pointer of IImsServingSystemManager object or nullptr in case of failure.
    *
    */
   virtual std::shared_ptr<IImsServingSystemManager> getImsServingSystemManager(SlotId slotId,
      telux::common::InitResponseCb callback = nullptr) = 0;

   /**
    * Get Supplementary service manager instance to set/get preference for supplementary services
    * like call waiting, call forwarding etc.
    *
    * @param [in] SlotId     @ref telux::common::SlotId
    * @param [in] callback   Optional callback pointer to get the response of the manager
    *                        initialisation.
    *
    * @returns Pointer of ISuppServicesManager object.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual std::shared_ptr<ISuppServicesManager> getSuppServicesManager(
      SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
   PhoneFactory();
   virtual ~PhoneFactory();
#endif

 private:
   PhoneFactory(const PhoneFactory &) = delete;
   PhoneFactory &operator=(const PhoneFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_phone */

}  // End of namespace tel

}  // End of namespace telux

#endif  // PHONEFACTORY_HPP
