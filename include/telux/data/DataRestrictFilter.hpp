/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       DataRestrictFilter.hpp
 * @brief      A data restrict filter, filters what data will be allowed from the modem to the
 *             application processor. Only data packets that match the filter will be sent to the
 *             apps processor.
 */

#ifndef DATARESTRICTFILTER_HPP
#define DATARESTRICTFILTER_HPP

#include <telux/data/DataDefines.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {
/** @addtogroup telematics_data
 * @{ */

/**
 * @brief  This class encapsulate a DataRestrictFilter interface
 * to set/get IPv4 and IPv6 IP level parameters that defines a
 * filter rule.
 * See @ref DataDefines
 *
 * Returned from @ref  getNewDataRestrictFilter() API in DataFactory
 */
class IDataRestrictFilter {
public:
  /**
   * Get IPv4 Source Address.
   *
   * @returns std::string
   *
   */
  virtual std::string getIPv4SrcAddr() = 0;

  /**
   * Set IPv4 source ip address
   *
   * @param [in] ipv4SrcAddr       - IPv4 Source Address.
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual telux::common::Status setIPv4SrcAddr(const std::string &ipv4SrcAddr) = 0;

  /**
   * Get IPv4 destination address.
   *
   * @returns std::string
   *
   */
  virtual std::string getIPv4DestAddr() = 0;

  /**
   * Set IPv4 destination ip address
   *
   * @param [in] ipv4DestAddr      - IPv4 Destination Address.
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual telux::common::Status setIPv4DestAddr(const std::string &ipv4DestAddr) = 0;

  /**
   * Get IPv6 Source Address.
   *
   * @returns std::string
   *
   */
  virtual std::string getIPv6SrcAddr() = 0;

  /**
   * Set IPv6 Source address
   *
   * @param [in] ipv6SrcAddr       - IPv6 Source Address.
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual telux::common::Status setIPv6SrcAddr(const std::string &ipv6SrcAddr) = 0;

  /**
   * Get IPv6 destination address.
   *
   * @returns std::string
   *
   */
  virtual std::string getIPv6DestAddr() = 0;

  /**
   * Set IPv6 Destination address
   *
   * @param [in] ipv6DestAddr      - IPv6 Destination Address.
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual telux::common::Status setIPv6DestAddr(const std::string &ipv6DestAddr) = 0;

  /**
   * Get current filter type based on derived class
   *
   * @returns ProtocolType based on below derived class type
   *          ITcpRestrictFilter
   *          IUdpRestrictFilter
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual ProtocolType getProtocolType() = 0;

  /**
   * Destructor for IDataRestrictFilter
   */
  virtual ~IDataRestrictFilter() {}
};

/**
 * @brief  This class represents a Filter for the TCP and IP Protocol.
 * See @ref DataDefines
 *
 * Returned from @ref getNewDataRestrictFilter in DataFactory
 */
class ITcpRestrictFilter : virtual public IDataRestrictFilter {
public:
  /**
   * Get TCP Source port.
   * Port number and range.
   *
   * @returns PortInfo
   *
   */
  virtual PortInfo getSourcePort() = 0;

  /**
   * Set TCP source port info
   *
   * @param [in] srcPort       - TCP port number and range.
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual telux::common::Status setSourcePort(PortInfo srcPort) = 0;

  /**
   * Get TCP Destination port.
   * Port number and range.
   *
   * @returns PortInfo
   *
   */
  virtual PortInfo getDestinationPort() = 0;

  /**
   * Set TCP destination port info
   *
   * @param [in] destPort       - TCP port number and range.
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual telux::common::Status setDestinationPort(PortInfo destPort) = 0;

  /**
   * Destructor for ITcpRestrictFilter
   */
  virtual ~ITcpRestrictFilter() {}
};

/**
 * @brief  This class represents a Filter for the UDP and IP Protocol.
 * See @ref DataDefines
 *
 * Returned from @ref getNewDataRestrictFilter in DataFactory
 */
class IUdpRestrictFilter : virtual public IDataRestrictFilter {
public:
  /**
   * Get UDP Source port.
   * Port number and range.
   *
   * @returns PortInfo
   *
   */
  virtual PortInfo getSourcePort() = 0;

  /**
   * Set UDP source port info
   *
   * @param [in] srcPort       - UDP port number and range.
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual telux::common::Status setSourcePort(PortInfo srcPort) = 0;

  /**
   * Get UDP Destination port.
   * Port number and range.
   *
   * @returns PortInfo
   *
   */
  virtual PortInfo getDestinationPort() = 0;

  /**
   * Set UDP destination port info
   *
   * @param [in] destPort       - UDP port number and range.
   *
   * @returns SUCCESS upon success. Error status otherwise.
   */
  virtual telux::common::Status setDestinationPort(PortInfo destPort) = 0;

  /**
   * Destructor for IUdpRestrictFilter
   */
  virtual ~IUdpRestrictFilter() {}
};
/** @} */ /* end_addtogroup telematics_data */
} // namespace data
} // namespace telux

#endif