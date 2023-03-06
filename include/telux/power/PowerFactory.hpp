/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       PowerFactory.hpp
 *
 * @brief      PowerFactory allows creation of TCU-activity manager class
 */

#ifndef POWERFACTORY_HPP
#define POWERFACTORY_HPP

#include <memory>

#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityDefines.hpp>

namespace telux {
namespace power {

/** @addtogroup telematics_power_manager
 * @{ */

/**
 * @brief   PowerFactory allows creation of TCU-activity manager instance.
 */
class PowerFactory {
public:
    /**
     * API to get the factory instance for TCU-activity management
     */
    static PowerFactory &getInstance();

    /**
     * Gets the TCU-activity manager instance.
     *
     * @param [in] config           TCU-activity manager configuration
     * @param [in] callback         Optional callback pointer to get the response of the manager
     *                              initialization.
     *
     * @returns Pointer to ITcuActivityManager object.
     *
     * @note    This API is recommended for both hypervisor and non-hypervisor based systems.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to
     *             change and could break backwards compatibility.
     */
    virtual std::shared_ptr<ITcuActivityManager> getTcuActivityManager(
        ClientInstanceConfig config, telux::common::InitResponseCb callback = nullptr) = 0;

    /**
     * API to get the TCU-activity Manager instance
     *
     * @param [in] clientType Type of the client that is going to access ITcuActivityManager APIs
     *                        @ref ClientType
     * @param [in] procType   Required processor type on which the operations will be performed
     *                        @ref telux::common::ProcType
     *                        @ref telux::common::ProcType::REMOTE_PROC is not supported
     * @param [in] callback   Optional callback pointer to get the response of the manager
     *                        initialization.
     *
     * @returns     Pointer of ITcuActivityManager object.
     *
     * @note        This API cannot be used on virtual machines or on systems with hypervisor.
     *              The alternative API @ref PowerFactory::getTcuActivityManager(
     *              ClientInstanceConfig config,telux::common::InitResponseCb callback)
     *              should be used.
     *
     * @deprecated  Use @ref PowerFactory::getTcuActivityManager(ClientInstanceConfig config,
     *              telux::common::InitResponseCb callback) API instead
     */
    virtual std::shared_ptr<ITcuActivityManager> getTcuActivityManager(
        ClientType clientType = ClientType::SLAVE,
        common::ProcType procType = common::ProcType::LOCAL_PROC,
        telux::common::InitResponseCb callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
protected:
    PowerFactory();
    virtual ~PowerFactory();
#endif

private:
    PowerFactory(const PowerFactory &) = delete;
    PowerFactory &operator=(const PowerFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_power_manager */

}  // end of namespace power
}  // end of namespace telux

#endif  // POWERFACTORY_HPP
