/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * Utility class that provides a framework to create statemachines.
 * As a framework, it comes with bare minimal functionality allowing
 * flexibility to the users.
 */

#ifndef BASESTATEMACHINE_HPP
#define BASESTATEMACHINE_HPP

#include <memory>
#include <mutex>
#include <string>
#include <sstream>

#define STATE_ID_INVALID 0
#define EVENT_ID_INVALID 0

namespace telux {

namespace common {

class BaseState;
class Event;

class BaseStateMachine {
 public:
    // Name of the statemachine for logging purpose
    const std::string name_;

    /**
     * Constructor for the base statemachine
     * @param [in] name - Name of the state machine, used for logging purpose
     */
    BaseStateMachine(std::string name);

    virtual ~BaseStateMachine();

    /**
     * Method to get the current state the statemachine is in. the statemachine
     * is traversed hierarchically to fetch the state we are in.
     *
     * @returns The id of the state the statemachine is in
     */
    virtual uint32_t getCurrentState();

    /**
     * Method used for generic handling of events - invokes the current
     * state's onEvent. Can be overridden for blocking events or additional
     * logging
     *
     * @param [in] event - The event to be handled @ref telux::commom::Event
     *
     * @returns true if the event was handled by the state or the underlying
     *          statemachines in the hierarchy
     */
    virtual bool onEvent(std::shared_ptr<Event> event);

    /**
     * Method used to start the statemachine activities like entering the
     * initial state, allowing event handling. This method should be called
     * to allow generic event handling by the framework.
     */
    virtual void start();

    /**
     * Method used to stop the statemachine activites - exits all active states
     * and disables generic event handling
     */
    virtual void stop();

    /**
     * Method to find if the state machine has been started (enabled)
     */
    bool isStarted() const;

    /**
     * A utility method to print the current schema of the statemachine
     * @param [in,out] ss - The stringstream to be populated for usage by
     *                      the caller
     */
    virtual void print(std::stringstream &ss);

    // Holds the current state this statemachine is in
    std::shared_ptr<BaseState> currentState_;

 protected:
    /**
     * Method used to request state transition. This method offers generic
     * state transition, i.e. exit the current state, enter the new state.
     * There are three possibilities for a state change
     * 1. With the current state being nullptr and state non-null to move
     * from an initial state to a valid, known state
     * 2. With the current state being non-null and state a nullptr to move
     * from a known, valid state to a final state to wind-up the state-machine
     * 3. With both current state and state being non-null to transit from
     * one valid state to another
     *
     * @param [in] state  - The new state to be entered, can be null to
     *                      move to a final state
     */
    virtual void changeState(std::shared_ptr<BaseState> state);

 private:
    friend class BaseState;
    // Flag indicating if the state machine was started
    bool started_;
};

}  // namespace common
}  // namespace telux

#endif  // BASESTATEMACHINE_HPP
