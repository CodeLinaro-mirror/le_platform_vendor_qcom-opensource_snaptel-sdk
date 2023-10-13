/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
 * /

/**
 * @file      DiagLogManager.hpp
 *
 * @brief     DiagLogManager provides APIs to configure, start and stop collecting diagnostics logs.
 *            The interface offers two methods for log collections; file and callback methods.
 *            in file method, all logs are stored in file. In Callback method, each log entry is
 *            delivered to client provided callback.
 *            Qshrink Database from the modem build must be available to decode qshrink4 format F3
 *            logs provided in callback API.
 *            The interface also offers three log collection modes for each log collection method;
 *            streaming, threshold and circular modes.
 *            Client can choose to enable log collection on either device level (MDM or EAP) or on
 *            peripheral level (such as Modem DSP, CDSP, etc.).
 *            Device level logging includes logs from all supported peripherals on the device.
 *            On platforms where hypervisor is supported, logs collected on MDM device level will
 *            also include logs from guest VMs.
 *            Multi client is not supported and single client trying to do concurrent file and
 *            callback logging is also not supported. Client running on MDM can collect logs
 *            from selected MDM peripherals or MDM device level. Client running on EAP can collect
 *            logs on device level from both MDM and EAP and can collect logs from selected EAP
 *            peripherals.
 *            Once logging is started, subsequent attempt to start logging will return
 *            @ref telux::common::ErrorCode::NO_EFFECT.
 *            Attempt to call @ref telux::platform::diag::IDiagLogManager::setConfig while logging
 *            has already started will return @ref telux::common::ErrorCode::INVALID_STATE error.
 */

#ifndef TELUX_PLATFORM_DIAG_DIAGLOGMANAGER_HPP
#define TELUX_PLATFORM_DIAG_DIAGLOGMANAGER_HPP

