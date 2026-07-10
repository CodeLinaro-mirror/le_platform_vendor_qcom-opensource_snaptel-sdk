/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file SubsystemManager.hpp
 * @brief Provides ability to monitor operational status of the various subsystems
 * and trigger graceful MPSS restart.
 */

#ifndef TELUX_PLATFORM_SUBSYSTEMMANAGER_HPP
#define TELUX_PLATFORM_SUBSYSTEMMANAGER_HPP

#include <memory>
#include <vector>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace platform {

/** @addtogroup telematics_platform
 * @{ */

/**
 * Interface for Subsystem listener object. Client needs to implement this interface to get
 * notifications like onStateChange.
 *
 */
class ISubsystemListener : public telux::common::ISDKListener {
 public:
    /**
     * API to receive notification whenever a subsystem's operational state is changed.
     * Provides latest state of the subsystem.
     *
     * @param[in] subsystemInfo Subsystem whose state has changed
     * @param[in] newOperationalStatus New functional state
     */
    virtual void onStateChange(telux::common::SubsystemInfo subsystemInfo,
        telux::common::OperationalStatus newOperationalStatus) {
    }

    /**
     * Destructor for ISubsystemListener.
     */
    virtual ~ISubsystemListener() {
    }
};

/**
 * This function is called as a response to
 * @ref telux::platform::ISubsystemManager::triggerMpssRestart API.
 *
 * @param[in] error - Return code which indicates whether the operation succeeded
 *                    or not.
 */
using MpssRestartResponseCb = std::function<void(telux::common::ErrorCode error)>;

/**
 * ISubsystemManager is used to monitor operational status of the various subsystems.
 *
 * Consider a fusion architecture where an external application processor (EAP) is
 * connected to the MDM SoC via some interconnect (e.g. USB, PCIe or Ethernet).
 * A client running on the EAP can use ISubsystemManager to monitor the state of the
 * MDM's subsystems and be notified when the MDM, or one of its subsystems, crashes
 * or shuts down.
 *
 * Similarly, in standalone architecture, an application running on the MDM SoC can
 * monitor state of the MDM's subsystems.
 *
 * ISubsystemManager can be used by clients running on an External Application Processor (EAP)
 * or from the integrated application processor on the MDM to trigger the MPSS restart.
 *
 */
class ISubsystemManager {
 public:
    /**
     * Registers the given listener to receive subsystem related notifications.
     *
     * @param[in] listener Receives notifications
     * @param[in] subsystems List of subsystems to monitor
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the listener is registered,
     *          otherwise, an appropriate error code
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode registerListener(std::weak_ptr<ISubsystemListener> listener,
        std::vector<telux::common::SubsystemInfo> subsystems)
        = 0;

    /**
     * Deregisters the given listener registered previously with @ref registerListener().
     *
     * @param[in] listener Listener to deregister
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the listener is deregistered,
     *          otherwise, an appropriate error code
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deRegisterListener(std::weak_ptr<ISubsystemListener> listener)
        = 0;

    /**
     * Gets the subsystem service status.
     *
     * @returns @ref telux::common::ServiceStatus::SERVICE_AVAILABLE if the service is ready
     *          for use, @ref telux::common::ServiceStatus::SERVICE_UNAVAILABLE if the service
     *          is temporarily unavailable (possibly undergoing initialization),
     *          @ref telux::common::ServiceStatus::SERVICE_FAILED if the service needs
     *          re-initialization
     */
    virtual telux::common::ServiceStatus getServiceStatus() = 0;

    /**
     * This API is used to trigger the graceful modem DSP restart from APSS/EAP.
     *
     * On platforms with Access control enabled, caller needs to have
     * TELUX_PLATFORM_SUBSYS_RESTART_CTRL permission to invoke this API successfully.
     *
     * @param [in] cb - Callback to get the actual result of the restart operation.
     *                  The callback will be invoked once the restart operation completes
     *                  or fails, with an appropriate error code.
     *
     * @returns @ref telux::common::Status::SUCCESS if the request to trigger graceful restart
     *          is successful, otherwise, an appropriate status.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::Status triggerMpssRestart(MpssRestartResponseCb cb) = 0;

    /**
     * Performs cleanup and destroys the ISubsystemManager instance.
     */
    virtual ~ISubsystemManager(){};
};

/** @} */ /* end_addtogroup telematics_platform */

}  // End of namespace platform
}  // End of namespace telux

#endif  // TELUX_PLATFORM_SUBSYSTEMMANAGER_HPP
