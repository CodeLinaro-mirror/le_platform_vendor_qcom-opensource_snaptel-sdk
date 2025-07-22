/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       EZMeshManager.hpp
 *
 * @brief      EZMeshManager is the primary interface for EZMesh functionality.
 *             Including enable, disable, getConfig, enableServicePriority
 *             and manageApService.
 *
 */

#ifndef TELUX_WLAN_EZMESHMANAGER_HPP
#define TELUX_WLAN_EZMESHMANAGER_HPP

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/wlan/WlanDefines.hpp>
#include "ApInterfaceManager.hpp"

namespace telux {
namespace wlan {

class IEZMeshListener;

/** @addtogroup telematics_wlan
 * @{ */

/**
 *@brief EZMeshManager is the primary interface for EZMesh functionality
 *
 */

/**
 * EZmesh capability type
 */
enum class EZMeshCapability {
    R1 = 0x01, /**<  1 --ezmesh R1 capability   */
    R2 = 0x02, /**<  2 --ezmesh R2 capability  */
    R3 = 0x03, /**<  3 --ezmesh R3 capability  */
    R4 = 0x04, /**<  4 --ezmesh R4 capability  */
    R5 = 0x05, /**<  5 --ezmesh R5 capability  */
    R6 = 0x06, /**<  6 --ezmesh R6 capability  */
};

/**
 * EZmesh device mode enum
 */
enum class EZMeshDeviceMode {
    DEVICE_MODE_UNKNOWN = 0x00, /**<  EZMesh device mode is unknown  */
    CONTROLLER_MODE     = 0x01, /**<  Enable EZMesh in Controller device mode  */
    EXTENDER_MODE       = 0x02, /**<  Enable EZMesh in Extender device mode  */
};

/**
 * EZmesh AP type
 */
enum class EZMeshApType {
    PRIMARY_FRONTHAUL_AP = 0x01, /**<  1 -- Primary Fronthaul AP  */
    GUEST_FRONTHAUL_AP = 0x02,   /**<  2 -- Guest Fronthaul AP  */
    GUEST_FRONTHAUL_AP_2 = 0x03, /**<  3 -- Guest Fronthaul AP 2  */
    GUEST_FRONTHAUL_AP_3 = 0x04, /**<  4 -- Guest Fronthaul AP 3  */
    ADDITIONAL_FH_AP_1 = 0x05,   /**<  5 -- Additional Fronthaul AP 1  */
    ADDITIONAL_FH_AP_2 = 0x06,   /**<  6 -- Additional Fronthaul AP 2  */
    ADDITIONAL_FH_AP_3 = 0x07,   /**<  7 -- Additional Fronthaul AP 3  */
    BACKHAUL_AP_R1 = 0x08,       /**<  8 -- Backhaul AP with R1 capability AP  */
    BACKHAUL_AP_R2 = 0x09,       /**<  9 -- Backhaul AP with R2 capability AP  */
    SMART_MONITOR_AP = 0x0A,     /**<  10 -- Smart Monitor AP   */
};

/**
 * EZmesh Vap Mode
 */
enum class EZMeshVapMode {
    LEGACY_VAP = 0x00, /**<  VAPs on hwmode 11ax /
                             VAPs on 11be but not part of any mld  */
    MLO_VAP = 0x01, /**<  VAPs on hwmode 11be and part of an mld  */
};

/**
 * EZMesh Service Prioritization State
 */
enum class EZMeshServicePriority {
    INVALID = -1,   /**<  EZMesh Service Prioritization not configed */
    DISABLE = 0x01, /**<  EZMesh Service Prioritization disabled */
    ENABLE  = 0x02, /**<  EZMesh Service Prioritization enabled */
};

/**
 * EZmesh status
 */
enum class EZMeshStatus {
    DISABLED = 0x00,            /**<  EZMesh feature is in Disabled state    */
    DISABLE_IN_PROGRESS = 0x01, /**<  EZMesh feature disable is in progress  */
    DISABLING_FAILED = 0x02,    /**<  Disabling EZMesh feature failed        */
    ENABLING_FAILED = 0x03,     /**<  Enabling EZMesh feature failed         */
    ENABLE_IN_PROGRESS = 0x04,  /**<  EZMesh feature enable is in progress   */
    ENABLED = 0x05,             /**<  EZMesh feature is in Enabled state     */
};

/**
 * EZMesh AP config info
 */
struct EZMeshApConfig {
    BandType       band;     /**<   Guest AP band in GHz (2 GHz/5 GHz/6 GHz) */
    ApInterworking guestApProfile; /**<   Guest AP access profile for guest AP */
    EZMeshApType   apType;   /**<   Ezmesh AP type */
    Id             apIndex;  /**<   WLAN AP type (primary, guest, etc.) */
    EZMeshVapMode  vapMode;  /**<   Indicates VAP is MLO/Legacy  */
    Id             mldIndex; /**<   MLD AP type (primary, guest, etc.) */
};

/**
 * EZMesh vlan mapping info
 */
struct EZMeshVlanMapping {
    EZMeshApType fhApType;  /**<   Fronthaul AP Type */
    int16_t vlanId;  /**<   VLAN ID ranging between 1-4094 */
};

/** 
 * Used in set/get EZMesh configuration APIs
 */
struct EZMeshConfig {
    EZMeshDeviceMode                mode;        /* EZMesh mode including controller
                                                    or extender mode */
    EZMeshCapability                capability;  /* EZMesh capability type */
    bool                            trafficSeparation; /* flag for traffic separation via vlan*/
    std::vector<EZMeshApConfig>     apList;      /* List for EZMesh ap config */
    std::vector<EZMeshVlanMapping>  r2List;      /* List for EZMesh vlan mapping information */
    EZMeshServicePriority           state;       /* Service priority, applicable for R3 onwards */
};

/* MLD Activate Hostapd Config */
struct EZMeshActivateHostapdConfig {
    ServiceOperation opr;       /** Indicates the action to be performed.
                                    Certain platforms might not support all
                                    possible operations.*/
    std::vector<Id> apList;     /** List of non-mlo ezmesh ap entry */
    std::vector<Id> mldApList;  /** List of mlo ezmesh ap entry */
};

/** @addtogroup telematics_wlan
 * @{ */

/**
 *@brief     EZMeshManager is a primary interface for EZMesh functionality.
 *           it provide APIs to setEZMeshConfig, getEZMeshConfig, setEZMeshServicePriority
 *           and restartEZMeshApService.
 */
class IEZMeshManager {
 public:
     /**
      * Enable EZMesh service: used to enable EZMesh with capability, device mode,
      * R2config, AP config etc.
      *
      * @param [in] newConfig       Indicate if enable EZMesh with newconfig. When enable EZMesh
      *                             for the first time, newConfig must be true.
      *
      * @returns Immediate status of enable() request i.e. success or suitable status.
      *
      */
     virtual telux::common::ErrorCode enable(bool newConfig) = 0;

