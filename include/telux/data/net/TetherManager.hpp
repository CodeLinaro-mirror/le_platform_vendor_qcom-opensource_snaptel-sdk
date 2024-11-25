/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       TetherManager.hpp
 *
 * @brief      TetherManager is a primary interface that controls tether service.
 *             Currently it supports Bluetooth tether functionality, including
 *             startBTTether, stopBTTether and requestBTTetherStatus.
 *
 */

#ifndef TELUX_DATA_NET_TETHERMANAGER_HPP
#define TELUX_DATA_NET_TETHERMANAGER_HPP

#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
namespace net {

class ITetherListener;

/**
 * Bluetooth tether mode,including LAN or WAN mode.
 * When set as LAN mode, MDM bt-pan will be mapped to default bridge.
 * When set as WAN mode, MDM bt-pan will act as backhaul.
 */
enum class BTTetherMode {
    LAN      = 0x00, /**<  0 -- Station will act as backhaul  */
    WAN      = 0x01, /**<  1 -- Bluetooth-Pan will act as backhaul  */
};

/**
 * Bluetooth tether status,including up or down status.
 */
enum class BTTetherStatus {
    UP      = 0x01, /**<  BT Tethering is UP  */
    DOWN    = 0x02, /**<  BT Tethering is DOWN  */
};

/**
 * This function is called as a response to @ref requestBTTetheringStatus()
 *
 * @param [in] btMode              the current bt tether mode
   @param [in] btStatus            the current bt tether status
 * @param [in] error               Return code which indicates whether the operation
 *                                 succeeded or not @ref telux::common::ErrorCode
 *
 */
using BTTetherCb = std::function<void(const BTTetherMode btMode,
    const BTTetherStatus btStatus, telux::common::ErrorCode error)>;

class ITetherManager
{
public:
    /**
     * Checks the status of Tether manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE      If Tether manager is ready for service.
     *          SERVICE_UNAVAILABLE    If Tether manager is temporarily unavailable.
     *          SERVICE_FAILED       - If Tether manager encountered an irrecoverable failure.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Start BT tether API, including BT wan/lan mode. This API is called by qc-bt-daemon to set
     * the network configuration for bt-pan interface.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] btMode              the bt tether mode to bring up
     * @param [in] callback            optional callback to get the qmi response for
     *                                 startBTTether
     *
     * @returns Status of startBTTether i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status startBTTether(const BTTetherMode  btMode,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Stop BT tether functionality API. This API is called by qc-bt-daemon to set
     * the network configuration for bt-pan interface.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] callback          optional callback to get the response stopBTTether
     *
     * @returns Status of stopBTTether i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status stopBTTether(
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request the BT Tether status, including the current bt tether mode and status
     *
     * @param [in] callback           Asynchronous callback to get current BT mode, BT status,
     *                                and the qmi error code
     *
     * @returns Status of requestBTTetherStatus i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestBTTetherStatus(BTTetherCb callback) = 0;

    /**
     * Register Tether Manager as listener for Data Service heath events like data service available
     * or data service not available.
     *
     * @param [in] listener    pointer of ITetherListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ITetherListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of ITetherListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<ITetherListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
    * Destructor for ITetherManager
    */
    ~ITetherManager() {};
};

/**
 * Interface for Tether listener object. Client needs to implement this interface to get
 * access to Tethering services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class ITetherListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for ITetheringListener
     */
    virtual ~ITetherListener(){};
};

}
}
}
#endif


