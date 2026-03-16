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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2021-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       TcuActivityManager.hpp
 *
 * @brief      TcuActivityManager class provides interface to register and receive notifications
 *             related to TCU-activity states, initiate TCU-activity state transition.
 */

#ifndef TCUACTIVITYMANAGER_HPP
#define TCUACTIVITYMANAGER_HPP

#include <future>
#include <memory>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/power/TcuActivityListener.hpp>

namespace telux {
namespace power {

/** @addtogroup telematics_power_manager
 * @{ */

/**
 * @brief   ITcuActivityManager provides interface to register and de-register listeners to get
 *          TCU-activity state updates. And also API to initiate TCU-activity state transition.
 *
 * An application can get the appropriate TCU-activity manager (i.e. @ref ClientType::SLAVE or
 * @ref ClientType::MASTER) object from the power factory. The TCU-activity manager configured as
 * the @ref ClientType::MASTER is responsible for triggering state transitions. TCU-activity manager
 * configured as a @ref ClientType::SLAVE is responsible for listening to state change indications
 * and acknowledging when it performs necessary tasks and prepares for the state transition. A
 * machine in this power management framework represents an application processor subsystem or a
 * host/guest virtual machine on hypervisor based platforms.
 *
 * - Only one @ref ClientType::MASTER is allowed in the system, and currently we only support
 *   allowing the @ref ClientType::MASTER on the primary/host machine and not on the guest virtual
 *   machine.
 * - It is expected that all processes interested in a TCU-activity state change should register as
 *   @ref ClientType::SLAVE.
 * - When the @ref ClientType::MASTER changes the TCU-activate state, @ref ClientType::SLAVEs
 *   connected to the impacted machine are notified.
 * - @ref ClientType::MASTER can trigger the TCU-activity state change of a specific machine or all
 *   machines at once.
 * - If the @ref ClientType::SLAVE wants to differentiate between a state change indication that is
 *   the result of a trigger for all machines or a trigger for its specific machines, it can be
 *   detected using the machine name provided in the listener API.
 * - When the @ref ClientType::MASTER triggers an all machines TCU-activity state change, only the
 *   machines that are not in the desired state will undergo the state transition, and the
 *   @ref ClientType::SLAVEs to those machines will be notified.
 * - In the case of
 *      - @ref TcuActivityState::SUSPEND or @ref TcuActivityState::SHUTDOWN trigger:
 *      - After becoming ready for state change, all @ref ClientType::SLAVE should acknowledge back.
 *      - The @ref ClientType::MASTER will get notification about the consolidated acknowledgement
 *        status of all @ref ClientType::SLAVEs.
 *      - On getting a successful consolidated acknowledgement from all the @ref ClientType::SLAVE
 *        for the suspend trigger, the power framework allows the respective machine to suspend. On
 *        getting a successful consolidated acknowledgement from all the @ref ClientType::SLAVEs for
 *        the shutdown trigger, the power framework triggers the respective machine shutdown without
 *        waiting further.
 *      - If the @ref ClientType::SLAVE sends a NACK to indicate that it is not ready for state
 *        transition or fails to acknowledge before the configured time, then the
 *        @ref ClientType::MASTER will get to know via a consolidated/slave acknowledgement status
 *        notification.
 *      - In such failed cases, if the @ref ClientType::MASTER wants to stop the state transition
 *        considering the information in the consolidated acknowledgement, then the
 *        @ref ClientType::MASTER is allowed to trigger a new TCU-activity state change, or else the
 *        state transition will proceed after the configured timeout.
 *   - @ref TcuActivityState::RESUME trigger:
 *      - Power framework will prevent the respective machine from going into suspend.
 *      - No acknowledgement will be required from @ref ClientType::SLAVE and the
 *        @ref ClientType::MASTER will not be getting consolidated/slave acknowledgement as machine
 *        will be already resumed.
 *
 * When the application is notified about the service being unavailable, the TCU-activity state
 * notifications will be inactive. After the service becomes available, the existing listener
 * registrations will be maintained.
 */
class ITcuActivityManager {
public:
   /**
    * This status indicates whether the ITcuActivityManager object is in a usable state.
    *
    * @returns @ref telux::common::ServiceStatus
    *
    */
   virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * Register a listener for updates on TCU-activity state changes.
     *
     * @param [in] listener Pointer of ITcuActivityListener object that processes the notification
     *
     * @returns Status of registerListener i.e success or suitable status code.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<ITcuActivityListener> listener) =0;

