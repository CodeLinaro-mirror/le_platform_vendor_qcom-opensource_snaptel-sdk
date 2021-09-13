/*
 *  Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
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
 * @file       NetworkSelectionManager.hpp
 *
 * @brief      Network Selection Manager class provides the interface to get and
 *             set network selection mode (Manual or Automatic), scan available
 *             networks and set and get preferred networks list.
 */

#ifndef NETWORKSELECTIONMANAGER_HPP
#define NETWORKSELECTIONMANAGER_HPP

#include <bitset>
#include <future>
#include <memory>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

namespace telux {
namespace tel {

// Forward declarations
class INetworkSelectionListener;
class OperatorInfo;

/** @addtogroup telematics_network_selection
* @{ */

/**
 * Defines network RAT type for preferred networks. Each value represents
 * corresponding bit for RatMask bitset.
 */
enum RatType {
   UMTS = 15, /**< UMTS */
   LTE = 14,  /**< LTE */
   GSM = 7,   /**< GSM */
   NR5G = 11, /**< NR5G */
};

/** @} */ /* end_addtogroup telematics_network_selection */

/**
 * 16 bit mask that denotes which of the radio access technologies defined in
 * RatType enum are used for preferred networks.
 */
using RatMask = std::bitset<16>;

/** @addtogroup telematics_network_selection
* @{ */

/**
 * Defines the preferred network information
 */
struct PreferredNetworkInfo {
   uint16_t mcc;    /**< mobile country code */
   uint16_t mnc;    /**< mobile network code */
   RatMask ratMask; /**< bit mask denotes which of the radio access technologies are
                         set */
};

/**
 * Defines the status of the network scan results
 */
enum class NetworkScanStatus {
   COMPLETE = 0,         /**<  Network scan is successful and completed. No more indications are
                               expected for the scan request */
   PARTIAL = 1,          /**<  Network scan results are partial, further results are expected in
                               subsequent indication */
   FAILED = 2,           /**<  Network scan failed either due to radio link failure or it is
                               aborted or due to problem in performing incremental search. */
};

/**
 * Defines network selection mode
 */
enum class NetworkSelectionMode {
   UNKNOWN = -1,  /**< Unknown */
   AUTOMATIC = 0, /**< Device registers according to provisioned mcc and mnc */
   MANUAL = 1     /**< Device registers to specified network as per provided mcc and mnc */
};

/**
 * Defines in-use status of network operator
 */
enum class InUseStatus {
   UNKNOWN,         /**< Unknown */
   CURRENT_SERVING, /**< Current serving */
   AVAILABLE        /**< Available */
};

/**
 * Defines roaming status of network operator
 */
enum class RoamingStatus {
   UNKNOWN, /**< Unknown */
   HOME,    /**< Home */
   ROAM     /**< Roaming */
};

/**
 * Defines forbidden status of network operator
 */
enum class ForbiddenStatus {
   UNKNOWN,      /**< Unknown */
   FORBIDDEN,    /**< Forbidden */
   NOT_FORBIDDEN /**< Not forbidden */
};

/**
 * Defines preferred status of network operator
 */
enum class PreferredStatus {
   UNKNOWN,      /**< Unknown */
   PREFERRED,    /**< Preferred */
   NOT_PREFERRED /**< Not preferred */
};

/**
 * Defines status of network operator
 */
struct OperatorStatus {
   InUseStatus inUse = InUseStatus::UNKNOWN;       /**< In-use status of network operator */
   RoamingStatus roaming = RoamingStatus::UNKNOWN; /**< Roaming status of network operator */
   ForbiddenStatus forbidden
      = ForbiddenStatus::UNKNOWN; /**< Forbidden status of network operator */
   PreferredStatus preferred
      = PreferredStatus::UNKNOWN; /**< Preferred status of network operator */
};

/**
 * Defines Network scan type
 */
enum class NetworkScanType {
    CURRENT_RAT_PREFERENCE = 1,      /**< Network scan based on current RAT preference */
    USER_SPECIFIED_RAT,              /**< Network scan based on user specified RAT(s) */
    ALL_RATS                         /**< Network scan on GSM/WCDMA/LTE/NR5G */
};

/**
 * Defines Network scan information
 */
struct NetworkScanInfo {
    NetworkScanType scanType;  /**< Network scan type */
    RatMask ratMask;           /**< Bit mask denotes which of the radio access technologies are
                                    set. ratMask is valid/set only when scanType is provided as
                                    NetworkScanType::USER_SPECIFIED_RAT */
};

/** @} */ /* end_addtogroup telematics_network_selection */

/**
 * This function is called with the response to requestNetworkSelectionMode API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] mode       @ref NetworkSelectionMode
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not
 *                        @ref ErrorCode
 */
using SelectionModeResponseCallback
   = std::function<void(NetworkSelectionMode mode, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestPreferredNetworks API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] info         3GPP preferred networks list i.e PLMN list.
 * @param [in] staticInfo   Static 3GPP preferred networks list i.e OPLMN list.
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not.
 *                          @ref ErrorCode
 */
using PreferredNetworksCallback = std::function<void(std::vector<PreferredNetworkInfo> info,
                                                     std::vector<PreferredNetworkInfo> staticInfo,
                                                     telux::common::ErrorCode error)>;

/**
 * This function is called with the response to performNetworkScan API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] operatorInfos   Operators info with details of network operator
 *                             name, MCC, MNC and status.
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not.
 *                             @ref ErrorCode
 */
using NetworkScanCallback
   = std::function<void(std::vector<OperatorInfo> operatorInfos, telux::common::ErrorCode error)>;

/** @addtogroup telematics_network_selection
* @{ */

/**
 * @brief   Network Selection Manager class provides the interface to get and set
 *          network selection mode, preferred network list and scan available networks.
 */
class INetworkSelectionManager {
public:
   /**
    * Checks the status of network subsystem and returns the result.
    *
    * @returns True if network subsystem is ready for service otherwise false.
    *
    * @deprecated Use INetworkSelectionManager::getServiceStatus() API.
    */
   virtual bool isSubsystemReady() = 0;

