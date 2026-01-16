/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CONFERENCEMENU_HPP
#define CONFERENCEMENU_HPP

#include "../CallMenu.hpp"

class ConferenceMenu : public CallMenu {
 public:
    // initialize menu and sdk
    bool init();
    ConferenceMenu(std::string appName, std::string cursor);
    ~ConferenceMenu();

 private:
    int getInputPhoneId();
    void listCalls(std::vector<std::string> userInput);
    void holdCall(std::vector<std::string> userInput);
    void resumeCall(std::vector<std::string> userInput);
    void hangup(std::vector<std::string> userInput);
    bool menuOptionsAdded_;
};
#endif
