/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef REALTIMETEXTMENU_HPP
#define REALTIMETEXTMENU_HPP

#include "../CallMenu.hpp"

class RttMenu : public CallMenu {
 public:
    // initialize menu and sdk
    bool init();
    RttMenu(std::string appName, std::string cursor);
    ~RttMenu();

 private:
    int getInputPhoneId();
    void dialRttCall(std::vector<std::string> userInput);
    void respondToModifyRequest(std::vector<std::string> userInput);
    void modifyCall(std::vector<std::string> userInput);
    void sendRttMessage(std::vector<std::string> userInput);
    void acceptCall(std::vector<std::string> userInput);
    std::shared_ptr<MyCallCommandCallback> myModifyCb_;
    std::shared_ptr<MyCallCommandCallback> myrespondToModifyRequestCb_;
    bool menuOptionsAdded_;
};
#endif
