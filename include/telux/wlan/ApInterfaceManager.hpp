/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       ApInterfaceManager.hpp
 *
 * @brief      Primary interface for Wi-Fi Access Points.
 *             It provide APIs for Access Points configurations and management.
 *
 */

#ifndef TELUX_WLAN_APINTERFACE_HPP
#define TELUX_WLAN_APINTERFACE_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDefines.hpp>

namespace telux {
namespace wlan {

class IApListener;

/** @addtogroup telematics_wlan
 * @{ */

#define INVALID_AP_ID    0

/**
 * AP Interworking Information
 */
enum class ApInterworking {
    INTERNET_ACCESS     = 0,   /**<  AP with internet access only - No LAN access   */
    FULL_ACCESS         = 1    /**<  AP Can Access LAN and Internet                 */
};

/**
 * AP Client Connection Status
 */
enum class ApDeviceConnectionEvent {
    CONNECTED    = 0,
    DISCONNECTED = 1,
    IPV4_UPDATED = 2,
    IPV6_UPDATED = 3,
};

/**
 * Ap Network Configuration
 */
struct ApNetConfig {
    ApInfo           info;                 /**< AP type                                      */
    ApInterworking   interworking;         /**< AP network access (internet/local)           */
};

/**
 * Ap Configuration
 */
struct ApConfig {
    Id          id;                        /**< AP id                                         */
    std::vector<ApNetConfig> network;      /**< Configurations supported by AP                */
};

/**
 * Wlan Client Device Indication Info
 */
struct DeviceIndInfo {
    Id           id;                   /**<  AP id device is connected to                      */
    std::string  macAddress;           /**<  MAC Address of Wi-Fi device                       */
};

/**
 * Wlan Client Device Info
 */
struct DeviceInfo {
    Id           id;                   /**<  AP id device is connected to                      */
    std::string  name;                 /**<  User friendly string that identifies Wi-Fi device */
    std::string  ipv4Address;          /**<  IPv4 Address of Wi-Fi device                      */
    std::string  ipv6Address;          /**<  IPv6 Address of Wi-Fi device                      */
    std::string  macAddress;           /**<  MAC Address of Wi-Fi device                       */
};

/**
 * Wlan Client Device Statistics
 */
struct DeviceStats {
    Id           id;                   /**<  AP id device is connected to */
    std::string  macAddress;           /**<  MAC Address of Wi-Fi device  */
    uint64_t     bytesTx;              /**< Number of bytes transmitted   */
    uint64_t     bytesRx;              /**< Number of bytes received      */
};

/** @addtogroup telematics_wlan_ap
 * @{ */

/**
 * @brief   Manager class for configuring Wlan Access Points
 */
class IApInterfaceManager {
 public:
    /**
     * Set Access Point config: Used to fully configure access points including venue type,
     * radio type (2.4/5 GHz), private/guest network and all other related settings.
     * Configurations will take effect after hostapd service is restarted by calling
     * @ref telux::wlan::IApInterfaceManager::manageApService.
     *
     * @param [in] config       AP configuration parameters @ref telux::wlan::ApConfig
     *
     * @returns  operation error code (if any). @ref telux::common::ErrorCode
     *           telux::common::Status::NOTALLOWED is returned if AP to be configured was not
     *           enabled in @ref telux::wlan::WlanDeviceManager::setMode.
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
     virtual telux::common::ErrorCode setConfig(ApConfig config) = 0;

    /**
     * Request Access Point Configurations
     *
     * @param [in] config         Vector of AP configurations @ref telux::wlan::ApConfig as set by
     *                            @ref telux::wlan::IApInterfaceManager::setConfig
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
     virtual telux::common::ErrorCode getConfig(std::vector<ApConfig>& config) = 0;

    /**
     * Request AP Status
     *
     * @param [in] status         Vector of AP network Status @ref telux::wlan::ApStatus
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
     virtual telux::common::ErrorCode getStatus(std::vector<ApStatus>& status) = 0;

    /**
     * Request Connected Devices to all enabled access points.
     * Each entry in returned list will contain information about a device such as access point
     * it is connected to and IP and MAC address as defined in @ref telux::wlan::DeviceInfo
     *
     * @param [in] clientsInfo     List of connected devices Info @ref telux::wlan::DeviceInfo
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode getConnectedDevices(std::vector<DeviceInfo>& clientsInfo) = 0;

    /**
     * Execute an operation on hostapd service. Provides ability for client to either stop/start or
     * restart hostapd service for selected access point. Restarting hostapd service is required
     * for any changes made to hosapd.conf file and changes made by
     * @ref telux::wlan::IApInterfaceManager::setConfig to take effect.
     * Stop/Start operation @ref telux::wlan::ServiceOperation will Stop/Start WiFi service for
     * access point.
     * Access points selected to execute operation on, will temporarily go out of service when this
     * API is called.
     * This API should be called only when access point is configured through
     * @ref telux::wlan::IDeviceManager::setMode
     *
     * @param [in] apId          AP identifier to execute operation on. @ref telux::wlan::Id
     * @param [in] opr           Operation to be performed on hostapd
     *                           @ref telux::wlan::ServiceOperation
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::ErrorCode manageApService(Id apId, ServiceOperation opr) = 0;

    /**
     * Register a listener for specific events in Access Point Manager
     *
     * @param [in] listener    pointer of IApListener object that processes the
     * notification
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IApListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IApListener object that needs to be removed
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     */
    virtual telux::common::ErrorCode deregisterListener(std::weak_ptr<IApListener> listener) = 0;

     virtual ~IApInterfaceManager(){};

     /**
     * Deprecated APIs
     *
     */

    /**
     * Request statistics for all devices connected to all access points.
     * Each entry in returned list will contains transmitted and recieved bytes for a device
     * as defined in @ref telux::common::DeviceStats
     *
     * @param [in] clientStats    List of connected clients statistics @ref telux::wlan::DeviceStats
     *
     * @returns operation error code (if any). @ref telux::common::ErrorCode
     *
     * @deprecated This API is no longer supported.
     *
     */
    virtual telux::common::ErrorCode getConnectedDevicesStats(
        std::vector<DeviceStats>& clientsStats) = 0;
};

class IApListener {
public:
    /**
     * This function is called when AP device status has changed
     *
     * @param [in] event       Event detected on device @ref telux::wlan::ApDeviceConnectionEvent
     * @param [in] info        Info about devices @ref telux::wlan::DeviceIndInfo
     */
    virtual void onApDeviceStatusChanged(ApDeviceConnectionEvent event,
        std::vector<DeviceIndInfo> info) {}

    /**
     * This function is called when AP switch to different operation band
     *
     * @param [in] radio        New AP operation band @ref telux::wlan::BandType
     */
    virtual void onApBandChanged(BandType radio) {}

    virtual ~IApListener() {}

     /**
     * Deprecated APIs
     *
     */

    /**
     * This function is called when AP device status has changed
     *
     * @param [in] event       Event detected on device @ref telux::wlan::ApDeviceConnectionEvent
     * @param [in] info        Info about devices @ref telux::wlan::DeviceInfo
     *
     * @note The name, ipv4Address, and ipv6Address fields from @ref telux::wlan::DeviceInfo are
     *       not populated in the indication data. please invoke
     *       @ref telux::wlan::IApInterfaceManager::getConnectedDevices for these details.
     */
     virtual void onApDeviceStatusChanged(ApDeviceConnectionEvent event,
        std::vector<DeviceInfo> info) {}
};

/** @} */ /* end_addtogroup telematics_wlan */
}
}
#endif
