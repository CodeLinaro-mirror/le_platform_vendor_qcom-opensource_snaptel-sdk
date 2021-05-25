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

/**
 * @file       PhoneFactory.hpp
 * @brief      PhoneFactory is the central factory to create all Telephony SDK
 *             classes and services.
 */

#ifndef PHONEFACTORY_HPP
#define PHONEFACTORY_HPP

#include <map>
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

namespace telux {

namespace tel {

/** @addtogroup telematics_phone_factory
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
   std::shared_ptr<IPhoneManager> getPhoneManager();

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
   std::shared_ptr<ISmsManager> getSmsManager(int phoneId = DEFAULT_PHONE_ID,
      telux::common::InitResponseCb callback = nullptr);

   /**
    * Get Call Manager instance to determine state of active calls and perform
    * other functions like dial, conference, swap call.
    *
    * @returns Pointer of ICallManager object.
    */
   std::shared_ptr<ICallManager> getCallManager();

   /**
    * Get Card Manager instance to handle services such as transmitting APDU,
    * SIM IO and more.
    *
    * @returns Pointer of ICardManager object.
    */
   std::shared_ptr<ICardManager> getCardManager();

   /**
    * Get Sap Card Manager instance associated with the provided slot id. This
    * object will handle services in SAP mode such as APDU, SIM Power On/Off
    * and SIM reset.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    *
    * @returns Pointer of ISapCardManager object.
    */
   std::shared_ptr<ISapCardManager> getSapCardManager(int slotId = DEFAULT_SLOT_ID);

   /**
    * Get Subscription Manager instance to get device subscription details
    *
    * @returns Pointer of ISubscriptionManager object.
    */
   std::shared_ptr<ISubscriptionManager> getSubscriptionManager();

   /**
    * Get Serving System Manager instance to get and set preferred network type.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IServingSystemManager object.
    */
   std::shared_ptr<IServingSystemManager> getServingSystemManager(int slotId = DEFAULT_SLOT_ID,
      telux::common::InitResponseCb callback = nullptr);

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
   std::shared_ptr<INetworkSelectionManager> getNetworkSelectionManager(
      int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr);

   /**
    * Get Remote SIM Manager instance to handle services like exchanging APDU,
    * SIM Power On/Off, etc.
    *
    * @param [in] slotId    Unique identifier for the SIM slot
    *
    * @returns Pointer of IRemoteSimManager object.
    */
   std::shared_ptr<IRemoteSimManager> getRemoteSimManager(int slotId = DEFAULT_SLOT_ID);

   /**
    * Get Multi SIM Manager instance to handle operations like high capabilty
    * switch.
    *
    * @returns Pointer of IMultiSimManager object.
    */
   std::shared_ptr<IMultiSimManager> getMultiSimManager();

   /**
    * Get CellBroadcast Manager instance for Slot ID. CellBroadcast manager used to receive
    * broacast messages and configure broadcast messages.
    *
    * @param [in] SlotId   @ref telux::common::SlotId
    *
    * @returns Pointer of ICellBroadcastManager object or nullptr in case of failure.
    */
   std::shared_ptr<ICellBroadcastManager> getCellBroadcastManager(SlotId slotId = DEFAULT_SLOT_ID);

   /**
    * Get SimProfileManager. SimProfileManager is a primary interface for remote
    * eUICC(eSIM) provisioning and local profile assistance.
    *
    * @returns Pointer of ISimProfileManager object or nullptr in case of failure.
    *
    */
   std::shared_ptr<ISimProfileManager> getSimProfileManager();

   /**
    * Get Ims Settings Manager instance to handle IMS service enable configuation parameters like
    * enable/disable voIMS.
    *
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IImsSettingsManager object.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   std::shared_ptr<IImsSettingsManager> getImsSettingsManager(
       telux::common::InitResponseCb  callback = nullptr);

   /**
    * Get HttpTransactionManager instance to handle HTTP related requests
    * from the modem for SIM profile update related operations.
    *
    * @param[in] callback   Optional callback pointer to get the response of the manager
    *                       initialisation.
    *
    * @returns Pointer of IHttpTransactionManager object or nullptr in case of failure.
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    *
    */
   std::shared_ptr<IHttpTransactionManager> getHttpTransactionManager(
      telux::common::InitResponseCb  callback = nullptr);

