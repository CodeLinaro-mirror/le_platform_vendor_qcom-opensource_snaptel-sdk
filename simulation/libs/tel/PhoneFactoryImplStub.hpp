/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       PhoneFactoryImplStub.hpp
 *
 * @brief      Implementation of PhoneFactory
 *
 */

#ifndef TEL_FACTORY_STUB_HPP
#define TEL_FACTORY_STUB_HPP

#include <memory>
#include <map>

#include <telux/tel/PhoneFactory.hpp>
#include "../common/Logger.hpp"


namespace telux {
namespace tel {

class PhoneFactoryImplStub : public PhoneFactory {
 public:
    static PhoneFactory &getInstance();

    virtual std::shared_ptr<IPhoneManager> getPhoneManager(
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ISmsManager> getSmsManager(int phoneId = DEFAULT_PHONE_ID,
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ICallManager> getCallManager(telux::common::InitResponseCb
        callback = nullptr) override;
    virtual std::shared_ptr<ICardManager> getCardManager(telux::common::InitResponseCb
        callback = nullptr) override;
    virtual std::shared_ptr<ISapCardManager> getSapCardManager(int slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ISubscriptionManager> getSubscriptionManager(
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<IServingSystemManager> getServingSystemManager(
        int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<INetworkSelectionManager> getNetworkSelectionManager(
        int slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IRemoteSimManager> getRemoteSimManager(int slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IMultiSimManager> getMultiSimManager(
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ICellBroadcastManager> getCellBroadcastManager(
        SlotId slotId = DEFAULT_SLOT_ID,
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<ISimProfileManager> getSimProfileManager(
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IImsSettingsManager> getImsSettingsManager(
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IEcallManager> getEcallManager(
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<IHttpTransactionManager> getHttpTransactionManager(
        telux::common::InitResponseCb  callback = nullptr) override;
    virtual std::shared_ptr<IImsServingSystemManager> getImsServingSystemManager(SlotId slotId,
        telux::common::InitResponseCb callback = nullptr) override;
    virtual std::shared_ptr<ISuppServicesManager> getSuppServicesManager(
        SlotId slotId = DEFAULT_SLOT_ID, telux::common::InitResponseCb  callback = nullptr)
            override;

 private:
    PhoneFactoryImplStub();
    ~PhoneFactoryImplStub();
    std::map<int, std::weak_ptr<ISmsManager>> SmsManagerMap_;
    std::shared_ptr<ICardManager> cardManager_;
    std::vector<telux::common::InitResponseCb> cardMgrCallbacks_;
    telux::common::ServiceStatus cardMgrInitStatus_;
    std::recursive_mutex mutex_;
    void onCardManagerResponse(telux::common::ServiceStatus status);
};

}  // namespace tel
}  // namespace telux

#endif  // TEL_FACTORY_STUB_HPP
