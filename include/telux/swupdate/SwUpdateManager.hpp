/*
Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/**
 * @file       SwUpdateFactory.hpp
 *
 * @brief      SwUpdateFactory allows the api's for swupdate support
 */

#ifndef TELUX_SWUPDATE_SWUPDATEMANAGER_HPP
#define TELUX_SWUPDATE_SWUPDATEMANAGER_HPP

#include <memory>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace swupdate {

enum class UpdateStatus {
    SUCCESS,                      /* Swupdate is successful */
    INPROGRESS,                   /* Swupdate is in progress */
    FAILED,                       /* Swupdate is failed */
    INVALIDSTATE,                 /* Invalid state */
    NOSUCH,                       /* No Such file or dir */
};

class ISwUpdateManager {
public:

    // Pure virtual function
    virtual telux::common::Status performUpdate(const std::string& path) = 0;
    virtual telux::swupdate::UpdateStatus readUpdateStatus() = 0;
};
}  // end of namespace swupdate
}  // end of namespace telux
#endif
