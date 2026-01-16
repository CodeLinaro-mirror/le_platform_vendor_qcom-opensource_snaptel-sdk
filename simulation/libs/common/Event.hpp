/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef EVENT_HPP
#define EVENT_HPP

#include <string>

namespace telux {

namespace common {

class Event {
 public:
    Event(uint32_t id, std::string name, int phoneId)
       : id_(id)
       , name_(name)
       , phoneId_(phoneId) {
    }

    virtual ~Event() {
    }

    const uint32_t id_;
    const std::string name_;
    int phoneId_;
};

}  // namespace common
}  // namespace telux

#endif  // EVENT_HPP
