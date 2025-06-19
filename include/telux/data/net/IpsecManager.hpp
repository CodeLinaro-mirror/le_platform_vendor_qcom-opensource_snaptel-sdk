/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       IpsecManager.hpp
 *
 * @brief      The IpsecManager class provides APIs to manage IPsec tunnels
 *             For example, you can use the IpsecManager class to:
 *             - Create IPsec IKE and CHILD tunnel with custom traffic selector.
 *             - Activate/Deactivate specified IPsec tunnel.
 *             - Delete IPsec tunnel configuration.
 *             - Get IPsec tunnel Info with connective state.
 */

#ifndef TELUX_DATA_NET_IPSEC_MANAGER_HPP
#define TELUX_DATA_NET_IPSEC_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

/** Max Child SA IPsec Tunnels Size Supported */
#define IPSEC_MAX_CHILD_NUM 10
/** Max IPsec Identifier string length */
#define IPSEC_MAX_IDENTIFIER_STRING_LEN 255

namespace telux {
namespace data {
namespace net {

/** IPsec Topology
 * Without NAT == NAT Traversal
 * Host-to-Host: Setup between two single hosts
 * Site-to-Site-Without-NAT: Setup between two network locations
 * Host-to-Host and Site-to-Site-Without-NAT: Both H2H and S2S setup
 */
enum class IpsecTopology {
    INVALID = 0x00,                             /**< Invalid Topology */
    HOST_TO_HOST,                               /**< IPsec H2H Topology */
    SITE_TO_SITE_WITHOUT_NAT,                   /**< IPsec S2S Without NAT Topology */
    HOST_TO_HOST_AND_SITE_TO_SITE_WITHOUT_NAT,  /**< IPsec H2H and S2S Without NAT Topology */
};

/** IPsec Tunnel Type
 * The outer IP header protocol choice of ESP packet
 * Including IPv4 and IPv6
 */
enum class IpsecTunnelType {
    INVALID = 0x00,     /**< Invalid Tunnel Type */
    V4_ESP_TUNNEL_MODE, /**< IPv4 Tunnel Type */
    V6_ESP_TUNNEL_MODE, /**< IPv6 Tunnel Type */
};

/** IPsec Authentication Type
 * IKE authentication method choice in phase one
 * Pre-Shared-Key(PSK): Represents Pre-Shared Key (PSK) authentication
 * X509: Represents X.509 certificate-based authentication
 */
enum class IpsecAuthenticationType {
    INVALID = 0x00, /**< Invalid Authentication Type */
    PSK,            /**< PSK Authentication Type */
    X509,           /**< X509 Authentication Type */
};

/** IPsec Protocol Type
 * The Transmission Protocol choice for traffic transformed by IPsec
 * Only the specified traffic is encrypted if the protocol is specified
 * Including TCP and UDP, NO_PROTOCOL works for all traffic type
 */
enum class IpsecProtocolType {
    NO_PROTOCOL = 0x00, /**< NO Protocol */
    TCP,                /**< TCP Protocol */
    UDP,                /**< UDP Protocol */
};

/** IPsec Get Tunnel State
 * DISCONNECTED: Tunnel is disconnected
 * INPROGRESS: Tunnel is in progress
 * CONNECTED: Tunnel is connected
 */
enum class IpsecGetTunnelState {
    TUNNEL_DISCONNECTED = 0x00, /**< IPsec tunnel disconnected state */
    TUNNEL_INPROGRESS,          /**< IPsec tunnel in_progress state */
    TUNNEL_CONNECTED,           /**< IPsec tunnel connected state */
};

/** IPsec Set Tunnel State
 * Use to activate/deactivate IPsec tunnel
 * Including State Activate and Deactivate
 */
enum class IpsecSetTunnelState {
    SET_TUNNEL_STATE_ACTIVATE = 0x00, /**< Activate IPsec tunnel State */
    SET_TUNNEL_STATE_DEACTIVATE,      /**< Deactivate IPsec tunnel State */
};

/** IPsec Backhaul Type
 * IPsec tunnel can be worked on different backhaul type
 * Including WWAN Cellular and Ethernet
 */
enum class IpsecBackhaulType {
    INVALID = 0x00, /**< Invalid Backhaul Type */
    WWAN,           /**< WWAN Backhaul Type */
    ETH,            /**< Ethernet Backhaul Type */
};

/** IPsec IKE/CHILD tunnel identifier
 */
struct IpsecIdentifier {
    std::string ikeIdentifier;      /**< IKE identifier for IKE session */
    std::string childIdentifier;    /**< IPsec child SA identifier */
};

/** IPsec port range for UDP/TCP packet
 */
struct IpsecPortRange {
    uint16_t startPortId;   /**< Start Port Id. Zero means port id/range is invalid */
    uint16_t endPortId;     /**< End Port Id if port range need.
                                 Zero means port range is invalid */
};

/** Structure to set the IPsec IKE configuration
 */
struct IkeConfig {
    std::string ikeIdentifier;      /**< IKE identifier for IKE session */
    IpsecTunnelType tunnelType;          /**< IPsec Tunnel type */
    IpsecAuthenticationType authType;    /**< IPsec Authentication Type */
    IpsecBackhaulType bhType;       /**< IPsec tunnel backhaul type */

