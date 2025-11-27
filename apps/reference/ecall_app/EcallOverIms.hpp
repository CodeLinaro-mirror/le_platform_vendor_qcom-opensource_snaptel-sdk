/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ECALLOVERIMS_HPP
#define ECALLOVERIMS_HPP

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "ConsoleApp.hpp"
#include "ECallManager.hpp"

/**
 * EcallOverImsMenu class provides an user-interactive console to trigger an custom number eCall and
 * update MSD over IMS.
 */
class EcallOverImsMenu : public ConsoleApp {
 public:
    EcallOverImsMenu(
        std::shared_ptr<ECallManager> eCallManger, std::string appName, std::string cursor);

    /**
     * Initialize the subsystems, console commands and display the menu.
     */
    void init();

    /**
     * Hangs up a triggered eCall and gracefully clears down the subsystems.
     */
    void cleanup();

    ~EcallOverImsMenu();

 private:
    /**
     * Trigger a Voice eCall to the specified phone number over IMS
     *
     */
    void makeCustomNumberECallOverIms();

    /**
     * Update MSD for custom number eCall over IMS
     *
     */
    void updateCustomNumberECallOverIms();

    /**
     * Function to get optional SIP request headers
     *
     */
    void getOptionalSIPHeader(std::string &contentType, std::string &acceptInfo);
    // Member variable to keep the eCall manager object alive until the application quits.
    std::weak_ptr<ECallManager> eCallMgr_;
};

#endif  // ECALLAPP_HPP
