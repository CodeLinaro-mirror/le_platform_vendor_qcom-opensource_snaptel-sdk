/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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
 * @file       DataProfileManager.hpp
 * @brief      ProfileManager is a primary interface for profile management.
 *
 *             This interface provides APIs to create, update, delete and lookup
 *             profiles in the device.
 *
 * @note       Eval: This is a new API and is being evaluated. It is subject to
 *             change and could break backwards compatibility.
 */

#ifndef DATAPROFILEMANAGER_HPP
#define DATAPROFILEMANAGER_HPP

#include <memory>

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataProfile.hpp>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace data {

class IDataCreateProfileCallback;
class IDataProfileListCallback;
class IDataProfileCallback;

/** @addtogroup telematics_data
 * @{ */

/**
 * IDataProfileManager is a primary interface for profile management.
 *
 * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
 *          break backwards compatibility.
 */
class IDataProfileManager {
public:
   /**
    * Request list of profiles supported by the device.
    *
    * @param [in, out ] callback    Callback pointer to get the response.
    *
    * @returns  Status of request profile i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual telux::common::Status
      requestProfileList(std::shared_ptr<IDataProfileListCallback> callback = nullptr)
      = 0;

   /**
    * Create profile based on data profile params.
    *
    * @param [in] profileParams     profileParams configuration to be passed for creating profile
    *                               either for 3GPP or 3GPP2
    * @param [in, out ] callback    Callback pointer to get the result of create profile
    *
    * @returns Status of create profile i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual telux::common::Status createProfile(const ProfileParams &profileParams,
                                               std::shared_ptr<IDataCreateProfileCallback> callback
                                               = nullptr)
      = 0;

   /**
    * Delete profile corresponding to profile identifier.
    *
    * The deletion of a profile does not affect profile index assignments.
    *
    * @param [in] profileId         Profile identifier
    * @param [in] techPreference    Technology Preference like 3GPP / 3GPP2
    * @param [in] callback          Callback pointer to get the result of delete profile
    *
    * @returns Status of delete profile i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual telux::common::Status
      deleteProfile(uint8_t profileId, TechPreference techPreference,
                    std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Modify existing profile with new profile params.
    *
    * @param [in] profileId         Profile identifier of profile to be modified
    * @param [in] profileParams     New profileParams configuration passed for updating existing
    *                               profile
    * @param [in] callback          Callback pointer to get the result of modify profile
    *
    * @returns Status of modify profile i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual telux::common::Status
      modifyProfile(uint8_t profileId, const ProfileParams &profileParams,
                    std::shared_ptr<telux::common::ICommandResponseCallback> callback = nullptr)
      = 0;

   /**
    * Lookup modem profile/s based on given profile params.
    *
    * @param [in] profileParams     ProfileParams configuration to be passed
    * @param [in] callback          Callback pointer to get the result of query profile
    *
    * @returns Status of query profile i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual telux::common::Status queryProfile(const ProfileParams &profileParams,
                                              std::shared_ptr<IDataProfileListCallback> callback
                                              = nullptr)
      = 0;

   /**
    * Get data profile corresponding to profile identifier.
    *
    * @param [in] profileId         Profile identifier
    * @param [in] techPreference    Technology preference
    *	     - @ref TechPreference
    * @param [in] callback          Callback pointer to get the result of get profile by ID
    *
    * @returns Status of requestProfile i.e. success or suitable error code.
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual telux::common::Status requestProfile(uint8_t profileId, TechPreference techPreference,
                                                std::shared_ptr<IDataProfileCallback> callback
                                                = nullptr)
      = 0;
   /**
    * Get associated slot id for the Data Profile Manager.
    *
    * @returns SlotId
    *
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual int getSlotId() = 0;

};  // end of IDataProfileManager

/**
 * Interface for create profile callback object.
 * Client needs to implement this interface to get single shot responses for command like
 * create profile.
 *
 * The methods in callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
 *          break backwards compatibility.
 */
class IDataCreateProfileCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to IDataProfileManager::createProfile API.
    *
    * @param [out] profileId    created profile Id for the response.
    *                           Use IDataProfileManager::requestProfile to get the data profile
    * @param [out] error        @ref telux::common::ErrorCode
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual void onResponse(int profileId, telux::common::ErrorCode error) {
   }
};

/**
 * @brief Interface for getting list of DataProfile using callback.
 * Client needs to implement this interface to get single shot responses for commands like
 * get profile list and query profile.
 *
 * The methods in callback can be invoked from different threads.
 * The implementation should be thread safe.
 *
 * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
 *          break backwards compatibility.
 */
class IDataProfileListCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to requestProfileList API or queryProfile API.
    *
    * @param [out] profiles   List of profiles supported by the device
    * @param [out] error      @ref telux::common::ErrorCode
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual void onProfileListResponse(const std::vector<std::shared_ptr<DataProfile>> &profiles,
                                      telux::common::ErrorCode error) {
   }
};

/**
 * Interface for getting DataProfile using callback.
 * Client needs to implement this interface to get single shot responses for command like
 * create profile.
 *
 * The methods in callback can be invoked from multiple different threads.
 * The implementation should be thread safe.
 *
 * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
 *          break backwards compatibility.
 */
class IDataProfileCallback : public telux::common::ICommandCallback {
public:
   /**
    * This function is called with the response to IDataProfileManager::requestProfile API.
    *
    * @param [out] profile      Response of data profile
    * @param [out] error        @ref telux::common::ErrorCode
    *
    * @note    Eval: This is a new API and is being evaluated.It is subject to change and could
    *          break backwards compatibility.
    */
   virtual void onResponse(const std::shared_ptr<DataProfile> &profile,
                           telux::common::ErrorCode error) {
   }
};
/** @} */ /* end_addtogroup telematics_data */
}
}

#endif  // DATAPROFILEMANAGER_HPP