/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * ListenerManager class to register and deregister listeners of specific events
 */
#ifndef LISTENERMANAGER_HPP
#define LISTENERMANAGER_HPP
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <bitset>
#include <set>
#include <map>

#include <telux/common/CommonDefines.hpp>
#include "Logger.hpp"
#include "../common/event-manager/EventManager.hpp"

namespace telux {

namespace common {

template <typename T>
class ListenerManager {
public:

ListenerManager() {
    LOG(DEBUG, __FUNCTION__);
}

~ListenerManager() {
    LOG(DEBUG, __FUNCTION__);
}

telux::common::Status registerListener(std::weak_ptr<T> listener) {
   auto sp = listener.lock();

   if(sp == nullptr) {
      LOG(ERROR, "Null listener");
      return telux::common::Status::INVALIDPARAM;
   }

   std::lock_guard<std::mutex> lock(listenerMutex_);
   // Check whether the listener existed ...
   auto itr = std::find_if(
      std::begin(listeners_), std::end(listeners_),
      [=](std::weak_ptr<T> listenerExisted) { return (listenerExisted.lock() == sp); });
   if(itr != std::end(listeners_)) {
      LOG(DEBUG, "registerListener() - listener already exists");
      return telux::common::Status::ALREADY;
   }
   LOG(DEBUG, "registerListener() - creates a new listener entry");
   listeners_.emplace_back(listener);  // store listener

   return telux::common::Status::SUCCESS;
}

telux::common::Status deRegisterListener(std::weak_ptr<T> listener) {
   bool listenerExisted = false;
   std::lock_guard<std::mutex> lock(listenerMutex_);
   for(auto it = listeners_.begin(); it != listeners_.end();) {
      auto sp = (*it).lock();
      if(!sp) {
         LOG(DEBUG, "Erasing obsolete weak pointer from Listener");
         it = listeners_.erase(it);
      } else if(sp == listener.lock()) {
         it = listeners_.erase(it);
         LOG(DEBUG, "removeListener success");
         listenerExisted = true;
      } else {
         ++it;
      }
   }
   if(listenerExisted) {
      return telux::common::Status::SUCCESS;
   } else {
      LOG(WARNING, "QmiClient removeListener: listener not found");
      return telux::common::Status::NOSUCH;
   }
}

void getAvailableListeners(
   std::vector<std::weak_ptr<T>> &availableListeners) {
   // Entering critical section, copy lockable shared_ptr from global listener
   std::lock_guard<std::mutex> lock(listenerMutex_);
   for(auto it = listeners_.begin(); it != listeners_.end();) {
      auto sp = (*it).lock();
      if(sp) {
         availableListeners.emplace_back(sp);
         ++it;
      } else {
         // if we unable to lock the listener, we should remove it from
         // listenerList
         LOG(DEBUG, "erased obsolete weak pointer from listeners");
         it = listeners_.erase(it);
      }
   }
}

private:

std::mutex listenerMutex_;
std::vector<std::weak_ptr<T>> listeners_;

};
}// common
} // telux
#endif