/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_DATA_NET_QOSMANAGER_HPP
#define TELUX_DATA_NET_QOSMANAGER_HPP

#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataDefines.hpp>
#include <telux/data/TrafficFilter.hpp>

namespace telux {
namespace data {
namespace net {

/**
 * @brief Provides a way to distinguish the data path. It indicates how data
 * transfers within internal components.
 */
enum class DataPath {
    TETHERED_TO_WAN_HW = 0, /** Hardware-accelerated data path from tethered client to WAN */
    TETHERED_TO_APPS_SW     /** Software data path from tethered client to to SW
                               running on Apps processor */
};

/**
 * Type of bandwidth associated with traffic class
 */
enum class BandwidthConfigType {
    BW_RANGE = 1, /**< Bandwidth range */
};

struct BandwidthRange {
    uint32_t minBandwidth; /**< minimum bandwidth in Mbps */
    uint32_t maxBandwidth; /**< maximum bandwidth in Mbps */
};

union BandwidthValue {
    BandwidthRange bandwidthRange; /**< Bandwidth in range
                                        The sum of the minimum bandwidths across all traffic
                                      classes should not exceed the link capacity. */
};

/**
 * @brief Bandwidth configuration
 */
struct BandwidthConfig {
    BandwidthConfigType dlBandwidthConfigType; /**< Type of dl bandwidth */
    BandwidthValue dlBandwidthValue;           /**< Value of dl bandwidth */

    void setDlBandwidthRange(uint32_t minBandwidth, uint32_t maxBandwidth) {
        dlBandwidthConfigType = BandwidthConfigType::BW_RANGE;
        dlBandwidthValue.bandwidthRange.minBandwidth = minBandwidth;
        dlBandwidthValue.bandwidthRange.maxBandwidth = maxBandwidth;
    }
};

/**
 * @brief Possible error codes while adding QoS filter config @ref addQoSFilter
 */
enum class QoSFilterErrorCode {
    SUCCESS = 0,
    MISSING_DIRECTION,                 /** Mandatory field 'data traffic direction' is missing */
    INVALID_MULTIPLE_SOURCE_INFO,      /** If Traffic descriptor is set, expect only
                                          one of the following: source IPv4, IPv6, VLAN
                                        */
    INVALID_MULTIPLE_DESTINATION_INFO, /** If Traffic descriptor is set, expect
                                          only one of the following: destination
                                          IPv4 or IPv6 */
};

/**
 * @brief Possible error codes while creating traffic class @ref
 * createTrafficClass.
 */
enum class TcConfigErrorCode {
    SUCCESS = 0,
    MISSING_TRAFFIC_CLASS, /** Mandatory field 'traffic class' is missing */
    MISSING_DATA_PATH,     /** Mandatory field software path or hardware IPA path is
                              missing */
    MISSING_DIRECTION,     /** Mandatory field 'data traffic direction' is missing */
};

/**
 * @brief Provide valid parameters in @ref TcConfig
 */
enum TcConfigValidField {
    TC_TRAFFIC_CLASS_VALID = (1 << 0),
    TC_DIRECTION_VALID = (1 << 1),
    TC_DATA_PATH_VALID = (1 << 2),
    TC_BANDWIDTH_CONFIG_VALID = (1 << 3),
};

/**
 * Bitmask containing TcConfigValidField bits,
 * e.g., a value of 0x5 represents that source IPv4 and ports are valid.
 */
using TcConfigValidFields = uint32_t;

/**
 * @brief Traffic class configuration.
 * The traffic class configuration contains the traffic class number, direction,
 * data path, and bandwidth configuration.
 *
 * @note Use getTcConfigValidFields to obtain a bitmask of @ref
 * TcConfigValidField, which indicates which fields are valid.
 */
class ITcConfig {
 public:
    /**
     * @brief Get the Traffic Class config Valid Fields
     * This function can be used to check whether the respective parameter is
     * valid.
     *
     * @return TcConfigValidFields bit mask
     */
    virtual TcConfigValidFields getTcConfigValidFields() = 0;

    /**
     * @brief Returns the traffic class.
     *
     * @return TrafficClass representing the traffic class.
     */
    virtual TrafficClass getTrafficClass() = 0;

