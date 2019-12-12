/*
 *  Copyright (c) 2019 The Linux Foundation. All rights reserved.
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
 * @file       NatManager.hpp
 *
 * @brief      NatManager is a primary interface for configuring static network
 *             address translation(SNAT) and DMZ (demilitarized zone)
 *
 */

#ifndef NATMANAGER_HPP
#define NATMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
namespace net {

/** @addtogroup telematics_net
 * @{ */
/**
 * Structure represents Network Address Translation (NAT) configuration
 */
struct NatConfig {
    std::string addr;    /**< Private IP address */
    uint16_t port;       /**< Private port */
    uint16_t globalPort; /**< Global port */
    IpProtocol proto;    /**< IP protocol @ref telux::net::IpProtocol */
};
/** @} */ /* end_addtogroup telematics_net */

/**
 * This function is called as a response to @ref requestStaticNatEntries()
 *
 * @param [in] snatEntries    list of static Network Address Translation (NAT)
 * @param [in] error          Return code which indicates whether the operation
 *                            succeeded or not @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
 *         break backwards compatibility.
 */
using StaticNatEntriesCb
    = std::function<void(const std::vector<NatConfig> &snatEntries, telux::common::ErrorCode error)>;

/** @addtogroup telematics_net
 * @{ */
/**
 *@brief    NatManager is a primary interface for configuring static network
 *          address translation(SNAT) and DMZ (demilitarized zone)
 */
class INatManager {
 public:
    /**
     * Checks if the NAT manager subsystem is ready.
     *
     * @returns True if NAT Manager is ready for service, otherwise
     * returns false.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for NAT manager subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when NAT manager is ready.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Adds a static Network Address Translation (NAT) entry in the NAT table, these
     * entries are persistent across object, connection and reboot lifetimes. To remove
     * an entry it needs a explicit call to removeStaticNatEntry() API, it supports both
     * IPv4 and IPv6
     *
     * @param [in] profileId         Profile identifier to which static entry will be mapped to.
     * @param [in] snatConfig        snatConfiguration @ref telux::net::NatConfig
     * @param [in] callback          optional callback to get the response addStaticNatEntry
     *
     * @returns Status of addStaticNatEntry i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status addStaticNatEntry(int profileId,
        const NatConfig &snatConfig, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Removes a static Network Address Translation (NAT) entry in the NAT table,
     * it supports both IPv4 and IPv6
     *
     * @param [in] profileId         Profile identifier to which static entry will be removed from.
     * @param [in] snatConfig        snatConfiguration @ref telux::net::NatConfig
     * @param [in] callback          optional callback to get the response removeStaticNatEntry
     *
     * @returns Status of removeStaticNatEntry i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status removeStaticNatEntry(int profileId,
        const NatConfig &snatConfig, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request list of static nat entries available in the NAT table
     *
     * @param [in] profileId         Profile identifier to which static entries will be retrieved.
     * @param[in] snatEntriesCb      Asynchronous callback to get the list of static
     *                               Network Address Translation (NAT) entries
     *
     * @returns Status of requestStaticNatEntries i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status requestStaticNatEntries(int profileId,
        StaticNatEntriesCb snatEntriesCb) = 0;

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
     * Destructor for INatManager
     */
    virtual ~INatManager(){};
};  // end of INatManager

/** @} */ /* end_addtogroup telematics_net */
}
}
}

#endif
