/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

 /**
 * @file       DataLinkManager.hpp
 *
 * @brief      Data Link Manager class provides the interface to data communication links.
 */

#ifndef TELUX_DATA_DATALINKMANAGER_HPP
#define TELUX_DATA_DATALINKMANAGER_HPP

#include <telux/data/DataDefines.hpp>
#include <memory>
#include <future>

namespace telux {
namespace data {

/** @addtogroup telematics_data
 * @{ */

// Forward declarations
class IDataLinkListener;

//Represents the current status of a data communication link.
struct LinkStatusInfo
{
    InterfaceType ifaceType;   /**< Type of network interface  */
    std::string   ifaceName;   /**< Name of the network interface */
    LinkStatus    status;      /**< Current link status */
};

/**
 * @brief The Data Link Manager class provides APIs related to data communication links.
 */
class IDataLinkManager {
public:
    /**
     * Checks if the data subsystem is ready.
     *
     * @returns True if DataLink Manager is ready for service, otherwise
     * returns false.
     *
     */
    virtual bool isSubsystemReady() = 0;

    /**
     * Wait for data subsystem to be ready.
     *
     * @returns A future that caller can wait on to be notified
     * when DataLink manager is ready.
     *
     */
    virtual std::future<bool> onSubsystemReady() = 0;

    /**
     * Registers with the Data Link Manager as a listener for service statuses and other events.
     *
     * @param [in] listener    Pointer to the IDataLinkListener object that processes the
     *                         notification
     *
     * @returns Status of registerListener success or suitable status code
     *
     */
    virtual telux::common::Status registerListener(std::weak_ptr<IDataLinkListener> listener) = 0;

    /**
     * Removes a previously added listener.
     *
     * @param [in] listener    Pointer to the IDataLinkListener object that needs to be removed
     *
     * @returns Status of deregisterListener success or suitable status code
     *
     */
    virtual telux::common::Status deregisterListener(std::weak_ptr<IDataLinkListener> listener) = 0;

    /**
     * Destructor of IDataLinkManager
     */
    virtual ~IDataLinkManager() {};
};

/**
 * Interface for the Data Link listener object. Client needs to implement this interface to be
 * notified of data link service notifications like onServiceStatusChange, etc.
 *
 * The listener methods can be invoked from multiple threads. The implementation should be thread
 * safe.
 */
class IDataLinkListener : public common::IServiceStatusListener {
public:

    /**
     * Called when the status of a data link changes.
     *
     * @param [in] info  @ref LinkStatusInfo
     *
     * @note     Eval: This is a new API and is being evaluated. It is subject to change and could
     *           break backwards compatibility.
     */
    virtual void onLinkStatusChange(const telux::data::LinkStatusInfo& info) {}

    /**
     * Destructor for IDataLinkListener
     */
    virtual ~IDataLinkListener() {}
};

/** @} */ /* end_addtogroup telematics_data */
} // namespace data
} // namespace telux

#endif //TELUX_DATA_DATALINKMANAGER_HPP