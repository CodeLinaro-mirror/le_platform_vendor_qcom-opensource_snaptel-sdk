/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "BaseState.hpp"
#include "Event.hpp"
#include "Logger.hpp"

namespace telux {

namespace common {

BaseState::BaseState(std::string name, uint32_t id, std::weak_ptr<BaseStateMachine> parent)
   : BaseStateMachine(name)
   , name_(name)
   , id_(id)
   , parent_(parent) {
}

BaseState::~BaseState() {
}

uint32_t BaseState::getCurrentState() {
    if (currentState_) {
        return currentState_->getCurrentState();
    } else {
        return id_;
    }
}

bool BaseState::onEvent(std::shared_ptr<Event> event) {
    // If the state does not want to handle the event OR
    // if the state hasn't handled the event, it reaches here.
    // We forward it to the underlying statemachine to check
    // if the event would be handled there
    return BaseStateMachine::onEvent(event);
}

void BaseState::onEnter() {
    LOG(INFO, "[enter] ", name_);
}

void BaseState::onExit() {
    // If we are a composite state, we stop the statemachine,
    // eventually exiting all the underlying states
    BaseStateMachine::stop();
    LOG(INFO, "[exit] ", name_);
}

void BaseState::changeState(std::shared_ptr<BaseState> state) {
    // Request the parent to handle state transition request
    auto parent = parent_.lock();
    if (parent) {
        parent->BaseStateMachine::changeState(state);
    }
}

void BaseState::changeSubState(std::shared_ptr<BaseState> state) {
    // Enter the requested sub-state, in this case the parent
    // would be this state (statemachine)
    BaseStateMachine::changeState(state);
}

}  // namespace common
}  // namespace telux
