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
 * @file       ServingSystemManager.hpp
 *
 * @brief      Serving System Manager class provides the interface to request and set
 *             service domain preference and radio access technology mode preference for
 *             searching and registering (CS/PS domain, RAT and operation mode).
 */

#ifndef SERVINGSYSTEMMANAGER_HPP
#define SERVINGSYSTEMMANAGER_HPP

#include <bitset>
#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>

namespace telux {
namespace tel {

// Forward declaration
class IServingSystemListener;

/** @addtogroup telematics_serving_system
 * @{ */

/**
 * Defines service domain preference
 */
enum class ServiceDomainPreference {
   UNKNOWN = -1,
   CS_ONLY, /**< Circuit-switched only */
   PS_ONLY, /**< Packet-switched only */
   CS_PS,   /**< Circuit-switched and packet-switched */
};

/**
 * Defines service domain
 */
enum class ServiceDomain {
   UNKNOWN = -1,  /**< Unknown, when the information is not available */
   NO_SRV,        /**< No Service */
   CS_ONLY,       /**< Circuit-switched only */
   PS_ONLY,       /**< Packet-switched only */
   CS_PS,         /**< Circuit-switched and packet-switched */
   CAMPED,        /**< Device camped on the network according to its provisioning, but not
                       registered */
};

/**
 * Defines current serving system information
 */
struct ServingSystemInfo {
   RadioTechnology rat;    /**< Current serving RAT */
   ServiceDomain   domain; /**< Current service domain registered on system for the serving RAT */
};

/**
 * Defines the radio access technology mode preference.
 */
enum RatPrefType {
   PREF_CDMA_1X,   /**< CDMA_1X */
   PREF_CDMA_EVDO, /**< CDMA_EVDO */
   PREF_GSM,       /**< GSM */
   PREF_WCDMA,     /**< WCDMA */
   PREF_LTE,       /**< LTE */
   PREF_TDSCDMA,   /**< TDSCDMA */
   PREF_NR5G       /**< NR5G */
};

/**
 * Defines ENDC(E-UTRAN New Radio-Dual Connectivity) Availability status on 5G NR
 */
enum class EndcAvailability {
   UNKNOWN = -1,   /**< Status unknown */
   AVAILABLE,      /**< ENDC is Available */
   UNAVAILABLE,    /**< ENDC is not Available */
};

/**
 * Defines DCNR(Dual Connectivity with NR) Restriction status on 5G NR
 */
enum class DcnrRestriction {
   UNKNOWN = -1,    /**< Status unknown */
   RESTRICTED,      /**< DCNR is Rescticted */
   UNRESTRICTED,    /**< DCNR is not Restricted */
};

/**
 * Defines Dual Connectivity status
 */
struct DcStatus {
   EndcAvailability endcAvailability;     /**< ENDC availability */
   DcnrRestriction  dcnrRestriction;      /**< DCNR restriction */
};

/**
 * 16 bit mask that denotes which of the radio access technology mode preference
 * defined in RatPrefType enum are used to set or get RAT preference.
 */
using RatPreference = std::bitset<16>;

/**
 * This function is called with the response to requestRatPreference API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] preference     @ref RatPreference
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not
 *                            @ref ErrorCode
 */
using RatPreferenceCallback
   = std::function<void(RatPreference preference, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestServiceDomainPreference
 * API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] preference   @ref ServiceDomainPreference
 * @param [in] error        Return code which indicates whether the operation
 *                          succeeded or not
 *                          @ref ErrorCode
 */
using ServiceDomainPreferenceCallback
   = std::function<void(ServiceDomainPreference preference, telux::common::ErrorCode error)>;

/**
 * @brief Serving System Manager class provides the API to request and set
 *        service domain preference and RAT preference.
 */
class IServingSystemManager {
public:
   /**
    * Checks the status of serving subsystem and returns the result.
    *
    * @returns True if serving subsystem is ready for service otherwise false.
    *
    * @deprecated Use IServingSystemManager::getServiceStatus() API.
    */
   virtual bool isSubsystemReady() = 0;

