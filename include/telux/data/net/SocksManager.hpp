/*
 *  Copyright (c) 2019 The Linux Foundation. All rights reserved.
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
 * @file       SocksManager.hpp
 *
 * @brief      SocksManager is a primary interface for configuring Socks Proxy Server
 *
 */

#ifndef SOCKSMANAGER_HPP
#define SOCKSMANAGER_HPP

#include <future>
#include <vector>
#include <list>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
namespace net {

/** @addtogroup telematics_net
 * @{ */
/**
 *@brief    SocksManager is a primary interface for configuring legacy
 *          Socks proxy server
 */
class ISocksManager {
 public:
    /**
     * Checks if the SocksManager subsystem is ready.
     *
     * @returns True if SocksManager is ready for service, otherwise
     * returns false.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for SocksManager subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when Socksanager is ready.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Enable or Disable Socks proxy service.
     *
     * @param [in] enble             true: enable proxy, false: disable proxy
     * @param [in] callback          optional callback to get the operation error code if any
     *
     * @returns Status of proxy enablement i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */

    virtual  telux::common::Status enableSocks(
        bool enable, telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Get the associated operation type for this instance.
     *
     * @returns OperationType of getOperationType i.e. LOCAL or REMOTE.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::data::OperationType getOperationType() = 0;

    /**
     * Destructor for Socks Manager
     */
    virtual ~ISocksManager(){};
};  // end of ISocksManager

/** @} */ /* end_addtogroup telematics_net */
}
}
}

#endif
