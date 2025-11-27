/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SMSTRIGGER_HPP
#define SMSTRIGGER_HPP

#include <memory>
#include <string>
#include <vector>
#include "telux/tel/SmsManager.hpp"

#include "../../Event.hpp"
#include "../../EventManager.hpp"
#include "../../IEventListener.hpp"
#include "../../common/ConfigParser.hpp"

class SMSTrigger : public telux::tel::ISmsListener,
                   public IEventListener,
                   public enable_shared_from_this<SMSTrigger> {
 private:
    std::map<string, TcuActivityState> triggerText_; /** map which stores trigger text with
                                                         respect to TcuActivityState */
    ConfigParser *config_; /** config parser to fetch data from config file */
    std::shared_ptr<EventManager> eventManager_; /** event management */
    std::shared_ptr<telux::tel::ISmsManager> smsManager_;

    bool loadConfig();
    void triggerEvent(TcuActivityState event, std::string machineName);
    bool validateTrigger(
        std::string text, TcuActivityState &tcuActivityState, std::string &machineName);

 public:
    SMSTrigger(std::shared_ptr<EventManager> eventManager);
    ~SMSTrigger();
    bool init();

    // SmsListener
    void onIncomingSms(
        int phoneId, std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs) override;

    // EventListener
    void onEventRejected(shared_ptr<Event> event, EventStatus reason) override;
    void onEventProcessed(shared_ptr<Event> event, bool success) override;
};

#endif  // SMSTRIGGER_HPP