   /**
    * Wait for network subsystem to be ready.
    *
    * @returns  A future that caller can wait on to be notified when network
    *           subsystem is ready.
    *
    * @deprecated Use InitResponseCb in PhoneFactory::getNetworkSelectionManager instead, to
    *             get notified about subsystem readiness.
    */
   virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * This status indicates whether the INetworkSelectionManager object is in a usable state.
    *
    * @returns SERVICE_AVAILABLE    -  If Serving System manager is ready for service.
    *          SERVICE_UNAVAILABLE  -  If Serving System manager is temporarily unavailable.
    *          SERVICE_FAILED       -  If Serving System manager encountered an irrecoverable
    *                                  failure.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

   /**
    * Get current network selection mode (i.e Manual or Automatic) asynchronously.
    *
    * @param [in] callback    Callback function to get the response of get
    *                         network selection mode request.
    *
    * @returns Status of requestNetworkSelectionMode i.e. success or suitable error code.
    */
   virtual telux::common::Status requestNetworkSelectionMode(SelectionModeResponseCallback callback)
      = 0;

   /**
    * Set current network selection mode and receive the response asynchronously.
    *
    * @param [in] selectMode    Selection mode for a network i.e. automatic or manual.
    *                           If selection mode is automatic then MCC and MNC are ignored.
    *                           If it is manual, client has to explicitly pass MCC and MNC
    *                           as arguments.
    * @param [in] callback      Optional callback function to get the response of
    *                           set network selection mode request.
    * @param [in] mcc           Mobile Country Code (Applicable only for MANUAL selection mode).
    * @param [in] mnc           Mobile Network Code (Applicable only for MANUAL selection mode).
    *
    * @returns  Status of setNetworkSelectionMode i.e. success or suitable error code.
    */
   virtual telux::common::Status setNetworkSelectionMode(NetworkSelectionMode selectMode,
                                                         std::string mcc, std::string mnc,
                                                         common::ResponseCallback callback
                                                         = nullptr)
      = 0;

   /**
    * Get 3GPP preferred network list and static 3GPP preferred network list
    * asynchronously. Higher priority networks appear first in the list. The networks
    * that appear in the 3GPP Preferred Networks list get higher priority than the
    * networks in the static 3GPP preferred networks list.
    *
    * @param [in] callback    Callback function to get the response of get preferred
    *                         networks request.
    *
    * @returns Status of requestPreferredNetworks i.e. success or suitable error code.
    */
   virtual telux::common::Status requestPreferredNetworks(PreferredNetworksCallback callback) = 0;

   /**
    * Set 3GPP preferred network list and receive the response asynchronously. It
    * overrides the existing preferred network list. The preferred network list
    * affects network selection selection when automatic registration is performed
    * by the device. Higher priority networks should appear first in the list.
    *
    * @param [in] preferredNetworksInfo   List of 3GPP preferred networks.
    * @param [in] clearPrevious           If flag is false then new 3GPP preferred
    *                                     network list is appended to existing
    *                                     preferred network list. If flag
    *                                     is true then old list is flushed and
    *                                     new 3GPP preferred network list is added.
    * @param [in] callback                Callback function to get the response
    *                                     of set preferred network list request.
    *
    * @returns Status of setPreferredNetworks i.e. success or suitable error code.
    */

