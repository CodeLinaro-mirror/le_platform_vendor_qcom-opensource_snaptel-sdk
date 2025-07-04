/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       BackhaulManager.hpp
 *
 * @brief      BackhaulManager is a primary interface that configure
 *             backhaul configuration e.g requestBackhaulStatus.
 *
 */

#ifndef TELUX_DATA_NET_BACKHAULMANAGER_HPP
#define TELUX_DATA_NET_BACKHAULMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {
namespace net {

/** @addtogroup telematics_data_net
  * @{ */

// Forward declarations
class IBackhaulManagerListener;

/*
 * Structure represents backhaul configuration
 */
struct BackhaulStatusInfo {
    /**
     * Whether IPv4 backhaul is available
     */
    bool isV4BackhaulAvailable;
    /**
     * Whether IPv6 backhaul is available
     */
    bool isV6BackhaulAvailable;
    /**
     * Whether Ethernet PDU is available
     */
    bool isEthPduAvailable;
    /**
     * Type of backhaul connection @ref BackhaulType
     */
    BackhaulType backhaulType;
};

/**
 * Structure for backhaul load balance info
 */
struct BHLoadBalanceInfo {
    bool     enable;  /**< backhaul load balance enable status. */
    int      wan;     /**< mwan3 wan weight. */
    int      waneth;  /**< mwan3 waneth weight. */

    BHLoadBalanceInfo()
        : enable(false), wan(0), waneth(0) {}

    BHLoadBalanceInfo(bool status, int wanWeight, int wanethWeight)  /* structure constructor */
        : enable(status), wan(wanWeight), waneth(wanethWeight) {}
};

/**
 * This function is called in response to requestBHLoadBalanceStatus.
 * Returned bhLoadBalanceInfoResp contains @ref telux::data::BHLoadBalanceInfo.
*
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] bhLoadBalanceInfoResp   Load balance info
 * @param [in] error                   Return code for whether the operation succeeded or failed
 *
 */
using RequestBHLoadBalanceStatusResponseCb =
    std::function<void(const BHLoadBalanceInfo& bhLoadBalanceInfoResp, telux::common::ErrorCode error)>;

using RequestBackhaulStatusInfoCb =
    std::function<void(const BackhaulStatusInfo &backhaulStatusInfo,
        telux::common::ErrorCode error)>;

class IBackhaulManager {
public:

    /**
     * Checks the status of BackhaulManager and returns the result.
     *
     * @returns SERVICE_AVAILABLE    If Backhaul manager object is ready for service.
     *          SERVICE_UNAVAILABLE  If Backhaul manager object is temporarily
     *                               unavailable.
     *          SERVICE_FAILED       If Backhaul manager object encountered an
     *                               irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
    * Register Backhaul Manager as listener for Data Service health events like data
    * service available or data service not available.
    *
    * @param [in] listener    pointer of IBackhaulManagerListener object that processes the
    * notification
    *
    * @returns Status of registerListener success or suitable status code
    *
    */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IBackhaulManagerListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IBackhaulManagerListener object that needs to be
     *                         removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IBackhaulManagerListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Request v4/v6/eth backhaul status.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] callback   Asynchronous callback to get the response of
     *                        requestBackhaulStatus
     *
     * requestBackhaulStatus API gives the highest priroity backhaul status.
     * BACKHAUL_STATUS_IND indication is sent to underlying layer if backhaul status changed.
     *
     * @returns Status of requestBackhaulStatus i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     */

    virtual telux::common::Status requestBackhaulStatus(RequestBackhaulStatusInfoCb
        callback) = 0;

    /**
     * Set backhaul load balance info, including enable status, LTE and ETH backhaul weight
     *
     * @param [in] bhLoadBalanceInfoReq  BH load balance info to set, including enable status,
                                         LTE and ETH backhaul weight
     * @param [out] callback             optional callback to set the bh load balance response
     *
     * @returns immediate status of the setBHLoadBalance() request sent, i.e., success or
     *          the suitable status code returned by QCMAP server and convert to telux error code.
     */
    virtual telux::common::Status setBHLoadBalance(const BHLoadBalanceInfo &bhLoadBalanceInfoReq,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Get backhaul load balance info, including enable status, LTE and ETH backhaul weight
     *
     * @param [out] callback             callback to get the bh load balance status response
     *
     * @returns immediate status of the requestBHLoadBalanceStatus() request sent, i.e., success or
     *          the suitable status code returned by QCMAP server and convert to telux error code.
     */
    virtual telux::common::Status requestBHLoadBalanceStatus(
        RequestBHLoadBalanceStatusResponseCb callback) = 0;

    /**
     * Destructor for IBackhaulManager
     */
    virtual ~IBackhaulManager(){};
};

/**
 * Interface for Backhaul manager listener object. Client needs to implement this interface
 * to get access to Backhaul related services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IBackhaulManagerListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * This function is called when backhaul status changes.
     *
     * @param [in] backhaulStatusInfo  - @ref BackhaulStatusInfo
     */
    virtual void onBackhaulStatusChange(BackhaulStatusInfo backhaulStatusInfo) {}

    /**
     * Destructor for IBackhaulManagerListener
     */
    virtual ~IBackhaulManagerListener() {}
};

/** @} */ /* end_addtogroup telematics_data_net */

}
}
}

#endif  //TELUX_DATA_NET_BACKHAULMANAGER_HPP

