/*
 *  Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       Version.hpp
 * @brief      Provide APIs to query the version of the SDK
 */

#ifndef VERSION_HPP
#define VERSION_HPP

#include <string>
#include "CommonDefines.hpp"

#define DEFAULT_VALUE -1

namespace telux {
namespace common {

/** @addtogroup telematics_common
 * @{ */

/**
 * Structure of major, minor and patch version
 */
struct SdkVersion {
   int major = DEFAULT_VALUE; /**< Major Version: This number will be incremented whenever
                                   significant changes  or features are introduced */
   int minor = DEFAULT_VALUE; /**< Minor Version: This number will be incremented when smaller
                                   features with some new APIs are introduced. */
   int patch = DEFAULT_VALUE; /**< Patch Version: If the release only contains bug fixes,
                                   but no API change then the patch version would be incremented.*/
};

/**
 * Structure contains the version of the platform software
 */
struct PlatformVersion {
   std::string meta; /**< Meta Version,
                                for example: SA2150P_SA515M.LE_LE.1-3_2-1-00297-STD.INT-1*/
   std::string modem; /**< Modem Version,
                                for example: MPSS.HI.3.1.c3-00114-SDX55_GENAUTO_TEST-1*/
   std::string externalApp; /**< External App Version,
                                for example: LE.UM.3.2.3-72102-SA2150p.Int-1*/
   std::string integratedApp; /**< Integrated App MDM Version,
                                for example: LE.UM.4.1.1-71802-sa515m.Int-1*/
};

/**
 * @brief Provides version of SDK.
 */
class Version {
public:
   /**
    * Get the release name.
    *
    * @returns String contains release name
    */
   static std::string getReleaseName();

   /**
    * Get the Telematics SDK version, for example: 01.00.00
    *
    * @returns @ref SdkVersion structure of major, minor and patch version
    */
   static SdkVersion getSdkVersion();

   /**
    * Get the platform version.
    * Need obtain required permissions from telux_allow_version.
    *
    * @param[in] @ref PlatformVersion structure of modem version, meta version, apps version and
    *                 apps mdm version.
    * @returns Status of getPlatformVersion i.e. success or suitable error code.
    *
    * @note Eval: This is a new API and is being evaluated.It is subject to change
    *       and could break backwards compatibility.
    */
   static Status getPlatformVersion(PlatformVersion & pv);
};
/** @} */ /* end_addtogroup telematics_common */
}
}

#endif
