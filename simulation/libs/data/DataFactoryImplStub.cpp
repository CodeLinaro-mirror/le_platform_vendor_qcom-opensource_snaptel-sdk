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

#include "DataFactoryImplStub.hpp"
#include "DataConnectionManagerStub.hpp"
#include "DataProfileManagerStub.hpp"
#include "ServingSystemManagerStub.hpp"

#include "../common/Logger.hpp"

std::mutex dataMutex_;

namespace telux {
namespace data {

DataFactoryImplStub::DataFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
}

DataFactoryImplStub::~DataFactoryImplStub() {
    LOG(DEBUG, __FUNCTION__);
    dataConnectionManagerMap_.clear();
    dataProfileManagerMap_.clear();
    dataServingSystemManagerMap_.clear();
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
    LOG(DEBUG, __FUNCTION__);
    std::function<std::shared_ptr<IDataConnectionManager>(telux::common::InitResponseCb)>
        createAndInit
        = [slotId, this](
              telux::common::InitResponseCb initCb) -> std::shared_ptr<IDataConnectionManager> {
        std::shared_ptr<DataConnectionManagerStub> manager
            = std::make_shared<DataConnectionManagerStub>(slotId);
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Data connection manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str(),
       " for slotId = ", static_cast<int>(slotId),
       " , callback = ", &dataConnectionCallbacks_[slotId]);
    auto manager = getManager<IDataConnectionManager>(
        type, dataConnectionManagerMap_[slotId],
        dataConnectionCallbacks_[slotId], clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<IDataProfileManager> DataFactoryImplStub::getDataProfileManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    LOG(DEBUG, __FUNCTION__);
    std::shared_ptr<IDataProfileManager> dataProfileMgr = nullptr;
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto ItrMgr = dataProfileManagerMap_.find(slotId);
    if (ItrMgr != dataProfileManagerMap_.end()) {
        dataProfileMgr = ItrMgr->second.lock();
    }
    if (dataProfileMgr) {
        LOG(DEBUG, "Found Data Profile Manager with slot id: ", static_cast<int>(slotId));
        telux::common::ServiceStatus status =  dataProfileMgr->getServiceStatus();
        if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
            //Manager has failed initialization but callback is not called yet hence we still
            //have valid shared pointer.
            LOG(DEBUG, __FUNCTION__, " Data Profile Manager initialization failed.");
            return nullptr;
        }
        else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Data Profile Manager initialization was successful");
            if (clientCallback) {
                dataProfileCallbacks_[slotId].push_back(clientCallback);
            }
            std::thread appCallback([this, status, slotId]() {
                this->initCompleteNotifierWithSlotId(dataProfileCallbacks_, status, slotId);});
            appCallback.detach();
        }
        else {
            LOG(DEBUG, __FUNCTION__, " Data Profile Manager initialization in progress.");
            if (clientCallback) {
                dataProfileCallbacks_[slotId].push_back(clientCallback);
            }
        }
        return dataProfileMgr;
    }
    else {
        std::shared_ptr<DataProfileManagerStub> dataProfileMgrImpl = nullptr;
        LOG(DEBUG, "Creating Data Profile Manager with slot id: ", slotId);
        auto initCb = [this, slotId](telux::common::ServiceStatus status) {
            if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                std::lock_guard<std::mutex> lock(dataMutex_);
                dataProfileManagerMap_.erase(slotId);
            }
            this->initCompleteNotifierWithSlotId(dataProfileCallbacks_, status, slotId);
        };
        try {
            dataProfileMgrImpl = std::make_shared<DataProfileManagerStub>(slotId, initCb);
        } catch (std::bad_alloc & e) {
            LOG(ERROR, __FUNCTION__ , e.what());
            return nullptr;
        }
        dataProfileManagerMap_[slotId] = dataProfileMgrImpl;
        if (clientCallback) {
            dataProfileCallbacks_[slotId].push_back(clientCallback);
        }
        return dataProfileMgrImpl;
    }
}

std::shared_ptr<IServingSystemManager> DataFactoryImplStub::getServingSystemManager(
    SlotId slotId, telux::common::InitResponseCb clientCallback) {
    std::shared_ptr<IServingSystemManager> servingSystemMgr = nullptr;
    std::lock_guard<std::mutex> lock(dataMutex_);
    auto ItrMgr = dataServingSystemManagerMap_.find(slotId);
    if (ItrMgr != dataServingSystemManagerMap_.end()) {
        servingSystemMgr = ItrMgr->second.lock();
    }
    if (servingSystemMgr) {
        LOG(DEBUG, "Found Serving System Manager with slot id: ", static_cast<int>(slotId));
        //Find the current status of the manager
        telux::common::ServiceStatus status = servingSystemMgr->getServiceStatus();
        if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
            //Manager has failed initialization but callback is not called yet hence we still
            //have valid shared pointer.
            LOG(DEBUG, __FUNCTION__, " Data Serving System Manager initialization failed.");
            return nullptr;
        }
        else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            LOG(DEBUG, __FUNCTION__, " Data Serving System Manager initialization was successful");
            if (clientCallback) {
                servingSystemCallbacks_[slotId].push_back(clientCallback);
            }
            std::thread appCallback([this, status, slotId]() {
                this->initCompleteNotifierWithSlotId(servingSystemCallbacks_, status, slotId);});
            appCallback.detach();
        }
        else {
            LOG(DEBUG, __FUNCTION__, " Data Serving System Manager initialization in progress.");
            if (clientCallback) {
                servingSystemCallbacks_[slotId].push_back(clientCallback);
            }
        }
        return servingSystemMgr;
    }
    else {
        std::shared_ptr<ServingSystemManagerStub> servingSystemMgrImpl = nullptr;
        LOG(DEBUG, "Creating Data Serving System Manager with slot id: ", slotId);
        auto initCb = [this, slotId](telux::common::ServiceStatus status) {
            if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                std::lock_guard<std::mutex> lock(dataMutex_);
                dataServingSystemManagerMap_.erase(slotId);
            }
            this->initCompleteNotifierWithSlotId(servingSystemCallbacks_, status, slotId);
        };
        servingSystemMgrImpl = std::make_shared<ServingSystemManagerStub>(slotId);
        if ((!servingSystemMgrImpl) ||
            (telux::common::Status::SUCCESS != servingSystemMgrImpl->init(initCb))) {
            LOG(DEBUG, "DataFactory unable to initialize ServingSystemManager");
            return nullptr;
        }
        dataServingSystemManagerMap_[slotId] = servingSystemMgrImpl;
        if (clientCallback) {
            servingSystemCallbacks_[slotId].push_back(clientCallback);
        }
        return servingSystemMgrImpl;
    }
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

void DataFactoryImplStub::initCompleteNotifierWithSlotId(
    std::map<SlotId, std::vector<telux::common::InitResponseCb>>& initCbs,
    telux::common::ServiceStatus status, SlotId slotId) {

    LOG(DEBUG, __FUNCTION__);
    std::vector<telux::common::InitResponseCb> Callbacks;
    {
        std::lock_guard<std::mutex> lock(dataMutex_);
        Callbacks = initCbs[slotId];
        initCbs.erase(slotId);
    }
    for (auto &callback : Callbacks) {
        callback(status);
    }
}

}  // namespace data
}
