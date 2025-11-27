/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef BASESTATE_HPP
#define BASESTATE_HPP

#include <memory>

#include "BaseStateMachine.hpp"

class Event;

namespace telux {

namespace common {

class BaseState : public BaseStateMachine {
 public:
    // The name of the state
    const std::string name_;
    // Unique ID of the state within the state machine
    const uint32_t id_;

    /**
     * Constructor to create a state
     *
     * @param [in] name   - Name of the state - used for logging
     * @param [in] id     - Unique ID of the state within the statemachine
     *                      Used for checking during state transitions.
     *                      Zero (0 = STATE_ID_INVALID) is reserved.
     * @param [in] parent - The parent state machine (or state) that should
     *                      be used to request state transitions
     */
    BaseState(std::string name, uint32_t id, std::weak_ptr<BaseStateMachine> parent);

    virtual ~BaseState();

    /**
     * Method to get the current state the statemachine is in. The statemachine
     * is traversed hierarchically to fetch the state we are in.
     *
     * @returns The id of the state the statemachine is in
     */
    virtual uint32_t getCurrentState() override;

    /**
     * This method should be overridden in the state to receive an event.
     * In case of a composite state, the base statemachine's onEvent can
     * be invoked to handle the event in the sub-states. The order of
     * invocation should be carefully considered. If the base state's
     * onEvent is invoked, the event handling would be in the deepest
     * state.
     *
     * @param [in] event - The event to be handled @ref telux::commom::Event
     *
     * @returns true if the event was handled by the state or the underlying
     *          statemachines in the hierarchy
     */
    virtual bool onEvent(std::shared_ptr<Event> event) = 0;

 protected:
    /**
     * Helper method for requesting state transition to the current
     * state machine. This request is passed on to the parent statemachine
     * for further handling.
     * @param [in] state  - The new state to be entered
     */
    virtual void changeState(std::shared_ptr<BaseState> state) override;

    /**
     * Helper method for requesting state transition to a sub-state. A call
     * to this method increases the depth of the statemachine by 1.
     *
     * @param [in] state  - The new sub-state to be entered
     */
    virtual void changeSubState(std::shared_ptr<BaseState> state);

    /**
     * Method with generic logging up on entering the state. This method
     * should be overridden by the implementation state for specific actions
     * up on entering the state
     */
    virtual void onEnter();

    /**
     * Method with generic logging just before exiting the state. This method
     * should be overridden by the implementation state for specific actions
     * to be taken care up exiting the state
     */
    virtual void onExit();

    // The parent statemachine that would be used to request
    // state transitions
    const std::weak_ptr<BaseStateMachine> parent_;

 private:
    friend class BaseStateMachine;
};

}  // namespace common
}  // namespace telux

#endif  // BASESTATE_HPP