    /**
     * Remove a previously registered listener.
     *
     * @param [in] listener Previously registered ITcuActivityListener that needs to be removed
     *
     * @returns Status of deregisterListener, success or suitable status code
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<ITcuActivityListener> listener)
                        = 0;

    /**
     * Register a listener for updates on TCU-activity management service status.
     *
     * @param [in] listener Pointer of IServiceStatusListener object that processes the notification
     *
     * @returns Status of registerServiceStateListener i.e success or suitable status code.
     */
    virtual telux::common::Status registerServiceStateListener(
                        std::weak_ptr<telux::common::IServiceStatusListener> listener) = 0;

    /**
     * Remove a previously registered listener for service status updates.
     *
     * @param [in] listener Previously registered IServiceStatusListener that needs to be removed
     *
     * @returns Status of deregisterServiceStateListener, success or suitable status code
     */
    virtual telux::common::Status deregisterServiceStateListener(
                        std::weak_ptr<telux::common::IServiceStatusListener> listener) = 0;

    /**
     * This API allows the caller to get the machine name where the client is running. It is
     * intended to identify the local machine name on a platform where multiple machines are
     * available in the power framework.
     *
     * @param [out] machineName     Machine name where the process is running
     *
     * @returns Status of getMachineName, success or suitable status code.
     */
    virtual telux::common::Status getMachineName(std::string& machineName) = 0;

    /**
     * Enumerates all machines in the system that are available and ready to be managed by the power
     * framework.
     *
     * This API is meant for clients that have instantiated the ITcuActivityManager instance using
     * @ref ClientType::MASTER. If the platform has multiple machines available, knowing their names
     * will be useful if the master is interested in individually modifying the activity state of
     * any available machine using @ref setActivityState.
     *
     * @param [out] machineNames     List of machine names that are available for power management.
     *
     * @returns Status of getAllMachineNames, success or suitable status code.
     */
    virtual telux::common::Status getAllMachineNames(std::vector<std::string>& machineNames) = 0;

    /**
     * Initiates a TCU-activity state transition.
     *
     * This API also initiates the relevant internal operation if the platform is configured to
     * change the modem activity state automatically when the TCU activity state changes.
     *
     * This API needs to be used cautiously, as it could change the power-state of the system and
     * may affect other processes. For example, if a master sets the SUSPEND state, all SLAVE
     * processes will suspend their activity, allowing the system to suspend.
     * This API can only be invoked by clients that have instantiated the ITcuActivityManager
     * instance using @ref ClientType::MASTER.
     *
     * Based on the final acknowledgements from all the slaves
     * @ref ITcuActivityListener::onSlaveAckStatusUpdate,
     *  1.  If the acknowledgement status is SUCCESS, then the framework attempts to state
     *      transition(SUSPEND/SHUTDOWN) immediately on the relevant machines.
     *  2.  If the acknowledgement status is not SUCCESS, then the framework waits for a configured
     *      timeout before attempting the state transition(SUSPEND/SHUTDOWN) on the relevant
     *      machines.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_POWER_CONTROL_STATE
     * permission to invoke this API successfully.
     *
     * @param [in] state            TCU-activity state that the system is intended to enter
     * @param [in] machineName      Machine name if the state transition is intended for the
     *                              specific machine only. If assigned to ALL_MACHINES, then the
     *                              state applies to the whole system.
     * @param [in] callback         Optional callback to get the response for the TCU-activity state
     *                              transition command
     *
     * @returns Status of setActivityState i.e. success or suitable status code.
     */
    virtual telux::common::Status setActivityState( TcuActivityState state,
                        std::string machineName,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Get the current TCU-activity state.
     *
     * @returns TcuActivityState
     */
    virtual TcuActivityState getActivityState() = 0;

    /**
     * Sends the acknowledgement after processing a TCU-activity state notification. This indicates
     * that the client is prepared for the state transition.
     * Only one acknowledgement should be sent per @ref ClientType::SLAVE instance of
     * @ref ITcuActivityManager, even if multiple listeners are registered with that instance.
     *
     * All slave clients that received a state change notification via
     * @ref TcuActivityListener::onTcuActivityStateUpdate must acknowledge using this API.
     *
     * @param [in] ack      Acknowledgement for a TCU-activity state notification
     *                      @ref StateChangeResponse.
     * @param [in] state    Represents the TCU activity state transition event corresponding to this
     *                      acknowledgement.
     *
     * @returns Status of sendActivityStateAck i.e. success or suitable status code.
     */
    virtual telux::common::Status sendActivityStateAck( StateChangeResponse ack,
                        TcuActivityState state) = 0;

    /**
     * Explicitly sets the modem state change.
     *
     * The platform could be configured to automatically manage the modem state when
     * @ref setTcuActivityState is called. For example, when suspend is called, the implementation
     * will also set the modem to suspend. In that case, this API need not be invoked after setting
     * the TCU state.
     *
     * This API needs to be used cautiously, as it could affect WWAN functionalities.
     *
     * This API is meant for clients that have instantiated the ITcuActivityManager instance using
     * @ref ClientType::MASTER
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_POWER_CONTROL_STATE
     * permission to invoke this API successfully.
     *
     * @param [in] state    Activity state that the modem is intended to enter
     *                      @ref TcuActivityState
     *                      SUSPEND - Reduce/Throttle the modem activities
     *                      RESUME  - Restore the activities that were throttled earlier
     *                      Any other input is considered invalid.
     *
     * @returns Status of setModemActivityState i.e. success or suitable status code.
     *
     */
    virtual telux::common::Status setModemActivityState(TcuActivityState state) = 0;

    /**
     * Checks the status of TCU-activity services, if other APIs are ready for use, and returns the
     * results.
     *
     * @returns  True if the services are ready; otherwise, false.
     *
     * @deprecated Use ITcuActivityManager::getServiceStatus() API.
     *             @ref telux::power::ITcuActivityManager::getServiceStatus
     */
    virtual bool isReady() = 0;

    /**
     * Waits for TCU-activity services to be ready.
     *
     * @returns  A future that caller can wait on to be notified when TCU-activity services
     *           are ready.
     *
     * @deprecated Use InitResponseCb in PowerFactory::getTcuActivityManager instead, to get
     *             get notified about subsystem readiness
     *             @ref telux::power::PowerFactory::getTcuActivityManager
     */
    virtual std::future<bool> onReady() = 0;

    /**
     * Initiates a TCU-activity state transition.
     * If platform is configured to change modem activity state automatically when TCU activity
     * state is changed, this API initiates the relevant internal operation.
     *
     * This API needs to be used cautiously, as it could change the power-state of the system and
     * may affect other processes.
     *
     * This API should only be invoked by a client that has instantiated the ITcuActivityManager
     * instance using ClientType::MASTER
     *
     * On platforms with access control enabled, the caller needs to have TELUX_POWER_CONTROL_STATE
     * permission to invoke this API successfully.
     *
     * @param [in] state        TCU-activity state that the system is intended to enter
     * @param [in] callback     Optional callback to get the response for the TCU-activity state
     *                          transition command
     *
     * @returns Status of setActivityState i.e. success or suitable status code.
     *
     *
     * @note    This API should not be used on virtual machines or on systems with hypervisor. The
     *          alternative API @ref setActivityState( TcuActivityState state,
     *          std::string machineName = "",telux::common::ResponseCallback callback = nullptr)
     *          should be used.
     *
     * @deprecated  Use @ref setActivityState(TcuActivityState state, std::string machineName,
     *              telux::common::ResponseCallback) API instead
     */
    virtual telux::common::Status setActivityState( TcuActivityState state,
                        telux::common::ResponseCallback callback = nullptr) = 0;

    /**
     * Sends acknowledgement after processing a TCU activity state notification.
     * This indicates that the client is prepared for state transition. Only one acknowledgement is
     * expected from a single client process, although it may have multiple listeners.
     *
     * All slave clients that received a state change notification via
     * @ref TcuActivityListener::onTcuActivityStateUpdate must acknowledge using this API.
     *
     * @param [in] ack          Acknowledgement for a TCU-activity state notification.
     *
     * @returns Status of sendActivityStateAck i.e. success or suitable status code.
     *
     * @deprecated  Use @ref sendActivityStateAck( TcuActivityState state,
                    StateChangeResponse ack) API instead
     */
    virtual telux::common::Status sendActivityStateAck(TcuActivityStateAck ack) = 0;

    /**
     * Destructor of ITcuActivityManager
     */
    virtual ~ITcuActivityManager(){};
};
/** @} */ /* end_addtogroup telematics_power_manager */

}  // end of namespace power
}  // end of namespace telux

#endif  // TCUACTIVITYMANAGER_HPP
