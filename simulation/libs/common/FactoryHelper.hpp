/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef FACTORYHELPER_HPP
#define FACTORYHELPER_HPP

#include <memory>
#include <mutex>

#include "Logger.hpp"
#include "AsyncTaskQueue.hpp"
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace common {

/**
 * @brief The factory helper class can be used to create and initialize a manager using boiler
 * plate code that is usually used in all factories. You will need to have the factory class have
 * this class as the parent class.
 * Example usage:
 * class DataFactoryImpl : public telux::common::FactoryHelper {
 * ...
 * };
 */
class FactoryHelper {
 protected:
    template <typename T>
    std::shared_ptr<T> getManager(std::string type,
        std::weak_ptr<T> &weakManager,
        std::vector<telux::common::InitResponseCb> &callbacks,
        telux::common::InitResponseCb clientCallback,
        std::function<std::shared_ptr<T>(telux::common::InitResponseCb)> createAndInit) {
        std::shared_ptr<T> manager = nullptr;
        std::lock_guard<std::mutex> lock(factoryMutex_);
        manager = weakManager.lock();

        if (manager) {
            LOG(DEBUG, type, " found:", manager.get());
            telux::common::ServiceStatus status = manager->getServiceStatus();
            if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                // manager has failed initialization but callback is not called yet hence we still
                // have valid shared pointer.
                LOG(ERROR, __FUNCTION__, type,
                    " initialization failed for ", manager.get());
                return nullptr;
            } else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                LOG(DEBUG, __FUNCTION__, type,
                    " initialization was successful for ", manager.get());
                if (clientCallback) {
                    std::thread appCallback([clientCallback, status]() { clientCallback(status); });
                    appCallback.detach();
                }
            } else {
                LOG(DEBUG, __FUNCTION__, type,
                    " initialization in progress for ", manager.get());
                if (clientCallback) {
                    callbacks.push_back(clientCallback);
                }
            }
            return manager;
        } else {
            auto initCb
                = [this, type, &callbacks, &weakManager](telux::common::ServiceStatus status) {
                      LOG(DEBUG, type, ": initCb invoked for ", weakManager.lock().get(),
                          " with status: ", static_cast<int>(status));
                      // Application's initCb is expected to get either SERVICE_AVAILABLE or
                      // SERVICE_FAILED. Any other subsystem status should not be notified through
                      // initCb.
                      if((status != telux::common::ServiceStatus::SERVICE_FAILED)
                          && (status != telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
                          return;
                      }
                      if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                          std::lock_guard<std::mutex> lock(factoryMutex_);
                          LOG(DEBUG, type, ": init failed for ", weakManager.lock().get(),
                              ". Removing instance");
                          weakManager.reset();
                      }
                      LOG(DEBUG, type, ": invoking client callbacks (", callbacks.size(),
                          ") for ", weakManager.lock().get(),
                          " with status: ", static_cast<int>(status));
                      initCompleteNotifier(type, callbacks, status);
                  };
            try {
                manager = createAndInit(initCb);
                LOG(DEBUG, "New ", type, " created ", manager.get());
                if (manager == nullptr) {
                    LOG(ERROR, type, " failed to initialize for ", manager.get());
                    return manager;
                }
            } catch (std::bad_alloc &e) {
                LOG(ERROR, __FUNCTION__, type,
                    " failed to create with exception: ", e.what());
                return nullptr;
            }
            weakManager = manager;
            if (clientCallback) {
                callbacks.push_back(clientCallback);
            }
            return manager;
        }
    }

    /**
     * @brief Generic method to get a manager instance for managers that do NOT take an InitResponseCb
     * for their initialization. Their init() is usually blocking and returns a Status directly.
     *
     * @tparam T The type of the manager interface (e.g., IApInterfaceManager, IStaInterfaceManager).
     * @param type A string describing the manager type for logging.
     * @param weakManager A weak pointer to the manager instance, used to store and retrieve
     *                    the singleton instance.
     * @param createAndInit A function that creates and initializes a new manager instance.
     *                      This function should take no parameters and return the manager.
     * @return A shared pointer to the manager instance, or nullptr if creation failed.
     */
    template <typename T>
    std::shared_ptr<T> getManager(std::string type,
        std::weak_ptr<T> &weakManager,
        std::function<std::shared_ptr<T>()> createAndInit) { // No InitResponseCb parameters
        std::shared_ptr<T> manager = nullptr;
        std::lock_guard<std::mutex> lock(factoryMutex_);
        manager = weakManager.lock();

        if (manager) {
            LOG(DEBUG, type, " found:", manager.get());
            // Assume if manager exists, it's either available or will become available.
            // For managers that don't use InitResponseCb, their init() is usually blocking
            // or handles its own internal state.
            return manager;
        } else {
            try {
                manager = createAndInit(); // Call createAndInit without a callback
                LOG(DEBUG, "New ", type, " created ", manager.get());
                if (manager == nullptr) {
                    LOG(ERROR, type, " failed to initialize for ", manager.get());
                    return nullptr;
                }
            } catch (std::bad_alloc &e) {
                LOG(ERROR, __FUNCTION__, type,
                    " failed to create with exception: ", e.what());
                return nullptr;
            }
            weakManager = manager;
            return manager;
        }
    }

    /**
     * Adding an overloaded API to synchronize cleanup and initSync via taskQ.
     */
    template <typename T>
    std::shared_ptr<T> getManager(std::string type,
        std::weak_ptr<T> &weakManager,
        std::vector<telux::common::InitResponseCb> &callbacks,
        telux::common::InitResponseCb clientCallback,
        std::function<std::shared_ptr<T>(telux::common::InitResponseCb,
        std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ)> createAndInit,
        std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ) {
        std::shared_ptr<T> manager = nullptr;
        std::lock_guard<std::mutex> lock(factoryMutex_);
        manager = weakManager.lock();

        if (manager) {
            LOG(DEBUG, type, " found:", manager.get());
            telux::common::ServiceStatus status = manager->getServiceStatus();
            if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                // manager has failed initialization but callback is not called yet hence we still
                // have valid shared pointer.
                LOG(ERROR, __FUNCTION__, type,
                    " initialization failed for ", manager.get());
                return nullptr;
            } else if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                LOG(DEBUG, __FUNCTION__, type,
                    " initialization was successful for ", manager.get());
                if (clientCallback) {
                    std::thread appCallback([clientCallback, status]() { clientCallback(status); });
                    appCallback.detach();
                }
            } else {
                LOG(DEBUG, __FUNCTION__, type,
                    " initialization in progress for ", manager.get());
                if (clientCallback) {
                    callbacks.push_back(clientCallback);
                }
            }
            return manager;
        } else {
            auto initCb
                = [this, type, &callbacks, &weakManager](telux::common::ServiceStatus status) {
                      LOG(DEBUG, type, ": initCb invoked for ", weakManager.lock().get(),
                          " with status: ", static_cast<int>(status));
                      // Application's initCb is expected to get either SERVICE_AVAILABLE or
                      // SERVICE_FAILED. Any other subsystem status should not be notified through
                      // initCb.
                      if((status != telux::common::ServiceStatus::SERVICE_FAILED)
                          && (status != telux::common::ServiceStatus::SERVICE_AVAILABLE)) {
                          return;
                      }
                      if (status == telux::common::ServiceStatus::SERVICE_FAILED) {
                          std::lock_guard<std::mutex> lock(factoryMutex_);
                          LOG(DEBUG, type, ": init failed for ", weakManager.lock().get(),
                              ". Removing instance");
                          weakManager.reset();
                      }
                      LOG(DEBUG, type, ": invoking client callbacks (", callbacks.size(),
                          ") for ", weakManager.lock().get(),
                          " with status: ", static_cast<int>(status));
                      initCompleteNotifier(type, callbacks, status);
                  };
            try {
                manager = createAndInit(initCb, taskQ);
                LOG(DEBUG, "New ", type, " created ", manager.get());
                if (manager == nullptr) {
                    LOG(ERROR, type, " failed to initialize for ", manager.get());
                    return manager;
                }
            } catch (std::bad_alloc &e) {
                LOG(ERROR, __FUNCTION__, type,
                    " failed to create with exception: ", e.what());
                return nullptr;
            }
            weakManager = manager;
            if (clientCallback) {
                callbacks.push_back(clientCallback);
            }
            return manager;
        }
    }

    void initCompleteNotifier(std::string type, std::vector<telux::common::InitResponseCb> &initCbs,
        telux::common::ServiceStatus status) {
        LOG(DEBUG, __FUNCTION__, ": ", type,
            ": Invoking client callbacks with status: ", static_cast<int>(status),
            " on callbacks: ", &initCbs);
        std::vector<telux::common::InitResponseCb> Callbacks;
        {
            std::lock_guard<std::mutex> lock(factoryMutex_);
            Callbacks = initCbs;
            initCbs.clear();
        }
        for (auto &callback : Callbacks) {
            callback(status);
        }
    }

    /**
     * Utility method to perform cleanup operation on the provided Manager object(typically of a
     * Manager's wrapper class).
     * The provided Manager class must have a cleanup() method.
     */
    template <typename T>
    void cleanup(T *impl) {
        impl->cleanup();
        delete impl;
    }

 private:
    std::mutex factoryMutex_;
};

}  // namespace common
}  // namespace telux

#endif  // #ifndef FACTORYHELPER_HPP