    /**
     * @brief Returns the direction (e.g., UPLINK, DOWNLINK).
     *
     * @return Direction enum representing the traffic direction.
     */
    virtual Direction getDirection() = 0;

    /**
     * @brief Get the data path of the QoS filter.
     *
     * @return DataPath enum representing the data path.
     */
    virtual DataPath getDataPath() = 0;

    /**
     * @brief Get the bandwidth configuration.
     *
     * @return BandwidthConfig representing the andwidth configuration.
     */
    virtual BandwidthConfig getBandwidthConfig() = 0;

    /**
     * @brief Converts the API object to a human-readable string.
     * @return A string representation of the API state.
     */
    virtual std::string toString() = 0;
};

/**
 * @brief Traffic class config builder is used to build @ref ITcConfig.
 * Set the expected parameters, and then call the @ref TcConfigBuilder::build
 * method.
 */
class TcConfigBuilder {
 public:
    /**
     * @brief Sets the traffic class for the filter configuration.
     *
     * @param [in] trafficClass     The desired traffic class.
     * @return Reference to this builder for method chaining.
     */
    TcConfigBuilder &setTrafficClass(TrafficClass trafficClass);

    /**
     * @brief Sets the direction for the filter configuration.
     *
     * @param [in] direction    The desired direction.
     * @return Reference to this builder for method chaining.
     */
    TcConfigBuilder &setDirection(Direction direction);

    /**
     * @brief Sets the expected data path ( @ref DataPath ) for the QoS filter. It
     * indicates how data transfers are expected to happen within internal
     * components.
     *
     * @param [in] dataPath     Expected data path
     * @return Reference to this builder for method chaining.
     */
    TcConfigBuilder &setDataPath(DataPath dataPath);

    /**
     * @brief Set the bandwidth configuration.
     *
     * @param [in] bandwidthConfig     Expected bandwidth configuration
     * @return Reference to this builder for method chaining.
     */
    TcConfigBuilder &setBandwidthConfig(BandwidthConfig bandwidthConfig);

    /**
     * @brief Builds the traffic class configuration.
     *
     * @return Shared pointer to the constructed traffic class configuration.
     */
    std::shared_ptr<ITcConfig> build();

 private:
    std::shared_ptr<ITcConfig> tcConfig_ = nullptr;
};

/**
 * @brief Handle of QoS filter @ref IQoSFilter
 */
using QoSFilterHandle = uint32_t;

/**
 * @brief QoS filter configuration
 * It is combination of traffic class and traffic filter
 */
struct QoSFilterConfig {
    TrafficClass trafficClass;
    std::shared_ptr<ITrafficFilter> trafficFilter;
};

/**
 * @brief QoS Filter information
 * It provides QoS filter handle, and QoS filter config such as traffic class
 * number and traffic filter.
 */
class IQoSFilter {
 public:
    static const QoSFilterHandle INVALID_HANDLE = 0;

    /**
     * @brief Returns the Quality of Service (QoS) filter handle.
     *
     * @return QoS filter handle as a @ref QoSFilterHandle.
     */
    virtual QoSFilterHandle getHandle() = 0;

    /**
     * @brief Returns the traffic class.
     *
     * @return TrafficClass representing the traffic class.
     */
    virtual TrafficClass getTrafficClass() = 0;

    /**
     * @brief Returns a shared pointer to the traffic descriptor.
     *
     * @return Shared pointer to ITrafficFilter.
     */
    virtual std::shared_ptr<ITrafficFilter> getTrafficFilter() = 0;

