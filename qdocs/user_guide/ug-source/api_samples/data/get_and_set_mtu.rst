..
   *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
   *  SPDX-License-Identifier: BSD-3-Clause-Clear

.. _get-and-set-mtu:

Get/Set 3GPP MTU
========================================================

This sample application demonstrates how to get and set the OEM-configured default 3GPP MTU
(IPv4-only) for a slot using
:cpp:func:`telux::data::IDataSettingsManager::setMtu` and
:cpp:func:`telux::data::IDataSettingsManager::getMtu`.

1. Implement initialization callback and get the DataFactory instance

Optionally initialization callback can be provided with get manager instance.
Data factory will call callback when manager initialization is complete.

.. code-block::

   auto initCb = [&](telux::common::ServiceStatus status) {
      std::lock_guard<std::mutex> lock(mtx);
      status_ = status;
      initCv.notify_all();
   };
   auto &dataFactory = telux::data::DataFactory::getInstance();


2. Get IDataSettingsManager instance

.. code-block::

   std::unique_lock<std::mutex> lck(mtx);
   auto dataSettingsMgr = dataFactory.getDataSettingsManager(
       telux::data::OperationType::DATA_LOCAL, initCb);


3. Wait for IDataSettingsManager initialization to be complete

.. code-block::

   initCv.wait(lck);


3.1 Check IDataSettingsManager initialization state

If initialization failed, a new initialization attempt can be made by calling step 2. If
initialization succeeded, proceed to step 4.

.. code-block::

   if (status_ == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      // Go to step 4
   }
   else {
      // Go to step 2 for another initialization attempt
   }


4. Set the default 3GPP MTU value for a slot

:cpp:func:`telux::data::IDataSettingsManager::setMtu` is synchronous: it persists the value to
non-volatile storage on the device and applies it at runtime (no reboot required) before
returning. Because every successful call writes to non-volatile storage, callers should use it
carefully/sparingly rather than as a frequently-polled runtime setting.

.. code-block::

   telux::data::MtuConfig mtuConfig;
   mtuConfig.slotId  = telux::data::SlotId::SLOT_ID_1;
   mtuConfig.ipv4Mtu = 1500;

   telux::common::ErrorCode errCode = dataSettingsMgr->setMtu(mtuConfig);
   if (errCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << "Set MTU succeeded." << std::endl;
   } else {
      std::cout << "Set MTU failed." << std::endl;
   }


5. Get the currently configured default 3GPP MTU value for a slot

:cpp:func:`telux::data::IDataSettingsManager::getMtu` returns the OEM-configured default set via
:cpp:func:`telux::data::IDataSettingsManager::setMtu` -- the fallback/default value used when no
network-supplied MTU is in effect for a PDN, not necessarily the MTU observed on an active data
call.

.. code-block::

   telux::data::MtuConfig mtuConfig;
   mtuConfig.slotId = telux::data::SlotId::SLOT_ID_1;

   telux::common::ErrorCode errCode = dataSettingsMgr->getMtu(mtuConfig);
   if (errCode == telux::common::ErrorCode::SUCCESS) {
      std::cout << "Current default IPv4 MTU: " << mtuConfig.ipv4Mtu << " bytes" << std::endl;
   } else {
      std::cout << "Get MTU failed." << std::endl;
   }
