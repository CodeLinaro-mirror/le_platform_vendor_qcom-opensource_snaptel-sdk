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
 * @file       FsManager.hpp
 * @brief      FsManager provides APIs related to File System(FS) management such as notifying
 *             the backup/restore operations.
 */

#ifndef TELUX_PLATFORM_FSMANAGER_HPP
#define TELUX_PLATFORM_FSMANAGER_HPP

#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/platform/FsDefines.hpp>
#include <telux/platform/FsListener.hpp>

namespace telux {

namespace platform {
/** @addtogroup telematics_platform_filesystem
 * @{ */

/**
 * @brief   IFsManager provides interface to to control and get notified about file system
 *          operations. This includes Embedded file system (EFS) operations.
 */
class IFsManager {
 public:
    /**
     * This status indicates whether the object is in a usable state.
     *
     * @returns @ref telux::common::ServiceStatus indicating the current status of the file system
     *          service.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Registers the listener for FileSystem Manager indications.
     *
     * @param [in] listener      - pointer to implemented listener.
     *
     * @returns status of the registration request.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IFsListener> listener) = 0;

    /**
     * Deregisters the previously registered listener.
     *
     * @param [in] listener      - pointer to registered listener that needs to be removed.
     *
     * @returns status of the deregistration request.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IFsListener> listener) = 0;

    /**
     * Request to trigger an EFS backup. If the request is successful, the status of EFS backup
     * is notified via @ref telux::platform::IFsListener::OnEfsBackupEvent.
     *
     * @returns The status of the request - @ref telux::common::Status
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::Status startEfsBackup() = 0;

    /**
     * Destructor of IFsManager
     */
    virtual ~IFsManager(){};
};

/** @} */ /* end_addtogroup telematics_platform_filesystem */
}  // end of namespace platform

}  // end of namespace telux

#endif  // TELUX_PLATFORM_FSMANAGER_HPP
