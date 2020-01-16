/*
 *  Copyright (c) 2020 The Linux Foundation. All rights reserved.
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
 * @file       BridgeManager.hpp
 *
 * @brief      BridgeManager is a primary interface to enable packet acceleration for non-standard
 *             WLAN and Ethernet physical interfaces. It can configure software bridge between the
 *             the interface and Hardware accelerator. It provide APIs to enable/disable and
 *             set/get/delete software bridges for various interfaces.
 *
 */

#ifndef BRIDGEMANAGER_HPP
#define BRIDGEMANAGER_HPP

#include <future>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataDefines.hpp>

namespace telux {
namespace data {
namespace net {

/**
 * Interface types supported for bridge configuration
 */
enum class BridgeIFaceType {
    UNKNOWN = 0,
    WLAN_AP = 1,    /**< Wireless Local Area Network (WLAN) in AP mode */
    WLAN_STA = 2,   /**< Wireless Local Area Network (WLAN) in STA mode */
    ETH = 3,        /**< Ethernet (ETH) */
};

/**
 * Structure to configure a software bridge for an interface
 */
struct BridgeInfo {
    std::string  ifaceName;     /**< Interface name */
    BridgeIFaceType ifaceType;  /**< Interface type */
    uint32_t bandwidth;         /**< Bandwidth(in Mbps) required for software bridge */
};

/**
 * This function is called as a response to @ref requestBridgeInfo
 *
 * @param [in] infos        List of the software bridge configurations in the system
 * @param [in] error        Return code which indicates whether the operation is succeeded or not
 *                          @ref telux::common::ErrorCode
 *
 * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
 *         break backwards compatibility.
 */
using BridgeInfoResponseCb
    = std::function<void(const std::vector<BridgeInfo> &infos, telux::common::ErrorCode error)>;

/** @addtogroup telematics_net
 * @{ */

/**
 * @brief      IBridgeManager provides APIs to enable/disable and set/get/delete software bridges
 *             for various WLAN and Ethernet interfaces.
 *
 */
class IBridgeManager {
 public:
    /**
     * Checks if the data subsystem is ready.
     *
     * @returns True if the Bridge Manager is ready for service, otherwise returns false.
     *
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for data subsystem to be ready.
     *
     * @returns A future that caller can wait until the Bridge Manager succeed/fail to be ready.
     *
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Enable/Disable the software bridge in the system.
     * It will affect all the configured software bridges for various interfaces.
     *
     * @param [in] enable    TRUE to enable, FALSE to disable the bridge
     * @param [in] callback  Optional callback to get the response for enableBridge
     *
     * @returns Status of enableBridge request i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated.It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status enableBridge( bool enable,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Add software bridge configuration for an interface.
     *
     * @param [in] config    configuration for an interface
     * @param [in] callback  Optional callback to get the response for addBridge
     *
     * @returns Status of addBridge request i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated.It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status addBridge( BridgeInfo config,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Request information about all the software bridge configurations in the system
     *
     * @param [in] callback    Response callback with list of bridge configurations
     *
     * @returns Status of requestBridgeInfo request i.e. success or suitable status code.
     *
     * @note     Eval: This is a new API and is being evaluated.It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status requestBridgeInfo(BridgeInfoResponseCb callback) = 0;

    /**
     * Delete a software bridge configuration for an interface.
     *
     * @param [in] ifaceName   Name of the interface whose configuration needs to be deleted
     * @param [in] callback    Optional callback to get the response for removeBridge
     *
     * @returns Status of removeBridge request i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated.It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status removeBridge( std::string ifaceName,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Destructor for IBridgeManager
     */
    virtual ~IBridgeManager(){};
};  // end of IBridgeManager

/** @} */ /* end_addtogroup telematics_net */
}
}
}
#endif
