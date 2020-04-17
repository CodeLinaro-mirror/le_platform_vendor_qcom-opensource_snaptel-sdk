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
 * @file       SimProfileDefines.hpp
 * @brief      SimProfileDefines provides the enumerations required for eUICC operations.
 */

#ifndef SIMPROFILETYPES_HPP
#define SIMPROFILETYPES_HPP

#include <bitset>
#include <vector>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace rsp {

/** @addtogroup telematics_rsp
 * @{ */

/**
 * Indicates profile type of card
 */
enum class ProfileType {
    UNKNOWN = -1,
    REGULAR = 0, /**< Regular profile */
    EMERGENCY,   /**< Emergency profile */
};

/**
 * Indicates profile icon type.
 */
enum class IconType {
    NONE = 0, /**< No icon information */
    JPEG,     /**< JPEG icon */
    PNG,      /**< PNG icon */
};

/**
 * Indicates profile class.
 */
enum class ProfileClass {
    UNKNOWN = -1, /**< No info about profile class */
    TEST = 0,     /**< Test profile */
    PROVISIONING, /**< Provisioning profile */
    OPERATIONAL   /**< Operational  profile */
};

/**
 * Indicates profile download status.
 */
enum class DownloadStatus {
    DOWNLOAD_ERROR = 1,   /**< Profile download error */
    DOWNLOAD_IN_PROGRESS, /**< Profile download in progress with download percentage */
    DOWNLOAD_COMPLETE_INSTALLATION_IN_PROGRESS, /**< Profile download is complete and installation
                                                  is in progress */
    INSTALLATION_COMPLETE,                      /**< Profile installation is complete */
    USER_CONSENT_REQUIRED,                      /**< User consent is required for proceeding
                                                  with download/installation of profile */
};

/**
 * Indicates profile download error cause.
 */
enum class DownloadErrorCause {
    GENERIC = 1, /**< Generic error */
    SIM,         /**< Error from the SIM card */
    NETWORK,     /**< Error from the network */
    MEMORY,      /**< Error due to no memory */
};

/**
 * Defines profile policy rules(PPR). Each value represents
 * corresponding bit for PprMask bitset.
 */
enum PolicyRuleType {
    PROFILE_DISABLE_NOT_ALLOWED = (1 << 0), /**< Disabling of the profile is not allowed */
    PROFILE_DELETE_NOT_ALLOWED = (1 << 1),  /**< Deletion of the profile is not allowed */
    PROFILE_DELETE_ON_DISABLE = (1 << 2),   /**< Deletion of the profile is required on successful
                                                      disabling */
};

/**
 * 16 bit mask that denotes which of the profile policy rules(PPR) defined in
 * Policy Rule Type enum are used.
 */
using PolicyRuleMask = std::bitset<16>;

/** @} */ /* end_addtogroup telematics_rsp */
}
}

#endif  // SIMPROFILEDEFINES_HPP
