/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file SubsystemManager.hpp
 * @brief Provides ability to monitor operational status of the various subsystems,
 *        trigger graceful MPSS restart and to manage EDL (Emergency Download) configuration
 *        and lifecycle.
 */

#ifndef TELUX_PLATFORM_SUBSYSTEMMANAGER_HPP
#define TELUX_PLATFORM_SUBSYSTEMMANAGER_HPP

#include <memory>
#include <string>
#include <vector>

#include <telux/common/SDKListener.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace platform {

/** @addtogroup telematics_platform
 * @{ */

/**
 * EDL (Emergency Download) configurable parameters.
 */
struct EdlConfigs {
    std::string imagePath; /**< Directory containing the flashing payload. */
    std::string imageRawProgramXml; /**< Rawprogram XML file describing the partition
                                         layout and write operations. */
    std::string imagePatchXml; /**< Patch XML file containing post-write adjustments,
                                    sparse region definitions, or additional patch
                                    steps to apply after the main flashing pass. */
};

/**
 * Represents the current phase of an EDL operation.
 */
enum class EdlState {
    IDLE = 0, /**< No EDL operation is currently running. */
    MOVING_TO_EDL, /**< An attempt to trigger EDL on the target device is
                        in progress. */
    MOVED_TO_EDL, /**< The target device has successfully entered EDL and
                       is ready for the flashing operation to begin. */
    FLASHING, /**< An EDL image flashing operation is currently in
                   progress. */
    FLASHED /**< The EDL flashing operation completed successfully. */
};

