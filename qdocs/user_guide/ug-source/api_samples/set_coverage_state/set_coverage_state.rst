..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _set-coverage-area:

Set coverage area
===================================================

This sample application demonstrates setting the 5G network coverage area to control and optimize
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

4. Set coverage area for the 5G network

.. code-block::

   int areaInput = -1;
   std::cout << "Enter coverage area (1-IN_5G-COVERAGE_HOLE, 2-OUT_OF_5G_COVERAGE_HOLE): ";
   std::cin >> areaInput;

   switch (areaInput) {
       case 1:
           area = telux::tel::CoverageArea::IN_5G_COVERAGE_HOLE;
           break;
       case 2:
           area = telux::tel::CoverageArea::OUT_OF_5G_COVERAGE_HOLE;
           break;
       default:
           std::cout << "Invalid coverage area input" << std::endl;
           return -EIO;
   }

   if (networkMgr) {
       ErrorCode err = networkMgr->setCoverageArea(area);
       std::cout << "ErrorCode: " << static_cast<int>(err) <<std::endl;
   }

