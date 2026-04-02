/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       WlanControlManager.hpp
 *
 * @brief      Manager class handling networking aspects of WLAN interfaces,
 *             while WLAN control is performed via Linux OSS APIs.
 */

#ifndef TELUX_WLAN_WLANCONTROLMANAGER_HPP
#define TELUX_WLAN_WLANCONTROLMANAGER_HPP

#include <memory>
#include <vector>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDefines.hpp>

namespace telux {
namespace wlan {

class IWlanControlListener;

/** @addtogroup telematics_wlan_control
 * @{ */

class IWlanControlManager {
 public:
    /**
     * Checks the framework service availability status of wlan control manager and returns
     * the result.
     *
     * @returns SERVICE_AVAILABLE    -  If wlan control manager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If wlan control manager is temporarily unavailable.
     *          SERVICE_FAILED       -  If wlan control manager encountered an irrecoverable
     *                                  failure.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Request current WLAN status: Returns Interface status of APs and Stations such as
     * active/inactive, network interface name and hardware device they are mapped to.
     * This API provides status for WLAN interfaces controlled via Linux OSS APIs.
     *
     * @param [out] status              vector of interface status @ref InterfaceStatus.
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode getInterfaceStatus(std::vector<InterfaceStatus> &status) = 0;

    /**
     * Register a listener for specific events in WLAN Control Manager
     *
     * @param [in] listener    pointer of IWlanControlListener object that processes the
     * notification
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IWlanControlListener> listener)
        = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IWlanControlListener object that needs to be removed
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode deregisterListener(
        std::weak_ptr<IWlanControlListener> listener)
        = 0;

    virtual ~IWlanControlManager(){};
};

class IWlanControlListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when the framework service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {
    }

    /**
     * This function is called when the AP status changes.
     *
     * @param [in] status     List of APs whose status has been updated @ref telux::wlan::ApStatus
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */

    virtual void onApStatusChanged(const std::vector<ApStatus> &status) {
    }

    /**
     * This function is called when Station Status Changes.
     *
     * @param [in] staStatus   List of station state @ref telux::wlan::StaStatus
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual void onStationStatusChanged(const std::vector<StaStatus> &staStatus) {
    }

    virtual ~IWlanControlListener() {
    }
};

/** @} */ /* end_addtogroup telematics_wlan_control */
}  // namespace wlan
}  // namespace telux
#endif  // TELUX_WLAN_WLANCONTROLMANAGER_HPP