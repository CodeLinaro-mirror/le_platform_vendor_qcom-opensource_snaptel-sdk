/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       IpaManager.hpp
 *
 * @brief
 *
 * IPA is standalone HW accelerator. IPA is configurable from multiple CPUs
 * to allow a flexible architecture for managing data path.
 *
 * Functionality
 *
 *  Used as a DMA between APPS and modem memory space & supports on the fly
 *  deciphering of packets as data is moved to apps space.
 *  Implements various packet processing functions to offload APPS CPU.
 *    --> Packet filtering, header addition and removal.
 *    --> TCP/UDP Checksum Offload.
 *    --> Aggregation and de-aggregation of packets.
 *    --> NAT/XLAT.
 *  Supports HW to HW connectivity with peripherals by interfacing with
 *  various peripherals ( USB, PCIe, WIFI, ETHERNET ).
 *
 * IpaManager is a primary interface that controls below functionalities.
 *
 *            1) IpPassThrough
 *            2) Ip Collision
 *            3) Factory Reset
 *            4) LAN Statistic
 *            5) VLAN Configuration & Priority
 *            6) XML Manipulations
 *            7) Filtering
 *            8) Packet Threshold
 *
 * This interface has to be used only when QCMAP interface SDK is not
 * available. Modifying the configurations using QCMAP & IPAManager in
 * parallel will result in functional failures. Below are the APIs which
 * can be triggered when QCMAP interface SDK not available.
 *
 *            1. setIpPassthrough
 *            2. setVlanConfig
 *            3. setIpCollision
 *            4. setFactoryReset
 *            5. monitorLanStatistics
 *            6. requestLanStatistics
 *            7. updateWlanMode
 *            8. updateInterfaceType
 *            9. setPacketThreshold
 *            10. setMacBasedSwFiltering
 *            11. setIpBasedSwFiltering
 *            12. setInterfaceBasedSwFiltering
 *            13. enableFileBasedSwFiltering
 */

#ifndef IPAMANAGER_HPP
#define IPAMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

/**
 * Client for which Passthrough or Lan Statistics needs to enabled / disabled
 */
enum class IpaDeviceType {
    /* usb */
    IPA_CLIENT_DEVICE_TYPE_USB = 0x00,
    /* eth */
    IPA_CLIENT_DEVICE_TYPE_ETH = 0x01,
    /* eth1 */
    IPA_CLIENT_DEVICE_TYPE_ETH1 =  0x02,
    /* odu */
    IPA_CLIENT_DEVICE_TYPE_ODU = 0x03,
    /* wifi */
    IPA_CLIENT_DEVICE_TYPE_WLAN = 0x04,
    /* any */
    IPA_CLIENT_DEVICE_MAX = 0x05,
};

/**
 * WLAN Mode of Operation
 */
enum class IpaXmlWlanMode {
    /* Full Mode, the interface can be a STA or client */
    IPA_XML_MODE_FULL = 0x00,
    /* INTERNET mode */
    IPA_XML_MODE_INTERNET = 0x01,
};

/**
 * Interface Category
 */
enum class IpaXmlInterfaceCategory {
    /* Set Xml config to odu mode */
    IPA_XML_ODU = 0x01,
    /* Set Xml config to wlan mode */
    IPA_XML_WLAN = 0x02,
    /* Set Xml config to wan mode */
    IPA_XML_WAN = 0x03
};

/**
 * Device Configuration Mode
 */
enum class IpaVlanConfigMode {
    /* Set the configuration */
    IPA_CONFIG_MODE_SET = 0x00,
    /* Unset the configuration */
    IPA_CONFIG_MODE_UNSET = 0x01,
    /* query the configuration */
    IPA_CONFIG_MODE_QUERY = 0x02,
};

/**
 * Data type for IP Passthrough configuration.
 */
