/*
Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/**
 * @file       SwUpdateFactory.hpp
 *
 * @brief      SwUpdateFactory allows the api's for swupdate support
 */

#ifndef TELUX_SWUPDATE_SWUPDATEFACTORY_HPP
#define TELUX_SWUPDATE_SWUPDATEFACTORY_HPP

#include <memory>
#include "SwUpdateManager.hpp"
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace swupdate {

class SwUpdateFactory {
public:
    /**
     * Get SwUpdate Factory instance.
     */
    static SwUpdateFactory &getInstance();

    /**
     * Get instance of SwUpdate Manager
     *
     * @param[in] callback   Optional callback to get the response of the manager
     *                       initialization.
     *
     * @returns Pointer of ISwUpdateManager object.
     */
    virtual std::shared_ptr<ISwUpdateManager> getSwUpdateManager(telux::common::InitResponseCb callback = nullptr) = 0;

#ifndef TELUX_DOXY_SKIP
protected:
    SwUpdateFactory();
    ~SwUpdateFactory();
#endif

private:
    SwUpdateFactory(const SwUpdateFactory &) = delete;
    SwUpdateFactory &operator=(const SwUpdateFactory &) = delete;
};
}
}
#endif
