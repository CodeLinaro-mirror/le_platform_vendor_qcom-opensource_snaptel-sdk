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
 *  Copyright (c) 2023, 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file       TcuActivityDefines.hpp
 *
 * @brief      This file contains types related to TCU activity.
 */

#include <string>
#include <telux/common/CommonDefines.hpp>

#ifndef TCUACTIVITYDEFINES_HPP
#define TCUACTIVITYDEFINES_HPP

namespace telux {
namespace power {

/** @addtogroup telematics_power_manager
 * @{ */

/**
 * Client information includes the client's name and the machine name on which the client is
 * registered. Client name is the first item in the pair, followed by machine name.
 */
typedef std::pair<std::string, std::string>     ClientInfo;

/**
 * Defines the supported TCU-activity states that the listeners will be notified about.
 */
enum class TcuActivityState {
    UNKNOWN,    /**< To indicate that system state information is not available */
    SUSPEND,    /**< System is going to SUSPEND state */
    RESUME,     /**< System is going to RESUME state */
    SHUTDOWN    /**< System is going to SHUTDOWN */
};

/**
 * Defines the acknowledgements to TCU-activity state transition.
 * The client process sends this response via @ref ITcuActivityManager::sendActivityStateAck after
 * processing the TCU-activity state change notification received via
 * @ref ITcuActivityListener::onTcuActivityStateUpdate.
 *
 * The framework does not require slave clients to respond when changing the state to
 * @ref TcuActivityState::RESUME.
 */
enum class StateChangeResponse {
    ACK,    /**< Processed TCU-activity state change */
    NACK    /**< Not prepared/ready for TCU-activity state change */
};

/**
 * Defines the type of client that would be using the ITcuActivityManager APIs. Client
 * that just needs the TcuActivityState notifications needs to choose ClientType::SLAVE.
 * And the client that determines the TcuActivityState would choose ClientType::MASTER.
 * Only a Master client can set the TcuActivityState. In a system, there should be a single Master
 * client.
 *
 * The ClientType needs to be chosen while instantiating the ITcuActivityManager, using the API
 * PowerFactory::getTcuActivityManager
 */
enum class ClientType {
    SLAVE,     /**< Client is a slave and interested in state change notification */
    MASTER,    /**< Client makes the decision on when the TcuActivityState should change */
};

/**
 * Defines the type of event with respect to machine availability.
 * This only represents the availability of the machine to manage its activity state and not whether
 * the machine itself is enabled.
 *
 * @ref ITcuActivityListener::onMachineUpdate() can be used to listen to changes in machine
 * availability.
 */
enum class MachineEvent {
    AVAILABLE,      /**< New machine available for power management */
    UNAVAILABLE     /**< machine unavailable for power management */
};

/**
 * This special name represents all the machines on the platform. A client could specify
 * this name when using ClientInstanceConfig::machineName to mean all machines as
 * opposed to a specific machine name.
 */
static const std::string ALL_MACHINES   =   "ALL_MACHINES";

/**
 * This special name represents the machine name where the process is running. A client
 * could specify this name when using ClientInstanceConfig::machineName to mean local
 * machine's name.
 * "LOCAL_MACHINE" in case of a hypervisor environment specifies that the client is interested in
 * the virtual machine the client is running on.
 */
static const std::string LOCAL_MACHINE  =   "LOCAL_MACHINE";

/**
 * TCU-activity Manager configuration
 */
struct ClientInstanceConfig {
    /** Type of the client that is going to access ITcuActivityManager APIs @ref ClientType.
     * There will be a single @ref ClientType::MASTER across all available machines.
     */
    ClientType clientType = ClientType::SLAVE;

    /** Identifies the client that is retrieving an instance of the TcuActivityManager.
     * This is a mandatory field and it needs to be unique across all TcuActivityManager clients
     * across all machines in the system. To make it unique, one example could be
     * machineName_ProcessName_ProcessId. This field will be used to provide a list of client
     * names to the master via @ref ITcuActivityListener::onSlaveAckStatusUpdate in case any slave
     * client does not acknowledge or provide a nack via
     * @ref ITcuActivityManager::sendActivityStateAck for state transition triggered by the master
     * via @ref ITcuActivityManager::setActivityState
     */
    std::string clientName;

    /**
     * This field is unnecessary for clients of type @ref ClientType::MASTER.
     * For clients of type @ref ClientType::SLAVE this field specifies whether the slave is
     * interested in the power state transition of all machines or only the machine where the slave
     * is running. For interest in all machines, this field should be assigned @ref ALL_MACHINES
     * and for local machine assign @ref LOCAL_MACHINE. For slaves, if this field is not provided,
     * then the local machine name will be used as the default.
     */
    std::string machineName = LOCAL_MACHINE;
};

/**
 * Defines the acknowledgements to TCU-activity states. The client process sends this after
 * processing the TcuActivityState notification, indicating that it is prepared for state transition
 *
 * Acknowledgement for TcuActivityState::RESUME is not required, as the state transition has already
 * happened.
 *
 * @deprecated  The API @ref ITcuActivityManager::sendActivityStateAck (TCUActivityStateAck) that
 *              uses this enum is deprecated. Instead, use
 *              @ref ITcuActivityManager::sendActivityStateAck (StateChangeResponse,
 *              TcuActivityState).
 */
enum class TcuActivityStateAck {
    SUSPEND_ACK,    /**< processed TcuActivityState::SUSPEND notification */
    SHUTDOWN_ACK,   /**< processed TcuActivityState::SHUTDOWN notification */
};

/** @} */ /* end_addtogroup telematics_power_manager */

}  // end of namespace power
}  // end of namespace telux

#endif  // TCUACTIVITYDEFINES_HPP
