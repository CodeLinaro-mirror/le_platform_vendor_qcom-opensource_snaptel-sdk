/*
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       NetworkSettingManager.hpp
 *
 * @brief      NetworkSettingManager is a primary interface that configure
 *             network e.g enable or disable ALG(Application Layer Gateway),
 *             configure port trigger feature.
 *
 */

#ifndef TELUX_DATA_NET_NETWORKSETTINGMANAGER_HPP
#define TELUX_DATA_NET_NETWORKSETTINGMANAGER_HPP

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
class INetworkSettingListener;

/**
 * @brief Port trigger packet Id.
 */
using PortConfigId = int;

/*
 * Structure represents Port Trigger and forward configuration
 */
struct PortTriggerConfig {
    PortConfigId   portConfigId;   /**< Id for the port trigger packet */
    uint16_t triggerStartPort;     /**< destination start port of the port
                                        trigger packet */
    uint16_t triggerEndPort;       /**< destination end port of the port
                                        trigger packet */
    uint16_t forwardStartPort;     /**< destination start port of the port
                                        forward packet */
    uint16_t forwardEndPort;       /**< destination end port of the port
                                        forward packet */
    uint16_t triggerProtocol;      /**< protocol type of the port trigger packet */
    uint16_t forwardProtocol;      /**< protocol type of the port forward packet */
    uint32_t timer;                /**< timeout value for Port Triggering in second.
                                        Default timeout value is 10 min. */
};

enum class AlgType {
    RTSP            = 1,    /**<   RTSP ALG Type  */
    SIP             = 2,    /**<   SIP ALG Type  */
};

/**
 * This function is called to @ref addPortTriggerEntry
 *
 * @param [in] portConfigId  Id for reference of port trigger entry
 * @param [in] error         Return code which indicates whether the operation
 *                           succeeded or not @ref telux::common::ErrorCode
 *
 */

using AddPortTriggerEntriesCb
    = std::function<void(PortConfigId &portConfigId, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref requestPortTriggerEntry()
 *
 * @param [in] portTriggerEntries  list of added port trigger entry
 * @param [in] error               Return code which indicates whether the operation
 *                                 succeeded or not @ref telux::common::ErrorCode
 *
 */
using RequestPortTriggerEntriesCb
    = std::function<void(const std::vector<PortTriggerConfig> &PortTriggerEntries,
    telux::common::ErrorCode error)>;

/**
 * This function is called in response to @ref requestDataPathOptStatus.
 *
 * @param [in] dataPathOptStatus  Status of Data Path Optimizer.
 * @param [in] error              Return code for whether the operation succeeded
                                  or failed.
 *
 */
using RequestDataPathOptStatusCb =
    std::function<void(bool &dataPathOptStatus, telux::common::ErrorCode error)>;


/**
 * @brief      NetworkSettingManager is a primary interface that configure
 *             network e.g enable or disable ALG(Application Layer Gateway),
 *             configure port trigger feature.
 *
 */

class INetworkSettingManager {
 public:
    /**
     * Checks the status of NetworkSetting manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE    If NetworkSetting manager object is ready for service.
     *          SERVICE_UNAVAILABLE  If NetworkSetting manager object is temporarily
     *                               unavailable.
     *          SERVICE_FAILED       If NetworkSetting manager object encountered an
     *                               irrecoverable failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;
    /**
    * Register NetworkSetting Manager as listener for Data Service health events like data
    * service available or data service not available.
    *
    * @param [in] listener    pointer of INetworkSettingListener object that processes the
    * notification
    *
    * @returns Status of registerListener success or suitable status code
    *
    */
    virtual telux::common::Status registerListener(
        std::weak_ptr<INetworkSettingListener>listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of INetworkSettingListener object that needs to be
     *                         removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<INetworkSettingListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Add Port Trigger configuration in the NAT table
     * This is persistent across object and reboot lifetimes.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in]  portTriggerCfg    Port and Protocol on which Port Trigger will be
     *                                configured.
     * @param [in]  callback          Asynchronous callback to get the response of
     *                                addPortTriggerEntry
     *
     * @returns Status of addPortTriggerEntry i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     */
    virtual telux::common::Status addPortTriggerEntry(const PortTriggerConfig portTriggerCfg,
        AddPortTriggerEntriesCb addPortTriggerEntrycb) = 0;

    /**
     * Delete Port Trigger configuration entry.
     * This is persistent across object and reboot lifetimes.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in]  portConfigId     Id to delete port trigger entry
     * @param [in]  callback         optional callback to get the response
     *                               deletePortTriggerEntry
     *
     * @returns Status of deletePortTriggerEntry i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change.
     */
    virtual telux::common::Status deletePortTriggerEntry(const PortConfigId portConfigId,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request list of Port Trigger entries
     * This is persistent across object and reboot lifetimes.
     *
     * @param [int] portConfigId     Id to get Port trigger Entry Configuration
     *                               from the NAT table.
     * @param [in]  portTriggerCb    Asynchronous callback to get the response
     *                               for requestPortTriggerEntry
     *
     * @returns Status of requestPortTriggerEntry i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     */
    virtual telux::common::Status requestPortTriggerEntry(const PortConfigId portConfigId,
        RequestPortTriggerEntriesCb requestPortTriggerCb) = 0;

    /**
     * Enable or disable RTSP/SIP ALG functionality
     * This is persistent across object and reboot lifetimes.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] algType           AlgType enum to enable or disable RTSP/SIP ALG.
     * @param [in] enable            flag to enable/disable ALG.
     * @param [in] callback          optional callback to update ALG functionality.
     *
     * @returns Status of updateAlg i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     */
    virtual telux::common::Status updateAlg(AlgType algType,
        bool enable, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Enable/disable SFE(shortcut forwarding engine) path to accelerate the packets
     * by bypass the network stack.
     * This is persistent across object and reboot lifetimes.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in]  dataPathOptStatus  Flag to enable/disable data path optimization.
     * @param [in]  callback           optional callback to get the response of
     *                                 setDataPathOptStatus.
     *
     * @returns Status of setDataPathOptStatus i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     */
    virtual telux::common::Status setDataPathOptStatus(bool dataPathOptStatus,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request the status of data path optimization (SFE path) whether enabled/disabled.
     * This is persistent across object and reboot lifetimes.
     *
     * @param [in]  callback  Asynchronous callback to get the response of getDataPathOptStatus.
     *
     * @returns Status of getDataPathOptStatus i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     */
    virtual telux::common::Status requestDataPathOptStatus(
        RequestDataPathOptStatusCb requestDataPathOptStatusCb) = 0;

    /**
     * Destructor for INetworkSettingManager
     */
    virtual ~INetworkSettingManager(){};
};

/**
 * Interface for NetworkSetting listener object. Client needs to implement this interface
 * to get access to NetworkSetting services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class INetworkSettingListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for INetworkSettingListener
     */
    virtual ~INetworkSettingListener() {}
};
/** @} */ /* end_addtogroup telematics_data_net */
}
}
}

#endif  //TELUX_DATA_NET_NETWORKSETTINGMANAGER_HPP

