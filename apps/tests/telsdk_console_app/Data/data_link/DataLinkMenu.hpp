/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This is a DataLink Manager Sample Application using Telematics SDK.
 * It is used to demonstrate API to exercise Data Link Features.
 */

#ifndef DATALINKMENU_HPP
#define DATALINKMENU_HPP

#include <iostream>
#include <memory>
#include <string>

#include <telux/data/DataDefines.hpp>
#include <telux/data/DataFactory.hpp>
#include <telux/data/DataLinkManager.hpp>

#include "console_app_framework/ConsoleApp.hpp"
#include "DataLinkListener.hpp"

class DataLinkMenu : public ConsoleApp {
public:
   // initialize menu and sdk
   bool init();

   DataLinkMenu(std::string appName, std::string cursor);
   ~DataLinkMenu();

   //API
   void registerListener(std::vector<std::string> inputCommand);
   void deregisterListener(std::vector<std::string> inputCommand);

private:
   bool initComplete_;
   bool addMenuCmds_;
   std::shared_ptr<telux::data::IDataLinkManager> dataLinkManager_;
   std::shared_ptr<telux::data::IDataLinkListener> dataLinkListener_;

   bool initDataLinkManagerAndListener();
};
#endif //DATALINKMENU_HPP
