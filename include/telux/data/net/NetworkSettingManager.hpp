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

/**
 * @brief This can be passed in requestPortTriggerEntry API to get all the entry.
 */
static int ALL_PORT_CONFIGS = 0;

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
 * structure for sw ip channel config
 */
struct SWIpChannelConfig{
  std::string ifName;  /**<   Name of interface */
  std::string staticIpAddr;  /**<   IP Address of logical interface*/
  std::string neighLinkLocalAddr;  /**<   Neighbor link local address */
  std::string neighIpAddr;  /**<   Neighbor IP address */
};

/**
 * DHCP reservation information
 */
struct DHCPReservationInfo {
    std::string clientMacAddr;     /**<   MAC address of the device. This field is valid for
                                            eth/wlan clients */
    std::string clientReservedIp;  /**<   Reserved IP for the AP client. */
    std::string clientDeviceName;  /**<   Device name. This field is valid for USB client */
    bool enable;                   /**<   To enable/disable DHCP reservation; bool value. */
};

/**
 * This function is called with the response to @ref requestDHCPReservationRecords API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] records          Vector for dhcp reservation record for all entries.
 * @param [in] error            Return code for whether the operation succeeded or failed.
 */
using RequestDHCPReservationRecordsResponseCb = std::function<void(
    const std::vector<DHCPReservationInfo>& records, telux::common::ErrorCode error)>;

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
 * This function is called in response to @ref requestSWIpChannelConfig.
 * Returned swIpChCfg contains @ref telux::data::SWIpChannelConfig.
*
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] swIpChCfg       sw ip channel config to obtain
 * @param [in] error           Return code for whether the operation succeeded or failed
 *
 */
using RequestSWIpChannelConfigResponseCb =
    std::function<void(const SWIpChannelConfig& swIpChCfg, telux::common::ErrorCode error)>;

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
     *                               from the NAT table. ALL_PORT_CONFIGS var can be used
     *                               to retrieve all the prort trigger entries.
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
     * Add sw ip channel config, including interfaceName, static ip address, neighbour link local
       address, neighbor ip address.
     *
     * @param [in] swIpChCfg          The sw ip config to set @ref telux::data::SWIpChannelConfig
     * @param [out] callback          callback to get addSWIPChannelConfig response
     *
     * @returns immediate status of the addSWIpChannelConfig() request sent, i.e., success or
     *          the suitable status code returned by QCMAP server and convert to telux error code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status addSWIpChannelConfig(const SWIpChannelConfig& swIpChCfg,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Remove sw ip channel config, including interfaceName, static ip address, neighbour
     * link local address, neighbor ip address.
     *
     * @param [in] ifName            Identifies the interface to be removed
     * @param [out] callback         callback to get removeSWIPChannelConfig response
     *
     * @returns immediate status of the removeSWIpChannelConfig() request sent, i.e., success or
     *          the suitable status code returned by QCMAP server and convert to telux error code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status removeSWIpChannelConfig(const std::string& ifName,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request sw ip channel config, including interfaceName, static ip address, neighbour
     * link local address, neighbor ip address.
     *
     * @param [out] callback             callback to get sw ip channel config response
     *
     * @returns immediate status of the requestSWIpChannelConfig() request sent, i.e., success or
     *          the suitable status code returned by QCMAP server and convert to telux error code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestSWIpChannelConfig(
        RequestSWIpChannelConfigResponseCb callback) = 0;

    /**
     * Allow IP family to enable or disable ipv4/ipv6 connectivity, applicable for wwan, eth,
     * BT and wlan backhaul based on v4 or v6 ip family not both.
     *
     * @param [in] allow         Identifies allow or not allow operation
     * @param [in] ipFamilyType  Identifies IP family type, IPV4 or IPV6
     * @param [out] callback     Optional callback to get allowIPFamily response
     *
     * @returns immediate status of the allowIPFamily() request sent, i.e., success or
     *          the suitable status code. The client receives asynchronous notifications
     *          indicating the data call tear-down.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status allowIpFamily(bool allow, IpFamilyType ipFamilyType,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * This API is to add DHCP reservation record.
     *
     * @param [in] entry          The DHCP reservation record single entry to set, including
                                  client mac addr, client reserved IP addr, client device name
                                  and enable/disable state @ref telux::data::DHCPReservationInfo.
     * @param [in] callback       callback to get response for addDHCPReservRecord.
     *
     * @returns Status of addDHCPReservationRecord i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status addDHCPReservationRecord(DHCPReservationInfo entry,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * This API is to get current DHCP reservation records.
     *
     * @param [in] callback         callback to get the DHCP reservation record
     *                                    and the operation status code.
     *
     * @returns Status of requestDHCPReservationRecords i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestDHCPReservationRecords(
        RequestDHCPReservationRecordsResponseCb callback) = 0;

    /**
     * This API is to edit DHCP reservation records.
     *
     * @param [in] addr             The existing client reserved IP addr saved to edit.
     * @param [in] entry            The new DHCP reservation record to edit, including client
                                    mac addr, client reserved IP addr, client device name and
                                    enable/disable state @ref telux::data::DHCPReservationInfo.
     * @param [in] callback         callback to get response for editDHCPReservRecord.
     *
     * @returns Status of editDHCPReservationRecord i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status editDHCPReservationRecord(std::string addr,
        DHCPReservationInfo entry, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * This API is to delete DHCP reservation records.
     *
     * @param [in] addr               The reserved IP addr to delete.
     * @param [in] callback           callback to get response for deleteDHCPReservRecord.
     *
     * @returns Status of deleteDHCPReservationRecord i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status deleteDHCPReservationRecord(std::string addr,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Active LAN API. Use this API after set lan config with @ref setLANConfig to
     * make the lan config effective
     *
     * @param [in] callback    Asynchronous callback to get the qmi error code if any
     *
     * @returns Status of ActivateLAN i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status activateLAN(
        telux::common::ResponseCallback callback = nullptr) = 0;

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

