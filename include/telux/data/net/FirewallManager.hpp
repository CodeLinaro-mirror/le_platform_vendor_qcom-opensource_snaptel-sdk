/*
 *  Copyright (c) 2019-2020 The Linux Foundation. All rights reserved.
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
 * @file       FirewallManager.hpp
 *
 * @brief      FirewallManager is a primary interface that filters and controls the network
 *             traffic on a pre-configured set of rules.
 *
 */

#ifndef FIREWALLMANAGER_HPP
#define FIREWALLMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataDefines.hpp>
#include <telux/data/IpFilter.hpp>

namespace telux {
namespace data {
namespace net {

// Forward declarations
class IFirewallEntry;

/**
 * This function is called as a response to @ref requestFirewallStatus()
 *
 * @param [in] enable            Indicates whether the firewall is enabled
 * @param [in] allowPackets      Indicates whether to accept or drop packets
 *                               matching the rules
 * @param [in] error       -     Return code which indicates whether the operation
 *                               succeeded or not. @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
 *         break backwards compatibility.
 */
using FirewallStatusCb
    = std::function<void(bool enable, bool allowPackets, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref requestFirewallEntries()
 *
 * @param [in] entries           list of firewall entries
 * @param [in] error       -     Return code which indicates whether the operation
 *                               succeeded or not. @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
 *         break backwards compatibility.
 */
using FirewallEntriesCb = std::function<void(
    std::vector<std::shared_ptr<IFirewallEntry>> entries, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref requestDmzEntries()
 *
 * @param [in] dmzEntries     list of dmz entries
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not. @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
 *         break backwards compatibility.
 */
using DmzEntriesCb
    = std::function<void(std::vector<std::string> dmzEntries, telux::common::ErrorCode error)>;

/** @addtogroup telematics_net
 * @{ */

/**
 *@brief    FirewallManager is a primary interface that filters and controls the network
 *          traffic on a pre-configured set of rules.
 */
class IFirewallManager {
 public:
    /**
     * Checks if the data subsystem is ready.
     *
     * @returns True if Firewall Manager is ready for service, otherwise
     * returns false.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for data subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when firewall manager is ready.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Sets firewall configuration to enable or disable and update configuration to
     * drop or accept the packets matching the rules.
     *
     * @param [in] profileId         Profile identifier on which firewall will be set.
     * @param [in] enable            Indicates whether the firewall is enabled
     * @param [in] allowPackets      Indicates whether to accept or drop packets
     *                               matching the rules
     * @param [in] callback          optional callback to get the response setFirewall
     *
     * @returns Status of setFirewall i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status setFirewall(int profileId,
        bool enable, bool allowPackets, telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Request status of firewall
     *
     * @param [in] profileId         Profile identifier for which firewall status is requested.
     * @param [in] callback          callback to get the response of requestFirewallStatus
     *
     * @returns Status of requestFirewallStatus i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestFirewallStatus(int profileId,
        FirewallStatusCb callback) = 0;

    /**
     * Adds the firewall rule
     *
     * @param [in] profileId        Profile identifier on which firewall rule will be added.
     * @param[in] entry             Firewall entry based on protocol type
     * @param[in] callback          optional callback to get the response addFirewallEntry
     *
     * @returns Status of addFirewallEntry i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status addFirewallEntry(int profileId,
        std::shared_ptr<IFirewallEntry> entry, telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Request Firewall rules
     *
     * @param[in] profileId         Profile identifier on which firewall entries are retrieved.
     * @param[in] callback          callback to get the response requestFirewallEntries.
     *
     * @returns Status of requestFirewallEntries i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestFirewallEntries(int profileId,
        FirewallEntriesCb callback) = 0;

    /**
     * Remove firewall entry
     *
     * @param[in] profileId         Profile identifier on which firewall entry will be removed.
     * @param[in] handle            handle of Firewall entry to be removed. To retrieve the handle,
     *                              first use requestFirewallEntries() to get the list of entries
     *                              added in the system. And then use IFirewallEntry::getHandle()
     * @param[in] callback          callback to get the response removeFirewallEntry
     *
     * @returns Status of removeFirewallEntry i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status removeFirewallEntry(int profileId, uint32_t handle,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Enable demilitarized zone (DMZ)
     *
     * @param [in] profileId     Profile identifier on which DMZ will be enabled.
     * @param [in] ipAddr        IP address for which DMZ will be enabled
     * @param [in] callback      optional callback to get the response addDmz
     *
     * @returns Status of enableDmz i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status enableDmz(int profileId,
        const std::string ipAddr, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Disable demilitarized zone (DMZ)
     *
     * @param [in] profileId     Profile identifier on which DMZ will be disabled.
     * @param [in] ipType        Specify IP type of the DMZ to be disabled
     * @param [in] callback      optional callback to get the response removeDmz
     *
     * @returns Status of disableDmz i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status disableDmz(int profileId, const telux::data::IpFamilyType ipType,
        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request DMZ entry that was previously set using enableDmz API
     *
     * @param [in] profileId     Profile identifier on which DMZ entries are requested.
     * @param [in] dmzCb         callback to get the response requestDmzEntry
     *
     * @returns Status of requestDmzEntry i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestDmzEntry(int profileId, DmzEntriesCb dmzCb) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Destructor for IFirewallManager
     */
    virtual ~IFirewallManager(){};
};  // end of IFirewallManager

/**
 * @brief   Firewall entry class is used for configuring firewall rules
 */
class IFirewallEntry {
 public:
    static const uint32_t INVALID_HANDLE = 0;

    /**
     * Get IProtocol filter type
     *
     * @returns @ref telux::data::IIpFilter.
     *
     */
    virtual std::shared_ptr<IIpFilter> getIProtocolFilter() = 0;

    /**
     * Get firewall direction
     *
     * @returns @ref telux::data::Direction.
     *
     */
    virtual telux::data::Direction getDirection() = 0;

    /**
     * Get Ip FamilyType
     *
     * @returns @ref telux::data::IpFamilyType.
     *
     */
    virtual telux::data::IpFamilyType getIpFamilyType() = 0;

    /**
     * Get the unique handle identifying this Firewall entry in the system
     *
     * @returns uint32_t handle if initialized or INVALID_HANDLE otherwise
     *
     */
    virtual uint32_t getHandle() = 0;

    /**
     * Destructor for IFirewallEntry
     */
    virtual ~IFirewallEntry(){};
};

/** @} */ /* end_addtogroup telematics_net */
}
}
}
#endif
