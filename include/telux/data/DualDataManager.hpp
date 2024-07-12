/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       DualDataManager.hpp
 *
 * @brief      The DualDataManager class provides APIs to manage dual data
 *             connectivity. For example, you can use the DualDataManager class to:
 *             - Check the dual data capability of the device.
 *             - Check the dual data usage recommendation.
 *             - Register for listener APIs to be notified about dual data changes
 *               (capability or usage recommendation).
 *
 */

#ifndef TELUX_DUAL_DATA_MANAGER_HPP
#define TELUX_DUAL_DATA_MANAGER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
/** @addtogroup telematics_data
 * @{ */


enum class DualDataUsageRecommendation
{
    ALLOWED = 0,            /** Long running data calls on both SIM slots is allowed. */
    NOT_ALLOWED = 1,        /** Long running data calls are not allowed on both SIM slots.
                                Data activities must be stopped on the nDDS slot. */
    NOT_RECOMMENDED = 2,    /** Recommendation is to have long running data calls only on the
                                DDS SIM slot. In this case, it is expected that data activities
                                on nDDS SIM slot are stopped. If data activities are continued
                                on both the slots for longer duration, the performance would be
                                degraded. */
};

class IDualDataListener;

class IDualDataManager {
public:

    /**
     * Checks the status of the DualDataManager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If DualDataManager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If DualDataManager is temporarily unavailable.
     *          SERVICE_FAILED       -  If DualDataManager encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * The response for this API allows the user determine if the device supports
     * dual data feature or not.
     *
     * @param [out] isCapable      Provides the dual data capability of the device
     *
     *                               - True:  Device supports dual data.
     *                               - False: Device does not support dual data.
     *
     *                             If the device supports dual data, use
     *                             @ref getDualDataUsageRecommendation to check, whether long
     *                             running data calls on both slots is allowed, not-allowed
     *                             or not-recommended.
     *
     * @returns Error code which indicates whether the operation succeeded or not.
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DUAL_DATA_INFO
     * permission to successfully invoke this API.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getDualDataCapability(bool &isCapable) = 0;

    /**
     * Queries the dual data usage recommendation.
     *
     * @param [out] recommendation   Recommendation about dual data usage.
     *
     * @returns Error code which indicates whether the operation succeeded or not
     *
     * On platforms with access control enabled, the caller needs to have the TELUX_DUAL_DATA_INFO
     * permission to successfully invoke this API.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getDualDataUsageRecommendation(
        DualDataUsageRecommendation &recommendation) = 0;

    /**
     * Registers with the DualDataManager as a listener for service status and other events.
     *
     * @param [in] listener    Pointer to the IDualDataListener object that processes the
     *                         notification
     *
     * @returns Status of registerListener.
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IDualDataListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Pointer to the IDualDataListener object that needs to be removed
     *
     * @returns Status of deregisterListener.
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IDualDataListener> listener) = 0;

    /**
     * Destructor for IDualDataManager
     */
    virtual ~IDualDataManager(){};
};

class IDualDataListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when the service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * This function is called when the dual data capability changes.
     *
     * @param [in] isDualDataCapable - Dual data capability.
     *
     */
    virtual void onDualDataCapabilityChange(bool isDualDataCapable) {}

    /**
     * This function is called when the dual data usage recommendation changes.
     *
     * @param [in] recommendation - Provides dual data usage recommendation.
     *
     */
    virtual void onDualDataUsageRecommendationChange(
        DualDataUsageRecommendation recommendation) {}

    virtual ~IDualDataListener() {}
};

/** @} */ /* end_addtogroup telematics_data */
}  // namespace data
}  // namespace telux

#endif  //TELUX_DUAL_DATA_MANAGER_HPP