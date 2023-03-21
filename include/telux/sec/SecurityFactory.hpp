/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file  SecurityFactory.hpp
 * @brief SecurityFactory allows creation of CryptoManager.
 */

#ifndef TELUX_SEC_SECURITYFACTORY_HPP
#define TELUX_SEC_SECURITYFACTORY_HPP

#include <telux/sec/CryptoManager.hpp>
#include <telux/sec/CryptoAcceleratorManager.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * @brief SecurityFactory allows creation of ICryptoManager and ICryptoAcceleratorManager.
 */
class SecurityFactory {
 public:
    /**
     * Gets the SecurityFactory instance.
     */
    static SecurityFactory &getInstance();

    /**
     * Provides a CryptoManager instance that can be used to perform key management
     * and cryptographic operations.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if ICryptoManager is created
     *                successfully, otherwise, an appropriate error code
     *
     * @returns ICryptoManager instance
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual std::shared_ptr<ICryptoManager> getCryptoManager(
       telux::common::ErrorCode &ec) = 0;

    /**
     * Provides a CryptoAcceleratorManager instance that can be used to perform
     * cryptographic operations requiring elliptic-curve cryptography (ECC)
     * verifications and calculations.
     *
     * @param[out] ec telux::common::ErrorCode::SUCCESS if ICryptoAcceleratorManager is created
     *                successfully, otherwise, an appropriate error code
     *
     * @param[in] mode Defines how users obtain verification and calculation results
     *
     * @param[in] cryptoAccelListener Optional, listener for ECC signature verification
     *                                and ECQV calculation results
     *
     * @returns ICryptoAcceleratorManager instance
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual std::shared_ptr<ICryptoAcceleratorManager> getCryptoAcceleratorManager(
      telux::common::ErrorCode &ec, Mode mode,
      std::weak_ptr<ICryptoAcceleratorListener> cryptoAccelListener = std::weak_ptr<
      ICryptoAcceleratorListener>()
    ) = 0;

#ifndef TELUX_DOXY_SKIP
 protected:
    SecurityFactory();
    virtual ~SecurityFactory();
#endif

 private:
    SecurityFactory(const SecurityFactory &) = delete;
    SecurityFactory &operator=(const SecurityFactory &) = delete;
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // End of namespace sec
}  // End of namespace telux

#endif  // TELUX_SEC_SECURITYFACTORY_HPP
