/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 * /

/**
 * @file       DiagnosticsFactory.hpp
 * @brief      DiagnosticsFactory is the central factory to create all Diagnostics instances
 */

#ifndef TELUX_PLATFORM_DIAG_DIAGNOSTICSFACTORY_HPP
#define TELUX_PLATFORM_DIAG_DIAGNOSTICSFACTORY_HPP

#include <memory>
#include <mutex>

#include <telux/common/CommonDefines.hpp>

#include <telux/platform/diag/DiagLogManager.hpp>

namespace telux {
namespace platform {
namespace diag {

/** @addtogroup telematics_diagnostics
 * @{ */

/**
 *@brief DiagnosticsFactory is the central factory to create Diagnostics manager class
 */
class DiagnosticsFactory {
 public:
    /**
     * Get Diagnostics Factory instance.
     */
    static DiagnosticsFactory &getInstance();

    /**
     * Get Diagnostics Manager
     *
     * @returns instance of IDiagLogManager
     */
    virtual std::shared_ptr<IDiagLogManager> getDiagLogManager(
        telux::common::InitResponseCb clientCallback = nullptr) = 0;

 protected:
    DiagnosticsFactory();
    virtual ~DiagnosticsFactory();

 private:
    DiagnosticsFactory(const DiagnosticsFactory &) = delete;
    DiagnosticsFactory &operator=(const DiagnosticsFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_diagnostics */
} // end of namespace diag
} // end of namespace platform
} // end of namespace telux

#endif // TELUX_PLATFORM_DIAG_DIAGNOSTICSFACTORY_HPP
