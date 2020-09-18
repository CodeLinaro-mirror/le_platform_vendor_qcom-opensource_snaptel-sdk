/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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
 * @file       SimProfileManager.hpp
 *
 * @brief      SimProfileManager is a primary interface for remote eUICCs (eSIMs or embedded SIMs)
 *             provisioning.This interface provides APIs to add, delete, set profile,
 *             request profile list, update nickname, providing user consent, get Eid on the eUICC.
 *
 */

#ifndef SIMPROFILEMANAGER_HPP
#define SIMPROFILEMANAGER_HPP

#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>

#include <telux/tel/SimProfileListener.hpp>
#include <telux/tel/SimProfile.hpp>

namespace telux {
namespace tel {

/**
 * This function is called with the response to requestProfileList API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] info       Profiles information @Ref SimProfile.
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not.  @ref ErrorCode.
 */
using ProfileListResponseCb = std::function<void(
    const std::vector<std::shared_ptr<SimProfile>> &profiles, telux::common::ErrorCode error)>;

/**
 * This function is called with the response to requestEid API.
 *
 * The callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @param [in] eid        eUICC identifier.
 * @param [in] error      Return code which indicates whether the operation
 *                        succeeded or not.  @ref ErrorCode.
 */
using EidResponseCb = std::function<void(std::string eid, telux::common::ErrorCode error)>;
/** @addtogroup telematics_rsp
 * @{ */

/**
 *@brief ISimProfileManager is a primary interface for remote eUICCs (eSIMs or embedded SIMs)
 * provisioning.This interface provides APIs to add, delete, set profile, update nickname,
 * provide user consent, get Eid on the eUICC.
 */
class ISimProfileManager {
 public:
    /**
     * Checks if the eUICC subsystem is ready.
     *
     * @returns True if ISimProfileManager is ready for service, otherwise returns false.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for eUICC subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified when card manager is ready.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Add new profile to eUICC card and download and install the profile on eUICC.
     *
     * @param [in] slotId                Slot identifier corresponding to the card.
     * @param [in] activationCode        Activation code.
     * @param [in] confirmationCode      Optional confirmation code required for downloading the
     *                                   profile.
     * @param [in] userConsentSupported  Optional User consent supported or not.
     * @param [in] callback              Callback function to get the result of add profile.
     *
     * @returns Status of add profile i.e. success or suitable error code.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status addProfile(SlotId slotId,
        const std::string &activationCode, const std::string &confirmationCode = "",
        bool userConsentSupported = false, common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Delete profile from eUICC card.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] profileId         Profile identifier
     * @param [in] callback          Callback function to get the result of delete profile.
     *
     * @returns Status of delete profile i.e. success or suitable error code.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status deleteProfile(SlotId slotId, int profileId,
        common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Enable or disable profile which allows to switch to other profile on eUICC card.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] profileId         Profile identifier.
     * @param [in] enable            Indicates whether a profile must be enabled or disabled.
     *                               true - Enable and false - Disable.
     * @param [in] callback          Callback function to get the result of set profile.
     *
     * @returns Status of set profile i.e. success or suitable error code.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status setProfile(SlotId slotId, int profileId, bool enable = false,
        common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Update nick name of the profile
     *
     * @param [in] slotId          Slot identifier corresponding to the card.
     * @param [in] profileId       Profile identifier
     * @param [in] nickName        New nick name for profile.
     * @param [in] callback        Callback function to get the result of update nickname.
     *
     * @returns Status of update nick name i.e. success or suitable error code.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status updateNickName(SlotId slotId, int profileId,
        const std::string &nickName, common::ResponseCallback callback = nullptr)
        = 0;

    /**
     * Request list of profiles supported by the eUICC card.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] callback          Callback function to get the result of request profile list.
     *
     * @returns  Status of request profile list i.e. success or suitable error code.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status requestProfileList(SlotId slotId, ProfileListResponseCb callback)
        = 0;

    /**
     * Request eUICC identifier(EID) for the slot.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] callback          Callback function to get the result of request EID.
     *
     * @returns  Status of request EID.e. success or suitable error code.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status requestEid(SlotId slotId, EidResponseCb callback)
        = 0;

    /**
     * Provide user consent required for downloading and installing profile.
     *
     * @param [in] slotId            Slot identifier corresponding to the card.
     * @param [in] userConsent       Consent for proﬁle download and install.
                                     True means user consent to download and install.
     * @param [in] callback          Callback function to get the result of user consent request.
     *
     * @returns  Status of user consent request i.e. success or suitable error code.
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status provideUserConsent(SlotId slotId, bool userConsent = false,
        common::ResponseCallback callback = nullptr)
        = 0;
    /**
     * Register a listener to listen for status of specific events like download and installation
     * of profile on eUICC.
     *
     * @param [in] listener    Pointer of ISimProfileListener object that processes the
     * notification.
     *
     * @returns Status of registerListener success or suitable status code
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ISimProfileListener> listener) = 0;

    /**
     * De-register the listener.
     *
     * @param [in] listener    Pointer of ISimProfileListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<ISimProfileListener> listener)
        = 0;

    /**
     * Destructor for ISimProfileManager
     */
    virtual ~ISimProfileManager() {
    }
};  // end of ISimProfileManager

/** @} */ /* end_addtogroup telematics_rsp */
}
}

#endif  // SIMPROFILEMANAGER_HPP