   virtual telux::common::Status
      setPreferredNetworks(std::vector<PreferredNetworkInfo> preferredNetworksInfo,
                           bool clearPrevious, common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Perform the network scan and returns a list of available networks.
    *
    * @param [in] callback    Callback function to get the response of perform
    *                         network scan request
    *
    * @returns Status of performNetworkScan i.e. success or suitable error code.
    *
    * @deprecated Use INetworkSelectionManager::performNetworkScan(
    *     common::ResponseCallback callback) API instead
    */
   virtual telux::common::Status performNetworkScan(NetworkScanCallback callback) = 0;

   /**
    * Perform the network scan. The available networks list is returned incrementally as they
    * become available, without waiting for the entire scan to complete through the
    * indication API (INetworkSelectionListener::onNetworkScanResults).
    * The scan status in indication will indicate if its a partial result or complete result.
    *
    * @param [in] info        Provides network scan type and if the network scan type is user
    *                         prefered RAT, includes RAT(s) information. @ref NetworkScanInfo
    * @param [in] callback    Callback function to get the response of network scan request
    *
    * @returns Status of performNetworkScan i.e. success or suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibilty.
    *
    */
   virtual telux::common::Status performNetworkScan(NetworkScanInfo info,
      common::ResponseCallback callback = nullptr) = 0;

   /**
    * Register a listener for specific updates from network access service.
    *
    * @param [in] listener    Pointer of INetworkSelectionListener object that
    *                         processes the notification
    *
    * @returns Status of registerListener i.e success or suitable status code.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<INetworkSelectionListener> listener)
      = 0;

   /**
    * Deregister the previously added listener.
    *
    * @param [in] listener    Previously registered INetworkSelectionListener
    *                         that needs to be removed
    *
    * @returns Status of removeListener success or suitable status code
    */
   virtual telux::common::Status
      deregisterListener(std::weak_ptr<INetworkSelectionListener> listener)
      = 0;

   virtual ~INetworkSelectionManager(){};
};

/**
 * Operator Info class provides operator name, MCC, MNC and network status.
 */
class OperatorInfo {
public:
   OperatorInfo(std::string networkName, std::string mcc, std::string mnc,
                OperatorStatus operatorStatus);

   OperatorInfo(std::string networkName, std::string mcc, std::string mnc, RadioTechnology rat,
      OperatorStatus operatorStatus);

   /**
    * Get Operator name or description
    *
    * @returns Operator name.
    */
   std::string getName();

   /**
    * Get mcc from the operator numeric.
    *
    * @returns MCC.
    */
   std::string getMcc();

   /**
    * Get mnc from operator numeric.
    *
    * @returns MNC.
    */
   std::string getMnc();

   /**
    * Get radio access technology.
    *
    * @returns Radio access technology(RAT) @ref RadioTechnology.
    */
   RadioTechnology getRat();

   /**
    * Get status of operator.
    *
    * @returns status of the operator @ref OperatorStatus.
    */
   OperatorStatus getStatus();

private:
   std::string networkName_;
   std::string mcc_;
   std::string mnc_;
   RadioTechnology rat_;
   OperatorStatus operatorStatus_;
};

/**
 * @brief Listener class for getting network selection mode change notification
 *
 *        The methods in listener can be invoked from multiple different threads.
 *        Client needs to make sure that implementation is thread-safe.
 */
class INetworkSelectionListener : public common::IServiceStatusListener{
public:
   /**
    * This function is called whenever network selection mode is changed.
    *
    * @param [in] mode    Network selection mode @ref NetworkSelectionMode
    */
   virtual void onSelectionModeChanged(NetworkSelectionMode mode) {
   }

   /**
    * This function is called in response to performNetworkScan API.
    * This API will be invoked multiple times in case of partial network scan results.
    * In case of network scan failure and network scan completed this API will not be invoked
    * further.
    *
    * @param [in] scanStatus      Status of the network scan results @ref NetworkScanStatus
    * @param [in] operatorInfos   Operators info with details of network operator name, MCC,
    *                             MNC, etc. In case of partial network scan results, the
    *                             operator info will have the information of the new set of
    *                             operator info along with previous partial network scan results.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibilty.
    *
    */
   virtual void onNetworkScanResults(NetworkScanStatus scanStatus,
      std::vector<telux::tel::OperatorInfo> operatorInfos) {
   }

   /**
    * Destructor of INetworkSelectionListener
    */
   virtual ~INetworkSelectionListener() {
   }
};

/** @} */ /* end_addtogroup telematics_network_selection */

}  // end of namespace tel
}  // end of namespace telux

#endif
