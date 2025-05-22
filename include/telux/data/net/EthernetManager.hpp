/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       EthernetManager.hpp
 *
 * @brief      Primary interface for ethernet nic config.
 *             It provides API to control ETH NIC config and ETH macsec config.
 *
 */

#ifndef TELUX_DATA_NET_ETHERNET_MANAGER_HPP
#define TELUX_DATA_NET_ETHERNET_MANAGER_HPP

#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#define MAX_ETH_NICS 8

namespace telux {
namespace data {
namespace net {

/**
 * Overall Ethernet mode(for all nics),
 * including LAN, WAN or LAN + WAN.
 */
enum class EthMode {
    LAN      = 0x00,  /**<   Ethernet mode LAN router  */
    WAN      = 0x01,  /**<   Ethernet mode WAN router  */
    WAN_LAN  = 0x02,  /**<   Ethernet mode WAN_LAN router  */
};

/**
 * Ethernet network type for single nic,
 * including LAN or WAN mode.
 */
enum class EthNetworkType {
    LAN     = 0x00,  /**<  LAN mode. Ethernet interface will be
                             added to the virtual bridge */
    WAN     = 0x01,  /**<  WAN mode. Ethernet will act as
                             ETH backhaul */
};

/**
 * Macsec operation type used both for set and get.
 * Including disable, enable or restart state.
 */
enum class MacsecOp {
    DISABLE = 0x00,  /**<  MACsec disable operation */
    ENABLE  = 0x01,  /**<  MACsec enable operation */
    RESTART = 0x02,  /**<  MACsec restart operation */
};

/**
 * Macsec mode, including supplicant, or authenticator mode.
   Indicate the macsec role to play.
 */
enum class MacsecMode {
    SUPPLICANT    = 0x01,  /**<   MACsec supplicant mode */
    AUTHENTICATOR = 0x02,  /**<   MACsec authenticator mode */
};

/**  Data structure for Ethernet NIC config.
 */
struct EthNicConfig {
    std::string    ifName;  /**<   Ethernet NIC configured interface name. */
    EthNetworkType type;    /**<   Ethernet NIC network type. */
};

/**  Data structure for Macsec Configuration.
 */
struct MacsecNicConfig {
    MacsecOp    opr;         /**<  MACsec operation type */
    std::string macsecIfName;  /**<  MACsec interface on which ethernet data
                                       communication will happen */
    MacsecMode  mode;          /**<  Mode on which MACsec will have to run */
    std::string ethNicIfName;  /**<  Ethernet interface name to set MACsec config. */
    uint32_t    mtuSize;       /**<  MTU size of the Ethernet iface in bytes. */
};

/** Data type for ETH mode and NIC configuration. */
struct EthConfig {
    EthMode                    mode;        /**< Ethernet mode. */
    std::vector<EthNicConfig>  configList;  /**< vector to manage ETH config */
};

/** Data type for macsec config. */
struct MacsecConfig {
    std::vector<MacsecNicConfig> configList;  /**< vector to manage ETH macsec config */
};

class IEthernetListener;

/**
 * This function is called as a response to @ref requestEthernetConfig()
 *
 * @param [in] ethConfig           list of eth config
 * @param [in] error               Return code which indicates whether the operation
 *                                 succeeded or not @ref telux::common::ErrorCode
 *
 */
using EthConfigCb
    = std::function<void(const EthConfig& ethConfig, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref requestMacsecConfig()
 *
 * @param [in] config              list of macsec config
 * @param [in] error               Return code which indicates whether the operation
 *                                 succeeded or not @ref telux::common::ErrorCode
 *
 */
using MacsecConfigCb
    = std::function<void(const MacsecConfig& config, telux::common::ErrorCode error)>;

class IEthernetManager
{
public:

    /**
     * Checks the status of Ethernet manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE      If Ethernet manager is ready for service.
     *          SERVICE_UNAVAILABLE    If Ethernet manager is temporarily unavailable.
     *          SERVICE_FAILED       - If Ethernet manager encountered an irrecoverable failure.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Set Ethernet config API, used to set eth mode and connectivity type.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] ethConfig           the eth config set by user
     * @param [in] callback            optional callback to get the qmi response of
     *                                 SetEthernetConfig
     *
     * @returns Status of setEthernetConfig i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status setEthernetConfig(const EthConfig&  ethConfig,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request Ethernet config API.
     *
     * @param [in] callback          Asynchronous callback to get the response of
     *                               requestEthernetConfig, including eth config
     *                               and the QMI error code
     *
     * @returns Status of requestEthernetConfig i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestEthernetConfig(EthConfigCb callback) = 0;

    /**
     * Enable mac security config API, requires eth connectivity type, macsec mode
     * and eth NIC name to start macsec service.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] config            macsec config to enable
     * @param [in] callback          optional callback to get the qmi response
     *                               of enableMacsec
     *
     * @returns Status of enableMacsec i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status enableMacsec(MacsecConfig& config,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Disable mac security config API based on the eth nic name.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DATA_NETWORK_CONFIG
     * permission to invoke this API successfully.
     *
     * @param [in] ethNicIfName      Indicate the eth nic name to disable macsec service
     * @param [in] callback          optional callback to get the qmi response
     *                               of disableMacsec
     *
     * @returns Status of disableMacsec i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status disableMacsec(std::string ethNicIfName,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request Macsec config API.
     *
     * @param [in] callback          Asynchronous callback to get the response of
     *                               requestMacsecConfig, including macsec config
     *                               and the QMI error code
     *
     * @returns Status of requestMacsecConfig i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestMacsecConfig(MacsecConfigCb callback) = 0;

    /**
     * Register ETH Manager as listener for Data Service health events like data service available
     * or data service not available.
     *
     * @param [in] listener    pointer of IEthernetListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IEthernetListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IEthernetListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IEthernetListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
    * Destructor for IEthernetManager
    */

    ~IEthernetManager() {};
};

/**
 * Interface for Ethernet listener object. Client needs to implement this interface to get
 * access to Ethernet services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IEthernetListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Destructor for IEthernetListener
     */
    virtual ~IEthernetListener(){};
};

}
}
}
#endif