typedef struct
{
    /** status
     *  0 to disable IpPassthrough
     *  1 to enable IpPassThrough
     */
    bool enable;
    /* Device Type */
    IpaDeviceType  deviceType;
    /** vlan id
     *  0 in case if is for non vlan interface.
     *  vlan id in case of vlan interface.
     *  vlan id has a range of 1-4095
     */
    uint16_t vlanId;
    /** mac_addr
     *  Specify mac address in the format of 12:34:56:78:21:43
     *  Optional Parameter
     *  Needed only to enable passthrough.
     */
    std::string macAddr;
    /** skip_nat to specify if it is IP Passthrough with NAT or without NAT.
     *  0 if it is with NAT enabled.
     *  1 if it is without NAT.
     *  Optional Parameter
     *  Needed only to enable passthrough.
     */
    bool skipNat;
    /** default_pdn to specify if it is default or on-demand PDN
     *  0 if it is non-default PDN.
     *  1 if it is default PDN.
     *  Optional Parameter
     *  Needed only to enable passthrough.
     */
    bool defaultPdn;
    /** Iface
     *  Specify the rmnet interface to which the Passthrough client is mapped.
     */
    std::string iface;
} IpPassthroughConfig;

/**
 * Data type for IP Collision configuration.
 */
typedef struct
{
    /** status
     *  0 to disable IpCollision
     *  1 to enable IpCollision
     */
    bool enable;
    /** vlan id
     *  0 in case if is for non vlan interface.
     *  vlan id in case of vlan interface.
     *  vlan id has a range of 1-4095
     */
    uint16_t vlanId;
    /** default_pdn to specify if it is default or on-demand PDN
     *  0 if it is non-default PDN.
     *  1 if it is default PDN.
     */
    bool defaultPdn;
    /** Iface
     *  Specify the rmnet interface to which the the collision is enabled.
     */
    std::string iface;
    /** IPV4 address of rmnet interface in presentation (x.x.x.x) format
     *  ( x ranging from 0-255 )
     */
    std::string ipAddress;
} IpCollisionConfig;

/**
 * Data type for Lan Statistics configuration.
 */
typedef struct
{
    /** enable
     *  0 when client is in disconnected state & stops the tracking.
     *  1 when client is in connected state & enables the tracking.
     */
    bool enable;
    /** devName
     *  Specify the actual interface for which Lan statistics need to track.
     */
    std::string devName;
    /** mac_addr
     *  MAC address of the client for which LAN Statistics needs to be supported.
     *  Specify mac address in the format of 12:34:56:78:21:43
     */
    std::string macAddr;
} LanStatisticsConfig;

/**
 * Data type for requesting Lan Statistics.
 */
typedef struct
{
    /* Device Type */
    IpaDeviceType  deviceType;
    /** Query stats for a specific client or all clients of particular deviceType
     *  0 : For a specific client.
     *  1: For all clients.
     */
    bool allClients;
    /** Specify if the client needs to be disconnected
     *  1 when client is in disconnected state
     *  0 when client is in connected state
     *  Optional Parameter
     *  Needed only if query is for all / maximum clients.
     *  This indicates that all the clients of the dev type has to be in disconnected
     *  state to get the stats. If any of the client is in active state,
     *  EAGAIN will be returned
     */
    bool disconnectStatus;
    /** Specify if the stats needs to be reset after query
     *  1 to reset the stats
     *  0 to not reset the stats
     */
    bool reset;
    /** mac_addr
     *  MAC address of the client for which LAN Statistics needs to be queried.
     *  Specify mac address in the format of 12:34:56:78:21:43
     *  Optional Parameter
     *  Needed only when the stats are queried for one specific client
     */
    std::string macAddr;
} GetLanStatisticsConfig;

/**
 * Data type for Lan Statistics.
 */
typedef struct
{
    /** mac_addr
     *  MAC address of the client for which LAN Statistics is updated.
     */
    std::string macAddr;
    /* Tx Bytes */
    uint64_t txBytes;
    /* Rx Bytes */
    uint64_t rxBytes;
} LanStatistics;

/**
 * Data type for IP Address Segment configuration.
 */
typedef struct
{
    /* Start ipv4 address of the range in dotted decimal notation
     * Reference : 192.168.1.1
     */
    std::string startAddr;
    /* Start ipv4 address of the range in dotted decimal notation
     * Reference : 192.168.1.5
     */
    std::string endAddr;
} IpAddressSegment;

/**
 * Data type for Iface based SW Filtering configuration.
 */
typedef struct
{
  /**
   * Mandatory parameter.
   * Bitmask indicating the interface types
   *
   *  Bit 0 --> eth0
   *  Bit 1 --> wlan0
   *  Bit 2 --> wlan1
   *  Bit 3 --> wlan2
   *
   *   1 (0001) :  eth0
   *   3 (0011) :  wlan0 | eth0
   *  14 (1110) :  wlan2 | wlan1 | wlan0
   */
    unsigned int ifaceBitMask;
} IfaceBitMaskConfig;

