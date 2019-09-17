/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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
 * @file       DataFilterManager.hpp
 *
 * @brief      It manages Data Restrict Filters. When the filters are enabled, only the data packets
 *             matching the filters will be sent by the modem to the apps processor. All other
 *             packets will be queued by the modem till the filters are disabled. One application
 *             of these filters is for power save purposes. When the apps processor goes to sleep,
 *             spurious incoming packets from the network could unnecessarily wake it up thereby
 *             draining the power. The DataFilterManager allows one to add filters only for
 *             necessary/important/wakeup packets. After adding these filters, one could enable them
 *             just before the apps processor goes to sleep. The apps proc will now be woken up only
 *             if a packet that we care about is received by the modem.
 *
 * @note       Eval: This is a new API and is being evaluated. It is subject to change and could
 *             break backwards compatibility.
 */

#ifndef DATAFILTERMANAGER_HPP
#define DATAFILTERMANAGER_HPP

#include <future>
#include <memory>

#include <telux/common/CommonDefines.hpp>
#include <telux/data/DataFilterListener.hpp>
#include <telux/data/DataRestrictFilter.hpp>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

/**
 * @brief   IDataFilterManager class provides interface to enable/disable the data restrict filters
 *          and register for data restrict filter.
 *          The filtering can be done at any time. One such use case is to do it when we want the AP
 *          to suspend so that we are not waking up the AP due to spurious incoming messages. Also
 *          to make sure the DataRestrict mode is enabled.
 *
 *          In contrary to when DataRestrict mode is disabled, modem will forward all the
 *          incoming data packets to AP and might wake up AP unnecessarily.
 *
 * @note    Eval: This is a new API and is being evaluated. It is subject to change and could break
 *          backwards compatibility.
 */
class IDataFilterManager {
public:
    /**
     * Checks the status of Data Filter Service and if the other APIs are ready for use,
     * and returns the result.
     *
     * @returns  True if the services are ready otherwise false.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual bool isReady() = 0;

    /**
     * Wait for Data Filter Service to be ready.
     *
     * @returns  A future that caller can wait on to be notified when Data Filter Service
     *           are ready.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual std::future<bool> onReady() = 0;

    /**
     * Register a listener for powersave filtering mode notifications.
     *
     * @param [in] listener - Pointer of IDataFilterListener object that processes the notification
     *
     * @returns Status of registerListener i.e success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IDataFilterListener> listener) = 0;

    /**
     * Remove a previously registered listener.
     *
     * @param [in] listener - Previously registered IDataFilterListener that needs to be removed
     *
     * @returns Status of deregisterListener, success or suitable status code
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status
    deregisterListener(std::weak_ptr<IDataFilterListener> listener) = 0;

    /**
     * Changes the Data Powersave filter mode and auto exit feature.
     *
     * This API enables or disables the powersave filtering mode of the packet data session..
     *
     * @param [in] mode - Enable or disable the powersave filtering mode.
     * @param [in] profileId - Profile ID for data connection.
     * @param [in] ipFamilyType - IP Family type @ref IpFamilyType.
     * @param [in] callback - Optional callback to get the response for the change in filter mode.
     *
     * @returns Status of setDataRestrictMode i.e. success or suitable status code.
     *
     * @note    Eval: This is a new API and is being evaluated. It is subject to change
     *          and could break backwards compatibility.
     */
    virtual telux::common::Status
    setDataRestrictMode(DataRestrictMode mode,
                        int profileId = DEFAULT_PROFILE_ID_MAX,
                        IpFamilyType ipFamilyType = IpFamilyType::UNKNOWN,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * This API adds a filter rules for a packet data session to achieve power savings.
     * In case when DataRestrict mode is enabled and AP is in suspended state, Modem will
     * filter all the incoming data packet and route them to AP only if filter rules added via
     * addDataRestrictFilter API matches the criteria, else they are queued at Modem itself and not
     * forwarded to AP, until filter mode is disabled.
     *
     * @param [in] filter - Filter rule.
     * @param [in] profileId - Profile ID for data connection.
     * @param [in] ipFamilyType - IP Family type @ref IpFamilyType.
     *
     * @param [in] callback - Optional callback to get the response.
     *
     * @returns Status of addDataRestrictFilter i.e. success or suitable status code.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status
    addDataRestrictFilter(std::shared_ptr<IDataRestrictFilter> &filter,
                          int profileId = DEFAULT_PROFILE_ID_MAX,
                          IpFamilyType ipFamilyType = IpFamilyType::UNKNOWN,
                          telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * This API removes all the previous added powersave filter for a packet data session
     *
     * @param [in] profileId - Profile ID for data connection.
     * @param [in] ipFamilyType - IP Family type @ref IpFamilyType.
     * @param [in] callback - Optional callback to get the response.
     *
     * @returns Status of removeAllDataRestrictFilters i.e. success or suitable status code.
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual telux::common::Status
    removeAllDataRestrictFilters(int profileId = DEFAULT_PROFILE_ID_MAX,
                                 IpFamilyType ipFamilyType = IpFamilyType::UNKNOWN,
                                 telux::common::ResponseCallback callback = nullptr) = 0;
    /**
     * Get associated slot id for the Data Filter Manager.
     *
     * @returns SlotId
     *
     */
    virtual int getSlotId() = 0;

    /**
     * Destructor of IDataFilterManager
     */
    virtual ~IDataFilterManager(){};
};
/** @} */ /* end_addtogroup telematics_data */

} // namespace data
} // end of namespace telux

#endif // DATAFILTERMANAGER_HPP
