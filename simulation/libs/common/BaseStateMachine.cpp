/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "BaseState.hpp"
#include "BaseStateMachine.hpp"
#include "Logger.hpp"

namespace telux {

namespace common {

BaseStateMachine::BaseStateMachine(std::string name)
   : name_(name)
   , currentState_(nullptr)
   , started_(false) {
}

uint32_t BaseStateMachine::getCurrentState() {
    // We are not in any state
    if (currentState_ == nullptr) {
        return STATE_ID_INVALID;
    }

    // Ask the current state to get the ID of the underlying active state
    return currentState_->getCurrentState();
}

bool BaseStateMachine::onEvent(std::shared_ptr<Event> event) {
    if (!currentState_) {
        return false;
    } else {
        return currentState_->onEvent(event);
    }
}

void BaseStateMachine::changeState(std::shared_ptr<BaseState> state) {
    if (!started_) {
        LOG(WARNING, "[request] ", name_, ": rejected since state machine is not started");
        return;
    }
    LOG(INFO, "[request] ", name_, ": ", (currentState_ ? currentState_->name_ : "null"), " -> ",
        (state ? state->name_ : "null"));

    // Some basic checks to ensure an actual state transition is requested

    if (currentState_ == state) {
        return;
    }
    if (currentState_) {
        if (state) {
            if (currentState_->id_ == state->id_) {
                return;
            }
        }
        // Exit the current state
        currentState_->onExit();
    }
    currentState_ = state;

    // Enter the new current state
    if (currentState_) {
        currentState_->onEnter();
    }
}

void BaseStateMachine::start() {
    started_ = true;
}

void BaseStateMachine::stop() {
    // Exit the current state, this eventually is propagated
    // to the underlying states (composite and simple) to
    // ensure all states are exited
    LOG(DEBUG, __FUNCTION__);
    if (currentState_) {
        LOG(DEBUG, __FUNCTION__, "Current state is ", currentState_->name_);
        currentState_->onExit();
    }
    currentState_ = nullptr;
    started_      = false;
}

bool BaseStateMachine::isStarted() const {
    return started_;
}

void BaseStateMachine::print(std::stringstream &ss) {
    // Put out our name
    ss << name_;

    // Ask the underlying states/statemachines to populate the stream
    if (currentState_) {
        ss << " --> ";
        currentState_->print(ss);
    } else {
        ss << " --> null";
    }
}

BaseStateMachine::~BaseStateMachine() {
    // Stop the statemachine in case we are being deleted before
    // having been stopped
    stop();
}

}  // namespace common
}  // namespace telux
