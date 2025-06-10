/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       StatsManager.hpp
 *
 * @brief      StatsManager is a primary interface that handles different types of
 *             packet stats requirement.
 *             Currently it supports enable/disable/reset packet stats and get
 *             DataUsage of client with certain ip/mac address.
 *
 */

#ifndef TELUX_DATA_NET_STATSMANAGER_HPP
#define TELUX_DATA_NET_STATSMANAGER_HPP

#include <vector>
#include <memory>

#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/common/SDKListener.hpp>

namespace telux {
namespace data {
namespace net {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IStatsListener;

/**
 * Stats type.
 * Certain platforms might not support all stats type.
 */
enum class StatsType {
    IP_BASED,   /**<   IP based stats. */
    MAC_BASED,  /**<   MAC based stats. */
};

/**
 * Stats config for the device.
 */
struct ClientStatsConfig {
    bool enable;      /**<   Enable or disable stats feature. */
    StatsType  type;  /**<   Stats type. */
};

/**
 * @brief Stats Manager class provides APIs related to different types of packet stats information.
 * Including set/reset packet stats feature, get the data usage of client with specific mac address.
 */
class IStatsManager {
public:

    /**
     * Checks the status of Stats manager object and returns the result.
     *
     * @returns SERVICE_AVAILABLE    -  If Stats manager object is ready for service.
     *          SERVICE_UNAVAILABLE  -  If Stats manager object is temporarily unavailable.
     *          SERVICE_FAILED       -  If Stats manager object encountered an irrecoverable
     *                                  failure.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Register listener with Stats manager for service status events and other notifications
     *
     * @param [in] listener    pointer of IStatsManager object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(
        std::weak_ptr<IStatsListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IStatsListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(
        std::weak_ptr<IStatsListener> listener) = 0;

    /**
     * Set data usage statistics which includes enable or disable operation and stats type.
     *
     * The data reported via @ref IStatsManager::getDeviceDataUsageStats and @ref
     * IClientManager::getConnectedDevicesInfo will be set once this API is called.
     *
     * @param [in] config         Stats Config to set @ref telux::data::net::StatsConfig.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_STATS_INFO
     * permission to successfully invoke this API.
     *
     * @returns     Return code for whether the operation succeeded or failed.
     *              If usage monitoring is not enabled, INVALID_STATE is returned.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ErrorCode setClientDataUsageStatsConfig(
        const ClientStatsConfig& config) = 0;

    /**
     * Get data usage for connected devices
     *
     * This API provides the usage of a backhaul (e.g. cellular WWAN connection) on the MDM by
     * various devices. The usage does not include any traffic sent between devices within the same
     * vehicle. A device is any entity with a unique MAC address that is connected to the MDM either
     * over a wired or wireless interconnect. Device data usage monitoring should be enabled for
     * this api to work. Status of device data usage monitoring can be obtained by using
     * @ref telux::data::net::IStatsManager::getDeviceDataUsageMonitoringInfo.
     *
     * Statistics are reset when a backhaul switch occurs, such as switching from a WWAN interface
     * to a WLAN interface. In this case the last known statistics of the device before the reset
     * will be provided via @ref IStatsManager::onClientDataUsageResetImminent. The statistics can
     * also be explicitly reset using @ref resetClientDataUsageStats. In this case, no
     * notification will be sent about the last known statistics before reset.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_STATS_INFO
     * permission to successfully invoke this API.
     *
     * @param [out] type               Current stats type @ref telux::data::net::StatsType.
     * @param [out] usageStats         List of data usage information, per device.
     *
     * @returns         Return code for whether the operation succeeded or failed
     *                  If usage monitoring is not enabled, INVALID_STATE is returned.
     *
     */
    virtual telux::common::ErrorCode getClientDataUsageStats(StatsType& type,
        std::vector<ClientDataUsage>& usageStats) = 0;

    /**
     * Reset client data usage statistics
     *
     * The data reported via @ref IStatsManager::getClientDataUsageStats will be reset once this
     * API is called.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_STATS_INFO
     * permission to successfully invoke this API.
     *
     * @returns     Return code for whether the operation succeeded or failed.
     *              If usage monitoring is not enabled, INVALID_STATE is returned.
     *
     */
    virtual telux::common::ErrorCode resetClientDataUsageStats() = 0;

    /**
     * Get client data usage monitoring information.
     *
     * This function can be used to obtain the current status of device data usage monitoring
     * and the stats type.
     *
     * @param [out] config           Current stats config, including stats type and enablement state.
     *
     * @returns    Return code for whether the operation succeeded or failed.
     *             If usage monitoring is not enabled, INVALID_STATE is returned.
     *
     */
    virtual telux::common::ErrorCode getClientDataUsageStatsConfig(ClientStatsConfig& config) = 0;

};

/**
 * Interface for Stats listener. Client needs to implement this interface to get
 * access to Stats services notifications like onServiceStatusChange.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 */
class IStatsListener : public telux::common::ISDKListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {}

    /**
     * Provides the last known statistics of a connected device, before the statistics become
     * unavailable or are reset.
     *
     * On platforms with access control enabled, the caller needs to have TELUX_DATA_STATS_INFO
     * permission to successfully invoke this API.
     *
     * @param [in] usageStats            List of disconnected device(with a unique MAC) data usage
     * @param [in] reason                The event/reason that triggered the data usage reset
     *
     */
    virtual void onClientDataUsageResetImminent(
        const std::vector<ClientDataUsage> devicesDataUsage,
        UsageResetReason reason) {}

    /**
     * Destructor for IStatsListener
     */
    virtual ~IStatsListener(){};
};

/** @} */ /* end_addtogroup telematics_data */
}
}
}

#endif
