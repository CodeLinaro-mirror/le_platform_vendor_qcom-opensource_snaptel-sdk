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
 * @file       VlanManager.hpp
 *
 * @brief      VlanManager is a primary interface for configuring VLAN (Virtual Local Area Network).
 *             it provide APIs for create, query, remove VLAN interfaces and associate or
               disassociate with profile IDs
 *
 */

#ifndef VLANCONFIG_HPP
#define VLANCONFIG_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
namespace net {

/**
 * This function is called as a response to @ref createVlan()
 *
 * @param [in] isAccelerated              Offload status returned by server
 * @param [in] error                      Return code which indicates whether the operation
 *                                        succeeded or not @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
 *         break backwards compatibility.
 */
using CreateVlanCb = std::function<void(bool isAccelerated, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref queryVlanInfo()
 *
 * @param [in] configs         List of VLAN configs
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
 *         break backwards compatibility.
 */
using QueryVlanResponseCb
    = std::function<void(const std::vector<VlanConfig> &configs, telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref queryVlanMappingList()
 *
 * @param [in] mapping         List of profile Id and Vlan id map
 *                             Key is Profile Id and value is VLAN id
 * @param [in] error           Return code which indicates whether the operation
 *                             succeeded or not @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
 *         break backwards compatibility.
 */
using VlanMappingResponseCb = std::function<void(
    const std::list<std::pair<int, int>> &mapping, telux::common::ErrorCode error)>;

/** @addtogroup telematics_net
 * @{ */

/**
 *@brief       VlanManager is a primary interface for configuring VLAN (Virtual Local Area Network).
 *             it provide APIs for create, query, remove VLAN interfaces and associate or
               disassociate with profile IDs
 */
class IVlanManager {
 public:
    /**
     * Checks if the data subsystem is ready.
     *
     * @returns True if VLAN Manager is ready for service, otherwise
     * returns false.
     *
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for data subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when VLAN manager is ready.
     *
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Create a VLAN associated with multiple interfaces
     *
     * @note       if interface configured as VLAN for the first time, it may trigger auto reboot.
     *
     * @param [in] vlanConfig       vlan configuration
     * @param [out] callback        optional callback to get the response createVlan
     *
     * @returns Immediate status of createVlan() request sent i.e. success or suitable status
     * code.
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     *
     */
    virtual telux::common::Status createVlan(
        const VlanConfig &vlanConfig, CreateVlanCb callback = nullptr)
        = 0;

    /**
     * Remove VLAN configuration
     *
     * @note   This will delete all clients associated with interface
     *
     * @param [in] vlanId          VLAN ID
     * @param [in] ifaceType       @ref telux::net::InterfaceType
     * @param [out] callback       optional callback to get the response removeVlan
     *
     * @returns Immediate status of removeVlan() request sent i.e. success or suitable status
     * code.
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status removeVlan(
        int16_t vlanId, InterfaceType ifaceType, telux::common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Query information about all the VLANs in the system
     *
     * @param [out] callback        Response callback with list of configured VLANs
     *
     * @returns Immediate status of queryVlanInfo() request sent i.e. success or suitable status
     * code.
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status queryVlanInfo(QueryVlanResponseCb callback) = 0;

    /**
     * Bind a Vlan with a particular profile ID. When a WWAN network interface is
     * brought up using IDataConnectionManager::startDataCall on that profile ID,
     * that interface will be accessible from this Vlan
     *
     * @param [in] profileId    profile id for vlan association
     * @param [in] vlanId       sets vlan id
     * @param [out] callback    callback to get the response of associateWithProfileId API
     *
     * @returns Immediate status of associateWithProfileId() request sent i.e. success or
     * suitable status code.
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status bindWithProfile(
        int profileId, int vlanId, telux::common::ResponseCallback callback)
        = 0;

    /**
     * Unbind VLAN id with given profile id
     *
     * @param [in] profileId    profile id for vlan association
     * @param [in] vlanId       vlan id
     * @param [in] callback     callback to get the response of associateWithProfileId API
     *
     * @returns Immediate status of disassociateFromProfileId() request sent i.e. success or
     * suitable status code
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status unbindFromProfile(
        int profileId, int vlanId, telux::common::ResponseCallback callback)
        = 0;

    /**
     * Query VLAN mapping list with associated profile id and vlan id
     *
     * @param [in] callback    callback to get the response of queryVlanMappingList API
     *
     * @returns Immediate status of queryVlanMappingList() request sent i.e. success or
     * suitable status code
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status queryVlanMappingList(VlanMappingResponseCb callback) = 0;

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
     * Destructor for IVlanManager
     */
    virtual ~IVlanManager(){};
};  // end of IVlanManager

/** @} */ /* end_addtogroup telematics_net */
}
}
}
#endif
