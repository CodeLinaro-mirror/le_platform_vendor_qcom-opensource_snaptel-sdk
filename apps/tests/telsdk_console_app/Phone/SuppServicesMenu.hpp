/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SUPP_SERVICES_MENU_HPP
#define SUPP_SERVICES_MENU_HPP

#include <telux/tel/SuppServicesManager.hpp>
#include "console_app_framework/ConsoleApp.hpp"

class SuppServicesMenu : public ConsoleApp {
 public:
    SuppServicesMenu(std::string appName, std::string cursor);
    ~SuppServicesMenu();
    bool init();

 private:
    void setCallWaitingPref(std::vector<std::string> userInput);
    void getCallWaitingPref(std::vector<std::string> userInput);
    void setCallForwardingPref(std::vector<std::string> userInput);
    void getCallForwardingPref(std::vector<std::string> userInput);
    void setOirPref(std::vector<std::string> userInput);
    void getOirPref(std::vector<std::string> userInput);
    void selectSimSlot(std::vector<std::string> userInput);

    SlotId slot_ = DEFAULT_SLOT_ID;
    std::vector<std::shared_ptr<telux::tel::ISuppServicesManager>> suppServicesManagers_;
};

#endif  // SUPP_SERVICES_MENU_HPP