/**
 * Data type for Packet Threshold configuration.
 */
typedef struct
{
    /** status
     *  0 to disable Packet Threshold
     *  1 to enable Packet Threshold
     */
    bool enable;
    /** Packet threshold value
     *  Optional Parameter
     *  Needs to be set only to enable Packet Threshold
     */
    uint64_t threshold;
} PacketThresholdConfig;

class IIpaListener;

/**
 * @brief Primary interface for managing IPA (IP Accelerator) functionality
 *
 * IIpaManager provides APIs to configure and control IPA hardware
 * accelerator features including IP passthrough, collision detection,
 * VLAN configuration, statistics monitoring, and software filtering.
 *
 * Typical usage:
 * 1. Get an instance through DataFactory::getIpaManager()
 * 2. Register a listener if status notifications are needed
 * 3. Configure IPA features using the provided APIs
 */
class IIpaManager
{
public:

/**
 * Checks the status of Ipa Manager and returns the result.
 *
 * @returns AVAILABLE    If Ipa Manager is ready for service.
 *          UNAVAILABLE  If Ipa Manager is temporarily unavailable.
 *          FAILED       If Ipa Manager encountered an irrecoverable failure.
 */
virtual telux::common::ServiceStatus getServiceStatus() = 0;

/**
 * Register IPA Manager as listener for Data Service events like
 * data service available or data service not available.
 *
 * @param [in] listener  pointer of IIpaListener object that processes the
 * notification
 *
 * @returns Status of registerListener success or suitable status code
 *
 */
virtual telux::common::Status registerListener(
    std::weak_ptr<IIpaListener> listener) = 0;

/**
 * Removes a previously added listener and status change events won't be
 * notified anymore to the client.
 *
 * @param [in] listener pointer of IIpaListener object that needs to be removed
 *
 * @returns Status of deregisterListener success or suitable status code
 *
 */
virtual telux::common::Status deregisterListener(
    std::weak_ptr<IIpaListener> listener) = 0;

/**
 * This function is called as a response to @ref  GetLanStatistics()
 *
 * @param [in] lanStatistics  list of lan stats per client
 */
using GetLanStatisticsResponseCb = std::function<void(
    const std::vector<LanStatistics> lanStatistics)>;

/**
 * @brief IP Passthrough Feature Description
 *
 * This feature enables assigning a public IP address to a client device on a
 * specified interface. It supports configurations with and without NAT and is
 * applicable to Ethernet, WLAN, and USB clients.
 *
 * General Behavior:
 * -----------------
 * - Provides the public IP on `dev_name` to the client using specified parameters:
 *     - Interface type (ETH/WLAN/USB)
 *     - VLAN ID
 *     - MAC address
 * - The public IP interface can be either a default PDN or a non-default PDN.
 * - Allows the client to originate traffic with a public IP, bypassing NAT on
 *   the IPA hardware of the DUT.
 * - The IP on `rmnet_pdn` is moved to a dummy IP.
 * - Applicable to ETH, WLAN, and USB clients (or potentially all clients).
 *
 * IP Passthrough With NAT:
 * ------------------------
 * - Private IPv4 clients coexist with the passthrough client.
 * - Clients can exist on the passthrough bridge and other bridges mapped to the same PDN.
 * - Embedded traffic is supported.
 *
 * IP Passthrough Without NAT:
 * ---------------------------
 * - Only the passthrough client retains IP connectivity.
 * - All other clients lose both IPv4 and IPv6 addresses.
 * - Embedded traffic is not supported.
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] ipPassthroughConfig  the ipa passthrough config set by user
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode setIpPassthrough(
    IpPassthroughConfig ipPassthroughConfig) = 0;

/**
 * Set Ip Collision API
 *
 * When the bridge ip on DUT is allocated manually and it is in the same subnet
 * as rmnet, there is a possibility that the Lan client would obtain the same IP
 * as the rmnet interface.
 *
 * To avoid this collision, ip on rmnet_datax will be modified to a dummy ip
 * address. IP based routing related to rmnet interface will be removed &
 * interface based routing will be added.
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] ipCollisionConfig  ipa collision config set by user
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode setIpCollision(
    IpCollisionConfig ipCollisionConfig) = 0;

/**
 * Set VLAN config API
 *
 * IPA needs to know on which peripheral VLAN is enabled.
 *
 * This API can be used to set any interface in non vlan mode or non-vlan mode.
 *
 * The interfaces for which vlan is enabled is updated in ipa_config.txt file.
 * Interfaces are needed only in set / unset modes with ecm rndis eth eth0 eth1
 * as valid ifaces
 *
 * Reboot is needed after this configuration is set.
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] mode       Mode (set / unset / query)
 * @param [in] ifaces     List of interfaces to configure in vlan mode.
 *                        Valid ifaces: ecm, rndis, eth, eth0, eth1 for vlan mode.
 *                        An empty vector is valid for query mode but will result
 *                        in an error for set/unset modes.
 *
 * @remarks None
 *
 * @return ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode setVlanConfig(IpaVlanConfigMode mode,
    const std::vector<std::string> ifaces) = 0;

/**
 * Enable Factory Reset
 *
 * This function is used for changing XML files owned by IPA to original
 * as on first bootup whenever user wants.
 *
 * Below are the files which will be reset
 *   IPACM_cfg.xml (via XML manipulations)
 *   ipa_config.txt (via VLAN Configurations)
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode setFactoryReset() = 0;

/**
 * Monitor LAN Statistics API
 *
 * This API needs to be invoked to monitor the Lan Statistics of an
 * interface.
 *
 * Installs counters in IPA HW to start monitoring the stats from
 * client with specified mac address, where the client is neighbor
 * of the specified device name.
 *
 * Removes the counters in IPA HW  & stops monitoring on client disconnect
 * request.
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] lanStatisticsConfig Vector of lan statistics configs set by user
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode monitorLanStatistics(
    const std::vector<LanStatisticsConfig> lanStatisticsConfig) = 0;

/**
 * Request LAN Statistics API
 *
 * This API provides the LAN statistics for the client
 * Interface name, number of client for which Lan Statistics needs to be
 * tracked needs to be provided.
 *
 * @param [in] lanStatisticsConfig  lan statistics config set by user
 * @param [in] callback    optional callback to get the lan stats
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode requestLanStatistics(
    GetLanStatisticsConfig lanStatisticsConfig,
    GetLanStatisticsResponseCb callback = nullptr) = 0;

/**
 * Update mode for Wlan interface API
 *
 * This API provides interfacing to modify the mode of wlan interface in
 * IPACM XML files.
 *
 * Nodes in IPACM_cfg.xml looks like below with iface name & Mode of operation
 *
 *  <Iface>
 *     <Name>wlan2</Name>
 *     <Category>WAN</Category>
 *     <WlanMode>full</WlanMode>
 *   </Iface>
 *
 * Reboot is needed after this configuration is set.
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] iface  wlan interface for which the Mode is configured interface
 * @param [in] wlanMode    Mode of operation for the interface
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode updateWlanMode(std::string iface,
        IpaXmlWlanMode wlanMode) = 0;

/**
 * Update Interface type API
 *
 * This API provides interfacing to modify the interface category in
 * IPACM XML files.
 *
 * Nodes in IPACM_cfg.xml looks like below with iface name, category under
 * which it falls ( LAN / WAN / ODU )
 *
 * 	<Iface>
 *     <Name>wlan2</Name>
 *     <Category>WAN</Category>
 *     <WlanMode>full</WlanMode>
 *   </Iface>
 *
 * Reboot is needed after this configuration is set.
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] iface          interface for which the Mode is configured interface 
 * @param [in] ifaceCategory  Category of the interface
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode updateInterfaceType(std::string iface,
            IpaXmlInterfaceCategory ifaceCategory) = 0;

/**
 * Set Packet Threshold API
 *
 * This API provides interfacing to control number of minimum packets
 * to flow over SW path across all interfaces before the NAT rules
 * will be installed for HW path to kick in
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] packetThresholdConfig   packet threshold config set by user
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *                to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode setPacketThreshold(
    PacketThresholdConfig packetThresholdConfig) = 0;

/**
 * Set Mac Based SwFiltering API
 *
 * This API provides interfacing to Switch packets to SW path from HW path
 * based on MAC address.
 *
 * A MAC based software filter rule will be added as part of this request.
 *
 * Traffic from the clients within these mac addresses will be routed
 * via SW path if enable is selected & will be routed back to HW path once
 * disable will be triggered.
 *
 * Submitting a request with new entries will not override existing filters.
 *
 * To disable software (SW) filtering for a specific client, a separate disable request
 * must be explicitly submitted.
 *
 * These entries are not persistent across reboots. The caller must submit the
 * request again after reboot.
 *
 * MAC addresses must be provided in hexadecimal format delimited by colons,
 * i.e. XX:XX:XX:XX:XX:XX
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] enable       enable flag set by user
 * @param [in] macAddresses mac address of the clients
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *          to change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode setMacBasedSwFiltering(bool enable,
    const std::vector<std::string> macAddresses) = 0;

/**
 * Set Ip Based SwFiltering API
 *
 * This API provides interfacing to Switch packets to SW path from HW path
 * based on IPV4 addresses.
 *
 * An IP based software filter rule will be added as part of this request.
 *
 * Traffic from the clients within these range of IP addresses will be routed
 * via SW path if enable is selected & will be routed back to HW path once
 * disable will be triggered.
 *
 * Submitting a request with new entries will not override existing filters.
 *
 * To disable software (SW) filtering for a specific segment, a separate disable
 * request must be explicitly submitted.
 *
 * These entries are not persistent across reboots. The caller must submit the
 * request again after reboot.
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] enable             enable flag set by user
 * @param [in] ipAddressSegments  vector of ip address segments of the clients
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject
 *          to change and could break backwards compatibility.
 *
 */
virtual telux::common::ErrorCode setIpBasedSwFiltering(bool enable,
    const std::vector<IpAddressSegment> ipAddressSegments) = 0;

/**
 * Set Interface Based SwFiltering API
 *
 * This API provides interfacing to Switch packets to SW path from HW path
 * based on interfaces.
 *
 * A MAC based software filter rule will be added as part of this request.
 *
 * Traffic from the clients over these interfaces will be routed via SW path
 * if enable is selected & will be routed back to HW path once disable will
 * be triggered.
 *
 * Submitting a request with new entries will not override existing filters.
 *
 * To disable software (SW) filtering for a specific iface, a separate disable
 * request must be explicitly submitted.
 *
 * These entries are not persistent across reboots. The caller must submit the
 * request again after reboot.
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] enable the enable flag set by user
 * @param [in] ifaceBitMask the interface bitmask set by user
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to
 *                change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode setInterfaceBasedSwFiltering(bool enable,
    IfaceBitMaskConfig ifaceBitMask) = 0;

/**
 * Enable SwFiltering with FILE path API
 *
 * This API provides interfacing to Switch packets to SW path from
 * HW path based on entries in a file.The entries can be mac, ip address
 * or an interface. Below is the suggested file format.
 *
 * Unified API designed to install rules based on MAC addresses, interfaces,
 * and IP segments
 *
 * File to be pushed to /data/ partition
 *
 * On platforms with Access control enabled, Caller needs to have
 * TELUX_DATA_NETWORK_CONFIG permission to invoke this API successfully.
 *
 * @param [in] file     File path with SW filter configurations set by user
 *
 *                      * File Format *
 *
 *                      # MAC Address List
 *                      AA:BB:CC:DD:EE:01
 *                      AA:BB:CC:DD:EE:02
 *                      # IP Address Segment List
 *                      192.168.1.10
 *                      192.168.1.20
 *                      # Interface List
 *                      wlan0
 *                      eth0
 *
 * @remarks None
 *
 * @returns ErrorCode indicating success or failure of the operation
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to
 *                change and could break backwards compatibility.
 */
virtual telux::common::ErrorCode enableFileBasedSwFiltering(std::string file) = 0;

/**
 * Destructor for IIpaManager
 */
~IIpaManager() {};
};

/**
 * Interface for Ipa listener object. Client needs to implement this
 * interface to get access to Ipa services notifications like
 * onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 */
class IIpaListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(
        telux::common::ServiceStatus status) {}

    /**
     * Destructor for IIpaListener
     */
    virtual ~IIpaListener(){};
};

}
}
#endif
