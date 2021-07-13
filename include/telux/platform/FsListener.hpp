/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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
 * @file    FsListener.hpp
 *
 * @brief   FsListener provides callback methods for listening to restore indications.
 *          Client need to implement these methods. The methods in listener can be invoked
 *          from multiple threads.So the client needs to make sure that the implementation
 *          is thread-safe.
 */

#ifndef TELUX_PLATFORM_FSLISTENER_HPP
#define TELUX_PLATFORM_FSLISTENER_HPP

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/FsDefines.hpp>

namespace telux {

namespace platform {

/** @addtogroup telematics_platform_filesystem
 * @{ */

/**
 * @brief Listener class for getting notifications related to EFS backup/restore operations.
 *        The client needs to implement these methods as briefly as possible and avoid blocking
 *        calls in it. The methods in this class can be invoked from multiple different threads.
 *        Client needs to make sure that the implementation is thread-safe.
 */
class IFsListener : public common::IServiceStatusListener {
 public:
    /**
     * This function is called when a EFS restore operation is detected.
     *
     * @param [in] event    Event related data.  @ref telux::platform::EfsEventInfo.
     */
    virtual void OnEfsRestoreEvent(EfsEventInfo event) {
    }

    /**
     * Destructor of IFsListener
     */
    virtual ~IFsListener() {
    }
};

/** @} */ /* end_addtogroup telematics_platform_filesystem */

}  // end of namespace platform

}  // end of namespace telux

#endif  // TELUX_PLATFORM_FSLISTENER_HPP
