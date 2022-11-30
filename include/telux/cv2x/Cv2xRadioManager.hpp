/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
* @file       Cv2xRadioManager.hpp
*
* @brief      Cv2xRadioManager manages instances of Cv2xRadio
*
*/

#ifndef CV2XRADIOMANAGER_HPP
#define CV2XRADIOMANAGER_HPP

#include <memory>
#include <future>

#include <telux/common/CommonDefines.hpp>
#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/cv2x/Cv2xRadioListener.hpp>


namespace telux {

namespace cv2x {

class ICv2xRadio;

/**
 *@brief Cv2x Radio Manager listeners implement this interface.
 */
class ICv2xListener : public common::IServiceStatusListener {
public:
    /**
     * Called when the status of the CV2X radio has changed.
     *
     * @param [in] status - CV2X radio status.
     *
     * @deprecated use onStatusChanged(Cv2xStatusEx status)
     */
    virtual void onStatusChanged(Cv2xStatus status) {}

    /**
     * Called when the status of the CV2X radio has changed.
     *
     * @param [in] status - CV2X radio status.
     */
    virtual void onStatusChanged(Cv2xStatusEx status) {}

    /**
     * Called when CV2X SLSS Rx is enabled and any of below events has occurred:
     *  - A new SLSS synce reference UE is detected, lost, or selected as the timing source,
     *    report the present sync reference UEs.
     *  - UE timing source switches from SLSS to GNSS, report 0 sync reference UE.
     *  - SLSS Rx is disabled, report 0 sync reference UE.
     *  - Cv2x is stopped, report 0 sync reference UE.
     * @param [in] info - CV2X SLSS Rx information.
     */
    virtual void onSlssRxInfoChanged(const SlssRxInfo& slssInfo) {}

