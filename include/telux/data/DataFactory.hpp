/*
 *  Copyright (c) 2018-2021 The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:

 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       DataFactory.hpp
 *
 * @brief      DataFactory is the central factory to create all data instances
 *
 */

#ifndef DATAFACTORY_HPP
#define DATAFACTORY_HPP

#include <map>
#include <memory>
#include <mutex>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

#include <telux/data/DataConnectionManager.hpp>
#include <telux/data/DataProfileManager.hpp>
#include <telux/data/ServingSystemManager.hpp>
#include <telux/data/DataFilterManager.hpp>
#include <telux/data/IpFilter.hpp>

#include <telux/data/net/FirewallManager.hpp>
#include <telux/data/net/NatManager.hpp>
#include <telux/data/net/VlanManager.hpp>
#include <telux/data/net/SocksManager.hpp>
#include <telux/data/net/BridgeManager.hpp>
#include <telux/data/net/L2tpManager.hpp>
#include <telux/data/DataSettingsManager.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

/**
 *@brief DataFactory is the central factory to create all data classes
 *
 */
class DataFactory {
 public:
    /**
     * Get Data Factory instance.
     */
    static DataFactory &getInstance();

    /**
     * Get Data Connection Manager
     *
     * @param [in] slotId           Unique identifier for the SIM slot
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              DataConnectionManager
     *                              @ref telux::data::DataInitSlotIdResponseCb.
     *
     * @returns instance of IDataConnectionManager
     *
     */
    std::shared_ptr<IDataConnectionManager> getDataConnectionManager(SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Data Profile Manager
     *
     * @param [in] slotId           Unique identifier for the SIM slot
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              DataProfileManager
     *                              @ref telux::data::DataInitSlotIdResponseCb.
     *
     * @returns instance of IDataProfileManager
     *
     */
    std::shared_ptr<IDataProfileManager> getDataProfileManager(SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Serving System Manager
     *
     * @param [in] slotId            Unique identifier for the SIM slot
     * @param [in] clientCallback    Callback to be called with initialization result
     *
     * @returns instance of IServingSystemManager
     *
     */
    std::shared_ptr<IServingSystemManager> getServingSystemManager(SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Data Filter Manager instance
     *
     * @param [in] slotId           Unique identifier for the SIM slot
     * @param [in] clientCallback   Callback to be called with initialization result
     *
     * @returns instance of IDataFilterManager.
     *
     */
    std::shared_ptr<IDataFilterManager> getDataFilterManager(SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Network Address Translation(NAT) Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              NAT manager @ref telux::common::InitResponseCb
     *
     * @returns instance of INatManager
     *
     */
    std::shared_ptr<telux::data::net::INatManager> getNatManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Firewall Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Firewall manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IFirewallManager
     *
     */
    std::shared_ptr<telux::data::net::IFirewallManager> getFirewallManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Firewall entry based on IP protocol and set respective filter (i.e. TCP or UDP)
     *
     * @param [in] proto            @ref telux::data::IpProtocol
     * @param [in] direction        @ref telux::data::Direction
     * @param [in] ipFamilyType     Identifies IP family type @ref telux::data::IpFamilyType
     *
     * @returns instance of IFirewallEntry
     *
     */
    std::shared_ptr<telux::data::net::IFirewallEntry> getNewFirewallEntry(IpProtocol proto,
        Direction direction, IpFamilyType ipFamilyType);

    /**
    * Get IIpFilter instance based on IP Protocol, This can be used in Firewall Manager and
    * Data Filter Manager
    *
    * @param [in] proto    @ref telux::data::IpProtocol
    *                      Some sample protocol values are
    *                      ICMP = 1    # Internet Control Message Protocol - RFC 792
    *                      IGMP = 2    # Internet Group Management Protocol - RFC 1112
    *                      TCP = 6     # Transmission Control Protocol - RFC 793
    *                      UDP = 17    # User Datagram Protocol - RFC 768
    *                      ESP = 50    # Encapsulating Security Payload - RFC 4303
    *
    * @returns instance of IIpFilter based on IpProtocol filter (i.e TCP, UDP)
    *
    */
    std::shared_ptr<IIpFilter> getNewIpFilter(IpProtocol proto);

    /**
     * Get VLAN Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Vlan manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IVlanManager
     *
     */
    std::shared_ptr<telux::data::net::IVlanManager> getVlanManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Socks Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Socks manager @ref telux::common::InitResponseCb
     *
     * @returns instance of ISocksManager
     *
     */
    std::shared_ptr<telux::data::net::ISocksManager> getSocksManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Software Bridge Manager
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Bridge manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IBridgeManager
     *
     */
    std::shared_ptr<telux::data::net::IBridgeManager> getBridgeManager(
        telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get L2TP Manager
     *
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              L2TP manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IL2tpManager
     *
     */
    std::shared_ptr<telux::data::net::IL2tpManager> getL2tpManager(
        telux::common::InitResponseCb clientCallback = nullptr);

    /**
     * Get Data Settings Manager
     *
     * @param [in] oprType          Required operation type @ref telux::data::OperationType
     * @param [in] clientCallback   Optional callback to get the initialization status of
     *                              Data Settings manager @ref telux::common::InitResponseCb
     *
     * @returns instance of IDataSettingsManager
     *
     */
    std::shared_ptr<telux::data::IDataSettingsManager> getDataSettingsManager(
        telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback = nullptr);

 private:
    /**
     * Call client callbacks after manager initialisation
     */
    void initCompleteNotifier(std::vector<telux::common::InitResponseCb>& initCbs,
                              telux::common::ServiceStatus status);
    void initCompleteNotifierWithSlotId(
        std::map<SlotId, std::vector<telux::common::InitResponseCb>>& initCbs,
        telux::common::ServiceStatus status, SlotId slotId);

    void initCompleteNotifierWithOprType(
        std::map<OperationType, std::vector<telux::common::InitResponseCb>> &initCbs,
        telux::common::ServiceStatus status, OperationType oprType);

    // mutex to protect member variables
    std::mutex dataMutex_;
    std::map<SlotId, std::weak_ptr<IDataConnectionManager>> dataConnectionManagerMap_;
    std::map<SlotId, std::weak_ptr<IDataProfileManager>> dataProfileManagerMap_;
    std::map<SlotId, std::weak_ptr<IServingSystemManager>> dataServingSystemManagerMap_;
    std::map<SlotId, std::weak_ptr<IDataFilterManager>> dataFilterManagerMap_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::net::INatManager>>
        natManagerMap_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::net::IFirewallManager>>
        fwManagerMap_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::net::IVlanManager>>
        vlanManagerMap_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::net::ISocksManager>>
        socksManagerMap_;
    std::weak_ptr<telux::data::net::IBridgeManager> bridgeManager_;
    std::weak_ptr<telux::data::net::IL2tpManager>  l2tpManager_;
    std::map<telux::data::OperationType, std::weak_ptr<telux::data::IDataSettingsManager>>
        dataSettingsManagerMap_;

    std::map<SlotId, std::vector<telux::common::InitResponseCb>> dataProfileCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> dataConnectionCallbacks_;
    std::vector<telux::common::InitResponseCb> natCallbacks_;
    std::vector<telux::common::InitResponseCb> fwCallbacks_;
    std::vector<telux::common::InitResponseCb> socksCallbacks_;
    std::vector<telux::common::InitResponseCb> vlanCallbacks_;
    std::vector<telux::common::InitResponseCb> bridgeCallbacks_;
    std::vector<telux::common::InitResponseCb> l2tpCallbacks_;
    std::map<OperationType, std::vector<telux::common::InitResponseCb>> dataSettingsCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> servingSystemCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> dataFilterCallbacks_;
    DataFactory();
    ~DataFactory();
    DataFactory(const DataFactory &) = delete;
    DataFactory &operator=(const DataFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_data */
}
}

#endif
