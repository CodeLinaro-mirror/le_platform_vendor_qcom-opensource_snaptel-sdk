/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CANTRIGGER_HPP
#define CANTRIGGER_HPP

#include "../../Event.hpp"
#include "../../EventManager.hpp"
#include "../../IEventListener.hpp"
#include "../../common/ConfigParser.hpp"

#include <CwBase.h>
#include <CanWrapper.h>
#include <CwFrame.h>

typedef int RegistrationToken;

class CANTrigger : public IEventListener, public enable_shared_from_this<CANTrigger> {
 private:
    static std::shared_ptr<CANTrigger> canTrigger_;
    static void triggerEvent(CwFrame *pf, void *userData, int ifNo);

    std::shared_ptr<EventManager> eventManager_;

    /** map to store can frame id along with respective expected TcuActivityState
     * and registration token*/
    std::map<uint32_t, pair<TcuActivityState, RegistrationToken>> triggers_;

    ConfigParser *config_; /** config parser to fetch data from config file */
    CanWrapper *canWrapper_;

    bool loadTrigger();
    bool registerCanListener();
    void deRegisterCanListener();
    bool loadTriggers();

 public:
    static std::shared_ptr<CANTrigger> getInstance(std::shared_ptr<EventManager> eventManager);
    bool init();

    CANTrigger(std::shared_ptr<EventManager> eventManager);
    ~CANTrigger();

    // EventListener
    void onEventRejected(shared_ptr<Event> event, EventStatus reason) override;
    void onEventProcessed(shared_ptr<Event> event, bool success) override;
};

#endif  // CANTRIGGER_HPP