    /**
     * Destructor for ICv2xListener
     */
    virtual ~ICv2xListener() {}
};

/**
 * This function is called as a response to @ref ICv2xRadioManager::startCv2x
 *
 * @param [in] error     - SUCCESS if Cv2x mode successfully started
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 *
 */
using StartCv2xCallback = std::function<void (telux::common::ErrorCode error)>;


/**
 * This function is called as a response to @ref ICv2xRadioManager::stopCv2x
 *
 * @param [in] error     - SUCCESS if Cv2x mode successfully stopped
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 *
 */
using StopCv2xCallback = std::function<void (telux::common::ErrorCode error)>;


/**
 * This function is called as a response to @ref ICv2xRadioManager::requestCv2xStatus
 *
 * @param [in] status    - Cv2x status
 * @param [in] error     - SUCCESS if Cv2x status was successully retrieved
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 *
 *
 * @deprecated use @ref RequestCv2xStatusCallbackEx
 */
using RequestCv2xStatusCallback = std::function<void (Cv2xStatus status,
                                                      telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref ICv2xRadioManager::requestCv2xStatus
 *
 * @param [in] status    - Cv2x status
 * @param [in] error     - SUCCESS if Cv2x status was successully retrieved
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 */
using RequestCv2xStatusCallbackEx = std::function<void (Cv2xStatusEx status,
                                                        telux::common::ErrorCode error)>;


/**
 * This function is called with the response to @ref ICv2xRadioManager::updateConfiguration
 *
 * @param [in] error     - SUCCESS if configuration was updated successfully
 *                       - SUCCESS
 *                       - GENERIC_FAILURE
 */
using UpdateConfigurationCallback =
    std::function<void (telux::common::ErrorCode error)>;

/**
 * This function is called as a response to @ref ICv2xRadioManager::getCv2xSlssRxInfo
 *
 * @param [out] info     - Cv2x SLSS Rx Information
 * @param [out] error    - SUCCESS if Cv2x SLSS Rx Information was successully retrieved
 *                       - @ref SUCCESS
 *                       - @ref GENERIC_FAILURE
 */
using GetSlssRxInfoCallback = std::function<void (const SlssRxInfo& info,
                                                  telux::common::ErrorCode error)>;


/** @addtogroup telematics_cv2x_cpp
 * @{ */

/**
 * @brief      Cv2xRadioManager manages instances of Cv2xRadio
 */
class ICv2xRadioManager {
public:
    /**
     * Checks if the Cv2x Radio Manager is ready.
     *
     * @returns True if Cv2x Radio Manager is ready for service, otherwise
     * returns false.
     *
     * @deprecated use getServiceStatus instead
     */
    virtual bool isReady() = 0;

    /**
     * Wait for Cv2x Radio Manager to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when Cv2x Radio Manager is ready.
     *
     * @deprecated the readiness can be notified via the callback passed to
     *             Cv2xFactory::getCv2xRadioManager.
     *
     */
    virtual std::future<bool> onReady() = 0;

    /**
     * This status indicates whether the Cv2xRadioManager is in a usable state.
     *
     * @returns SERVICE_AVAILABLE    -  If cv2x radio manager is ready for service.
     *          SERVICE_UNAVAILABLE  -  If cv2x radio manager is temporarily unavailable.
     *          SERVICE_FAILED       -  If cv2x radio manager encountered an irrecoverable failure.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject to change and
     *             could break backwards compatibility.
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Get Cv2xRadio instance
     *
     * @param [in] category - Specifies the category of the client application.
     *                        This field is currently unused.
     *
     * @returns Reference to Cv2xRadio interface that corresponds to the Cv2x Traffic
     *          Category specified.
     */
    virtual std::shared_ptr<ICv2xRadio> getCv2xRadio(TrafficCategory category) = 0;

    /**
     * Put modem into CV2X mode.
     *
     * @param [in] cb      - Callback that is invoked when Cv2x mode is started
     *
     * @returns SUCCESS on success. Error status otherwise.
     */
    virtual telux::common::Status startCv2x(StartCv2xCallback cb) = 0;

    /**
     * Take modem outo of CV2X mode
     *
     * @param [in] cb      - Callback that is invoked when Cv2x mode is stopped
     *
     * @returns SUCCESS on success. Error status otherwise.
     */
    virtual telux::common::Status stopCv2x(StopCv2xCallback cb) = 0;

    /**
     * request CV2X status from modem
     *
     * @param [in] cb      - Callback that is invoked when Cv2x status is retrieved
     *
     * @returns SUCCESS on success. Error status otherwise.
     *
     * @deprecated use requestCv2xStatus(RequestCv2xCalbackEx)
     */
    virtual telux::common::Status requestCv2xStatus(RequestCv2xStatusCallback cb) = 0;

    /**
     * request CV2X status from modem
     *
     * @param [in] cb      - Callback that is invoked when Cv2x status is retrieved
     *
     * @returns SUCCESS on success. Error status otherwise.
     */
    virtual telux::common::Status requestCv2xStatus(RequestCv2xStatusCallbackEx cb) = 0;

    /**
     * Registers a listener for this manager.
     *
     * @param [in] listener - Listener that implements Cv2xListener interface.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ICv2xListener> listener) = 0;

    /**
     * Deregisters a Cv2xListener for this manager.
     *
     * @param [in] listener - Previously registered CvListener that is to be
     *        deregistered.
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<ICv2xListener> listener) = 0;

    /**
     * Updates CV2X configuration.
     * Requires CV2X TX/RX radio status be Inactive. If CV2X radio status is
     * Active or Suspended, call @ref stopCv2x before updateConfiguration.
     *
     * @param [in] configFilePath - Path to config file.
     * @param [in] cb             - Callback that is invoked when the send is complete.
     *                              This may be null.
     *
     * @deprecated Use ICv2xConfig instead
     */
    virtual telux::common::Status updateConfiguration(const std::string & configFilePath,
                                                      UpdateConfigurationCallback cb) = 0;

    /**
     * Set RF peak cv2x transmit power.
     * This affects the power for all existing flows and for any flow created int the future
     *
     * @param [in] txPower - Desired global Cv2x peak tx power in dbm
     * @param [in] cb      - Callback that is invoked when Cv2x peak tx power is set
     *
     * @returns SUCCESS on success. Error status otherwise.
     */
    virtual telux::common::Status setPeakTxPower(int8_t txPower, common::ResponseCallback cb) = 0;

    /**
     * Request to install remote UE src L2 filters.
     * This affects receiving of the UEs' packets in specified period with specified PPPP
     *
     * @param [in] filterList - remote UE src L2 Id, filter duration and PPPP list, max size 50
     * @param [in] cb         - Callback that is invoked when the request is sent
     *
     * @returns SUCCESS on success. Error status otherwise.
     */
    virtual telux::common::Status setL2Filters(const std::vector<L2FilterInfo> &filterList,
        common::ResponseCallback cb) = 0;

    /**
     * Remove the previously installed filters matching src L2 address list.
     * Hence forth this would allow reception of packets from specified UE's
     *
     * @param [in] l2IdList - remote UE src L2 Id list, max size 50
     * @param [in] cb       - Callback that is invoked when the request is sent
     *
     * @returns SUCCESS on success. Error status otherwise.
     */
    virtual telux::common::Status removeL2Filters(const std::vector<uint32_t> &l2IdList,
        common::ResponseCallback cb) = 0;

    /**
     * Get CV2X SLSS Rx information from modem.
     *
     * @param [in] cb   - Callback that is invoked when Cv2x SLSS Rx information is retrieved.
     *
     * @returns SUCCESS on success. Error status otherwise.
     */
    virtual telux::common::Status getSlssRxInfo(GetSlssRxInfoCallback cb) = 0;

    virtual ~ICv2xRadioManager() {}
};

/** @} */ /* end_addtogroup telematics_cv2x_cpp */

} // namespace cv2x

} // namespace telux

#endif // #ifndef CV2XRADIOMANAGER_HPP
