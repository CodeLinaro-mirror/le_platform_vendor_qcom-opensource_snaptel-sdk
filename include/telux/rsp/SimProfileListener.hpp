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
 * @file       SimProfileListener.hpp
 *
 * @brief      The interface listens for profile download indication and keep track of
 *             download and install progress of profile.
 */

#ifndef SIMPROFILELISTENER_HPP
#define SIMPROFILELISTENER_HPP

#include <telux/rsp/SimProfileDefines.hpp>

namespace telux {
namespace rsp {

/** @addtogroup telematics_rsp
 * @{ */

/**
 * @brief The interface listens for profile download indication and keep track of
 *         download and install progress of profile.
 *
 *        The methods in the listener can be invoked from multiple threads.
 *        It is client's responsibility to make sure the implementation is thread safe.
 */
class ISimProfileListener : public telux::common::IServiceStatusListener {
 public:
    /**
     * This function is called when indication about status of profile download and installation
     * comes.
     *
     * @param [in] slotId                   Slot on which profile get downloaded and installed.
     * @param [in] userConsentRequired      User consent required or not.
     * @param [in] status                   @Ref ProfileDownloadStatus.
     * @param [in] percentage               Download and installation percentage.
     * @param [in] cause                    @Ref ProfileDownloadErrorCause.
     * @param [in] mask                     @Ref PprMask (Profile policy rules Mask)
     */
    virtual void onAddProfileUpdate(int slotId, bool userConsentRequired, DownloadStatus status,
        uint8_t percentage, DownloadErrorCause cause, PolicyRuleMask mask) {
    }

    /**
     * Destructor of ISimProfileListener
     */
    virtual ~ISimProfileListener() {
    }
};
/** @} */ /* end_addtogroup telematics_rsp */
}  // end of namespace rsp

}  // end of namespace telux

#endif  // SIMPROFILELISTENER_HPP
