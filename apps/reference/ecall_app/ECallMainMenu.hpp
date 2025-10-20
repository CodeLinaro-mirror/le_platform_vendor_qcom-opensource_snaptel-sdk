/* Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ECALLMAINMENU_HPP
#define ECALLMAINMENU_HPP

#include "console_app_framework/ConsoleApp.hpp"

class ECallMainMenu : public ConsoleApp {
public:
   /**
     * Get an instance of ECallApp
     */
    static ECallMainMenu &getInstance();

    /**
     * Initialize the subsystems, console commands and display the menu.
     */
    bool init();

   /**
     * Hangs up a triggered eCall and gracefully clears down the subsystems.
     */
   void cleanup();
   void ecallAppMenu(std::vector<std::string> userInput);
   void aecsCallMenu(std::vector<std::string> userInput);

private:
   ECallMainMenu(std::string appName, std::string cursor);
   ~ECallMainMenu();
};

#endif  // ECALLMAINMENU_HPP