     /**
      * Disable EZMesh functionality.
      *
      * @param [in] cleanUp        Indicate if to cleanup the existing EZMesh config
      *                            when disable EZMesh.
      *
      * @returns Immediate status of disable() request i.e. success or suitable status.
      *
      */
     virtual telux::common::ErrorCode disable(bool cleanUp) = 0;

     /**
      * Set EZMesh config: used to set EZMesh with capability, device mode,
      * R2config, AP config etc.
      *
      * @param [in] config          The EZMesh config to set. @ref telux::wlan::EZMeshConfig
      *                             Including capability, device mode, R2config, AP config
      *                             and service prioritization state.
      *
      * @returns Immediate status of enable() request i.e. success or suitable status.
      *
      */
     virtual telux::common::ErrorCode setConfig(const EZMeshConfig& config) = 0;

     /**
      * Get EZMesh config: used to get EZMesh capability, device mode, R2config, AP config etc.
      *
      * @param [out] status          The EZMesh running status to get.
      *                             The status can be @ref telux::wlan::EZMeshStatus.
      * @param [out] config          The EZMesh config to get. @ref telux::wlan::EZMeshConfig
      *                             Including capability, device mode, R2config, AP config
      *                             and service prioritization state.
      *
      * @returns Immediate status of getConfig() request i.e. success or suitable status.
      *
      */
     virtual telux::common::ErrorCode getConfig(EZMeshStatus &status,
          EZMeshConfig& config) = 0;

     /**
      * Enable or disable EZMesh service priority.
      *
      * @param [in] enable           Indicate to enable or disable EZMesh service priority.
      *
      * @returns Immediate status of enableServicePriority() request i.e. success or suitable status.
      *
      */
     virtual telux::common::ErrorCode enableServicePriority(bool enable) = 0;

     /**
      * Execute an operation on EZMesh hostapd service. Provides ability for client to restart, start
      * or stop hostapd service for selected EZMesh access point.
      *
      * @param [in] config          EZMesh activate hostapd config.
      *                             @ref telux::wlan::EZMeshActivateHostapdConfig
      *
      * @returns Immediate status of manageApService() request i.e. success or suitable status.
      *
      */
     virtual telux::common::ErrorCode manageApService(const EZMeshActivateHostapdConfig& config) = 0;

     /**
     * Register a listener for specific events in the Wlan Manager
     *
     * @param [in] listener    pointer of IEZMeshListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<IEZMeshListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IEZMeshListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::ErrorCode deregisterListener(std::weak_ptr<IEZMeshListener> listener) = 0;

    /**
     * Destructor for IEZMeshManager
     */
    virtual ~IEZMeshManager(){};
};  // end of IEZMeshManager

class IEZMeshListener : public telux::common::ISDKListener {
public:
    /**
     * This function is called when AP device status has changed
     *
     * @param [in] event       Event detected on device @ref telux::wlan::ApDeviceConnectionEvent
     * @param [in] info        Info about devices @ref telux::wlan::DeviceIndInfo
     */
    virtual void onDeviceStatusChanged(ApDeviceConnectionEvent event,
        std::vector<DeviceIndInfo> info) {}

    /**
     * This function is called when AP configuration has changed
     *
     * @param [in] apId        @ref telux::wlan::Id of Ap it's configuration has changed
     */
    virtual void onApConfigChanged(Id apId) {}

    virtual ~IEZMeshListener() {}
};

/** @} */ /* end_addtogroup telematics_wlan */
}
}

#endif // TELUX_WLAN_EZMESHMANAGER_HPP

