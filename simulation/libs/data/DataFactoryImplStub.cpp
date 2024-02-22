/*
 * Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "DataFactoryImplStub.hpp"

namespace telux {
namespace data {

DataFactoryImplStub::DataFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
}

DataFactoryImplStub::~DataFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
}

DataFactory::DataFactory() {
    LOG(DEBUG, __FUNCTION__);
}

DataFactory::~DataFactory() {
    LOG(DEBUG, __FUNCTION__);
}

DataFactory &DataFactoryImplStub::getInstance() {
    static DataFactoryImplStub instance;
    return instance;
}

DataFactory &DataFactory::getInstance() {
    return DataFactoryImplStub::getInstance();
}


std::shared_ptr<IDataConnectionManager> DataFactoryImplStub::getDataConnectionManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<IDataProfileManager> DataFactoryImplStub::getDataProfileManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<IServingSystemManager> DataFactoryImplStub::getServingSystemManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<IDataFilterManager> DataFactoryImplStub::getDataFilterManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<IIpFilter> DataFactoryImplStub::getNewIpFilter(IpProtocol proto) {
    return nullptr;
}

std::shared_ptr<telux::data::net::INatManager> DataFactoryImplStub::getNatManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<telux::data::net::IFirewallManager> DataFactoryImplStub::getFirewallManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<telux::data::net::IFirewallEntry> DataFactoryImplStub::getNewFirewallEntry(
    IpProtocol proto, Direction direction, IpFamilyType ipFamilyType) {
    return nullptr;
}

std::shared_ptr<telux::data::net::IVlanManager> DataFactoryImplStub::getVlanManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<telux::data::net::ISocksManager> DataFactoryImplStub::getSocksManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<telux::data::net::IBridgeManager> DataFactoryImplStub::getBridgeManager(
    telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<telux::data::net::IL2tpManager> DataFactoryImplStub::getL2tpManager(
    telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<telux::data::IDataSettingsManager> DataFactoryImplStub::getDataSettingsManager(
    telux::data::OperationType oprType, telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

std::shared_ptr<IDataLinkManager> DataFactoryImplStub::getDataLinkManager(
    telux::common::InitResponseCb clientCallback) {
    return nullptr;
}

}  // namespace data
}