    std::string remoteEpAddr;       /**< IPsec remote endpoint address */

    std::string localIdentifier;    /**< IPsec local identifier
                                         Only effective when authType is X509 */
    std::string remoteIdentifier;   /**< IPsec remote identifier
                                         Only effective when authType is X509 */

    uint32_t tunnelIp;              /**< IPsec tunnel ip */
    uint16_t rekeyIntervalMins;     /**< IPsec IKE rekey time in minutes */
};

/**  Structure to get the IPsec Tunnel State
  */
struct IpsecTunnelState {
    bool enableStatus;          /**< IPsec tunnel enable status */
    IpsecGetTunnelState state;  /**< IPsec tunnel current state */
};

/**  Structure to set the IPsec Child configuration
  */
struct ChildConfig {
    std::string childIdentifier;            /**< IPsec child SA identifier */
    struct IpsecPortRange localPortRange;   /**< IPsec local port range */
    struct IpsecPortRange remotePortRange;  /**< IPsec remote port range */
    IpsecProtocolType protocolType;         /**< Protocol type for the tunnel */
    std::string localAddr;                  /**< IPsec traffic selector local address */
    std::string remoteAddr;                 /**< IPsec traffic selector remote address */
    bool hwOffload;                         /**< IPsec hardware offload option */
    bool trapAction;                        /**< IPsec start action: trap/start */
    uint16_t rekeyIntervalMins;             /**< IPsec child rekey time in minutes */
};

/**  Structure to set the IPsec configuration
  */
struct IpsecConfig {
    uint32_t profileId;                 /**< Profile id of IPsec tunnel */
    IpsecTopology topology;             /**< IPsec tunnel topology */
    struct IkeConfig ikeConfig;         /**< IPsec IKE configuration */
    uint32_t childEntries;              /**< Number of child SAs in IKE session */
    std::vector<ChildConfig> childCfg;  /**< IPsec child SAs configurations */
};

class IIpsecListener;

typedef std::unordered_map<std::string, IpsecTunnelState> IpsecTunnelStateMap;

class IIpsecManager
{
public:

    /**
     * Checks the status of the IPsecManager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If IPsecManager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If IPsecManager is temporarily unavailable.
     *          SERVICE_FAILED       -  If IPsecManager encountered an irrecoverable
     *                                  failure.
     *
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Enable IPsec Feature Mode API
     *
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode enableIpsec() = 0;

    /**
     * Disable IPsec Feature Mode API
     *
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode disableIpsec() = 0;

    /**
     * Get IPsec Feature Mode API
     *
     * @param [out] enable True if the IPsec feature is set currently otherwise false
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getIpsecEnabled(bool &enable) = 0;

    /**
     * Set IPsec Tunnel config API, including profile id, topology, IKE config and Child config
     *
     * @param [in] ipsecConfig         the ipsec config set by user
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode setTunnelConfig(const IpsecConfig &ipsecConfig) = 0;

    /**
     * Set IPsec Tunnel state API, activate/deactivate
     *
     * @param [in] identifier          the ipsec tunnel identifier be set by user
     * @param [in] state               operation(activate/deactivate) set by user
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode setTunnelState(const IpsecIdentifier &identifier,
        const IpsecSetTunnelState &state) = 0;

    /**
     * Delete IPsec Tunnel API
     *
     * @param [in] identifier          the ipsec tunnel identifier be deleted by user
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode deleteTunnel(const IpsecIdentifier &identifier) = 0;

    /**
     * Get IPsec Tunnel Info API, with optional State
     *
     * @param [in] identifier          the ipsec tunnel be get by user
     * @param [out] ipsecConfig        the ipsec config to get, identifier setting
     *                                 by the first argument `ipsecIdentifier`
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getTunnelConfig(const IpsecIdentifier &identifier,
        IpsecConfig &ipsecConfig) = 0;

    /**
     * Get IPsec Tunnel Info API, with optional State
     *
     * @param [in] identifier              the ipsec tunnel be get by user
     * @param [out] ipsecTunnelState       the map of IPsec tunnel state, the key is child
     *                                     identifier, the value is IPsec tunnel status
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode.
     *
     */
    virtual telux::common::ErrorCode getTunnelState(const IpsecIdentifier &identifier,
        IpsecTunnelStateMap &ipsecTunnelState) = 0;

    /**
     * Registers with the IIpsecManager as a listener for service status and other events.
     *
     * @param [in] listener    Pointer to the IIpsecListener object that processes the
     *                         notification
     *
     * @returns Status of registerListener.
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IIpsecListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Pointer to the IIpsecListener object that needs to be removed
     *
     * @returns Status of deregisterListener.
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IIpsecListener> listener) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Destructor for IIpsecManager
     */
    virtual ~IIpsecManager() {};
};

/**
 * Interface for IPsec listener object. Client needs to implement this interface to get
 * access to IPsec services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IIpsecListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when the service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}


    /**
     * Destructor for IIpsecListener
     */
    virtual ~IIpsecListener() {};
};

/** @} */ /* end_addtogroup telematics_data */
}  // namespace net
}  // namespace data
}  // namespace telux

#endif  //TELUX_DATA_NET_IPSEC_MANAGER_HPP