    /**
     * @brief Converts the API object to a human-readable string.
     * @return A string representation of the API state.
     */
    virtual std::string toString() = 0;
};

// Forward declarations
class IQoSListener;

/**
 * @brief The QoS Manager class provides a set of APIs related to Quality of
 * Service (QoS) for the various data flows that flow via the NAD. Its purpose
 * is to manage aspects like assigning priority to the data flow, limiting the
 * bandwidth of each flow, relative to other flows, etc.
 *
 * Here are the key points:
 *    - Data Flow Identification @ref ITrafficFilter :
 *          - Data flows can be identified using various parameters from network
 * layers 2, 3, and 4.
 *          - These parameters include: Five-tuple (source and destination IP
 * addresses, source and destination port numbers, and IP protocol), VLAN ID,
 * and PCP number (assigned to VLAN using @ref IVlanManager::createVlan) etc.
 *          - A data flow is described using a Traffic Filter @ref
 * ITrafficFilter. A Traffic Filter is created using @ref TrafficFilterBuilder.
 *
 *    - Traffic Classes:
 *          - A traffic class is similar to a class in Linux traffic control
 * (tc).
 *          - Each traffic class can have multiple associated data flows.
 *          - Each traffic class is identified by a unique ID. Traffic class IDs
 * start from 0 (highest priority) and go up to the maximum allowed traffic
 * class.
 *          - Lower value of Traffic Class corresponds to higher priority.
 *
 *    - Traffic bandwidth configuration:
 *          - One can specify constraints/limits on the bandwidth that each
 * Traffic class is allowed using @ref createTrafficClass.
 *          - Currently this is used to configure the bandwidth on the traffic
 * egressing the NAD via the Eth link, to other devices/ECUs.
 *
 *    - Creating Qos filter:
 *          - Associating a data flow with a traffic class allows one to assign
 * relative priorities between the data flows and QoS filter.
 *          - This association is done by @ref QoSFilterConfig
 *          - Once a QoS filter config is created, it needs to be added to the
 * system using
 *            @ref addQoSFilter. Adding a filter returns a handle. This handle
 * can then be used to perform operations like deleting a QoS filter @ref
 * deleteQosFilter
 */
class IQoSManager {
 public:
    /**
     * Checks the status of QoS manager and returns the result.
     *
     * @returns SERVICE_AVAILABLE     If QoS manager object is ready for service.
     *          SERVICE_UNAVAILABLE   If QoS manager object is temporarily
     * unavailable. SERVICE_FAILED        If QoS manager object encountered an
     * irrecoverable failure
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * @brief Create traffic class.
     *
     * To create a traffic class, provide the traffic class configuration using
     * @ref ITcConfig, which is constructed using @ref TcConfigBuilder. Traffic
     * classes are uniquely identified by their traffic class number and
     * direction. Additionally, the data path (hardware accelerated or software
     * path) is a mandatory parameter. Also, an optional parameter bandwidth
     * configuration can be provided for the downlink direction (traffic egressing
     * the NAD via the Ethernet link).
     *
     * If any attribute of the traffic class needs to be updated (e.g. bandwidth),
     * - Delete the existing traffic class using @ref deleteTrafficClass. This
     * action will also delete all QoS filters associated with that traffic class.
     * - Create the traffic class with the updated configuration.
     * - Create and add required QoS filters using @ref addQoSFilter.
     *
     * Traffic class creation is persistence across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have
     * TELUX_DATA_QOS_OPS permission to invoke this API successfully.
     *
     * @param [in] tcConfig             Traffic class configuration.
     * @param [out] tcConfigErrorCode   Error code specific to @ref ITcConfig
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode createTrafficClass(
        std::shared_ptr<ITcConfig> tcConfig, TcConfigErrorCode &tcConfigErrorCode)
        = 0;

    /**
     * @brief Retrieves all traffic class configurations.
     *
     * @param [out] tcConfigs     Vector of traffic class configurations.
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getAllTrafficClasses(
        std::vector<std::shared_ptr<ITcConfig>> &tcConfigs)
        = 0;

    /**
     * @brief Deletes a traffic class.
     *
     * To delete a traffic class, provide the traffic class configuration using
     * @ref ITcConfig. The traffic class configuration is built via @ref
     * TcConfigBuilder. The traffic class number and direction are mandatory
     * parameters that need to be set via the builder.
     *
     * On platforms with Access control enabled, Caller needs to have
     * TELUX_DATA_QOS_OPS permission to invoke this API successfully.
     *
     * @param [in] tcConfig   Traffic class config
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deleteTrafficClass(std::shared_ptr<ITcConfig> tcConfig) = 0;

    /**
     * @brief Adds a QoS filter.
     *
     * A QoS filter configuration ( @ref QoSFilterConfig) associates data flow
     * identifiers ( @ref ITrafficFilter) with a traffic class. The traffic filter
     * is constructed using @ref TrafficFilterBuilder. The direction is a
     * mandatory parameter that must be set via the @ref TrafficFilterBuilder
     * Other parameters are optional. In a single traffic filter, multiple source
     * or destination information are not expected. For example, when dealing with
     * the @ref FieldType::SOURCE, only one of the following options can be set:
     * @ref TrafficFilterBuilder::setIPv4Address, @ref
     * TrafficFilterBuilder::setIPv6Address, or
     * @ref TrafficFilterBuilder::setVlanList.
     * The same rule applies to the @ref FieldType::DESTINATION.
     *
     * Associating a data flow with a traffic class allows one to assign relative
     * priorities between the data flows and build QoS filter. Adding a filter
     * returns a handle. This handle can then be used to perform operations like
     * deleting a QoS filter @ref deleteQosFilter
     *
     * If any attribute of the QoS filter needs to be updated,
     * - Delete the existing QoS filter using @ref deleteQosFilter.
     * - Create and add the QoS filter with the updated configuration.
     *
     * Once a QoS filter is added, it remains persistent across reboots.
     *
     * On platforms with Access control enabled, Caller needs to have
     * TELUX_DATA_QOS_OPS permission to invoke this API successfully.
     *
     * @param [in]  qosFilterConfig     QoS filter configuration
     * @param [out] filterHandle        On successful addition QoS filter handle
     * will be provided
     * @param [out] qosFilterErrorCode  Error code specific to @ref
     * QoSFilterConfig
     * @return Error code which indicates whether the operation succeeded or not.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode addQoSFilter(QoSFilterConfig qosFilterConfig,
        QoSFilterHandle &filterHandle, QoSFilterErrorCode &qosFilterErrorCode)
        = 0;

    /**
     * @brief Retrieves information about existing QoS policies.
     *
     * @param [out] qosFilters     Vector of shared pointers to IQoSFilter.
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getQosFilters(
        std::vector<std::shared_ptr<IQoSFilter>> &qosFilters)
        = 0;

    /**
     * @brief Deletes a QoS filter.
     *
     * QoS filter handle is used to delete QoS filter. QoS filter handle can be
     * obtained by two ways
     * 1. Using filter handle provided during addition of QoS filter @ref
     * addQoSFilter
     * 2. Get QoS policies @ref getQosFilters provide IQoSFilter which has
     *    @ref IQoSFilter::getHandle to get filter handle.
     *
     * On platforms with Access control enabled, Caller needs to have
     * TELUX_DATA_QOS_OPS permission to invoke this API successfully.
     *
     * @param [in] qosFilterHandle      QoS filter handle to be deleted.
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deleteQosFilter(QoSFilterHandle qosFilterHandle) = 0;

    /**
     * @brief Deletes all traffic classes and QoS policies.
     *
     * This API will delete all configurations added via @ref addQoSFilter and
     * @ref createTrafficClass.
     *
     * On platforms with Access control enabled, Caller needs to have
     * TELUX_DATA_QOS_OPS permission to invoke this API successfully.
     *
     * @return Error code which indicates whether the operation succeeded or not
     *         @ref telux::common::ErrorCode
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     * change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deleteAllQosConfigs() = 0;

    /**
     * Register QoS Manager as a listener for QoS Service health events like QoS
     * service available or QoS service not available.
     *
     * @param [in] listener    pointer of IQoSListener object that processes the
     * notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IQoSListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    pointer of IQoSListener object that needs to be
     * removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IQoSListener> listener) = 0;

    /**
     * Destructor for IQoSManager
     */
    virtual ~IQoSManager(){};
};  // end of IQoSManager

class IQoSListener {
 public:
    /**
     * This function is called when service status changes.
     *
     * @param [in] status - @ref ServiceStatus
     */
    virtual void onServiceStatusChange(telux::common::ServiceStatus status) {
    }

    /**
     * Destructor for IQoSListener
     */
    virtual ~IQoSListener(){};
};

/** @} */ /* end_addtogroup telematics_data_net */
}  // namespace net
}  // namespace data
}  // namespace telux
#endif  // TELUX_DATA_NET_QOSMANAGER_HPP
