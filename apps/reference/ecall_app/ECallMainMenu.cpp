/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include "ECallApp.hpp"
#include "aecsCall/AecsCall.hpp"
#include "../../common/utils/Utils.hpp"
#include "../../common/utils/SignalHandler.hpp"

#include "ECallMainMenu.hpp"

ECallMainMenu::ECallMainMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

ECallMainMenu::~ECallMainMenu() {
}


ECallMainMenu &ECallMainMenu::getInstance() {
    static ECallMainMenu instance("eCall App Menu", "eCall> ");
    return instance;
}

bool ECallMainMenu::init() {
   std::shared_ptr<ConsoleAppCommand> ecallAppCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", "eCall", {}, std::bind(&ECallMainMenu::ecallAppMenu,
                                                            this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> aecsCallCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("2", "AECS_Call", {},
                        std::bind(&ECallMainMenu::aecsCallMenu, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> mainMenuCommands
      = {ecallAppCommand, aecsCallCommand};

   addCommands(mainMenuCommands);
   ConsoleApp::displayMenu();
   return true;
}

void ECallMainMenu::ecallAppMenu(std::vector<std::string> userInput) {
   ECallApp ecallAppMenu("ECall", "ecall> ");
   if (ecallAppMenu.init()) {
       ecallAppMenu.mainLoop();  // Main loop to continuously read and execute commands
   }
   ConsoleApp::displayMenu();
}

void ECallMainMenu::aecsCallMenu(std::vector<std::string> userInput) {
   AecsCall aecsCallMenu("AECS_call", "aecs> ");
   if (aecsCallMenu.init()) {
       aecsCallMenu.mainLoop();  // Main loop to continuously read and execute commands
   }
   ConsoleApp::displayMenu();
}

/**
 * Executes any cleanup procedure if necessary
 */
void ECallMainMenu::cleanup() {
    std::cout << "Exiting the application.." << std::endl;
}


// Main function that displays the interactive console for eCall related operations
int main(int argc, char **argv) {

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigaddset(&sigset, SIGHUP);
    SignalHandlerCb cb = [](int sig) {
        // We can call exit() here if no cleanups needed,
        // or maybe just set a flag, and let the main thread to decide
        // when to exit.
        ECallMainMenu::getInstance().cleanup();
        exit(sig);
    };
    SignalHandler::registerSignalHandler(sigset, cb);

    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag", "locclient", "logd"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << "Adding supplementary groups failed!" << std::endl;
    }
    auto &eCallApp = ECallMainMenu::getInstance();
    eCallApp.init();             // initialize commands and display
    return eCallApp.mainLoop();  // Main loop to continuously read and execute commands
}
