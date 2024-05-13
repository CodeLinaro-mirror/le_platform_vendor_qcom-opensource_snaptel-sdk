/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_DATA_TRAFFICFILTER_HPP
#define TELUX_DATA_TRAFFICFILTER_HPP

#include <memory>
#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {

/**
 * @brief Provide valid parameters in @ref ITrafficFilter
 */
enum TrafficFilterValidField {
    TF_DIRECTION_VALID = (1 << 0),
    TF_PCP_VALID = (1 << 1),
    TF_IP_PROTOCOL_VALID = (1 << 2),
    TF_SOURCE_IPV4_ADDRESS_VALID = (1 << 3),
    TF_SOURCE_IPV6_ADDRESS_VALID = (1 << 4),
    TF_SOURCE_PORT_VALID = (1 << 5),
    TF_SOURCE_VLAN_LIST_VALID = (1 << 6),
    TF_DESTINATION_IPV4_ADDRESS_VALID = (1 << 7),
    TF_DESTINATION_IPV6_ADDRESS_VALID = (1 << 8),
    TF_DESTINATION_PORT_VALID = (1 << 9),
    TF_DESTINATION_VLAN_LIST_VALID = (1 << 10),
};

/**
 * Bitmask containing TrafficFilterValidField bits,
 * e.g., a value of 0x5 represents that source IPv4 and ports are valid.
 */
using TrafficFilterValidFields = uint32_t;

/**
 * @brief Indicates whether the parameter is for the source or destination.
 */
enum class FieldType {
    SOURCE,
    DESTINATION,
};

/**
 * @brief Traffic filter is group of generic data flow identification via source
 * info, destination info and protocol which is built via @ref
 * TrafficFilterBuilder.
 */
class ITrafficFilter {
 public:
    /**
     * @brief Get the Traffic Filter Valid Fields
     * This function can be used to check whether the respective parameter is
     * valid.
     *
     * @return TrafficFilterValidFields bit mask
     */
    virtual TrafficFilterValidFields getTrafficFilterValidFields() = 0;

    /**
     * @brief Returns the direction (e.g., UPLINK, DOWNLINK).
     *
     * @return Direction enum representing the traffic direction.
     */
    virtual Direction getDirection() = 0;

    /**
     * @brief Returns the Priority Code Point (PCP) value.
     *
     * @return PCP value as an int8_t.
     */
    virtual int8_t getPCP() = 0;

    /**
     * @brief Retrieves the IP protocol.
     * @return The IP protocol value @ref IpProtocol.
     */
    virtual IpProtocol getIPProtocol() = 0;

    /**
     * @brief Retrieves the IPv4 Address
     *
     * @param [out] ipv4Addr    IPv4 address
     * @param [in]  fieldType   Indicates whether get is for the source or
     * destination.
     */
    virtual std::string getIPv4Address(FieldType fieldType) = 0;

    /**
     * @brief Retrieves the IPv6 address.
     *
     * @param [in]  fieldType   Indicates whether get is for the source or
     * destination.
     * @return IPv6 address.
     */
    virtual std::string getIPv6Address(FieldType fieldType) = 0;

    /**
     * @brief Retrieves a vector of valid ports.
     *
     * @param [in] fieldType     Indicates whether get is for the source or
     * destination.
     * @return Source port.
     */
    virtual int getPort(FieldType fieldType) = 0;

    /**
     * @brief Retrieves the list of VLANs.
     *
     * @param [in] fieldType     Indicates whether get is for the source or
     * destination.
     * @return A vector of integers representing the source VLANs.
     */
    virtual std::vector<int> getVlanList(FieldType fieldType) = 0;

    /**
     * @brief Converts the API object to a human-readable string.
     * @return A string representation of the API state.
     */
    virtual std::string toString() = 0;

    /**
     * Destructor for ITrafficFilter
     */
    virtual ~ITrafficFilter() {
    }
};

/**
 * @brief Traffic Filter Builder is used to build @ref ITrafficFilter.
 *
 * Set the expected parameters, and then call the @ref
 * TrafficFilterBuilder::build method. It will return an instance of @ref
 * ITrafficFilter.
 */
class TrafficFilterBuilder {
 public:
    /**
     * @brief Constructs a TrafficFilterBuilder.
     */
    TrafficFilterBuilder();

    /**
     * @brief Builds the traffic filter.
     * @return Shared pointer to the constructed traffic filter.
     */
    std::shared_ptr<ITrafficFilter> build();

    /**
     * @brief Sets the direction for the filter configuration.
     *
     * @param [in] direction    The desired direction.
     * @return Reference to this builder for method chaining.
     */
    TrafficFilterBuilder &setDirection(Direction direction);

    /**
     * @brief Sets the priority code point (PCP) for the filter configuration.
     *
     * @param [in] pcp      The PCP value.
     * @return Reference to this builder for method chaining.
     */
    TrafficFilterBuilder &setPCP(int8_t pcp);

    /**
     * @brief Sets the IP protocol. The protocol numbers are defined by Internet
     * Assigned Numbers Authority (IANA)
     *
     * @param [in] ipProtocol   IP protocol (e.g., TCP, UDP).
     * @return Reference to this builder for method chaining.
     */
    TrafficFilterBuilder &setIPProtocol(IpProtocol ipProtocol);

    /**
     * @brief Sets the IPv4 address and subnet.
     *
     * @param [in] ipv4Addr     IPv4 address.
     * @param [in] fieldType    Indicates whether the set is for the source or
     * destination.
     * @return Reference to this builder for method chaining.
     */
    TrafficFilterBuilder &setIPv4Address(std::string ipv4Addr, FieldType fieldType);

    /**
     * @brief Sets the IPv6 address and prefix length.
     *
     * @param [in] ipv6Addr     IPv6 address.
     * @param [in] fieldType    Indicates whether the set is for the source or
     * destination.
     * @return Reference to this builder for method chaining.
     */
    TrafficFilterBuilder &setIPv6Address(std::string ipv6Addr, FieldType fieldType);

    /**
     * @brief Sets the port.
     *
     * @param [in] port         port number
     * @param [in] fieldType    Indicates whether the set is for the source or
     * destination.
     * @return Reference to this builder for method chaining.
     */
    TrafficFilterBuilder &setPort(int port, FieldType fieldType);

    /**
     * @brief Sets the source VLAN list.
     *
     * @param [in] vlanList     Vector of VLAN IDs.
     * @param [in] fieldType    Indicates whether the set is for the source or
     * destination.
     * @return Reference to this builder for method chaining.
     */
    TrafficFilterBuilder &setVlanList(std::vector<int> vlanList, FieldType fieldType);

 private:
    std::shared_ptr<ITrafficFilter> trafficFilter_;
};

}  // namespace data
}  // namespace telux
#endif  // TELUX_DATA_TRAFFICFILTER_HPP