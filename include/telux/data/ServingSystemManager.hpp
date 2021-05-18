/*
 *  Copyright (c) 2020-2021 The Linux Foundation. All rights reserved.
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
 * @file       ServingSystemManager.hpp
 *
 * @brief      Serving System Manager class provides the interface to access network and modem
 *             low level services.
 */

#ifndef DATASERVINGSYSTEMMANAGER_HPP
#define DATASERVINGSYSTEMMANAGER_HPP

#include <future>
#include <memory>

#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

//Forward Declaration
class IServingSystemListener;

/**
 * @brief Dedicated Radio Bearer (DRB) status.
 */
enum class DrbStatus {
    ACTIVE  ,   /**< At least one of the physical links across all PDNs is UP */
    DORMANT ,   /**< All the Physlinks across all PDNs are DOWN */
    UNKNOWN ,   /**< No PDN is active */
};

/**
 * @brief Roaming Type.
 */
enum class RoamingType {
    UNKNOWN       ,      /**< Device roaming mode is unknown */
    DOMESTIC      ,      /**< Device is in Domestic roaming network            */
    INTERNATIONAL ,      /**< Device is in International roaming network       */
};

/**
 * @brief Roaming Status
 */
struct RoamingStatus {
   bool isRoaming;          /**< True: Roaming on, False: Roaming off                 */
   RoamingType type;        /**< International/Domestic. Valid only if roaming is on  */
};

/**
 * @brief Data Service State. Indicates whether data service is ready to setup a data call or not.
 */
enum class DataServiceState {
    UNKNOWN        ,        /**< Service State not available */
    IN_SERVICE     ,        /**< Service Available           */
    OUT_OF_SERVICE ,        /**< Service Not Available       */
};

/**
 * @brief Data Network RATs.
 */
enum class NetworkRat {
    UNKNOWN    ,    /**< UNKNOWN   */
    CDMA_1X    ,    /**< CDMA_1X   */
    CDMA_EVDO  ,    /**< CDMA_EVDO */
    GSM        ,    /**< GSM       */
    WCDMA      ,    /**< WCDMA     */
    LTE        ,    /**< LTE       */
    TDSCDMA    ,    /**< TDSCDMA   */
    NR5G       ,    /**< NR5G      */
};

/**
 * @brief Data Service Status Info.
 */
struct ServiceStatus {
    DataServiceState serviceState;
    NetworkRat networkRat;
};

/**
 * This function is called in response to requestServiceStatus API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] serviceStatus       Current service status @ref telux::data::ServiceStatus
 * @param [in] error               Return code for whether the operation succeeded or failed.
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to change
 *          and could break backwards compatibility.
 */
using RequestServiceStatusResponseCb
    = std::function<void(ServiceStatus serviceStatus, telux::common::ErrorCode error)>;

/**
 * This function is called in response to requestRoamingStatus API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] roamingStatus       Current roaming status @ref telux::data::RoamingStatus
 * @param [in] error               Return code for whether the operation succeeded or failed.
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to change
 *          and could break backwards compatibility.
*/
using RequestRoamingStatusResponseCb
    = std::function<void(RoamingStatus roamingStatus, telux::common::ErrorCode error)>;

/**
 * @brief Serving System Manager class provides APIs related to the serving system for data
 *        functionality. For example, ability to query or be notified about the state of
 *        the platform's WWAN PS data serving information
 */
class IServingSystemManager {
public:
    /**
     * Checks the status of serving manager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If serving manager object is ready for service.
     *          SERVICE_UNAVAILABLE  -  If serving manager object is temporarily unavailable.
     *          SERVICE_FAILED       -  If serving manager object encountered an irrecoverable
     *                                  failure.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Get the dedicated radio bearer (DRB) status
     *
     * @returns current DrbStatus @ref DrbStatus.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual DrbStatus getDrbStatus() = 0;

    /**
     * Queries the current serving network status
     *
     * @param [in] callback          callback to get response for requestServiceStatus
     *
     * @returns Status of requestServiceStatus i.e. success or suitable status code.
     *          if requestServiceStatus returns failure, callback will not be invoked.
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestServiceStatus(RequestServiceStatusResponseCb callback) = 0;

    /**
     * Queries the current roaming status
     *
     * @param [in] callback          callback to get response for requestRoamingStatus
     *
     * @returns Status of requestRoamingStatus i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestRoamingStatus(RequestRoamingStatusResponseCb callback) = 0;

   /**
    * Register a listener for specific updates from serving system.
    *
    * @param [in] listener     Pointer of IServingSystemListener object that
    *                          processes the notification
    *
    * @returns Status of registerListener i.e success or suitable status code.
    */
   virtual telux::common::Status registerListener(std::weak_ptr<IServingSystemListener> listener)
      = 0;

   /**
    * Deregister the previously added listener.
    *
    * @param [in] listener     Previously registered IServingSystemListener that
    *                          needs to be removed
    *
    * @returns Status of removeListener i.e. success or suitable status code
    */
   virtual telux::common::Status deregisterListener(std::weak_ptr<IServingSystemListener> listener)
      = 0;

   /**
    * Get associated slot id for the Serving System Manager.
    *
    * @returns SlotId
    *
    */
   virtual SlotId getSlotId() = 0;

   /**
    * Destructor of IServingSystemManager
    */
   virtual ~IServingSystemManager() {};
};

/**
 * @brief Listener class for data serving system change notification.
 *
 * The listener method can be invoked from multiple different threads.
 * Client needs to make sure that implementation is thread-safe.
 */
class IServingSystemListener {
public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {};

   /**
    * This function is called whenever Drb status is changed.
    *
    * @param [in] status      @ref DrbStatus
    */
   virtual void onDrbStatusChanged(DrbStatus status) {};

   /**
    * This function is called whenever service state is changed.
    *
    * @param [in] status      @ref ServiceStatus
    */
   virtual void onServiceStateChanged(ServiceStatus status) {};

   /**
    * This function is called whenever roaming status is changed.
    *
    * @param [in] status      @ref RoamingStatus
    */
   virtual void onRoamingStatusChanged(RoamingStatus status) {};

   /**
    * Destructor of IServingSystemListener
    */
   virtual ~IServingSystemListener() {};
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif
