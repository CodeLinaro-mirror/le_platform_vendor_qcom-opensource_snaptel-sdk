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

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       TcuActivityListener.hpp
 *
 * @brief      TcuActivityListener provides callback methods for listening to TCU-activity service
 *             notifications, like TCU-activity state change.Client need to implement these methods.
 *             The methods in listener can be invoked from multiple threads.So the client needs to
 *             make sure that the implementation is thread-safe.
 */

#ifndef TCUACTIVITYLISTENER_HPP
#define TCUACTIVITYLISTENER_HPP

#include <memory>
#include <vector>

#include <telux/power/TcuActivityDefines.hpp>

namespace telux {
namespace power {

/** @addtogroup telematics_power_manager
 * @{ */

/**
 * @brief Listener class for getting notifications related to TCU-activity state and also the
 *        updates related to TCU-activity service status. The client needs to implement these
 *        methods as briefly as possible and avoid blocking calls in it.
 *        The methods in this class can be invoked from multiple different threads. Client
 *        needs to make sure that the implementation is thread-safe.
 */
class ITcuActivityListener {
public:
    /**
     * This function is called when the TCU activity state of the machine(that the client is
     * registered for) is going to change. When the master triggers state change of a machine
     * using  @ref ITcuActivityManager::setActivityState, the slave clients interested in that
     * machine will receive this notification. This notification will not be received by the Master.
     * State change of @ref ALL_MACHINES via @ref ITcuActivityManager::setActivityState could lead
     * to an individual machine's state change, resulting in a notification to clients of all
     * machines.
     * Slave clients who got this indication must acknowledge it with
     * @ref ITcuActivityManager::sendActivityStateAck.
     *
     * @param [in] state            TCU-activity state that the machine is about to enter
     * @param [in] machineName      Machine name that is undergoing the state change. Assigned
     *                              @ref ALL_MACHINES for a global state change and
     *                              @ref LOCAL_MACHINE for a local state change.
     *
     */
    virtual void onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) {
    }

    /**
     * Informs the master with the consolidated acknowledgement from all slave clients for the state
     * change previously triggered by the master client.
     *
     * This API will be invoked only for the MASTER client.
     *
     * On platforms with access control enabled, the client needs to have TELUX_POWER_CONTROL_STATE
     * permission for this listener API to be invoked.
     *
     * @param [in] status                   This is the status of acknowledgements corresponding to
     *                                      a particular request. If any slave doesn't acknowledge
     *                                      within the configured timeout, then Status::EXPIRED
     *                                      is reported. If any slave sends a negative
     *                                      acknowledgement, then  Status::NOTREADY is reported. If
     *                                      both types of acknowledgement errors exist, then the
     *                                      status code corresponding to most number of clients is
     *                                      reported.
     * @param [in] machineName              Machine name that is undergoing the state change.
     *                                      Assigned @ref ALL_MACHINES for a global state change and
     *                                      @ref LOCAL_MACHINE for a local state change.
     * @param [in] unresponsiveClients      List of client and respective machine names that have
     *                                      not responded via @ref sendActivityStateAck for state
     *                                      transitions of suspend or shutdown triggered by the
     *                                      master via @ref ITcuActivityManager::setActivityState.
     * @param [in] nackResponseClients      List of client and respective machine name who responded
     *                                      with @ref TcuActivityStateChangeResponse::NACK for state
     *                                      transitions of suspend or shutdown triggered by the
     *                                      master via @ref ITcuActivityManager::setActivityState.
     *
     * @note    This API is recommended for systems with and without hypervisor.
     */
    virtual void onSlaveAckStatusUpdate(const telux::common::Status status,
        const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
        const std::vector<ClientInfo> nackResponseClients) {
    }

    /**
     * This API will be invoked if any machine availability changes with respect to power
     * management.
     *
     * User can use @ref ITcuActivityManager::getAllMachineNames() to get all updated available
     * machines.
     * It will be useful for the master client if they are interested in setting the
     * TCUActivityState of a specific machine @ref ITcuActivityManager::setActivityState().
     *
     * This API is meant for clients that have instantiated the ITcuActivityManager instance using
     * @ref ClientType::MASTER
     *
     * @param [in]  machineName             Name of the machine
     * @param [in]  machineEvent            Machine event ( @ref MachineEvent)
     */
    virtual void onMachineUpdate(const std::string machineName, const MachineEvent machineEvent) {
    }

    /**
     * This function is called with the overall acknowledgement status from all the SLAVE clients,
     * for state change triggered previously by MASTER client.
     *
     * This API will be invoked only for the MASTER client.
     * If at least one SLAVE client does not acknowledge within the configured timeout, then
     * Status::EXPIRED would be reported.
     *
     * On platforms with Access control enabled, the client needs to have TELUX_POWER_CONTROL_STATE
     * permission for this listener API to be invoked.
     *
     * @param [in]  status                  Status of the SLAVE clients' acknowledgements
     *
     * @note        This API should not be used on virtual machines or on systems with hypervisor.
     *              The alternative API @ref onSlaveAckStatusUpdate(telux::common::Status status,
     *              std::string machineName, std::vector<std::string> unresponsiveClients,
     *              std::vector<std::string> nackResponseClients) should be used.
     * @deprecated  Use @ref onSlaveAckStatusUpdate(const telux::common::Status status,
     *              const std::string machineName,
     *              const std::vector<std::pair<std::string, std::string>> unresponsiveClients,
     *              const std::vector<std::pair<std::string, std::string>> nackResponseClients)
     *              API instead
     */
    virtual void onSlaveAckStatusUpdate(telux::common::Status status) {
    }

    /**
     * This function is called when the TCU-activity state is going to change.
     *
     * @param [in] state                TCU-activity state that system is about to enter
     *
     * @deprecated  Use @ref onTcuActivityStateUpdate(TcuActivityState state,
     *              bool isGlobalStateChange) API instead
     */
    virtual void onTcuActivityStateUpdate(TcuActivityState state) {
    }

    /**
     * Destructor of ITcuActivityListener
     */
    virtual ~ITcuActivityListener() {
    }
};

/** @} */ /* end_addtogroup telematics_power_manager */

}  // end of namespace power
}  // end of namespace telux

#endif  // TCUACTIVITYLISTENER_HPP