#include <future>
#include <memory>
#include <stdint.h>

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace platform {
namespace diag {

/** @addtogroup telematics_diagnostics
 * @{ */

#define MAX_DIAG_FILE_SIZE_MB    100
#define MAX_NUM_DIAG_FILES       100
//Forward declarations
class IDiagListener;

/**
 * Specifies the mode of logging to be used (Streaming, threshold, circular buffer)
 */
enum class DiagLogMode {
    STREAMING = 0,      /**< Logs are flushed immediately from the buffer when available. Logs are
                             saved to a file (in file method) or passed to the client in real time
                             through listener @ref IDiagListener::onAvailableLogs()
                             (in callback method) */
    THRESHOLD,          /**< Can be used to conserve power. Logs are flushed out when the buffer is
                             full. This is only applicable to peripherals with its own buffer such
                             as Modem DSP. It is not recommended to enable too many
                             logs in logmask file passed to @ref telux::platform::diag::setConfig
                             API. Too many logs will cause frequent processor wake ups and
                             consequently result in the threshold being crossed and logs being
                             flushed frequently resembling streaming mode. */
    CIRCULAR_BUFFER,    /**< Can be used to conserve power. Logs are continuously written to a
                             buffer until client triggers buffer drain command to collect the
                             logs.
                             This is only applicable to peripherals with its own buffer such
                             as Modem DSP. Old logs are overwritten when buffer is
                             full. Logs in the buffer will be flushed only upon client's request. */
};

/**
 * Enables log collection from selected device(s) which includes logs from Integrated AP and
 * all peripherals on the device.
 * Note: If device logging is enabled, peripheral logging must be disabled.
 */
enum DeviceType {
    DIAG_DEVICE_NONE = 0,                  /**< Device logging is disabled. Only peripheral logging
                                                on device application is running on is enabled */
    DIAG_DEVICE_EXTERNAL_AP = 1 << 0,      /**< Log collection from External Application Processor
                                                is enabled */
    DIAG_DEVICE_MDM = 1 << 1,              /**< Log collection from MDM is enabled */
};

/* This is a bitmask which takes the devices from
 * @ref telux::platform::diag::DeviceType.
 */
using Devices = uint8_t;

/**
 * Enables log collection from selected peripheral (if it exists) and/or application processor in
 * the device where the client of the API is running.
 * Note: If peripheral logging is enabled, device logging must be disabled.
 */
enum PeripheralType {
    DIAG_PERIPHERAL_NONE = 0,                /**< Disable peripherals log collection           */
    DIAG_PERIPHERAL_INTEGRATED_AP = 1 << 0,  /**< Enable integrated AP log collection.
                                                  On platforms where hypervisor is present,
                                                  this indicates logs from PVM                 */
    DIAG_PERIPHERAL_MODEM_DSP = 1 << 1,      /**< Enable modem DSP log collection              */
    DIAG_PERIPHERAL_SVM = 1 << 2,            /**< Enable log collection on all SVMs            */
    DIAG_PERIPHERAL_LPASS = 1 << 3,          /**< Enable LPASS log collection                  */
    DIAG_PERIPHERAL_CDSP = 1 << 4,           /**< Enable CDSP log collection                   */
};

/**
 * This is a bitmask which takes the peripherals from
 * @ref telux::platform::diag::PeripheralType using Peripheral.
 */
using Peripherals = uint8_t;

/**
 * Diagnostic logging methods.
 * Only one logging method can be selected
 */
enum class LogMethod{
    NONE = 0,                 /**< No valid logging method */
    FILE,                     /**< File logging method. Collected Logs will be saved to file located
                              under platform.diag.diag_output_log_path key in tel.conf file or under /tmp/diag if
                              such key does not exist  */

    CALLBACK,                 /**< Callback logging method. Collected logs are provided to clients
                                   via @ref telux::platform::diag::IDiagListener::onAvailableLogs */
};

/**
 * Diagnostic log source.
 * When device level logging is selected from @ref telux::platform::diag::Device and logs from all
 * peripherals in such device will be collected.
 * When peripheral logging is selected from @ref telux::platform::diag::Peripheral, logs from
 * selected peripherals will be collected.
 */
enum class SourceType {
    NONE = 0,                        /**< No valid logging source                                 */
    DEVICE,                          /**< Device level logging source.                            */
    PERIPHERAL,                      /**< Peripheral level logging source                         */
};

/**
 * Current diagnostic status
 */
struct DiagStatus{
    LogMethod logMethod;             /**< Current successfully configured logging method          */
    bool isLoggingInProgress;        /**< True: If logging has already started. False otherwise   */
    bool isLogDrainInProgress;       /**< True: If log drain has already started. False otherwise */
};

/**
 * Configure device(s) or peripheral(s) from which logs to be collected.
 * Logging source can be either device level or peripheral level.
 */
union SourceInfo {
    Devices device = DeviceType::DIAG_DEVICE_NONE; /**< @ref telux::platform::diag::Device */
    Peripherals peripheral;
                                                   /**< @ref telux::platform::diag::Peripheral */
};

/**
 * @brief Represents the config relevant to File method.
 *        Logs are saved to a file under the directory specified by
 *        platform.diag.diag_output_log_path in tel.conf file. if Key does not exist, log will be
 *        stored in default location /tmp/diag
 */
struct FileMethodConfig {
    uint32_t maxSize;                   /**< Optional. Maximum file size in MB after which it will
                                             create a new file. The maximum size should not exceed
                                             MAX_DIAG_FILE_SIZE_MB. */
    uint32_t maxNumber;                 /**< Optional. Maximum number of log files. Files are
                                             replaced once this number is reached. The maximum value
                                             should not exceed MAX_NUM_DIAG_FILES */
};

struct DiagConfig {
    SourceType srcType;                /**< @ref telux::platform::diag::SourceType                */
    SourceInfo srcInfo;                /**< @ref telux::platform::diag::SourceInfo. Based on the
                                            source type selected in DiagConfig.srcType, the
                                            corresponding field in DiagConfig.srcInfo would be
                                            valid.                                                */
    std::string mdmLogMaskFile = "";   /**< Optional - Full path to file that contains the MDM mask
                                            to filter logs. It is generated using QxDM and can be
                                            either cfg or cfg2 format. Needed only if logs are
                                            generated from MDM device or MDM peripherals */
    std::string eapLogMaskFile = "";   /**< Optional - Full path to file that contains the EAP mask
                                            to filter logs. It is generated using QxDM and can be
                                            either cfg or cfg2 format. Needed only if logs are
                                            generated from EAP device or EAP peripherals */
    DiagLogMode modeType = DiagLogMode::STREAMING;   /**< @ref telux::platform::diag::DiagLogMode */
    LogMethod method;                       /**< @ref telux::platform::diag::LogMethod            */
    union MethodConfig {                    /**< Configuration of selected logging method         */
        FileMethodConfig fileConfig;        /**< Configuration specific to file logging method    */
    } methodConfig;
};

/**
 *@brief IDiagLogManager is a primary interface for Diagnostics.
 *       The interface offers APIs to configure log collection method (File or Callback) and
 *       configure log mode for selected method (streaming, threshold, or circular).
 *       The interface also provides APIs to start and stop diagnostics log collection in
 *       configured method and mode.
 */


class IDiagLogManager {
 public:
    /**
     * Returns the current status of logging subsystem.
     *
     * @returns Status of logging subsystem @ref telux::platform::diag::DiagStatus
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual DiagStatus getStatus() = 0;

    /**
     * Register a listener for specific events in the Diag log Manager like availability of logs,
     * checking for unexpected error.
     *
     * @param [in] listener         Pointer of IDiagListener object that processes the notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IDiagListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener         Pointer of IDiagListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IDiagListener> listener) = 0;

    /**
     * Sets the logging configurations.
     *
     * This API must be called when logging is not in progress. Calling this API while logging has
     * already started will return @ref telux::common::ErrorCode::INVALID_STATE error.
     * If error is returned, clients need to invoke the API again with the correct parameters.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @param [in] config           @ref telux::platform::diag::DiagConfig
     *
     * @returns                     Return code for whether the operation succeeded or failed
     *                              @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode setConfig(const DiagConfig config) = 0;

    /**
     * Get current configuration settings
     *
     * @returns current log configuration @ref telux::platform::diag::DiagConfig.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual DiagConfig getConfig() = 0;

    /**
     * Starts draining the logs from the circular buffer.
     * Until @ref telux::platform::diag::stopDrainingLogsLogs is called, logs will continue to be
     * written to circular buffer, getting flushed and written to log file or provided to client
     * via @ref telux::platform::diag::IDiagListener::onAvailableLogs based on selected logging
     * method @ref telux::platform::diag::LogMethod. If this API is called while logging has
     * not started, @ref telux::common::ErrorCode::INVALID_STATE error is returned.
     * Attempt to call this API while draining log is already in progress, will not affect the state
     * of logging and will return @ref telux::common::ErrorCode::NO_EFFECT error.
     *
     * This API should be used only in circular buffering mode.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @returns                     Return code for whether the operation succeeded or failed
     *                              @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode startDrainingLogs() = 0;

    /**
     * Stops the draining of logs and continues collecting logs in circular buffer mode.
     * This API should be used only in circular buffering mode.
     * If this API is called while logging has not started
     * @ref telux::common::ErrorCode::INVALID_STATE error is returned.
     * Attempt to call this API while draining log is not in progress, will not affect the
     * state of logging and will return @ref telux::common::ErrorCode::NO_EFFECT error.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @returns                     Return code for whether the operation succeeded or failed
     *                              @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode stopDrainingLogs() = 0;

    /**
     * Start the Log collection session
     * This API starts the log collection. It is expected the configuration to be set successfully
     * via @ref telux::platform::diag::IDiagLogManager::setConfig before calling this API.
     * Calling this API after logging has already started will not affect logging state and will
     * return @ref telux::common::ErrorCode::NO_EFFECT error.
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @returns                     Return code for whether the operation succeeded or failed
     *                              @ref telux::common::ErrorCode
     *                              Error is returned if configuration is not set correctly.
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode startLogCollection() = 0;

    /**
     * Stop the Log collection session
     *
     * On platforms with Access control enabled, Caller needs to have TELUX_DIAG_OPS permission
     * to invoke this API successfully.
     *
     * @returns                     Return code for whether the operation succeeded or failed
     *                              @ref telux::common::ErrorCode
     *
     * @note   Eval: This is a new API and is being evaluated. It is subject to change and could
     *         break backwards compatibility.
     */
    virtual telux::common::ErrorCode stopLogCollection() = 0;

    /**
     * Destructor for IDiagLogManager
     */
    virtual ~IDiagLogManager(){};
}; // end of IDiagLogManager

/**
 * Interface for Diag listener object. Client needs to implement this interface to get
 * access to diag log service notifications like onAvailableLogs, onError.
 *
 * The methods in listener can be invoked from multiple different threads. The implementation
 * should be thread safe.
 *
 * The notification delivery mechanism uses the same thread to deliver all the queued notifications
 * to ensure they are delivered in order.
 * Considering this, the thread on which the notifications are delivered should not be blocked for
 * longer operations since this would result in delay in delivery of further notifications that are
 * in the queue waiting to be dispatched.
 *
 */
class IDiagListener {
 public:
    /**
     * Invoked when logs are available in callback method to when they are available.
     *
     * @param [in] ptr                  Pointer to the log data
     * @param [in] len                  Length of the data
     *
     */
    virtual void onAvailableLogs(uint8_t* ptr, int len){};

    /**
     * Destructor for IDiagListener
     */
    virtual ~IDiagListener(){};
}; // end of IDiagListener

/** @} */ /* end_addtogroup telematics_diagnostics */
} // end of namespace diag
} // end of namespace platform
} // end of namespace telux

#endif // TELUX_PLATFORM_DIAG_DIAGLOGMANAGER_HPP
