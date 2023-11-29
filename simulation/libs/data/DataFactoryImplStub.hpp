/*
 * Copyright (c) 2021,2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       DataFactoryImplStub.hpp
 *
 * @brief      Implementation of DataFactory
 *
 */

#ifndef DATA_FACTORY_IMPL_STUB_HPP
#define DATA_FACTORY_IMPL_STUB_HPP

#include <memory>
#include <map>

#include <telux/data/DataFactory.hpp>
#include "../common/AsyncTaskQueue.hpp"
#include "../common/FactoryHelper.hpp"

namespace telux {
namespace data {

class DataFactoryImplStub : public DataFactory,
                            public telux::common::FactoryHelper {
 public:
    static DataFactory &getInstance();

    virtual std::shared_ptr<IDataConnectionManager> getDataConnectionManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<IDataProfileManager> getDataProfileManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<IServingSystemManager> getServingSystemManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<IDataFilterManager> getDataFilterManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::INatManager> getNatManager(
        telux::data::OperationType oprType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IFirewallManager> getFirewallManager(
        telux::data::OperationType oprType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IFirewallEntry> getNewFirewallEntry(
        IpProtocol proto, Direction direction, IpFamilyType ipFamilyType) override;

    virtual std::shared_ptr<IIpFilter> getNewIpFilter(IpProtocol proto) override;

    virtual std::shared_ptr<telux::data::net::IVlanManager> getVlanManager(
        telux::data::OperationType oprType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::ISocksManager> getSocksManager(
        telux::data::OperationType oprType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IBridgeManager> getBridgeManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::net::IL2tpManager> getL2tpManager(
        telux::common::InitResponseCb clientCallback = nullptr) override;

    virtual std::shared_ptr<telux::data::IDataSettingsManager> getDataSettingsManager(
        telux::data::OperationType,
        telux::common::InitResponseCb clientCallback = nullptr) override;

 private:
    DataFactoryImplStub();
    ~DataFactoryImplStub();

    void initCompleteNotifierWithSlotId(
        std::map<SlotId, std::vector<telux::common::InitResponseCb>> &initCbs,
        telux::common::ServiceStatus status, SlotId slotId);

    std::map<SlotId, std::weak_ptr<IDataProfileManager>> dataProfileManagerMap_;
    std::map<SlotId, std::weak_ptr<IDataConnectionManager>> dataConnectionManagerMap_;
    std::map<SlotId, std::weak_ptr<IServingSystemManager>> dataServingSystemManagerMap_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> dataProfileCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> servingSystemCallbacks_;
    std::map<SlotId, std::vector<telux::common::InitResponseCb>> dataConnectionCallbacks_;
};

}  // namespace data
}  // namespace telux

#endif  // DATA_FACTORY_IMPL_STUB_HPP
