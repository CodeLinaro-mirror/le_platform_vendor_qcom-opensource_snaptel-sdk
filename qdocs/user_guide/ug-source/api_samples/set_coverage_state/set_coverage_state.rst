..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _set-coverage-state:

Set coverage state
===================================================

This sample application demonstrates setting the 5G network coverage state to control and optimize
5G scan behavior.

1. Get phone factory and network selection manager instances

.. code-block::

   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> prom;
   auto networkMgr
       = phoneFactory.getNetworkSelectionManager(DEFAULT_SLOT_ID),
       [&](telux::common::ServiceStatus status) {
           prom.set_value(status);
   });


2. Wait for the network selection subsystem initialization

.. code-block::

   telux::common::ServiceStatus serviceStatus = prom.get_future().get();

3. Exit the application if the network selection subsystem cannot be initialized

.. code-block::

   if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "Network selection manager service unavailable, status "
           << static_cast<int>(serviceStatus) << std::endl;
       return -EIO;
   }

4. Set coverage state for the 5G network

.. code-block::

   int stateInput = -1;
   std::cout << "Enter coverage state (1-IN_5G-COVERAGE, 2-OUT_OF_5G_COVERAGE): ";
   std::cin >> stateInput;

   switch (stateInput) {
       case 1:
           state = telux::tel::CoverageState::IN_5G_COVERAGE;
           break;
       case 2:
           state = telux::tel::CoverageState::OUT_OF_5G_COVERAGE;
           break;
       default:
           std::cout << "Invalid coverage state input" << std::endl;
           return -EIO;
   }

   if (networkMgr) {
       ErrorCode err = networkMgr->setCoverageState(state);
       std::cout << "ErrorCode: " << static_cast<int>(err) <<std::endl;
   }