   /**
    * Wait for serving subsystem to be ready.
    *
    * @returns  A future that caller can wait on to be notified when serving
    *           subsystem is ready.
    *
    * @deprecated Use InitResponseCb in PhoneFactory::getServingSystemManager instead, to
    *             get notified about subsystem readiness.
    */
   virtual std::future<bool> onSubsystemReady() = 0;

   /**
    * This status indicates whether the IServingSystemManager object is in a usable state.
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
    * Set the preferred radio access technology mode that the device should use
    * to acquire service.
    *
    * @param [in] ratPref       Radio access technology mode preference.
    * @param [in] callback      Callback function to get the response of set RAT
    *                           mode preference.
    *
    * @returns Status of setRatPreference i.e. success or suitable error code.
    */
   virtual telux::common::Status setRatPreference(RatPreference ratPref,
                                                  common::ResponseCallback callback = nullptr)
      = 0;

   /**
    * Request for preferred radio access technology mode.
    *
    * @param [in] callback  Callback function to get the response of request
    *                       preferred RAT mode.
    *
    * @returns Status of requestRatPreference i.e. success or suitable error
    *          code.
    */
   virtual telux::common::Status requestRatPreference(RatPreferenceCallback callback) = 0;

   /**
    * Initiate service domain preference like CS, PS or CS_PS and receive the
    * response asynchronously.
    *
    * @param [in] serviceDomain  @ref ServiceDomainPreference.
    *
    * @param [in] callback       Callback function to get the response of
    *                            set service domain preference request.
    *
    * @returns Status of setServiceDomainPreference i.e. success or suitable
    *          error code.
    */
   virtual telux::common::Status setServiceDomainPreference(ServiceDomainPreference serviceDomain,
                                                            common::ResponseCallback callback
                                                            = nullptr)
      = 0;

   /**
    * Request for Service Domain Preference asynchronously.
    *
    * @param [in] callback    Callback function to get the response of request
    *                         service domain preference.
    *
    * @returns Status of requestServiceDomainPreference i.e. success or suitable
    *          error code.
    */
   virtual telux::common::Status
      requestServiceDomainPreference(ServiceDomainPreferenceCallback callback)
      = 0;

   /**
    * Get the Serving system information. Supports only 3GPP RATs.
    *
    * @param [out] sysInfo  Serving system information
    *                       @ref ServingSystemInfo
    *
    * @returns Status of getServingSystemInfo i.e. success or suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual telux::common::Status getSystemInfo(ServingSystemInfo &sysInfo) = 0;

   /**
    * Request for Dual Connectivity status on 5G NR.
    *
    * @returns @ref DcStatus
    *
    */
   virtual telux::tel::DcStatus getDcStatus() = 0;

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
    * Destructor of IServingSystemManager
    */
   virtual ~IServingSystemManager() {
   }
};

/**
 * @brief Listener class for getting radio access technology mode preference
 *        change notification.
 *
 *        The listener method can be invoked from multiple different threads.
 *        Client needs to make sure that implementation is thread-safe.
 */
class IServingSystemListener : public common::IServiceStatusListener{
public:
   /**
    * This function is called whenever RAT mode preference is changed.
    *
    * @param [in] preference      @ref RatPreference
    */
   virtual void onRatPreferenceChanged(RatPreference preference) {
   }

   /**
    * This function is called whenever service domain preference is changed.
    *
    * @param [in] preference      @ref ServiceDomainPreference
    */
   virtual void onServiceDomainPreferenceChanged(ServiceDomainPreference preference) {
   }

   /**
    * This function is called whenever the Serving System information is changed.
    * Supports only 3GPP RATs.
    *
    * @param [in] sysInfo    @ref ServingSystemInfo
    *
    * @note Eval: This is a new API and is being evaluated. It is subject to change and
    *             could break backwards compatibility.
    */
   virtual void onSystemInfoChanged(ServingSystemInfo sysInfo) {
   }

   /**
    * This function is called whenever the Dual Connnectivity status is changed on 5G NR.
    *
    * @param [in] dcStatus       @ref DcStatus
    *
    */
   virtual void onDcStatusChanged(DcStatus dcStatus) {
   }

   /**
    * Destructor of IServingSystemListener
    */
   virtual ~IServingSystemListener() {
   }
};

/** @} */ /* end_addtogroup telematics_serving_system */
}
}

#endif
