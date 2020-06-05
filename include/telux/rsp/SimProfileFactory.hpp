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
 * @file       SimProfileFactory.hpp
 *
 * @brief      SimProfileFactory is the central factory to create all Sim Profile related
               class instances.
 *
 */

#ifndef SIMPROFILEFACTORY_HPP
#define SIMPROFILEFACTORY_HPP

#include <memory>
#include <mutex>

#include <telux/common/CommonDefines.hpp>

#include <telux/rsp/SimProfileManager.hpp>
#include <telux/rsp/HttpTransactionManager.hpp>

namespace telux {
namespace rsp {

/** @addtogroup telematics_rsp
 * @{ */

/**
 *@brief SimProfileFactory is the central factory to create all eUICC manager class instances.
 *
 */
class SimProfileFactory {
 public:
    /**
     * Get SIM Profile Factory instance.
     */
    static SimProfileFactory &getInstance();

    /**
     * Get SimProfileManager. SimProfileManager is a primary interface for remote
     * eUICC(eSIM) provisioning and local profile assistance.
     *
     * @returns instance of ISimProfileManager
     *
     */
    std::shared_ptr<ISimProfileManager> getSimProfileManager();

    /**
     * Get HttpTransactionManager. HttpTransactionManager is a primary interface for sending the
     * response for HTTP post request to modem and listen for indication for HTTP post request to
     * download the profile.
     *
     * @returns instance of IHttpTransactionManager
     *
     */
    std::shared_ptr<IHttpTransactionManager> getHttpTransactionManager();

 private:
    // mutex to protect member variables
    std::mutex mutex_;
    std::shared_ptr<ISimProfileManager> simProfileManager_;
    std::shared_ptr<IHttpTransactionManager> httpTransactionManager_;

    SimProfileFactory();
    ~SimProfileFactory();
    SimProfileFactory(const SimProfileFactory &) = delete;
    SimProfileFactory &operator=(const SimProfileFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_rsp */
}
}

#endif
