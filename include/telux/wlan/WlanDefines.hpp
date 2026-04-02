/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       WlanDefines.hpp
 * @brief      WlanDefines contains enumerations and variables used for wlan services
 *
 */

#ifndef TELUX_WLAN_WLANDEFINES_HPP
#define TELUX_WLAN_WLANDEFINES_HPP

#include <string>
#include <vector>
#include <bitset>

namespace telux {
namespace wlan {

/** @addtogroup telematics_wlan
 * @{ */

/**
 * Radio Band Types:
 */
enum class BandType {
    BAND_5GHZ = 1,
    BAND_2GHZ = 2,
    BAND_6GHZ = 3,
};

/**
 * Connection Status
 */
enum class ConnectionStatus {
    UNKNOWN      = 0, /**< Device connection is unknown    */
    CONNECTED    = 1, /**< Device is connected             */
    DISCONNECTED = 2, /**< Device is disconnected          */
};

/**
 * Identifiers for Ap, Sta, P2p
 */
enum class Id {
    PRIMARY    = 1,
    SECONDARY  = 2,
    TERTIARY   = 3,
    QUATERNARY = 4,
};

/**
 * AP Types:
 */
enum class ApType {
    UNKNOWN = 0,
    PRIVATE = 1,
    GUEST   = 2,
};

/**
 * Station Interface Status
 */
enum class StaInterfaceStatus {
    UNKNOWN              = 0, /**< Station interface is unknown                  */
    CONNECTING           = 1, /**< Station interface is connecting               */
    CONNECTED            = 2, /**< Station interface is connected                */
    DISCONNECTED         = 3, /**< Station interface is disconnected             */
    ASSOCIATION_FAILED   = 4, /**< Station is unable to associate with AP        */
    IP_ASSIGNMENT_FAILED = 5, /**< Station is unable to get IP address via DHCP  */
};

/**
 * Station Connection Status
 */
enum class StaConnectionStatus {
    UNKNOWN       = 0, /**< Station connection status is unknown              */
    SUCCESS       = 1, /**< Station connection attempt was successful         */
    INCORRECT_PSK = 2, /**< Station connection attempt failed with incorrect
                            password/passkey */
    AP_NOT_FOUND = 3, /**< Station connection attempt failed with AP not in
                           range */
};

/**
 * AP Info - captures ap type (private/guest)
 */
struct ApInfo {
    BandType apRadio; /**< Radio type (2.4/5.0/6.0 GHz) */
    ApType apType; /**< Ap type (private/guest) */
};

/**
 * Ap Network Info
 */
struct ApNetInfo {
    ApInfo info; /**< Ap information (AP type)              */
    std::string ssid; /**< SSID associated with this network     */
};

/**
 * AP Status for enabled Networks
 */
struct ApStatus {
    Id id; /**< AP id                                 */
    std::string name; /**< AP network interface name             */
    std::string ipv4Address; /**< Local AP IP V4 address                */
    std::string macAddress; /**< AP MAC address                        */
    std::vector<ApNetInfo> network; /**< Settings for AP info                  */
};

/**
 * AP Interworking Information
 */
enum class ApInterworking {
    INTERNET_ACCESS = 0, /**<  AP with internet access only - No LAN access   */
    FULL_ACCESS     = 1 /**<  AP Can Access LAN and Internet                 */
};

/**
 * Station Status
 */
struct StaStatus {
    Id id; /**< Station Id                       */
    std::string name; /**< Network interface name           */
    std::string ipv4Address; /**< Public IP V4 address             */
    std::string ipv6Address; /**< Public IP V6 address             */
    std::string macAddress; /**< MAC address                      */
    StaInterfaceStatus status; /**< Interface status                 */
    StaConnectionStatus connectionStatus; /**< Station connection status        */
};

/**
 * This applies in architectures where the modem is attached to an External Application
 * Processor(EAP). An API that sets or configure Wlan can be invoked from the EAP or from
 * the modem's Internal Application Processor (IAP). This type  specifies where the operation
 * should be carried out.
 */
enum class OperationType {
    WLAN_LOCAL = 0, /**< Perform the operation on the processor where the API is invoked.*/
    WLAN_REMOTE, /**< Perform the operation on the application processor other than where
                        the API is invoked. */
};

/**
 * Preferred IP family for the connection
 */
enum class IpFamilyType {
    UNKNOWN = -1,
    IPV4    = 0x04, /**< IPv4 data connection */
    IPV6    = 0x06, /**< IPv6 data connection */
    IPV4V6  = 0x0A, /**< IPv4 and IPv6 data connection */
};

/**
 * Service operations to be performed
 */
enum class ServiceOperation {
    STOP    = 0x00, /**<  Stop service       */
    START   = 0x01, /**<  Start service      */
    RESTART = 0x02, /**<  Restart service    */
};

/**
 * Station Connection IP Type.
 */
enum class StaIpConfig {
    DYNAMIC_IP = 1, /**< Station is configured with dynamic IP */
    STATIC_IP  = 2, /**< Station is configured with Static IP  */
};

/**
 * Static IP Configuration.
 */
struct StaStaticIpConfig {
    std::string ipAddr; /**<   IPv4 address to be assigned. */
    std::string gwIpAddr; /**<   IPv4 address of the gateway. */
    std::string netMask; /**<   Subnet mask.                 */
    std::string dnsAddr; /**<   DNS IPv4 address.            */
};

/**
 * Wlan Interface State
 */
enum class InterfaceState {
    INACTIVE = 0x00, /**<  Interface is Inactive  */
    ACTIVE   = 0x01, /**<  Interface is Active    */
};

/**
 * Wlan Interface Device
 */
enum class HwDeviceType {
    UNKNOWN = 0, /**<  Wlan device is Unknown   */
    QCA6574 = 1, /**<  Wlan device is QCA6574   */
    QCA6696 = 2, /**<  Wlan device is QCA6696   */
    QCA6595 = 3, /**<  Wlan device is QCA6595   */
    QCA6797 = 4, /**<  Wlan device is QCA6797   */
};

/**
 * Wlan Interface status
 */
struct InterfaceStatus {
    HwDeviceType device; /**< WiFi hardware type           */
    std::vector<ApStatus> apStatus; /**< Vector of active APs status  */
    std::vector<StaStatus> staStatus; /**< Vector of active Sta status  */
};

/** @} */ /* end_addtogroup telematics_wlan */
}  // namespace wlan
}  // namespace telux

#endif  // TELUX_WLAN_WLANDEFINES_HPP