/**
 * Interface for Subsystem listener object. Client needs to implement this interface to get
 * notifications like onStateChange, as well as EDL-related notifications.
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
     * Invoked when any client successfully updates the system-wide EDL configuration
     * by calling @ref ISubsystemManager::setEdlConfigurations().
     *
     * All registered listeners receive this notification regardless of which client
     * made the change.
     *
     * @param[in] edlConfigs The updated EDL configuration parameters now in effect.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual void onEdlConfigUpdate(const EdlConfigs &edlConfigs) {
    }

    /**
     * Invoked each time the EDL operation advances to a new phase during the
     * lifecycle. Notifications are delivered asynchronously after
     * @ref ISubsystemManager::triggerEdl() has been accepted.
     *
     * This callback covers the progressive state transitions:
     * @ref EdlState::IDLE, @ref EdlState::MOVING_TO_EDL,
     * @ref EdlState::MOVED_TO_EDL, @ref EdlState::FLASHING and
     * @ref EdlState::FLASHED.
     *
     * The final outcome of the EDL operation is separately reported via
     * @ref onEdlOperationResult().
     *
     * @param[in] edlState The EDL phase that has just been entered.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual void onEdlStateChanged(EdlState edlState) {
    }

    /**
     * Invoked once when the EDL operation concludes, reporting the final outcome.
     * Notifications are delivered asynchronously after
     * @ref ISubsystemManager::triggerEdl() has been accepted.
     *
     * A result of true indicates the EDL and flashing operations completed
     * successfully. A result of false indicates a failure at any stage of the
     * EDL lifecycle. The last state reported via @ref onEdlStateChanged() can
     * be used to determine at which stage the failure occurred.
     *
     * @param[in] success True if the EDL operation completed successfully,
     *                    false otherwise.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual void onEdlOperationResult(bool success) {
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
 * In addition to the subsystem monitoring, this interface provides the ability
 * to configure and initiate EDL operations, and to track EDL progress
 * through asynchronous notifications.
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
     *          otherwise, an appropriate error code, for example:
     *          - INVALID_ARG if a requested subsystem cannot be monitored from the
     *            specified location.
     *
     * @note To receive EDL notifications (@ref ISubsystemListener::onEdlConfigUpdate,
     *       @ref ISubsystemListener::onEdlStateChanged,
     *       @ref ISubsystemListener::onEdlOperationResult), register interest in
     *       @ref telux::common::Subsystem::APSS with location set to
     *       @ref telux::common::ProcType::REMOTE_PROC.
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
     *          or if the listener was not registered, otherwise, an appropriate error code.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode deRegisterListener(std::weak_ptr<ISubsystemListener> listener)
        = 0;

    /**
     * Sets the system-wide EDL configuration parameters. The configuration is
     * global and persists across boot cycles.
     *
     * If multiple clients call this API the last write takes effect. All registered
     * listeners receive an @ref ISubsystemListener::onEdlConfigUpdate() notification
     * when the configuration is stored successfully.
     *
     * On platforms with Access control enabled, caller needs to have
     * TELUX_PLATFORM_SUBSYS_EDL_CTRL permission to invoke this API successfully.
     *
     * @param[in] edlConfigs The EDL configuration parameters to apply.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the configuration was
     *          accepted and stored successfully, otherwise an appropriate error code,
     *          for example:
     *          - ACCESS_DENIED if the caller lacks the required permission.
     *          - SUBSYSTEM_UNAVAILABLE if the service is not yet ready.
     *          - OP_IN_PROGRESS if another call to this API is already in progress.
     *          - INTERNAL_ERR or GENERIC_FAILURE for other failures.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode setEdlConfigurations(const EdlConfigs &edlConfigs) = 0;

    /**
     * Retrieves the current system-wide EDL configuration parameters that were
     * last set via @ref setEdlConfigurations().
     *
     * On platforms with Access control enabled, caller needs to have
     * TELUX_PLATFORM_SUBSYS_EDL_CTRL permission to invoke this API successfully.
     *
     * @param[out] edlConfigs Populated with the current EDL configuration on success.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the configuration was
     *          retrieved successfully, otherwise an appropriate error code, for example:
     *          - ACCESS_DENIED if the caller lacks the required permission.
     *          - SUBSYSTEM_UNAVAILABLE if the service is not yet ready.
     *          - OP_IN_PROGRESS if another call to this API is already in progress.
     *          - INTERNAL_ERR or GENERIC_FAILURE for other failures.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getEdlConfigurations(EdlConfigs &edlConfigs) = 0;

    /**
     * Initiates an EDL operation. The request returns immediately and the operation
     * proceeds asynchronously.
     *
     * Once triggered, the EDL lifecycle transitions from @ref EdlState::IDLE to
     * @ref EdlState::MOVING_TO_EDL. If the target device successfully enters EDL mode,
     * the state advances to @ref EdlState::MOVED_TO_EDL.
     *
     * On reaching @ref EdlState::MOVED_TO_EDL, the image write begins and the state
     * advances to @ref EdlState::FLASHING. If the flashing operation completes
     * successfully the state transitions to @ref EdlState::FLASHED and the lifecycle
     * concludes at @ref EdlState::IDLE.
     *
     * Phase transitions are reported through @ref ISubsystemListener::onEdlStateChanged()
     * notifications delivered to all registered listeners. The final outcome is
     * reported through @ref ISubsystemListener::onEdlOperationResult(). A valid EDL
     * configuration must have been set via @ref setEdlConfigurations() before calling
     * this API, otherwise the request will be rejected.
     *
     * On platforms with Access control enabled, caller needs to have
     * TELUX_PLATFORM_SUBSYS_EDL_CTRL permission to invoke this API successfully.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the EDL request was accepted
     *          and queued successfully, otherwise an appropriate error code, for example:
     *          - ACCESS_DENIED if the caller lacks the required permission.
     *          - SUBSYSTEM_UNAVAILABLE if the service is not yet ready.
     *          - OP_IN_PROGRESS if an EDL operation is already in progress.
     *          - INTERNAL_ERR or GENERIC_FAILURE for other failures.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode triggerEdl() = 0;

    /**
     * Retrieves the current phase of the EDL operation. This API may be polled at
     * any time.
     *
     * For event-driven updates, register a listener and handle
     * @ref ISubsystemListener::onEdlStateChanged() and
     * @ref ISubsystemListener::onEdlOperationResult() notifications instead.
     *
     * @param[out] edlState Populated with the current @ref EdlState value on success.
     *
     * @returns @ref telux::common::ErrorCode::SUCCESS if the state was retrieved
     *          successfully, otherwise an appropriate error code, for example:
     *          - SUBSYSTEM_UNAVAILABLE if the service is not yet ready.
     *          - OP_IN_PROGRESS if another call to this API is already in progress.
     *          - INTERNAL_ERR or GENERIC_FAILURE for other failures.
     *
     * @note Eval: This is a new API and is being evaluated. It is subject
     *             to change and could break backwards compatibility.
     */
    virtual telux::common::ErrorCode getEdlState(EdlState &edlState) = 0;

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