   /**
    * Get IMS Serving System Manager instance to query IMS registration status
    *
    * @returns Pointer of IImsServingSystemManager object or nullptr in case of failure.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and
    *          could break backwards compatibility.
    */
   std::shared_ptr<IImsServingSystemManager> getImsServingSystemManager(SlotId slotId,
      telux::common::InitResponseCb callback = nullptr);

private:
   std::shared_ptr<IPhoneManager> phoneManager_;
   std::shared_ptr<ICallManager> callManager_;
   std::shared_ptr<ICardManager> cardManager_;
   std::shared_ptr<ISubscriptionManager> subscriptionManager_;
   std::shared_ptr<IMultiSimManager> multiSimManager_;
   std::shared_ptr<ISimProfileManager> simProfileManager_;
   std::shared_ptr<IImsSettingsManager> imsSettingsManager_;
   std::shared_ptr<IHttpTransactionManager> httpTransactionManager_;
   std::map<SlotId, std::shared_ptr<IImsServingSystemManager>> imsServSysManagerMap_;
   std::map<int, std::shared_ptr<ISmsManager>> smsMap_;
   std::map<int, std::shared_ptr<ICellBroadcastManager>> cbMap_;
   std::map<int, std::shared_ptr<IServingSystemManager>> servingSystemManagerMap_;
   std::map<int, std::shared_ptr<INetworkSelectionManager>> networkSelectionManagerMap_;
   std::map<int, std::shared_ptr<ISapCardManager>> sapCardManagerMap_;
   std::map<int, std::shared_ptr<IRemoteSimManager>> remoteSimManagerMap_;
   std::vector<telux::common::InitResponseCb> imssCallbacks_;
   telux::common::ServiceStatus imssInitStatus_;
   std::vector<telux::common::InitResponseCb> httpTransactionCallbacks_;
   std::map<int, std::vector<telux::common::InitResponseCb>> servingSysMgrCallbacks_;
   std::map<int, std::vector<telux::common::InitResponseCb>> networkSelMgrCallbacks_;
   telux::common::ServiceStatus httpTransactionInitStatus_;
   std::map<int, telux::common::ServiceStatus> servingSysMgrInitStatus_;
   std::map<int, telux::common::ServiceStatus> networkSelMgrInitStatus_;
   std::map<int, std::vector<telux::common::InitResponseCb>> smsMgrCallbacks_;
   std::map<int, telux::common::ServiceStatus> smsMgrInitStatus_;

   void onImsSettingsManagerResponse(telux::common::ServiceStatus status);
   void onHttpTransactionManagerResponse(telux::common::ServiceStatus status);
   void onServingSystemInitResponse(int slotId, telux::common::ServiceStatus status);
   void onNetworkSelectionInitResponse(int slotId, telux::common::ServiceStatus status);
   std::map<SlotId, std::vector<telux::common::InitResponseCb>> imsServSysCallbacks_;
   telux::common::ServiceStatus imsServSysInitStatus_;
   void initImsServSysManagerNotifier(telux::common::ServiceStatus status, SlotId slotId);
   void onSmsMgrInitResponse(int phoneId, telux::common::ServiceStatus status);

   PhoneFactory();
   ~PhoneFactory();
   PhoneFactory(const PhoneFactory &) = delete;
   PhoneFactory &operator=(const PhoneFactory &) = delete;
   std::recursive_mutex mutex_;
};

/** @} */ /* end_addtogroup telematics_phone_factory */

}  // End of namespace tel

}  // End of namespace telux

#endif  // PHONEFACTORY_HPP
